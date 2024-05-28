#include <expressif/http/server/Request.h>
#include <expressif/http/server/util/URIPathParser.h>
#include <expressif/http/server/util/URIUtils.h>

#include "detail/EndpointData.h"
#include "sdkconfig.h"

namespace expressif::http::server {
Request::Request(httpd_req_t *req)
    : m_req(req) {}

std::string Request::getHeader(std::string_view name) const {
    auto size = httpd_req_get_hdr_value_len(m_req, name.data());
    std::string result(size, '\0');
    httpd_req_get_hdr_value_str(m_req, name.data(), result.data(), size);
    return result;
}

bool Request::hasHeader(std::string_view name) const {
    return httpd_req_get_hdr_value_len(m_req, name.data()) > 0;
}

const std::string& Request::getPathVar(std::string_view name) {
    return getPathVars().at(name.data());
}

const PathVars& Request::getPathVars() {
    if (!m_pathVars.has_value()) {
        auto context = static_cast<detail::EndpointData*>(m_req->user_ctx);
        m_pathVars = URIPathParser::parse(context->uriTemplate, m_req->uri);
    }

    return m_pathVars.value();
}

bool Request::hasPathVar(std::string_view name) {
    auto &vars = getPathVars();
    return vars.contains({name.begin(), name.end()});
}

std::string Request::getQueryStr() const {
    auto size = httpd_req_get_url_query_len(m_req) + 1;
    std::string result(size, '\0');
    httpd_req_get_url_query_str(m_req, result.data(), size);
    return result;
}

std::string Request::getQueryParam(std::string_view queryStr, std::string_view name) {
    char value[CONFIG_HTTP_SERVER_QUERY_VALUE_MAX_LEN] = {0};
    char decValue[CONFIG_HTTP_SERVER_QUERY_VALUE_MAX_LEN] = {0};

    if (httpd_query_key_value(queryStr.data(), name.data(), value, sizeof(value)) != ESP_OK)
        return {};

    URIUtils::decode(decValue, {value, strnlen(value, CONFIG_HTTP_SERVER_QUERY_VALUE_MAX_LEN)});

    return decValue;
}

std::string Request::getQueryParam(std::string_view name) const {
    return getQueryParam(getQueryStr(), name);
}

size_t Request::getContentLength() const {
    return m_req->content_len;
}

int Request::readChunk(Buffer buff) const {
    return httpd_req_recv(m_req, reinterpret_cast<char*>(buff.data()), buff.size());
}

bool Request::isValid() const {
    return m_req != nullptr;
}

Response Request::response() {
    return Response {m_req};
}
}
