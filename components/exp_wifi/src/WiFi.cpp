#include <expressif/wifi/WiFi.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include <cstring>

#include "sdkconfig.h"

namespace expressif::wifi {
constexpr static auto TAG = "expressif::wifi::WiFi";

WiFi::WiFi()
    : m_netif(),
      m_retryCount(0)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

WiFi::~WiFi() = default;

static void espShutdownHandler() {
    WiFi::getInstance()->shutdown();
}

void WiFi::start(const NetinfConfig &config) {
    m_netif = esp_netif_create_wifi(WIFI_IF_STA, &config);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_register_shutdown_handler(espShutdownHandler));
}

void WiFi::stop() {
    esp_err_t err = esp_wifi_stop();

    if (err == ESP_ERR_WIFI_NOT_INIT)
        return;

    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(m_netif));
    esp_netif_destroy(m_netif);

    m_netif = nullptr;

    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(espShutdownHandler));
}

esp_err_t WiFi::connect(std::string_view ssid, std::string_view password, bool async) {
    Config config {};
    config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    config.sta.threshold.rssi = -127;
    config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    assert(ssid.size() < sizeof(config.sta.ssid));
    assert(password.size() < sizeof(config.sta.password));

    std::strcpy(reinterpret_cast<char*>(&config.sta.ssid[0]), ssid.data());
    std::strcpy(reinterpret_cast<char*>(&config.sta.password[0]), password.data());

    return connect(config, async);
}

esp_err_t WiFi::connect(Config &config, bool async) {
    if (m_netif == nullptr)
        start();

    if (!async) {
        m_ipv4Sem = xSemaphoreCreateBinary();

        if (m_ipv4Sem == nullptr) {
            return ESP_ERR_NO_MEM;
        }

#if CONFIG_ENABLE_IPV6
        m_ipv6Sem = xSemaphoreCreateBinary();

        if (m_ipv6Sem == nullptr) {
            vSemaphoreDelete(m_ipv4Sem);
            return ESP_ERR_NO_MEM;
        }
#endif
    }

    m_retryCount = 0;

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &onDisconnected, this));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &onConnected, this));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &onIPv4Obtained, this));

#if CONFIG_ENABLE_IPV6
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &onIPv6Obtained, this));
#endif

    ESP_LOGD(TAG, "Connecting to %s...", config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

    if (esp_err_t ret = esp_wifi_connect(); ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed! ret: %x", ret);
        return ret;
    }

    if (!async) {
        ESP_LOGD(TAG, "Waiting for IP(s)");
        xSemaphoreTake(m_ipv4Sem, portMAX_DELAY);

#if CONFIG_ENABLE_IPV6
        xSemaphoreTake(m_ipv6Sem, portMAX_DELAY);
#endif

        if (m_retryCount > CONFIG_WIFI_CONN_MAX_RETRY) {
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t WiFi::disconnect() {
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &onDisconnected));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &onConnected));

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &onIPv4Obtained));

    if (m_ipv4Sem) {
        vSemaphoreDelete(m_ipv4Sem);
    }

#if CONFIG_ENABLE_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &onIPv6Obtained));

    if (m_ipv6Sem != nullptr) {
        vSemaphoreDelete(m_ipv6Sem);
    }
#endif

    return esp_wifi_disconnect();
}

void WiFi::shutdown() {
    disconnect();
    stop();
}

void WiFi::addOnConnectedHandler(const Handler<> &handler) {
    m_onConnectedHandlers.emplace_back(handler);
}

void WiFi::addOnDisconnectedHandler(const Handler<> &handler) {
    m_onDisconnectedHandlers.emplace_back(handler);
}

WiFi* WiFi::getInstance() {
    static WiFi wifi;
    return &wifi;
}

void WiFi::addOnIPv4ObtainedHandler(const Handler<const IPv4Info&> &handler) {
    m_onIPv4Handlers.emplace_back(handler);
}

void WiFi::addOnIPv6ObtainedHandler(const Handler<const IPv6Info&> &handler) {
    m_onIPv6Handlers.emplace_back(handler);
}

void WiFi::onConnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto wifi = static_cast<WiFi*>(arg);

#if CONFIG_ENABLE_IPV6
    esp_netif_create_ip6_linklocal(wifi->m_netif);
#endif

    for (const auto &handler : wifi->m_onConnectedHandlers) {
        handler();
    }
}

void WiFi::onDisconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto wifi = static_cast<WiFi*>(arg);
    wifi->m_retryCount++;

    if (wifi->m_retryCount > CONFIG_WIFI_CONN_MAX_RETRY) {
        ESP_LOGD(TAG, "WiFi Connect failed %d times, stop reconnect.", wifi->m_retryCount);

        // let connect() return
        if (wifi->m_ipv4Sem != nullptr) {
            xSemaphoreGive(wifi->m_ipv4Sem);
        }

#if CONFIG_ENABLE_IPV6
        if (wifi->m_ipv6Sem != nullptr) {
            xSemaphoreGive(wifi->m_ipv6Sem);
        }
#endif

        return;
    }

    ESP_LOGD(TAG, "Wi-Fi disconnected, trying to reconnect...");

    if (esp_err_t err = esp_wifi_connect(); err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    } else {
        ESP_ERROR_CHECK(err);
    }

    for (const auto &handler : wifi->m_onDisconnectedHandlers) {
        handler();
    }
}

void WiFi::onIPv4Obtained(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto wifi = static_cast<WiFi*>(arg);
    auto event = static_cast<ip_event_got_ip_t*>(event_data);

    wifi->m_retryCount = 0;

    if (wifi->m_netif != event->esp_netif)
        return;

    ESP_LOGD(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
             esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));

    if (wifi->m_ipv4Sem) {
        xSemaphoreGive(wifi->m_ipv4Sem);
    } else {
        ESP_LOGD(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
    }

    for (const auto &handler : wifi->m_onIPv4Handlers) {
        handler(event->ip_info);
    }
}

#if CONFIG_ENABLE_IPV6
constexpr static std::string_view ipv6_addr_types_to_str[6] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};

// TODO
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL

void WiFi::onIPv6Obtained(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto wifi = static_cast<WiFi*>(arg);
    auto event = static_cast<ip_event_got_ip6_t*>(event_data);

    if (wifi->m_netif != event->esp_netif)
        return;

    esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGD(TAG, "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s", esp_netif_get_desc(event->esp_netif),
             IPV62STR(event->ip6_info.ip), ipv6_addr_types_to_str[ipv6_type].data());

    if (ipv6_type == EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE) {
        if (wifi->m_ipv6Sem) {
            xSemaphoreGive(wifi->m_ipv6Sem);
        } else {
            ESP_LOGD(TAG, "- IPv6 address: " IPV6STR ", type: %s",
                     IPV62STR(event->ip6_info.ip),
                     ipv6_addr_types_to_str[ipv6_type].data());
        }
    }

    for (const auto &handler : wifi->m_onIPv6Handlers) {
        handler(event->ip6_info);
    }
}
#endif

}
