#include <fstream>

#include <esp_log.h>
#include <esp_spiffs.h>
#include <nvs_flash.h>

#include <expressif/wifi/WiFi.h>
#include <expressif/http/server/HTTPServer.h>

#ifndef EXAMPLE_WIFI_SSID
#define EXAMPLE_WIFI_SSID ""
#endif

#ifndef EXAMPLE_WIFI_PASSWORD
#define EXAMPLE_WIFI_PASSWORD ""
#endif

using namespace expressif::http::server;
using namespace expressif::wifi;

constexpr static auto TAG = "http_server_example";

#define LOG(...) ESP_LOGI(TAG, __VA_ARGS__)

// Based on:
// https://github.com/espressif/esp-idf/blob/master/examples/storage/spiffs/main/spiffs_example_main.c
static esp_err_t spiffs_init() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = nullptr,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }

        return ret;
    }

#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return ret;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check the consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ret;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    return ESP_OK;
}

extern "C" [[noreturn]] void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(spiffs_init());

    HTTPServer server;

    auto wifi = WiFi::getInstance();
    wifi->start();
    wifi->connect(EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_PASSWORD);

    /* Register event handlers to stop the server when Wi-Fi is disconnected,
     * and re-start it upon connection.
     */
    wifi->addOnConnectedHandler([&server] {
        if (!server.isValid()) {
            ESP_ERROR_CHECK(server.start());
        }
    });

    wifi->addOnDisconnectedHandler([&server] {
        if (server.isValid()) {
            server.stop();
        }
    });

    ESP_ERROR_CHECK(server.start());
    LOG("Server started");

    server.addEndpoint(HTTPMethod::Post, "/api/echo", [](Request &req) {
        LOG("POST /api/echo");

        auto resp = req.response();
        size_t chunkNumber = 0;
        size_t totalSize = 0;

        req.readChunks([&](ConstBuffer buffer) {
            ++chunkNumber;
            totalSize += buffer.size();
            LOG("Chunk #%i received, size=%i", chunkNumber, buffer.size());
            resp.writeChunk(buffer);
            LOG("Chunk #%i sent", chunkNumber);
        });

        resp.flush();

        LOG("Done, totalCount=%i, totalSize=%i", chunkNumber, totalSize);
    });

    server.addEndpoint(HTTPMethod::Post, "/api/echo-txt", [](Request &req) {
        auto bytes = req.readAll().value();
        auto n = bytes.size();
        LOG("POST /api/echo-txt: %.*s, size=%i", n, bytes.data(), n);
        req.response().writeAll({bytes});
    });

    server.addEndpoint(HTTPMethod::Get, "/api/hello/{username}", [](Request &req) {
        auto &username = req.getPathVar("username");
        LOG("GET /api/hello/{username}: username=%s", username.c_str());
        req.response().write("Hello, " + username);
    });

    server.addEndpoint(HTTPMethod::Get, "/api/hello/{name}/{surname}", [](Request &req) {
        auto &name = req.getPathVar("name");
        auto &surname = req.getPathVar("surname");
        LOG("GET /api/hello/{name}/{surname}: name=%s, surname=%s", name.c_str(), surname.c_str());
        req.response().write("Hello, " + name + " " + surname);
    });

    server.addEndpoint(HTTPMethod::Get, "/api/path/{path}*", [](Request &req) {
        auto &path = req.getPathVar("path");
        LOG("GET /api/path/{path}*, path=%s", path.c_str());
        req.response().write("Path: " + path);
    });

    server.addEndpoint(HTTPMethod::Get, "/{file}*", [](Request &req) {
        auto path = req.getPathVar("file");

        if (path.empty())
            path = "index.html";

        if (std::ifstream file("/spiffs/" + path); file.is_open()) {
            std::string type;

            if (auto ext = std::string_view {path}.substr(path.find_last_of('.') + 1); ext == "html") {
                type = "text/html";
            } else if (ext == "css") {
                type = "text/css";
            } else if (ext == "js") {
                type = "text/javascript";
            } else if (ext == "png") {
                type = "image/png";
            } else if (ext == "ico") {
                type = "image/x-icon";
            }

            req.response().setType(type);

            char buff[128];

            req.response().writeChunks([&]() {
                if (file) {
                    auto n = file.readsome(buff, sizeof(buff));
                    return ConstBuffer(reinterpret_cast<byte_t*>(buff), n);
                } else {
                    return ConstBuffer {};
                }
            });
        } else {
            req.response().error404();
        }
    });

    uint64_t time = 0;

    printf("\n");

    while (true) {
        printf("\e[1A\e[Kuptime: %llum %llus\n", time / 60, time % 60);
        sleep(5);
        time += 5;
    }
}