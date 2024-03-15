#ifndef EXPRESSIF_BUFFER_H
#define EXPRESSIF_BUFFER_H

#include <span>
#include <string>
#include <string_view>

#include <cstdint>

namespace expressif::http::server {
using byte_t = int8_t;

namespace detail {
template<bool isConst>
struct Buffer;

template<> struct Buffer<true> {
    using type = std::span<const byte_t>;
};

template<> struct Buffer<false> {
    using type = std::span<byte_t>;
};

template<bool isConst>
using Buffer_t = Buffer<isConst>::type;
}

using Buffer = detail::Buffer_t<false>;
using ConstBuffer = detail::Buffer_t<true>;

inline ConstBuffer toBuffer(std::string_view str) {
    return {reinterpret_cast<ConstBuffer::element_type*>(str.data()), str.size()};
}

inline ConstBuffer toBuffer(const char *str) {
    return toBuffer(std::string_view {str});
}

inline ConstBuffer toBuffer(const std::string &str) {
    return toBuffer(std::string_view {str});
}
}

#endif //EXPRESSIF_BUFFER_H
