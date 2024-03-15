#ifndef EXPRESSIF_URIPATHPARSER_H
#define EXPRESSIF_URIPATHPARSER_H

#include <cstddef>

#include "PathVars.h"

namespace expressif::http::server {
/**
 * <h1>URIPathParser</h1>
 *
 * Parses uri path based on template, e.g.:
 *   template: /user/{userId}/group/{groupId}
 *   uri:      /user/1234abcd/group/qwerty123
 * Results in:
 *   userId:  1234abcd
 *   groupId: qwerty123
 *
 * <h2>Valid templates:</h2>
 * <ul>
 *   <li>/foo/{p1}/bar</li>
 *   <li>/{p1}/foo/{p2}</li>
 *   <li>/foo/bar/{p1} - matches '/foo/bar/value1',
 *                       <b>but not</b> '/foo/bar/value1/.../value_n'</li>
 *   <li>/foo/{args}*  - matches '/foo/bar/value1/.../value_n',
 *                       <b>but not</b> '/foo/bar/value1',
 *                       since this template has the lower priority than the previous one</li>
 *   <li>/foo</li>
 *   <li>etc.</li>
 * </ul>
 *
 * <h2>Invalid templates:</h2>
 * <ul>
 *   <li>foo - must be /foo</li>
 *   <li>/foo/{args}* /bar - variadic variable can only be specified in the end, i.e.
 *       /foo/bar/{args}*</li>
 *   <li>/foo/ - must be /foo</li>
 * </ul>
 *
 * <h2>Notes</h2>
 * <ul>
 *   <li>if your template contains the characters that need to be
 *       encoded, encode them first</li>
 * </ul>
 */
class URIPathParser {
public:
    /**
     * Calculates the priority of the specified template.
     *
     * <br>For example:
     * <ul>
     *   <li>p('/') == 1</li>
     *   <li>p('/foo') == 1</li>
     *   <li>p('/foo/bar') == 2</li>
     *   <li>p('/baz/{arg}') == 2</li>
     *   <li><b>but</b> p('/qux/{args}*') == 1</li>
     * </ul>
     *
     * <br>That means, that if you have 3 different templates:
     * <ol>
     *   <li>/foo/bar</li>
     *   <li>/foo/{args}*</li>
     *   <li>/{args}*</li>
     * </ol>
     *
     * URI /foo/bar will use the handler registered for the 1st template,
     * /foo/bar/baz will use the handler registered for the 2nd template,
     * and the rest of URIs will use the 3rd handler (of priority 0)
     *
     * @param tmp The template to calculate priority for
     * @return The priority
     */
    static ssize_t calcPriority(std::string_view tmp);

    static bool isMatches(std::string_view uriTemplate, std::string_view uri);
    static PathVars parse(std::string_view uriTemplate, std::string_view uri);

private:
    URIPathParser() = default;
};
}

#endif //EXPRESSIF_URIPATHPARSER_H
