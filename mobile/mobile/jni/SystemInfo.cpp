/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <sys/time.h>
#include <time.h>

#include "jni_obfuscation.h"
#include "platform.h"
#include "SystemInfo.h"
#include "com_opera_android_browser_obml_Platform.h"

extern "C" {

#include "core/encoding/tstring.h"
#include "core/pi/BootStage.h"
#include "core/pi/SystemInfo.h"
#include "core/protocols/obsp/ObspProperties.h"
#include "core/settings/SettingsManager.h"
#include "generic/pi/GenericSystemInfo.h"

}

static jmethodID g_platform_handleSpecialLink;
static jmethodID g_platform_getMCC;
static jmethodID g_platform_getMNC;
static jmethodID g_platform_getExternalCookie;

static char *g_referrer;
static char *g_branding;
static char *g_dist_source;

void initSystemInfo(JNIEnv *env) {
#define M(n) g_platform_##n = env->GetStaticMethodID(g_platform_class, M_com_opera_android_browser_obml_Platform_##n##_ID)
  M(handleSpecialLink);
  M(getMCC);
  M(getMNC);
  M(getExternalCookie);
}

UINT32 MOpSystemInfo_getDeviceFormFactor()
{
#ifdef MOP_HAVE_IS_TABLET
    if (MOpGenSystemInfo_isTablet())
    {
        return OBSP_REQUEST_FORM_FACTOR_TABLET;
    }
#endif /* MOP_HAVE_IS_TABLET */
    return OBSP_REQUEST_FORM_FACTOR_MOBILE;
}

UINT32 MOpSystemInfo_getTickCount()
{
    struct timeval now;
    if (gettimeofday(&now, NULL) == 0)
    {
        return (UINT32)now.tv_sec * 1000 + (UINT32)now.tv_usec / 1000;
    }
    return 0;
}

UINT32 MOpSystemInfo_getTimeFromEpoch()
{
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return (UINT32) tm.tv_sec;
}

MOP_STATUS MOpSystemInfo_getTimeZoneName(SCHAR ** name, BOOL * del)
{
    time_t ltime;
    struct tm *processed;
    char str[256];

#ifdef UR_ALLOW_TZ_OVERRIDE
    if (system_info.time_zone_override[0])
    {
        *name = system_info.time_zone_override;
        *del = FALSE;
        return MOP_STATUS_OK;
    }
#endif // UR_ALLOW_TZ_OVERRIDE

    ltime = time (NULL);
    if (ltime == (time_t) -1)
        return MOP_STATUS_ERR;

    processed = localtime (&ltime);
    if (!processed)
        return MOP_STATUS_ERR;

    if (strftime (str, sizeof (str), "%Z", processed) == 0)
        return MOP_STATUS_ERR;

    *name = mop_strdup (str);
    *del = TRUE;

    return *name ? MOP_STATUS_OK : MOP_STATUS_NOMEM;
}

INT32 MOpGenSystemInfo_getLocalTimeOffset()
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday (&tv, &tz);
    return -tz.tz_minuteswest * 60;
}

UINT64 MOpGenSystemInfo_getMSFromEpoch()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (UINT64)tv.tv_sec * 1000 + (UINT64)tv.tv_usec / 1000;
}

BOOL MOpSystemInfo_isTouch()
{
    return TRUE;
}

#ifdef MOP_HAVE_DEBUG
#include <android/log.h>

void MOpDebug_print(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Opera", format, ap);
    va_end(ap);
}

#ifdef MOP_HAVE_DEBUG_EX
void MOpDebugEx_print(const char* file, unsigned int line, MOpDebugScope scope,
                      MOpDebugSeverity severity, const char* format, ...)
{
    (void)file, (void)line, (void)scope, (void)severity;
    va_list ap;
    va_start(ap, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Opera", format, ap);
    va_end(ap);
}
#endif

void MOpBootStage(INT32 stage, MOP_STATUS status)
{
    MOpDebug_print("Boot stage %d: status %d", stage, status);
}
#endif /* MOP_HAVE_DEBUG */

UINT32 MOpSystemInfo_getMaxTilesPerFrame(void)
{
    return 1;
}

void MOpSystemInfo_getRandom(UINT8 * buf, UINT32 bufSize)
{
    // /dev/urandom is supposed to always be properly seeded on Android, so we
    // don't really have to worry about it running dry with a known state.
    // That means we shouldn't need to use /dev/random and risk blocking.
    static FILE* randomFile = fopen("/dev/urandom", "rb");
    if (randomFile)
    {
        size_t nread = fread(buf, 1, bufSize, randomFile);
        bufSize -= nread;
        buf += nread;
    }
    // OK, not very random. But this should only get used if the above fails.
    while (bufSize--)
    {
        *buf++ = random() % 256;
    }
}

#if defined(MOP_FEATURE_SPECIALLINK) || defined(MOP_FEATURE_PLATFORM_REQUEST)
BOOL MOpSystemInfo_handleNormalLink(const TCHAR *link UNUSED) {
  return FALSE;
}

BOOL MOpSystemInfo_handleSpecialLink(const TCHAR *link) {
  if (link == NULL)
    return FALSE;

  JNIEnv *env = getEnv();
  scoped_localref<jstring> jlink = NewStringUTF8(env, link);
  return env->CallStaticBooleanMethod(
      g_platform_class, g_platform_handleSpecialLink, *jlink);
}
#endif /* defined(MOP_FEATURE_SPECIALLINK) ||
    defined(MOP_FEATURE_PLATFORM_REQUEST)*/

UINT32 MOpSystemInfo_getMCC() {
  return getEnv()->CallStaticIntMethod(g_platform_class, g_platform_getMCC);
}

UINT32 MOpSystemInfo_getMNC() {
  return getEnv()->CallStaticIntMethod(g_platform_class, g_platform_getMNC);
}

void MOpSystemInfo_getScreenDPI(UINT16 *dpiX, UINT16 *dpiY) {
  *dpiX = g_screen_dpi_x;
  *dpiY = g_screen_dpi_y;
}

void MOpSystemInfo_getScreenDimensions(UINT16 *width, UINT16 *height) {
  if (width) {
    *width = g_screen_width;
  }
  if (height) {
    *height = g_screen_height;
  }
}

MOP_STATUS MOpSystemInfo_getPhoneUserAgentName(SCHAR **name, BOOL *del) {
  MOP_ASSERT(g_user_agent);
  *name = const_cast<SCHAR*>(g_user_agent);
  *del = FALSE;
  return MOP_STATUS_OK;
}

#ifdef MOP_TWEAK_GET_BRANDING
const TCHAR* MOpSystemInfo_getBranding()
{
  return g_branding;
}
#endif /* MOP_TWEAK_GET_BRANDING */

#ifdef MOP_HAVE_AU_GETCLIENTDISTRIBUTIONSOURCE
TCHAR* MOpSystemInfo_getClientDistributionSource()
{
  return mop_strdup(g_dist_source);
}
#endif /* MOP_HAVE_AU_GETCLIENTDISTRIBUTIONSOURCE */

#ifdef MOP_HAVE_GETINSTALLREFERRER
TCHAR* MOpSystemInfo_getPlatformReferrer(BOOL* freeIt)
{
  *freeIt = FALSE;
  return g_referrer;
}
#endif /* MOP_HAVE_GETINSTALLREFERRER */

#ifdef MOP_HAVE_IS_TABLET
BOOL MOpGenSystemInfo_isTablet() {
  return g_is_tablet;
}
#endif /* MOP_HAVE_IS_TABLET */

TCHAR * MOpSystemInfo_getPlatformCookie(BOOL *del)
{
    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
            env, (jstring) env->CallStaticObjectMethod(g_platform_class,
                                                       g_platform_getExternalCookie));
    if (!str) return NULL;
    TCHAR *value = mop_fromutf8(str.string(), -1);
    if (!value) return NULL;
    *del = TRUE;
    return value;
}

void Java_com_opera_android_browser_obml_Platform_setReferrer(
    JNIEnv *env, jclass, jstring referrer) {
  if (g_referrer) {
    free(g_referrer);
  }
  g_referrer = GetStringUTF8Chars(env, referrer).pass();
}

void Java_com_opera_android_browser_obml_Platform_setBranding(
    JNIEnv *env, jclass, jstring branding) {
  if (g_branding) {
    free(g_branding);
  }
  g_branding = GetStringUTF8Chars(env, branding).pass();
}

void Java_com_opera_android_browser_obml_Platform_setDistSource(
    JNIEnv *env, jclass, jstring dist_source) {
  if (g_dist_source) {
    free(g_dist_source);
  }
  g_dist_source = GetStringUTF8Chars(env, dist_source).pass();
}

void Java_com_opera_android_browser_obml_Platform_setMiniServer(
        JNIEnv *env, jclass, jstring socket, jstring http,
        jboolean useEncryption, jstring encryptionKey)
{
    jnistring socketHost = GetStringUTF8Chars(env, socket);
    jnistring httpHost = GetStringUTF8Chars(env, http);
    MOpSettingsManager *sm = MOpSettingsManager_getSingleton();
    MOpSettingsManager_setINT32(sm, T("MiniServer"), T("Cluster"),
                                MOP_CUSTOM_MINI_SERVER);
    MOpSettingsManager_setString(sm, T("MiniServer"), T("Socket"),
                                 socketHost.string(), -1);
    MOpSettingsManager_setString(sm, T("MiniServer"), T("Http"),
                                 httpHost.string(), -1);
    MOpSettingsManager_setBOOL(sm, T("MiniServer"), T("UseThumbnailsServer"), FALSE);

    MOpSettingsManager_setBOOL(sm, T("MiniServer"), T("UseEncryption"),
                               useEncryption ? TRUE : FALSE);
    if (useEncryption && encryptionKey) {
        jnistring key = GetStringUTF8Chars(env, encryptionKey);
        MOpSettingsManager_setString(sm, T("Encryption"), T("MasterKey"),
                                     key.string(), -1);
    }
}

#ifndef MOP_TWEAK_MINISERVER_USE_THUMBNAILS_SERVER
#define MOP_TWEAK_MINISERVER_USE_THUMBNAILS_SERVER TRUE
#endif /* MOP_TWEAK_MINISERVER_USE_THUMBNAILS_SERVER */

void Java_com_opera_android_browser_obml_Platform_resetMiniServer(
        JNIEnv *, jclass)
{
    MOpSettingsManager *sm = MOpSettingsManager_getSingleton();
    MOpSettingsManager_setINT32(sm, T("MiniServer"), T("Cluster"),
                                MOP_TWEAK_DEFAULT_MINI_SERVER);
    MOpSettingsManager_setString(sm, T("MiniServer"), T("Socket"),
                                 MOP_TWEAK_MINISERVER_SOCKET, -1);
    MOpSettingsManager_setString(sm, T("MiniServer"), T("Http"),
                                 MOP_TWEAK_MINISERVER_HTTP, -1);
    MOpSettingsManager_setBOOL(sm, T("MiniServer"), T("UseThumbnailsServer"),
                               MOP_TWEAK_MINISERVER_USE_THUMBNAILS_SERVER);
    MOpSettingsManager_setBOOL(sm, T("MiniServer"), T("UseEncryption"), TRUE);
    MOpSettingsManager_setString(sm, T("Encryption"), T("MasterKey"),
                                 MOP_TWEAK_CRYPTOSTREAM_MASTER_KEY, -1);
}
