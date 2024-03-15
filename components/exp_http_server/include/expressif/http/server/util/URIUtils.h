#ifndef EXPRESSIF_URIUTILS_H
#define EXPRESSIF_URIUTILS_H

#include <cstddef>
#include <sys/types.h>

#include <string_view>

namespace expressif::http::server {
class URIUtils {
public:
    /**
     * @brief Encode an URI
     *
     * @param dest       a destination memory location
     * @param src        the source string
     * @return uint32_t  the count of escaped characters
     *
     * @note Please allocate the destination buffer keeping in mind that encoding a
     *       special character will take up 3 bytes (for '%' and two hex digits).
     *       In the worst-case scenario, the destination buffer will have to be 3 times
     *       that of the source string.
     */
    static uint32_t encode(char *dest, std::string_view src);

    /**
     * Encodes an URI. Allocates buffer 3 times bigger than the source string.
     * @param src The source string
     * @return An encoded string
     */
    static std::string encode(std::string_view src);

    /**
     * @brief Decode an URI
     *
     * @param dest  a destination memory location
     * @param src   the source string
     *
     * @note Please allocate the destination buffer keeping in mind that a decoded
     *       special character will take up 2 less bytes than its encoded form.
     *       In the worst-case scenario, the destination buffer will have to be
     *       the same size that of the source string.
     */
    static void decode(char *dest, std::string_view src);

    /**
     * Decodes an URI. Allocates buffer of the same size as the source string.
     * @see URIUtils::decode(char*, std::string_view)
     * @param src The source string
     * @return A decoded string
     */
    static std::string decode(std::string_view src);
};
}

#endif //EXPRESSIF_URIUTILS_H
