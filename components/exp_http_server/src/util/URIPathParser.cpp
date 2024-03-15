#include <expressif/http/server/util/URIPathParser.h>
#include <expressif/http/server/util/URIUtils.h>

#include <algorithm>

namespace expressif::http::server {
ssize_t URIPathParser::calcPriority(std::string_view tmp) {
    if (tmp.empty() || tmp.front() != '/' || tmp.back() == '/')
        return -1;

    ssize_t priority = std::ranges::count(tmp, '/');

    // check if the last argument is not a vararg
    auto pos = tmp.find_last_of('/') + 1;

    if (tmp.size() - pos >= 3 && tmp[pos] == '{' && tmp.substr(tmp.size() - 2) == "}*")
        priority -= 1;

    return priority;
}

/**
 * Parses the specified uri using the specified template
 * @param uriTemplate The template
 * @param uri The uri to parse
 * @param result The variable to write result in (may be null)
 * @return `true` if the uri matches template, `false` otherwise
 */
static bool parse(
        std::string_view uriTemplate,
        std::string_view uri,
        PathVars *result = nullptr
) {
    auto tmpLen = uriTemplate.size();
    auto uriLen = uri.size();

    if (tmpLen == 0 || uriLen == 0)
        return false;

    if (uriTemplate.front() != '/' || uri.front() != '/')
        return false;

    size_t tmpPos = 0;
    size_t uriPos = 0;

    while (true) {
        // skip '/'
        tmpPos = std::min(tmpPos + 1, tmpLen);
        uriPos = std::min(uriPos + 1, uriLen);

        auto tmpPathEnd = std::min(uriTemplate.find('/', tmpPos), tmpLen);
        auto uriPathEnd = std::min(uri.find('/', uriPos), uriLen);

        // both reached an end
        if (tmpPos == tmpPathEnd && uriPos == uriPathEnd)
            return true;

        std::string_view tmpPath {&uriTemplate[tmpPos], &uriTemplate[tmpPathEnd]};
        std::string_view uriPath {&uri[uriPos], &uri[uriPathEnd]};

        if (tmpPath.front() == '{') {
            int keyTailLen;

            if (tmpPath.back() == '}') {
                // path variable is obligatory, but uri has reached the end
                if (uriPos == uriLen)
                    return false;

                keyTailLen = 1; // '}'
            } else if (tmpPath.back() == '*' && tmpPathEnd == tmpLen) {
                keyTailLen = 2; // "}*"
                uriPathEnd = uriLen;
                uriPath = {&uri[uriPos], &uri[uriPathEnd]};
            } else {
                // invalid template
                return false;
            }

            if (result != nullptr) {
                std::string key {tmpPath.begin() + 1, tmpPath.end() - keyTailLen};
                std::string val {URIUtils::decode(uriPath)};
                (*result)[std::move(key)] = std::move(val);
            }
        } else if (tmpPath != uriPath) {
            return false;
        }

        tmpPos = tmpPathEnd;
        uriPos = uriPathEnd;
    }
}

inline static auto removeQuery(std::string_view uri) {
    return uri.substr(0, uri.find('?'));
}

bool URIPathParser::isMatches(std::string_view uriTemplate, std::string_view uri) {
    return server::parse(uriTemplate, removeQuery(uri));
}

PathVars URIPathParser::parse(std::string_view uriTemplate, std::string_view uri) {
    if (PathVars result; server::parse(uriTemplate, removeQuery(uri), &result))
        return result;
    return {};
}
}
