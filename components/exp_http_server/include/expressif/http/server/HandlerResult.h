#ifndef EXPRESSIF_HANDLERRESULT_H
#define EXPRESSIF_HANDLERRESULT_H

namespace expressif::http::server {
enum class HandlerResult {
    Discard,
    Keep
};
}

#endif //EXPRESSIF_HANDLERRESULT_H
