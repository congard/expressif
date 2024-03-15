#ifndef EXPRESSIF_HTTPSOCKETERROR_H
#define EXPRESSIF_HTTPSOCKETERROR_H

#include <esp_http_server.h>

namespace expressif::http::server {
enum class HTTPSocketError {
    Fail = HTTPD_SOCK_ERR_FAIL,
    Invalid = HTTPD_SOCK_ERR_INVALID,
    Timeout = HTTPD_SOCK_ERR_TIMEOUT
};
}

#endif //EXPRESSIF_HTTPSOCKETERROR_H
