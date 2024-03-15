#ifndef EXPRESSIF_ENDPOINTHANDLER_H
#define EXPRESSIF_ENDPOINTHANDLER_H

#include <functional>

#include "Request.h"
#include "HandlerResult.h"

namespace expressif::http::server {
using EndpointHandler = std::function<HandlerResult(Request&)>;

namespace detail {
template<typename T>
using is_complete_endpoint_handler =
        std::is_invocable_r<EndpointHandler::result_type, T, EndpointHandler::argument_type>;

template<typename T>
using is_partial_endpoint_handler =
        std::is_invocable_r<void, T, EndpointHandler::argument_type>;

template<typename T>
constexpr static bool is_complete_endpoint_handler_v = is_complete_endpoint_handler<T>::value;

template<typename T>
constexpr static bool is_partial_endpoint_handler_v = is_partial_endpoint_handler<T>::value;
}
}

#endif //EXPRESSIF_ENDPOINTHANDLER_H
