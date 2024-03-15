/*
 * Utility functions for protocol examples
 *
 * SPDX-FileCopyrightText: 2002-2021 Igor Sysoev
 *                         2011-2022 Nginx, Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * SPDX-FileContributor: 2023 Espressif Systems (Shanghai) CO LTD
 */

/*
 * Copyright (C) 2002-2021 Igor Sysoev
 * Copyright (C) 2011-2022 Nginx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <expressif/http/server/util/URIUtils.h>

#include <string>

/* Type of Escape algorithms to be used */
#define NGX_ESCAPE_URI            (0)
#define NGX_ESCAPE_ARGS           (1)
#define NGX_ESCAPE_URI_COMPONENT  (2)
#define NGX_ESCAPE_HTML           (3)
#define NGX_ESCAPE_REFRESH        (4)
#define NGX_ESCAPE_MEMCACHED      (5)
#define NGX_ESCAPE_MAIL_AUTH      (6)

/* Type of Unescape algorithms to be used */
#define NGX_UNESCAPE_URI          (1)
#define NGX_UNESCAPE_REDIRECT     (2)

namespace expressif::http::server {
static uintptr_t ngx_escape_uri(u_char *dst, u_char *src, size_t size, unsigned int type) {
    unsigned int      n;
    uint32_t       *escape;
    static u_char   hex[] = "0123456789ABCDEF";

    /*
     * Per RFC 3986 only the following chars are allowed in URIs unescaped:
     *
     * unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
     * gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
     * sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
     *               / "*" / "+" / "," / ";" / "="
     *
     * And "%" can appear as a part of escaping itself.  The following
     * characters are not allowed and need to be escaped: %00-%1F, %7F-%FF,
     * " ", """, "<", ">", "\", "^", "`", "{", "|", "}".
     */

    /* " ", "#", "%", "?", not allowed */

    static uint32_t   uri[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xd000002d, /* 1101 0000 0000 0000  0000 0000 0010 1101 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x50000000, /* 0101 0000 0000 0000  0000 0000 0000 0000 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /* " ", "#", "%", "&", "+", ";", "?", not allowed */

    static uint32_t   args[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xd800086d, /* 1101 1000 0000 0000  0000 1000 0110 1101 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x50000000, /* 0101 0000 0000 0000  0000 0000 0000 0000 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /* not ALPHA, DIGIT, "-", ".", "_", "~" */

    static uint32_t   uri_component[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc009fff, /* 1111 1100 0000 0000  1001 1111 1111 1111 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /* " ", "#", """, "%", "'", not allowed */

    static uint32_t   html[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x500000ad, /* 0101 0000 0000 0000  0000 0000 1010 1101 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x50000000, /* 0101 0000 0000 0000  0000 0000 0000 0000 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /* " ", """, "'", not allowed */

    static uint32_t   refresh[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x50000085, /* 0101 0000 0000 0000  0000 0000 1000 0101 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x50000000, /* 0101 0000 0000 0000  0000 0000 0000 0000 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xd8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    /* " ", "%", %00-%1F */

    static uint32_t   memcached[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000021, /* 0000 0000 0000 0000  0000 0000 0010 0001 */

        /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

    /* mail_auth is the same as memcached */

    static uint32_t  *map[] =
            { uri, args, uri_component, html, refresh, memcached, memcached };


    escape = map[type];

    if (dst == nullptr) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1U << (*src & 0x1f))) {
                n++;
            }
            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while (size) {
        if (escape[*src >> 5] & (1U << (*src & 0x1f))) {
            *dst++ = '%';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;

        } else {
            *dst++ = *src++;
        }
        size--;
    }

    return (uintptr_t) dst;
}


static void ngx_unescape_uri(u_char **dst, u_char **src, size_t size, unsigned int type) {
    u_char  *d, *s, ch, c, decoded;

    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    d = *dst;
    s = *src;

    state = sw_usual;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
            case sw_usual:
                if (ch == '?'
                    && (type & (NGX_UNESCAPE_URI | NGX_UNESCAPE_REDIRECT))) {
                    *d++ = ch;
                    goto done;
                }

                if (ch == '%') {
                    state = sw_quoted;
                    break;
                }

                *d++ = ch;
                break;

            case sw_quoted:

                if (ch >= '0' && ch <= '9') {
                    decoded = (u_char) (ch - '0');
                    state = sw_quoted_second;
                    break;
                }

                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'f') {
                    decoded = (u_char) (c - 'a' + 10);
                    state = sw_quoted_second;
                    break;
                }

                /* the invalid quoted character */

                state = sw_usual;

                *d++ = ch;

                break;

            case sw_quoted_second:

                state = sw_usual;

                if (ch >= '0' && ch <= '9') {
                    ch = (u_char) ((decoded << 4) + (ch - '0'));

                    if (type & NGX_UNESCAPE_REDIRECT) {
                        if (ch > '%' && ch < 0x7f) {
                            *d++ = ch;
                            break;
                        }

                        *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                        break;
                    }

                    *d++ = ch;

                    break;
                }

                c = (u_char) (ch | 0x20);
                if (c >= 'a' && c <= 'f') {
                    ch = (u_char) ((decoded << 4) + (c - 'a') + 10);

                    if (type & NGX_UNESCAPE_URI) {
                        if (ch == '?') {
                            *d++ = ch;
                            goto done;
                        }

                        *d++ = ch;
                        break;
                    }

                    if (type & NGX_UNESCAPE_REDIRECT) {
                        if (ch == '?') {
                            *d++ = ch;
                            goto done;
                        }

                        if (ch > '%' && ch < 0x7f) {
                            *d++ = ch;
                            break;
                        }

                        *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                        break;
                    }

                    *d++ = ch;

                    break;
                }

                /* the invalid quoted character */

                break;
        }
    }

    done:

    *dst = d;
    *src = s;
}

void URIUtils::decode(char *dest, std::string_view src) {
    if (src.empty() || !dest) {
        return;
    }

    auto src_ptr = (unsigned char*) src.data();
    auto dst_ptr = (unsigned char*) dest;

    ngx_unescape_uri(&dst_ptr, &src_ptr, src.size(), NGX_UNESCAPE_URI);
}

std::string URIUtils::decode(std::string_view src) {
    std::string result(src.size(), '\0');
    decode(result.data(), src);

    // remove extra \0 from the string (note: capacity will remain unchanged)
    if (auto index = result.find('\0'); index != std::string::npos)
        result.erase(index);

    return result;
}

uint32_t URIUtils::encode(char *dest, std::string_view src) {
    if (src.empty() || !dest) {
        return 0;
    }

    uintptr_t ret = ngx_escape_uri(
            (unsigned char*) dest,
            (unsigned char*) src.data(),
            src.size(),
            NGX_ESCAPE_URI_COMPONENT);

    return (uint32_t)(ret - (uintptr_t)dest);
}

std::string URIUtils::encode(std::string_view src) {
    std::string result(src.size() * 3, '\0');
    auto count = encode(result.data(), src);
    return result.erase(count);
}
}
