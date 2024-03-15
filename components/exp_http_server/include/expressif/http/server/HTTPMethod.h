#ifndef EXPRESSIF_HTTPMETHOD_H
#define EXPRESSIF_HTTPMETHOD_H

#include <http_parser.h>

namespace expressif::http::server {
enum class HTTPMethod {
    Get = HTTP_GET,
    Post = HTTP_POST,
    Put = HTTP_PUT,
    Delete = HTTP_DELETE
};
}

#endif //EXPRESSIF_HTTPMETHOD_H
