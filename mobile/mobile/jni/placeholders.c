/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Opera Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

/* Placeholders for functions that haven't been implemented yet. */

#include <stdlib.h>
#include <android/log.h>

#define D(f) void f() { __android_log_assert(#f, "OM", "Placeholder called: %s", #f); }

D(MOpClipboard_create)
D(MOpClipboard_getText)
D(MOpClipboard_hasText)
D(MOpClipboard_placeText)
D(MOpClipboard_release)
D(MOpSystemInfo_getPlatformName)
