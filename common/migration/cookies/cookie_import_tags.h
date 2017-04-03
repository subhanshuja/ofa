// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIES_COOKIE_IMPORT_TAGS_H_
#define COMMON_MIGRATION_COOKIES_COOKIE_IMPORT_TAGS_H_
#include <stdint.h>

namespace opera {
namespace common {
namespace migration {

const uint8_t MSB_VALUE = 0x80;
const uint8_t TAG_COOKIE_DOMAIN_ENTRY = 0x0001;  // Record sequence
const uint8_t TAG_COOKIE_PATH_ENTRY = 0x0002;  // Record sequence
const uint8_t TAG_COOKIE_ENTRY = 0x0003;  // Record sequence
// no content, End of domainsequence
const uint8_t TAG_COOKIE_DOMAIN_END = (0x0004 | MSB_VALUE);
// no content, End of path sequence
const uint8_t TAG_COOKIE_PATH_END = (0x0005 | MSB_VALUE);

const uint8_t TAG_COOKIE_NAME = 0x0010;  // string
const uint8_t TAG_COOKIE_VALUE = 0x0011;  // string
const uint8_t TAG_COOKIE_EXPIRES = 0x0012;  // unsigned, truncated
const uint8_t TAG_COOKIE_LAST_USED = 0x0013;  // unsigned, truncated
const uint8_t TAG_COOKIE_COMMENT = 0x0014;  // string
const uint8_t TAG_COOKIE_COMMENT_URL = 0x0015;  // string
const uint8_t TAG_COOKIE_RECVD_DOMAIN = 0x0016;  // string
const uint8_t TAG_COOKIE_RECVD_PATH = 0x0017;  // string
const uint8_t TAG_COOKIE_PORT = 0x0018;  // string
const uint8_t TAG_COOKIE_SECURE = (0x0019 | MSB_VALUE);  // True if present
const uint8_t TAG_COOKIE_VERSION = 0x001A;  // unsigned, truncated
const uint8_t TAG_COOKIE_ONLY_SERVER = (0x001B | MSB_VALUE);  // True if present
const uint8_t TAG_COOKIE_PROTECTED = (0x001C | MSB_VALUE);  // True if present
const uint8_t TAG_COOKIE_PATH_NAME = 0x001D;  // string
const uint8_t TAG_COOKIE_DOMAIN_NAME = 0x001E;  // string
const uint8_t TAG_COOKIE_SERVER_ACCEPT_FLAG = 0x001F;  // 8 bit value, values are:
        // 1 All cookies from this domain are accepted
        // 2 No cookies from this domain are accepted
        // 3 All cookies from this server are accepted.
        // Overrides 1 and 2 for higher level domains automatics (show cookie will work)
        // 4 No cookies from this server are accepted.
        // Overrides 1 and 2 for higher level domains (show cookie will work)
// True if present (Cookie not to be used for prefix path matches)
const uint8_t TAG_COOKIE_NOT_FOR_PREFIX = (0x0020 | MSB_VALUE);
const uint8_t TAG_COOKIE_DOMAIN_ILLPATH = 0x0021;  // 8bit value, values are
        // 1 Cookies with illegal paths are refused automatically for this domain
        // 2 Cookies with illegal paths are accepted automatically for this domain
const uint8_t TAG_COOKIE_HAVE_PASSWORD = (0x0022 | MSB_VALUE);  // True is present
const uint8_t TAG_COOKIE_HAVE_AUTHENTICATION = (0x0023 | MSB_VALUE);  // True is present
const uint8_t TAG_COOKIE_ACCEPTED_AS_THIRDPARTY = (0x0024 | MSB_VALUE);  // True is present
const uint8_t TAG_COOKIE_SERVER_ACCEPT_THIRDPARTY = 0x0025;  // 8 bit value, values are:
        // 1 All thirdparty cookies from this domain are accepted
        // 2 No thirdpartycookies from this domain are accepted
        // 3 All thirdparty cookies from this server are accepted.
        // Overrides 1 and 2 for higher level domains automatics (show cookie will work)
        // 4 No thirdparty cookies from this server are accepted.
        // Overrides 1 and 2 for higher level domains (show cookie will work)
// 8 bit value, values are:
const uint8_t TAG_COOKIE_DELETE_COOKIES_ON_EXIT = 0x0026;
        // 1 No cookies in this domain is to be discarded on exit by default
        // 2 All cookies in this domain is to be discarded on exit by default
const uint8_t TAG_COOKIE_HTTP_ONLY = (0x0027 | MSB_VALUE);  // True if present
const uint8_t TAG_COOKIE_LAST_SYNC = 0x0028;  // unsigned, truncated
// True if the cookie is assigned. For example, it's TRUE for "c=3" and "c=", but not for "c"
const uint8_t TAG_COOKIE_ASSIGNED = (0x0029 | MSB_VALUE);

}  // namespace migration
}  // namespace common
}  // namespace opera


#endif  // COMMON_MIGRATION_COOKIES_COOKIE_IMPORT_TAGS_H_
