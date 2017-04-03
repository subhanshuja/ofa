// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_VLINKS_VLINK4_TAGS_H_
#define COMMON_MIGRATION_VLINKS_VLINK4_TAGS_H_

namespace opera {
namespace common {
namespace migration {

const uint8_t TAG_VISITED_FILE_ENTRY = 0x0002;  // Record sequence
const uint8_t TAG_URL_NAME = 0x0003;  // string
const uint8_t TAG_LAST_VISITED = 0x0004;  // 32bit unsigned time
const uint8_t TAG_RELATIVE_ENTRY = 0x0022;  // Sequence of TAG_RELATIVEentries
const uint8_t TAG_RELATIVE_NAME = 0x0023;  // string
const uint8_t TAG_RELATIVE_VISITED = 0x0024;  // 32bit unsigned, truncated

}  // namespace migration
}  // namespace common
}  // namespace opera
#endif  // COMMON_MIGRATION_VLINKS_VLINK4_TAGS_H_
