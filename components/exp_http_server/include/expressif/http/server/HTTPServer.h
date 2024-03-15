#ifndef EXPRESSIF_HTTPSERVER_H
#define EXPRESSIF_HTTPSERVER_H

#include "Request.h"
#include "HTTPMethod.h"
#include "EndpointHandler.h"

#include <functional>
#include <memory>

namespace expressif::http::server {
namespace detail {
class EndpointData;
}

class HTTPServer {
public:
    using Config = httpd_config_t;

    using ErrorHandler = std::function<HandlerResult(Request&, httpd_err_code_t)>;

public:
    HTTPServer();
    ~HTTPServer();

    esp_err_t start(Config config = HTTPD_DEFAULT_CONFIG());
    bool stop();

    bool addEndpoint(HTTPMethod method, std::string_view uriTemplate, EndpointHandler handler);

    // TODO: doc
    template<typename T>
    bool addEndpoint(HTTPMethod method, std::string_view uriTemplate, T &&handler);

    bool removeEndpoint(HTTPMethod method, std::string_view uriTemplate);

    bool setErrorHandler(httpd_err_code_t error, ErrorHandler handler);
    bool removeErrorHandler(httpd_err_code_t error);

    bool isValid() const;

private:
    // calls the corresponding EndpointHandler based on method and uri
    static esp_err_t requestHandler(httpd_req_t *nativeRequest);

private:
    httpd_handle_t m_server;

private:
    // sorted by priority in descending order
    std::vector<detail::EndpointData> m_endpoints;

    decltype(m_endpoints)::iterator findEndpointData(HTTPMethod method, std::string_view uri);

private:
    // std::vector instead of std::map to reduce memory usage
    std::vector<std::pair<httpd_err_code_t, ErrorHandler>> m_errorHandlers;

    decltype(m_errorHandlers)::iterator findErrorHandler(httpd_err_code_t error);
};

template<typename T>
bool HTTPServer::addEndpoint(HTTPMethod method, std::string_view uriTemplate, T &&handler) {
    if constexpr (detail::is_complete_endpoint_handler_v<T>) {
        return addEndpoint(method, uriTemplate, EndpointHandler {std::forward<T>(handler)});
    } else if constexpr (detail::is_partial_endpoint_handler_v<T>) {
        EndpointHandler endpoint = [handler = std::forward<T>(handler)](Request &req) {
            handler(req);
            return HandlerResult::Keep;
        };
        return addEndpoint(method, uriTemplate, std::move(endpoint));
    } else {
        // https://stackoverflow.com/a/64354296/9200394
        []<bool flag = false>() {
            static_assert(flag, "Unsupported handler's signature");
        }();
        return false;
    }
}
}

#endif //EXPRESSIF_HTTPSERVER_H
