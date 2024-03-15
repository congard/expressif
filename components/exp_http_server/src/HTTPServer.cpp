#include <expressif/http/server/HTTPServer.h>
#include <expressif/http/server/util/URIPathParser.h>

#include "detail/EndpointData.h"

#include <algorithm>
#include <esp_log.h>

namespace expressif::http::server {
constexpr static auto TAG = "expressif::http::server::HTTPServer";

HTTPServer::HTTPServer()
    : m_server() {}

HTTPServer::~HTTPServer() {
    stop();
}

decltype(HTTPServer::m_endpoints)::iterator HTTPServer::findEndpointData(HTTPMethod method, std::string_view uri) {
    return std::ranges::find_if(m_endpoints, [=](const detail::EndpointData &data) {
        return data.method == method && URIPathParser::isMatches(data.uriTemplate, uri);
    });
}

esp_err_t HTTPServer::requestHandler(httpd_req_t *nativeRequest) {
    auto server = static_cast<HTTPServer*>(httpd_get_global_user_ctx(nativeRequest->handle));
    auto dataIt = server->findEndpointData(static_cast<HTTPMethod>(nativeRequest->method), nativeRequest->uri);

    Request request(nativeRequest);

    if (dataIt != server->m_endpoints.end()) {
        // ok, handle request
        nativeRequest->user_ctx = dataIt.base();
        return dataIt->handler(request) == HandlerResult::Keep ? ESP_OK : ESP_FAIL;
    } else {
        // error, 404
        auto it404 = server->findErrorHandler(HTTPD_404_NOT_FOUND);

        if (it404 != server->m_errorHandlers.end()) {
            return it404->second(request, HTTPD_404_NOT_FOUND) == HandlerResult::Keep ? ESP_OK : ESP_FAIL;
        } else {
            return request.response().error404();
        }
    }
}

esp_err_t HTTPServer::start(HTTPServer::Config config) {
    config.uri_match_fn = [](auto, auto, auto) { return true; };
    config.global_user_ctx = this;

    if (auto ret = httpd_start(&m_server, &config); ret != ESP_OK)
        return ret;

    auto mkMethodHandler = [](HTTPMethod method) {
        return httpd_uri_t {
            .uri       = "",
            .method    = static_cast<httpd_method_t>(method),
            .handler   = HTTPServer::requestHandler,
            .user_ctx  = nullptr
        };
    };

    auto getHandler = mkMethodHandler(HTTPMethod::Get);
    auto postHandler = mkMethodHandler(HTTPMethod::Post);
    auto putHandler = mkMethodHandler(HTTPMethod::Put);
    auto deleteHandler = mkMethodHandler(HTTPMethod::Delete);

#define registerUriHandler(handler) \
    if (auto ret = httpd_register_uri_handler(m_server, &handler); ret != ESP_OK) { stop(); return ret; }

    registerUriHandler(getHandler)
    registerUriHandler(postHandler)
    registerUriHandler(putHandler)
    registerUriHandler(deleteHandler)

#undef registerUriHandler

    return ESP_OK;
}

bool HTTPServer::stop() {
    if (!isValid())
        return false;

    httpd_stop(m_server);
    m_server = nullptr;

    return true;
}

bool HTTPServer::addEndpoint(
    HTTPMethod method,
    std::string_view uriTemplate,
    EndpointHandler handler
) {
    detail::EndpointData data {method, uriTemplate.data(), std::move(handler)};

    // O(log(n))
    auto index = std::ranges::upper_bound(m_endpoints, data, [](const auto &d1, const auto &d2) {
        return d1.priority > d2.priority;
    });

    // O(n)
    auto &inserted = *m_endpoints.insert(index, std::move(data));

    ESP_LOGD(TAG, "New endpoint: template = %s, priority: %i",
             inserted.uriTemplate.c_str(), inserted.priority);

    return true;
}

bool HTTPServer::removeEndpoint(HTTPMethod method, std::string_view uriTemplate) {
    auto endpointByNamePred = [=](const detail::EndpointData &data) {
        return data.method == method && data.uriTemplate == uriTemplate;
    };

    // delete the corresponding endpoint
    if (auto it = std::ranges::find_if(m_endpoints, endpointByNamePred); it != m_endpoints.end()) {
        m_endpoints.erase(it);
        return true;
    }

    return false;
}

decltype(HTTPServer::m_errorHandlers)::iterator HTTPServer::findErrorHandler(httpd_err_code_t error) {
    return std::ranges::find_if(m_errorHandlers, [error](const auto &handler) {
        return handler.first == error;
    });
}

bool HTTPServer::setErrorHandler(httpd_err_code_t error, ErrorHandler handler) {
    // calls the corresponding ErrorHandler based on context
    auto nativeHandler = [](httpd_req_t *nativeRequest, httpd_err_code_t error) -> esp_err_t {
        auto server = static_cast<HTTPServer*>(httpd_get_global_user_ctx(nativeRequest->handle));

        Request request(nativeRequest);
        auto result = server->findErrorHandler(error)->second(request, error);

        return result == HandlerResult::Keep ? ESP_OK : ESP_FAIL;
    };

    if (httpd_register_err_handler(m_server, error, nativeHandler) != ESP_OK)
        return false;

    // update ErrorHandler vector
    if (auto it = findErrorHandler(error); it != m_errorHandlers.end()) {
        (*it).second = std::move(handler);
    } else {
        m_errorHandlers.emplace_back(error, std::move(handler));
    }

    return true;
}

bool HTTPServer::removeErrorHandler(httpd_err_code_t error) {
    if (httpd_register_err_handler(m_server, error, nullptr) != ESP_OK)
        return false;

    m_errorHandlers.erase(findErrorHandler(error));

    return true;
}

bool HTTPServer::isValid() const {
    return m_server != nullptr;
}
}
