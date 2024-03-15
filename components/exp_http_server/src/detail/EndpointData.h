#ifndef EXPRESSIF_ENDPOINTDATA_H
#define EXPRESSIF_ENDPOINTDATA_H

#include <string>
#include "../../include/expressif/http/server/EndpointHandler.h"
#include "../../include/expressif/http/server/HTTPMethod.h"
#include "../../include/expressif/http/server/util/URIPathParser.h"

namespace expressif::http::server {
class HTTPServer;

namespace detail {
class EndpointData {
public:
    inline EndpointData(HTTPMethod method, std::string_view tmp, EndpointHandler handler)
        : method(method),
          uriTemplate(tmp),
          handler(std::move(handler)),
          priority(URIPathParser::calcPriority(tmp)) {}

public:
    HTTPMethod method;
    std::string uriTemplate;
    EndpointHandler handler;
    ssize_t priority;
};
}
}

#endif //EXPRESSIF_ENDPOINTDATA_H
