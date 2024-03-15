#ifndef EXPRESSIF_WIFI_H
#define EXPRESSIF_WIFI_H

#include <functional>
#include <vector>
#include <string_view>

#include <esp_netif.h>
#include <esp_wifi_types.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace expressif::wifi {
class WiFi {
public:
    template<typename... Args>
    using Handler = std::function<void(Args...)>;

    using NetinfConfig = esp_netif_inherent_config_t;
    using Config = wifi_config_t;

public:
    WiFi(const WiFi&) = delete;
    WiFi(WiFi&&) = delete;

    WiFi& operator=(const WiFi&) = delete;
    WiFi& operator=(WiFi&&) = delete;

    ~WiFi();

    void start(const NetinfConfig &config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA());
    void stop();

    esp_err_t connect(std::string_view ssid, std::string_view password, bool async = false);
    esp_err_t connect(Config &config, bool async = false);
    esp_err_t disconnect();

    void shutdown();

    void addOnConnectedHandler(const Handler<> &handler);
    void addOnDisconnectedHandler(const Handler<> &handler);

    static WiFi* getInstance();

private:
    WiFi();

    static void onConnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void onDisconnected(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

private: // core
    esp_netif_t *m_netif;
    int m_retryCount; // TODO: add a delay between attempts

private:
    std::vector<Handler<>> m_onConnectedHandlers;
    std::vector<Handler<>> m_onDisconnectedHandlers;

public: // IPv4
    using IPv4Info = esp_netif_ip_info_t;

    void addOnIPv4ObtainedHandler(const Handler<const IPv4Info&> &handler);

    static void onIPv4Obtained(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    SemaphoreHandle_t m_ipv4Sem {};
    std::vector<Handler<const IPv4Info&>> m_onIPv4Handlers;

#if CONFIG_ENABLE_IPV6
public: // IPv6
    using IPv6Info = esp_netif_ip6_info_t;

    void addOnIPv6ObtainedHandler(const Handler<const IPv6Info&> &handler);

    static void onIPv6Obtained(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    SemaphoreHandle_t m_ipv6Sem {};
    std::vector<Handler<const IPv6Info&>> m_onIPv6Handlers;
#endif //CONFIG_ENABLE_IPV6
};
}

#endif //EXPRESSIF_WIFI_H
