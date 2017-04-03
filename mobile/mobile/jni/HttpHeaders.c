/*
 * Copyright (C) 2013 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */


/* Should move to platform.cpp, moved to here to bypass warnings. extern "C" is not enough */

#include "pch.h"

#ifdef MOP_FEATURE_HTTP_PLATFORM_PROXY
#include "platform.h"

#include "mini/c/shared/core/encoding/tstring.h"
#include "mini/c/shared/core/pi/HttpHeaders.h"
MOP_STATUS MOpHttpProtocol_getPlatformProxy(BOOL * useProxy, TCHAR ** httpProxy, BOOL *freeProxy, TCHAR ** userName,
                                            BOOL * freeUser, TCHAR ** pass, BOOL * freePass)
{
    TCHAR *proxy = getHTTPProxy();
    BOOL _useProxy = (proxy != NULL && mop_tstrlen(proxy) > 0);

#define SET_IF_NOT_NULL(x, val) if ((x)) *(x) = (val)

    SET_IF_NOT_NULL(userName, T(""));
    SET_IF_NOT_NULL(freeUser, FALSE);
    SET_IF_NOT_NULL(pass, T(""));
    SET_IF_NOT_NULL(freePass, FALSE);

    SET_IF_NOT_NULL(useProxy, _useProxy);
    SET_IF_NOT_NULL(httpProxy, _useProxy ? proxy : T(""));
    SET_IF_NOT_NULL(freeProxy, _useProxy);
#undef SET_IF_NOT_NULL

    return MOP_STATUS_OK;
}
#endif /* MOP_FEATURE_HTTP_PLATFORM_PROXY */
