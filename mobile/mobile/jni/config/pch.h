/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef CONFIG_PCH_H
#define CONFIG_PCH_H

#ifdef __cplusplus
#  define OP_EXTERN_C extern "C"
#  define OP_EXTERN_C_BEGIN extern "C" {
#  define OP_EXTERN_C_END }
#else
#  define OP_EXTERN_C extern
#  define OP_EXTERN_C_BEGIN
#  define OP_EXTERN_C_END
#endif

#ifdef __GNUC__
# define UNUSED __attribute__((unused))
#else
# define UNUSED
#endif
#ifdef NDEBUG
# define NDEBUG_UNUSED UNUSED
#else
# define NDEBUG_UNUSED
#endif

OP_EXTERN_C_BEGIN

#define MOP_INTEGRATION

#undef YES
#define YES 1

#undef NO
#define NO 0

#if defined(__arm__) || defined(__thumb__)
# define ARCHITECTURE_ARM
#elif defined(__i386__)
# define ARCHITECTURE_IA32
#elif defined(__mips__)
# define ARCHITECTURE_MIPS
#endif // defined(__mips__)

#include "system.h"
#include "system_stdlib.h"
#include "features.h"

typedef struct _GlobalMemory GlobalMemory;
extern GlobalMemory g_global_memory;
static inline GlobalMemory *GlobalMemory_get()
{
    return &g_global_memory;
}

#include "core/hardcore/base.h"
#include "core/hardcore/baseIncludes.h"
#include "core/stdlib/stdlib.h"

#include "generic/api/base.h"
#include "generic/api/baseIncludes.h"

#include "core/pi/Debug.h"

static inline MOP_STATUS GlobalMemory_alloc()
{
    return MOP_STATUS_OK;
}

static inline void GlobalMemory_free()
{
}

OP_EXTERN_C_END

#endif /* CONFIG_PCH_H */
