#ifndef EXPRESSIF_REQUEST_H
#define EXPRESSIF_REQUEST_H

#include <esp_http_server.h>

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <span>

#include "util/PathVars.h"
#include "HTTPSocketError.h"
#include "Response.h"

namespace expressif::http::server {
class Request {
public:
    explicit Request(httpd_req_t *req);

    std::string getHeader(std::string_view name) const;
    bool hasHeader(std::string_view name) const;

    /**
     * Returns the specified path variable.
     * @param name The name of the path variable.
     * @return The path variable's value.
     * @see [URIPathParser] for more details
     */
    const std::string& getPathVar(std::string_view name);

    /**
     * @return The path variables.
     * @see [URIPathParser] for more details
     */
    const PathVars& getPathVars();

    bool hasPathVar(std::string_view name);

    std::string getQueryParam(std::string_view name) const;

    template<typename... Args>
    std::map<std::string, std::string> getQueryParams(Args &&...args) const;

    size_t getContentLength() const;

    int readChunk(Buffer buff) const;

    template<size_t L = CONFIG_HTTP_SERVER_CHUNK_SIZE, typename C, typename E>
    void readChunks(C &&onRead, E &&onError) const;

    template<size_t L = CONFIG_HTTP_SERVER_CHUNK_SIZE, typename C>
    void readChunks(C &&onRead) const;

    template<size_t L = CONFIG_HTTP_SERVER_CHUNK_SIZE>
    std::optional<std::vector<int8_t>> readAll() const;

    bool isValid() const;

    Response response();

private:
    std::string getQueryStr() const;

    static std::string getQueryParam(std::string_view queryStr, std::string_view name) ;

private:
    httpd_req_t *m_req;

private:
    std::optional<PathVars> m_pathVars;
};

template<typename ...Args>
std::map<std::string, std::string> Request::getQueryParams(Args &&...args) const {
    std::map<std::string, std::string> result;
    auto queryStr = getQueryStr();

    // https://en.cppreference.com/w/cpp/language/fold
    ([&] {
        result[args] = getQueryParam(queryStr, args);
    } (), ...);

    return result;
}

template<size_t L, typename C, typename E>
void Request::readChunks(C &&onRead, E &&onError) const {
    using ChunkBuffer = std::array<byte_t, L>;

    static_assert(std::is_invocable_r_v<void, C, Buffer>);
    static_assert(std::is_invocable_r_v<bool, E, HTTPSocketError>);
    static_assert(L > 0);

    ChunkBuffer buff;

    auto remaining = static_cast<ssize_t>(m_req->content_len);

    while (remaining > 0) {
        if (int ret = readChunk({buff.data(), std::min<size_t>(remaining, L)}); ret <= 0) {
            // error
            if (onError(static_cast<HTTPSocketError>(ret))) {
                continue;
            } else {
                return;
            }
        } else {
            onRead(Buffer(buff.begin(), ret));
            remaining -= ret;
        }
    }
}

template<size_t L, typename C>
void Request::readChunks(C &&onRead) const {
    readChunks<L>(std::forward<C>(onRead), [](HTTPSocketError) {
        return false;
    });
}

template<size_t L>
std::optional<std::vector<int8_t>> Request::readAll() const {
    std::vector<int8_t> result;
    result.reserve(L);

    bool error = false;

    readChunks<L>([&result](Buffer chunk) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }, [&error](HTTPSocketError) {
        error = true;
        return false;
    });

    using R = std::invoke_result_t<decltype(&Request::readAll<L>), Request>;

    return error ? R {} : result;
}
}

#endif //EXPRESSIF_REQUEST_H
