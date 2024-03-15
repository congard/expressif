#include <expressif/http/server/Response.h>
#include <expressif/expressif_info.h>

#include <sdkconfig.h>

#define HTTP_SERVER_VERSION_INFO \
EXPRESSIF_DISPLAY_NAME " " EXPRESSIF_VERSION_STR " HTTP Server running on " CONFIG_IDF_TARGET

#define HTTP_SERVER_VERSION_INFO_FORMATTED \
"<small><i>" HTTP_SERVER_VERSION_INFO "</small></i>"

namespace expressif::http::server {

constexpr static auto default404Message = R"(
<h1>404 Not Found</h1>
<br>The requested resource cannot be found on this server.
<br><br>
)" HTTP_SERVER_VERSION_INFO_FORMATTED;

constexpr static auto default408Message = R"(
<h1>408 Request Timeout</h1>
<br>Connection timed out.
<br><br>
)" HTTP_SERVER_VERSION_INFO_FORMATTED;

constexpr static auto default500Message = R"(
<h1>500 Internal Server Error</h1>
<br>The server encountered an unexpected condition that prevented it from fulfilling the request.
<br><br>
)" HTTP_SERVER_VERSION_INFO_FORMATTED;

Response::Response(httpd_req_t *&req)
    : m_req(req) {}

esp_err_t Response::setHeader(std::string_view header, std::string_view value) {
    return httpd_resp_set_hdr(m_req, header.data(), value.data());
}

esp_err_t Response::setType(std::string_view type) {
    return httpd_resp_set_type(m_req, type.data());
}

esp_err_t Response::setStatus(std::string_view status) {
    return httpd_resp_set_status(m_req, status.data());
}

esp_err_t Response::writeAll(ConstBuffer data) {
    auto status = httpd_resp_send(
        m_req,
        reinterpret_cast<const char *>(data.data()),
        static_cast<ssize_t>(data.size())
    );

    if (status == ESP_OK)
        invalidate();

    return status;
}

esp_err_t Response::writeChunk(ConstBuffer chunk) {
    if (chunk.empty()) {
        return flush();
    } else {
        return httpd_resp_send_chunk(
            m_req,
            reinterpret_cast<const char *>(chunk.data()),
            static_cast<ssize_t>(chunk.size())
        );
    }
}

esp_err_t Response::writeChunks(ConstBuffer data, size_t chunkSize, bool flush) {
    auto chunkCount = data.size() / chunkSize + (data.size() % chunkSize == 0 ? 0 : 1);

    for (size_t i = 0; i < chunkCount; ++i) {
        using diff_type = decltype(data.begin())::difference_type;

        auto begin = static_cast<diff_type>(chunkSize * i);
        auto end = static_cast<diff_type>(std::min(begin + chunkSize, data.size()));

        if (auto ret = writeChunk({data.begin() + begin, data.begin() + end}); ret != ESP_OK) {
            return ret;
        }
    }

    if (flush)
        return this->flush();

    return ESP_OK;
}

esp_err_t Response::flush() {
    if (auto status = httpd_resp_send_chunk(m_req, nullptr, 0); status == ESP_OK) {
        invalidate();
        return status;
    } else {
        return status;
    }
}

esp_err_t Response::error(httpd_err_code_t code, std::string_view message) {
    auto status = httpd_resp_send_err(m_req, code, message.data());

    if (status == ESP_OK)
        invalidate();

    return status;
}

esp_err_t Response::error404() {
    return error(HTTPD_404_NOT_FOUND, default404Message);
}

esp_err_t Response::error408() {
    return error(HTTPD_408_REQ_TIMEOUT, default408Message);
}

esp_err_t Response::error500() {
    return error(HTTPD_500_INTERNAL_SERVER_ERROR, default500Message);
}

void Response::invalidate() {
    m_req = nullptr;
}
}
