#ifndef EXPRESSIF_RESPONSE_H
#define EXPRESSIF_RESPONSE_H

#include <esp_http_server.h>

#include <string_view>
#include <span>
#include <vector>

#include "Buffer.h"

namespace expressif::http::server {
class Response {
public:
    explicit Response(httpd_req_t *&req);

    esp_err_t setHeader(std::string_view header, std::string_view value);
    esp_err_t setType(std::string_view type);
    esp_err_t setStatus(std::string_view status);

    /**
     * Writes buffer and flushes the transmission.
     * @param data The data to be written.
     * @return ESP_OK in case of success
     */
    esp_err_t writeAll(ConstBuffer data);

    template<typename F>
    esp_err_t writeAll(F &&factory);

    /**
     * Writes a single chunk to the stream.
     * @param chunk The chunk to be written.
     * @return ESP_OK in case of success
     */
    esp_err_t writeChunk(ConstBuffer chunk);

    esp_err_t writeChunks(
            ConstBuffer data,
            size_t chunkSize = CONFIG_HTTP_SERVER_CHUNK_SIZE, bool flush = true);

    /**
     * Writes chunks returned by the factory F. If returned chunk is empty,
     * the function finishes.
     * @tparam F An invocable type with 0 args that returns [ConstBuffer]
     * @param factory The factory
     * @param flush If `true`, the transmission will be flushed
     * @return ESP_OK in case of success
     */
    template<typename F>
    esp_err_t writeChunks(F &&factory, bool flush = true);

    /**
     * Writes the data of the specified type. The data may be written in chunks
     * or all at once.
     * @tparam T The type, one of: factory, char*, std::string, std::string_view
     * @param data The data of type [T]
     * @param chunkSize If equals 0, the transmission will be sent in one portion and
     * automatically flushed, regardless of the [flush] value.
     * <br>Otherwise, it will be sent in chunks of [chunkSize] bytes.
     * @param flush If `true`, the transmission will be flushed
     * @return ESP_OK in case of success
     */
    template<typename T>
    esp_err_t write(T &&data, size_t chunkSize = 0, bool flush = true);

    /**
     * Specialized version of HTTPServerResponse::write\<T\>
     * @param str The string to be sent
     * @param chunkSize The chunk size
     * @param flush Whether flush or not
     * @see HTTPServerResponse::write\<T\> for detailed documentation
     * @return ESP_OK in case of success
     */
    esp_err_t write(std::string_view str, size_t chunkSize = 0, bool flush = true);

    /**
     * Flushes the current transmission.
     * @note After the transmission was flushed, in cannot be used any more.
     * You will not even able to read headers/path vars after call to this
     * function. From ESP doc: all request headers are purged, so request headers
     * need be copied into separate buffers if they are required later.
     * @return
     */
    esp_err_t flush();

    esp_err_t error(httpd_err_code_t code, std::string_view message);
    esp_err_t error404();
    esp_err_t error408();
    esp_err_t error500();

private:
    void invalidate();

    template<typename T>
    inline static constexpr bool is_chunk_factory =
            std::is_invocable_r_v<ConstBuffer, T>;

private:
    httpd_req_t *&m_req;
};

template<typename F>
esp_err_t Response::writeAll(F &&factory) {
    static_assert(is_chunk_factory<F>);

    std::vector<byte_t> buffer;

    while (true) {
        if (auto chunk = factory(); chunk.empty()) {
            return writeAll({buffer});
        } else {
            buffer.insert(chunk.begin(), chunk.end());
        }
    }
}

template<typename F>
esp_err_t Response::writeChunks(F &&factory, bool flush) {
    static_assert(is_chunk_factory<F>);

    while (true) {
        if (auto chunk = factory(); chunk.empty()) {
            if (flush)
                return this->flush();
            return ESP_OK;
        } else {
            if (auto ret = writeChunk(chunk); ret != ESP_OK) {
                return ret;
            }
        }
    }
}

template<typename T>
esp_err_t Response::write(T &&data, size_t chunkSize, bool flush) {
    using U = std::decay_t<T>;

    constexpr bool is_string =
            std::is_same_v<U, std::string_view> ||
            std::is_same_v<U, std::string> ||
            std::is_same_v<std::remove_const_t<std::remove_pointer_t<U>>, char>;

    if constexpr (is_chunk_factory<U>) {
        if (chunkSize > 0) {
            return writeChunks(std::forward<T>(data), flush);
        } else {
            return writeAll(std::forward<T>(data));
        }
    } else if constexpr (is_string) {
        if (chunkSize > 0) {
            return writeChunks(toBuffer(data), chunkSize, flush);
        } else {
            return writeAll(toBuffer(data));
        }
    } else {
        // https://stackoverflow.com/a/64354296/9200394
        []<bool flag = false>() {
            static_assert(flag, "Unsupported type");
        }();
        return ESP_FAIL;
    }
}

inline esp_err_t Response::write(std::string_view str, size_t chunkSize, bool flush) {
    return write<>(str, chunkSize, flush);
}
}

#endif //EXPRESSIF_RESPONSE_H
