/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <pthread.h>
#include <jni.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "bream.h"
#include "canvascontext.h"
#include "font.h"
#include "platform.h"
#include "pushedcontent.h"
#include "reksio.h"
#include "vminstance_ext.h"
#include "LowLevelFile.h"
#include "SystemInfo.h"

OP_EXTERN_C_BEGIN

#include "bream/vm/c/inc/bream_host.h"
#include "bream/vm/c/graphics/internal_io.h"
#include "mini/c/shared/core/encoding/tstring.h"
#include "mini/c/shared/core/hardcore/GlobalMemory.h"
#include "mini/c/shared/core/images/decoders/Jaypeg.h"
#include "mini/c/shared/core/images/decoders/minpng.h"
#include "mini/c/shared/core/images/decoders/webp/decode.h"
#include "mini/c/shared/core/pi/Graphics.h"
#include "mini/c/shared/core/pi/Keychain.h"
#include "mini/c/shared/core/pi/SystemInfo.h"
#include "mini/c/shared/core/settings/SettingsManager.h"

#include "com_opera_android_browser_obml_Platform.h"

OP_EXTERN_C_END

#define REQUESTED_JNI_VERSION JNI_VERSION_1_4

/* JavaVM pointer and version, used for getting JNIEnv for new threads. */
static JavaVM *jvm;
static pthread_key_t jniEnvKey;

JNIEnv* getEnv() {
    JNIEnv* env = (JNIEnv*)pthread_getspecific(jniEnvKey);

    /* If it's the first time called from this thread it needs to be
     * initialized. */
    if (!env) {
        /* Retrieve JNIEnv* from the JavaVM environment */
        BREAM_ASSERT(jvm);
        void* ptr;
        jint ret = jvm->GetEnv(&ptr, REQUESTED_JNI_VERSION);
        BREAM_ASSERT(!ret);
        if (ret) {
            return NULL;
        }
        pthread_setspecific(jniEnvKey, ptr);
        env = (JNIEnv*)ptr;
    }

    return env;
}

static void fontMetrics(bgContext *context, bgFont *font,
                        unsigned int *ascent, unsigned int *descent,
                        unsigned int *height, unsigned int *lineHeight);
static void fontMetricsStretched(bgContext *context, bgFont *font,
                                 unsigned int *ascent, unsigned int *descent,
                                 unsigned int *height, unsigned int *lineHeight,
                                 int unscaled, int scaled);
static unsigned int stringWidth(bgContext *context, bgFont *font,
                                const void *text, size_t length,
                                BG_TEXT_ENCODING encoding);
static unsigned int stringWidthStretched(bgContext *context, bgFont *font,
                                         const void *text, size_t length,
                                         BG_TEXT_ENCODING encoding,
                                         int unscaled, int scaled);
static void drawStringStretched(bgContext *context,
                                const void *text, size_t length,
                                BG_TEXT_ENCODING encoding, int x, int y,
                                int unscaled, int scaled, bgFont *font,
                                BG_BLEND blend);
static void drawString(bgContext *context, const void *text, size_t length,
                       BG_TEXT_ENCODING encoding, int x, int y, bgFont *font,
                       BG_BLEND blend);
static void drawTextSegment(bgContext *context, const void *text, size_t length,
                            int bidi_level, BG_TEXT_ENCODING encoding,
                            int x, int y, bgFont *font, BG_BLEND blend);
static bgImage *decodeImage(bgContext *context,
                            const UINT8 *data, size_t size,
                            BOOL dither, bream_error_t *status);

static BOOL supportsHebrewBidi(void);
static BOOL supportsArabicBidi(void);

jclass g_platform_class;
static jmethodID g_platform_getCountry;
static jmethodID g_platform_getIntValue;
static jmethodID g_platform_getLanguage;
static jmethodID g_platform_getMCC;
static jmethodID g_platform_getMNC;
static jmethodID g_platform_getParcelFileDescriptor;
static jmethodID g_platform_getStringValue;
static jmethodID g_platform_getUserStats;
static jmethodID g_platform_handleSpecialLink;
static jmethodID g_platform_onSettingChanged;
static jmethodID g_platform_openParcelFileDescriptor;
static jmethodID g_platform_readProxyConfig;
static jmethodID g_platform_getAdvertisingId;
static jmethodID g_platform_limitAdTracking;
static jfieldID g_platform_sAndroidId;

bgPlatform g_bg_platform;
const char *g_user_agent;
unsigned int g_screen_dpi_x;
unsigned int g_screen_dpi_y;
unsigned int g_screen_width;
unsigned int g_screen_height;
BOOL g_is_tablet;
static int g_bidi_support;

jint JNI_OnLoad(JavaVM *vm, void *reserved UNUSED) {
    jvm = vm;

    int ret = pthread_key_create(&jniEnvKey, NULL);
    BREAM_ASSERT(!ret);

    JNIEnv *env = getEnv();
    BREAM_ASSERT(env);

    {
        scoped_localref<jclass> clazz(env, env->FindClass(C_com_opera_android_browser_obml_Platform));
        BREAM_ASSERT(clazz);
        g_platform_class = (jclass)env->NewGlobalRef(*clazz);

#define M(n) g_platform_##n = env->GetStaticMethodID(*clazz, M_com_opera_android_browser_obml_Platform_##n##_ID)
        M(getCountry);
        M(getIntValue);
        M(getLanguage);
        M(getMCC);
        M(getMNC);
        M(getParcelFileDescriptor);
        M(getStringValue);
        M(getUserStats);
        M(handleSpecialLink);
        M(onSettingChanged);
        M(openParcelFileDescriptor);
        M(readProxyConfig);
        M(getAdvertisingId);
        M(limitAdTracking);
        g_platform_sAndroidId = env->GetStaticFieldID(*clazz, F_com_opera_android_browser_obml_Platform_sAndroidId_ID);
    }

    initSystemInfo(env);
    initLowLevelFile(env);
    initPushedContent(env);
    canvasContextStaticInit(env);
    breamStaticInit(env);

    bgInitializePlatform(&g_bg_platform);
    g_bg_platform.fontMetrics = fontMetrics;
    g_bg_platform.fontMetricsStretched = fontMetricsStretched;
    g_bg_platform.stringWidth = stringWidth;
    g_bg_platform.stringWidthStretched = stringWidthStretched;
    g_bg_platform.drawString = drawString;
    g_bg_platform.drawTextSegment = drawTextSegment;
    g_bg_platform.drawStringStretched = drawStringStretched;
    g_bg_platform.decodeImage = decodeImage;
    g_bg_platform.supportsHebrewBidi = supportsHebrewBidi;
    g_bg_platform.supportsArabicBidi = supportsArabicBidi;
#ifndef BREAM_GRAPHICS_DISABLE_BREAM
    g_bg_platform.loadFont = MOpGraphics_loadFont;
    g_bg_platform.loadImage = MOpGraphics_loadImage;
    g_bg_platform.paintOBML = MOpGraphics_paintObml;
#endif

    return REQUESTED_JNI_VERSION;
}

void Java_com_opera_android_browser_obml_Platform_nativeInit(
        JNIEnv *env, jclass, jstring userAgent, jint screenWidth, jint screenHeight,
        jint dpiX, jint dpiY, jboolean isTablet, jint bidiSupport, jstring basePath) {
    g_user_agent = GetStringUTF8Chars(env, userAgent).pass();
    g_screen_width = screenWidth;
    g_screen_height = screenHeight;
    g_screen_dpi_x = dpiX;
    g_screen_dpi_y = dpiY;
    g_is_tablet = isTablet;
    g_bidi_support = bidiSupport;

    internal_io_init(GetStringUTF8Chars(env, basePath).pass());

    bool success = AndroidFont::InitGlobals();
    BREAM_ASSERT(success);

    initReksio(env);
}

static BOOL supportsHebrewBidi() {
    return g_bidi_support & 2;
}

static BOOL supportsArabicBidi() {
    return g_bidi_support & 12;
}

int MOpSystemInfo_getBidiSupport() {
    return g_bidi_support;
}

static void fontMetrics(bgContext *context, bgFont *font,
                        unsigned int *ascent, unsigned int *descent,
                        unsigned int *height, unsigned int *lineHeight)
{
    fontMetricsStretched(context, font, ascent, descent, height, lineHeight,
                         1, 1);
}

static void fontMetricsStretched(bgContext *context UNUSED, bgFont *font,
                                 unsigned int *ascent NDEBUG_UNUSED,
                                 unsigned int *descent NDEBUG_UNUSED,
                                 unsigned int *height, unsigned int *lineHeight,
                                 int unscaled, int scaled)
{
    BREAM_ASSERT(!ascent && !descent);

    AndroidFont* f = reinterpret_cast<AndroidFont*>(font);
    unsigned int h = f->GetHeight(unscaled, scaled);

    if (height)
        *height = h;

    if (lineHeight)
        *lineHeight = h;
}

static unsigned int stringWidth(bgContext *context UNUSED, bgFont *font,
                                const void *text, size_t length,
                                BG_TEXT_ENCODING encoding NDEBUG_UNUSED)
{
    BREAM_ASSERT(encoding == BG_TEXT_UTF8);
    AndroidFont* f = reinterpret_cast<AndroidFont*>(font);
    return f->StringWidth(static_cast<const char*>(text), length);
}

static unsigned int stringWidthStretched(bgContext *context UNUSED, bgFont *font,
                                         const void *text, size_t length,
                                         BG_TEXT_ENCODING encoding NDEBUG_UNUSED,
                                         int unscaled, int scaled)
{
    BREAM_ASSERT(encoding == BG_TEXT_UTF8);
    AndroidFont* f = reinterpret_cast<AndroidFont*>(font);
    return f->StringWidth(static_cast<const char*>(text), length, unscaled,
                          scaled);
}

static void drawStringStretched(bgContext *context,
                                const void *text, size_t length,
                                BG_TEXT_ENCODING encoding NDEBUG_UNUSED, int x,
                                int y, int unscaled, int scaled, bgFont *font,
                                BG_BLEND blend)
{
    BREAM_ASSERT(encoding == BG_TEXT_UTF8);
    AndroidFont* f = reinterpret_cast<AndroidFont*>(font);
    f->DrawString(context, static_cast<const char*>(text), length, x, y,
                  unscaled, scaled, blend);
}

static void drawString(bgContext *context, const void *text, size_t length,
                       BG_TEXT_ENCODING encoding NDEBUG_UNUSED, int x, int y,
                       bgFont *font, BG_BLEND blend)
{
    BREAM_ASSERT(encoding == BG_TEXT_UTF8);
    AndroidFont* f = reinterpret_cast<AndroidFont*>(font);
    f->DrawString(context, static_cast<const char*>(text), length, x, y, blend);
}

static void drawTextSegment(bgContext *context, const void *text, size_t length,
                            int bidi_level UNUSED, BG_TEXT_ENCODING encoding,
                            int x, int y, bgFont *font, BG_BLEND blend)
{
    /* TODO */
    drawString(context, text, length, encoding, x, y, font, blend);
}

static bgImage *decodeWebpImage(bgContext *context,
                                const UINT8 *data, size_t size,
                                BOOL dither, bream_error_t *status)
{
    WebPDecoderConfig config;
    VP8StatusCode vp_status;
    BG_FORMAT ret_format, decode_format;
    bgImage* ret;
    swImage* img;
    int width, height;

    if (!WebPInitDecoderConfig(&config))
    {
        *status = BREAM_ERR;
        return NULL;
    }
    if (WebPGetFeatures(data, size, &config.input) != VP8_STATUS_OK)
    {
        *status = BREAM_ERR;
        return NULL;
    }

    width = config.input.width;
    height = config.input.height;
    ret_format = decode_format = context->optimalImageFormat(context, config.input.has_alpha ?
                                                             BG_ALPHA_FULL : BG_ALPHA_NONE);
    switch (ret_format)
    {
    case BG_RGB_888:
        config.output.colorspace = MODE_RGB;
        break;
    case BG_RGBA_8888:
        config.output.colorspace = MODE_RGBA;
        break;
    case BG_ARGB_8888:
        config.output.colorspace = MODE_ARGB;
        break;
    case BG_RGB_565:
        config.output.colorspace = MODE_RGB_565;
        break;
    default:
        decode_format = BG_BGRA_8888;
        config.output.colorspace = MODE_BGRA;
    }

    vp_status = WebPDecode(data, size, &config);
    if (vp_status != VP8_STATUS_OK)
    {
        WebPFreeDecBuffer(&config.output);
        if (vp_status == VP8_STATUS_OUT_OF_MEMORY)
            *status = BREAM_NO_MEMORY;
        else
            *status = BREAM_ERR;
        return NULL;
    }

    img = (swImage*)context->createImage(context, width, height, ret_format);
    if (!img)
    {
        *status = BREAM_NO_MEMORY;
        WebPFreeDecBuffer(&config.output);
        return NULL;
    }

    if (dither)
    {
        swImage tmp;
        swWrapImageData(&tmp, config.output.u.RGBA.rgba, width, height,
                        config.output.u.RGBA.stride, decode_format);
        swImageDither(&tmp);
        swBlitCopy(tmp.data, tmp.stride, tmp.format, img->data, img->stride, img->format,
                   width, height);
    }
    else
    {
        swBlitCopy(config.output.u.RGBA.rgba, config.output.u.RGBA.stride, decode_format,
                   img->data, img->stride, img->format, width, height);
    }

#ifdef MOP_DEBUG_WEBP
    swBlitSolid(img->data, img->stride, img->format, 4, 4, 0xff000000);
#endif

    ret = context->convertImage(context, &img->image, FALSE, FALSE);
    img->image.dispose(&img->image);

    WebPFreeDecBuffer(&config.output);

    *status = ret ? BREAM_OK : BREAM_NO_MEMORY;

    return ret;
}

static bgImage *decodePNGImage(bgContext *context,
                               const UINT8 *data, size_t size,
                               BOOL dither, bream_error_t *status)
{
    struct PngFeeder input;
    struct PngRes res;
    bgImage* ret;
    swImage* img;
    BG_FORMAT format, res_format;

    minpng_init_result(&res);
    minpng_init_feeder(&input);

    input.data = (unsigned char *)data;
    input.len = (int)size;
    memset(&res, 0, sizeof(res));

    switch ((PngRes::Result)minpng_decode(&input, &res))
    {
    case PngRes::OOM_ERROR:
        *status = BREAM_NO_MEMORY;
        minpng_clear_feeder(&input);
        minpng_clear_result(&res);
        return NULL;
    case PngRes::NEED_MORE:
    case PngRes::ILLEGAL_DATA:
        *status = BREAM_ERR;
        minpng_clear_feeder(&input);
        minpng_clear_result(&res);
        return NULL;
    case PngRes::OK:
        break;
    }

    BREAM_ASSERT(!res.nofree_image);

#if defined(MINPNG_ARGB_BYTEORDER)
    res_format = res.has_alpha ? BG_ARGB_8888 : BG_XRGB_8888;
#elif defined(MINPNG_RGBA_BYTEORDER)
    res_format = res.has_alpha ? BG_RGBA_8888 : BG_RGBX_8888;
#else
    res_format = res.has_alpha ? BG_BGRA_8888 : BG_BGRX_8888;
#endif

    format = context->optimalImageFormat(context, res.has_alpha ? BG_ALPHA_FULL : BG_ALPHA_NONE);
    img = (swImage*)context->createImage(context, res.xsize, res.ysize, format);
    if (!img)
    {
        *status = BREAM_NO_MEMORY;
        minpng_clear_feeder(&input);
        minpng_clear_result(&res);
        return NULL;
    }

    if (dither && (res_format == BG_RGBX_8888 || res_format == BG_RGBA_8888 ||
                   res_format == BG_BGRX_8888 || res_format == BG_BGRA_8888))
    {
        swImage tmp;
        swWrapImageData(&tmp, res.image, res.xsize, res.ysize, res.xsize * 4,
                        res_format);
        swImageDither(&tmp);
        swBlitCopy(tmp.data, tmp.stride, tmp.format,
                   img->data, img->stride, img->format, res.xsize, res.ysize);
    }
    else
    {
        swBlitCopy(res.image, res.xsize * 4, res_format,
                   img->data, img->stride, img->format, res.xsize, res.ysize);
    }

    ret = context->convertImage(context, &img->image, FALSE, FALSE);
    img->image.dispose(&img->image);

    minpng_clear_feeder(&input);
    minpng_clear_result(&res);

    *status = ret ? BREAM_OK : BREAM_NO_MEMORY;

    return ret;
}

static bgImage *decodeJPEGImage(bgContext *context,
                                const UINT8 *data, size_t size,
                                BOOL dither, bream_error_t *status)
{
#if !defined(MOP_HAVE_JPEG_DECODER) && !defined(MOP_TWEAK_NO_JPEG_DECODER)
#ifdef MOP_JAYPEG_SCALEFACTOR
    JpegScaleFactor f = JAYPEG_SCALE_FACTOR_NONE;
#endif /* MOP_JAYPEG_SCALEFACTOR */
    MOpJaypeg* jaypeg;
    bgImage* ret;
    swImage* img;
    BG_FORMAT format, res_format;
    UINT32 width, height;
    void* res_data;

    if (MOpJaypeg_create(&jaypeg) != MOP_STATUS_OK)
    {
        *status = BREAM_NO_MEMORY;
        return NULL;
    }

    switch (MOpJaypeg_decode(jaypeg, data, 0, (INT32)size,
#ifdef MOP_JAYPEG_SCALEFACTOR
                             f,
#endif /* MOP_JAYPEG_SCALEFACTOR */
                             FALSE))
    {
    case JAYPEG_NO_MEM:
        *status = BREAM_NO_MEMORY;
        MOpJaypeg_release(jaypeg);
        return NULL;
    case JAYPEG_NOT_ENOUGH_DATA:
    case JAYPEG_ERR:
        *status = BREAM_ERR;
        MOpJaypeg_release(jaypeg);
        return NULL;
    case JAYPEG_OK:
        break;
    }

    res_data = MOpJaypeg_getPixelData(jaypeg, TRUE);
    width = MOpJaypeg_getWidth(jaypeg);
    height = MOpJaypeg_getHeight(jaypeg);

#ifdef MOP_JAYPEG_SCALEFACTOR
    width >>= f;
    height >>= f;
#endif /* MOP_JAYPEG_SCALEFACTOR */

#ifdef MOP_JAYPEG_CS_565
    res_format = BG_RGB_565;
#else
    res_format = BG_BGRX_8888;
#endif

    format = context->optimalImageFormat(context, BG_ALPHA_NONE);
    img = (swImage*)context->createImage(context, width, height, format);
    if (!img)
    {
        *status = BREAM_NO_MEMORY;
        mop_free(res_data);
        MOpJaypeg_release(jaypeg);
        return NULL;
    }

    if (dither && res_format == BG_BGRX_8888)
    {
        swImage tmp;
        swWrapImageData(&tmp, res_data, width, height, width * 4, res_format);
        swImageDither(&tmp);
        swBlitCopy(tmp.data, tmp.stride, tmp.format,
                   img->data, img->stride, img->format, width, height);
    }
    else
    {
        swBlitCopy(res_data, width * bgPixelSize(res_format), res_format,
                   img->data, img->stride, img->format, width, height);
    }
    mop_free(res_data);

    ret = context->convertImage(context, &img->image, FALSE, FALSE);
    img->image.dispose(&img->image);

    MOpJaypeg_release(jaypeg);

    *status = ret ? BREAM_OK : BREAM_NO_MEMORY;

    return ret;
#else
    *status = BREAM_ERR;
    return NULL;
#endif
}

bgImage *decodeImage(bgContext *context, const UINT8 *data, size_t size,
                     BOOL dither, bream_error_t *status)
{
    bream_error_t err;
    bgImage *img;
    img = decodePNGImage(context, data, size, dither, &err);
    if (img || err == BREAM_NO_MEMORY)
    {
        if (status) *status = err;
        return img;
    }
    img = decodeJPEGImage(context, data, size, dither, &err);
    if (img || err == BREAM_NO_MEMORY)
    {
        if (status) *status = err;
        return img;
    }
    img = decodeWebpImage(context, data, size, dither, &err);
    if (img || err == BREAM_NO_MEMORY)
    {
        if (status) *status = err;
        return img;
    }
    if (status) *status = BREAM_ERR;
    return NULL;
}

// Effectively, these are dummy implementations. Should look into getting rid
// of them...
void MOpGraphics_selectTextDrawingPlatform(bgPlatform* platform UNUSED)
{
    /*platform->fontMetrics = MOp_PIFontMetrics;
      platform->fontMetricsStretched = MOp_PIFontMetricsStretched;
      platform->stringWidth = MOp_PIStringWidth;
      platform->stringWidthStretched = MOp_PIStringWidthStretched;
      platform->drawString = MOp_PIDrawString;
      platform->drawStringStretched = MOp_PIDrawStringStretched;
      platform->drawTextSegment = MOp_PIDrawTextSegment;*/
}

MOP_STATUS MOpGraphics_createContext(bgContext **screenContext, bgPlatform *platform)
{
    *screenContext = (bgContext*)swCreateEmptyContext(platform, BG_RGB_565);
    return MOP_STATUS_OK;
}

static char* getStringValue(const char* key)
{
    JNIEnv* env = getEnv();
    scoped_localref<jstring> jkey = NewStringUTF8(env, key);
    scoped_localref<jstring> value(
        env, (jstring) env->CallStaticObjectMethod(g_platform_class,
                                                   g_platform_getStringValue,
                                                   *jkey));
    if (value)
    {
        jnistring str = GetAndReleaseStringUTF8Chars(value);
        return mop_strdup(str.string());
    }
    return NULL;
}

static jint getIntValue(const char *key)
{
    JNIEnv* env = getEnv();
    scoped_localref<jstring> jkey = NewStringUTF8(env, key);
    return env->CallStaticIntMethod(g_platform_class, g_platform_getIntValue, *jkey);
}

MOP_STATUS MOpSystemInfo_getLanguage(SCHAR ** name, BOOL * del)
{
    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
        env, (jstring) env->CallStaticObjectMethod(g_platform_class, g_platform_getLanguage));
    *name = mop_strdup(str.string());
    *del = TRUE;
    return MOP_STATUS_OK;
}

MOP_STATUS MOpSystemInfo_getCountry(SCHAR ** name, BOOL * del)
{
    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
        env, (jstring) env->CallStaticObjectMethod(g_platform_class, g_platform_getCountry));
    *name = mop_strdup(str.string());
    *del = TRUE;
    return MOP_STATUS_OK;
}

MOP_STATUS MOpKeychain_getUniqueData(UINT8 * uData, UINT32 * uDataSize)
{
    if (!uData || !uDataSize)
    {
        return MOP_STATUS_NULLPTR;
    }

    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
        env, (jstring) env->GetStaticObjectField(g_platform_class, g_platform_sAndroidId));
    *uDataSize = MIN(*uDataSize, strlen(str.string()));
    memcpy(uData, str.string(), *uDataSize);
    return MOP_STATUS_OK;
}

MOP_STATUS MOpSystemInfo_getUserStats(SCHAR** stats, BOOL* del, void* handle UNUSED)
{
    JNIEnv* env = getEnv();
    scoped_localref<jstring> str(
        env,
        (jstring) env->CallStaticObjectMethod(g_platform_class, g_platform_getUserStats));
    if (str)
    {
        jnistring jstr = GetAndReleaseStringUTF8Chars(str);
        *stats = mop_strdup(jstr.string());
        *del = TRUE;
    }
    else
    {
        *stats = NULL;
        *del = FALSE;
    }
    return MOP_STATUS_OK;
}

void MOpSystemInfo_userStatsSent(void *handle UNUSED, BOOL success UNUSED)
{
    // Not implemented. We currently happily assume sending statistics always succeeds.
}

#ifdef MOP_HAVE_SM_MODIFY_DEFAULTS
MOP_STATUS MOpSettingsManager_modifyDefaults(MOpSettingsManager *_this)
{
    TCHAR* mopStr = getStringValue("Campaign");

    if (mopStr)
    {
        MOpSettingsManager_setString(_this, T("General"), T("Campaign"), mopStr,
                                     (UINT32)mop_strlen(mopStr));
        MOpSettingsManager_enableProtection(_this, T("General"), T("Campaign"));
        mop_freeif(mopStr);
    }
    return MOP_STATUS_OK;
}
#endif  // MOP_HAVE_SM_MODIFY_DEFAULTS

#ifdef MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
static void overrideNetworkSettings(MOpSettingsManager *_this)
{
    TCHAR *proxy = getHTTPProxy();
    BOOL freeProxy = TRUE;
    if (!proxy)
    {
        freeProxy = FALSE;
        proxy = (TCHAR *)T("");
    }
    MOpDebug("overrideNetworkSettings, proxy = %s", proxy);
    BOOL useProxy = *proxy ? TRUE : FALSE;

#ifdef MOP_TWEAK_FORCE_HTTP_IF_PROXY_SET
    // Note: we should enable sockets before set default server type to socket.
    MOpSettingsManager_setBOOL(_this, T("MiniServer"), T("EnableSockets"), !useProxy);
#endif // MOP_TWEAK_FORCE_HTTP_IF_PROXY_SET

    const TCHAR *connectionType = useProxy ? T("Http") : T("Socket");
    MOpSettingsManager_setString(_this, T("MiniServer"), T("DefaultServerType"), connectionType,
      mop_tstrlen(connectionType));

    MOpSettingsManager_setBOOL(_this, T("General"), T("UseProxy"), useProxy);
    MOpSettingsManager_setString(_this, T("Proxy"), T("Server"), proxy, mop_tstrlen(proxy));
    if (freeProxy)
    {
        mop_free(proxy);
    }
}
#endif // MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE

#ifdef MOP_HAVE_SETTINGS_PROXY
void MOpSettingsManager_afterLoad(MOpSettingsManager *_this, const TCHAR *filename)
{
    (void)(filename);
    MOpDebug("MOpSettingsManager_afterLoad, filename = %s", filename);
#ifdef MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
    if (0 == mop_tstrcmp(filename, MOP_SETTINGS_FILE))
    {
        overrideNetworkSettings(_this);
    }
#endif // MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
}

void MOpSettingsManager_beforeSave(MOpSettingsManager* _this UNUSED, const TCHAR* filename UNUSED)
{
}
#endif // MOP_HAVE_SETTINGS_PROXY

void onReachabilityChanged(void *userData UNUSED, MOpReachabilityStatus status)
{
    (void)(status);
    MOpDebug("onReachabilityChanged, status = %d", status);
#ifdef MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
    if (status & MOP_REACHABILITY_REACHABLE)
    {
        overrideNetworkSettings(MOpSettingsManager_getSingleton());
    }
#endif // MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
}

void onSettingChanged(void *userData UNUSED, UINT32 param UNUSED, void *change_)
{
    MOpSettingChange *change = (MOpSettingChange*)change_;
    TCHAR *section = change->section;
    TCHAR *option = change->option;

    // TODO The iOS version checks for updated traffic routing rules here. We should too, unless
    // there's another way we use to tell the Turbo client about changed rules.

    if ((strcmp(section, "Cookies") && strcmp(option, "Client")) ||
        (strcmp(section, "General") && strcmp(option, "Campaign"))) {
        return;
    }

    JNIEnv* env = getEnv();
    scoped_localref<jstring> jsection = NewStringUTF8(env, section);
    scoped_localref<jstring> joption = NewStringUTF8(env, option);
    scoped_localref<jstring> jvalue = NewStringUTF8(env, change->newValue);
    env->CallStaticVoidMethod(g_platform_class, g_platform_onSettingChanged, *jsection, *joption, *jvalue);
}

jstring Java_com_opera_android_browser_obml_Platform_getSettingValue(JNIEnv* env, jclass, jstring jsection, jstring joption) {
    jnistring section = GetStringUTF8Chars(env, jsection);
    jnistring option = GetStringUTF8Chars(env, joption);

    TCHAR *value = NULL;
    if (MOpSettingsManager_getString(MOpSettingsManager_getSingleton(),
            section.string(), option.string(), &value) == MOP_STATUS_OK) {
        return NewStringUTF8(env, value).pass();
    }
    return NULL;
}

void Java_com_opera_android_browser_obml_Platform_initBreamVmIo(JNIEnv* env, jclass, jstring basePath) {
    internal_io_init(GetStringUTF8Chars(env, basePath).pass());
}

jstring Java_com_opera_android_browser_obml_Platform_readFromKeychain(JNIEnv* env, jclass, jstring keyName) {
    TCHAR *value = NULL;
    jnistring key = GetStringUTF8Chars(env, keyName);
    MOpKeychain *chain;

    if (MOpKeychain_create((MOpKeychain **) &chain) != MOP_STATUS_OK) {
        // If keychain creation fails, it is released automatically.
        return NULL;
    }
    // Original code belongs to SettingsManager.c, MOpSM_readFromKeychain
    {
        TCHAR *newValue = NULL;
        UINT32 valueSize = 64, bytesRead = 0;

        MOP_STATUS status = MOP_STATUS_OK;

        do {
            newValue = (TCHAR *) mop_malloc(valueSize + sizeof(TCHAR));
            MOP_BREAKIF_NOMEM(newValue, status);

            // 0 is the settings keychain type.
            status = MOpKeychain_get(chain, 0, key.string(),
              (UINT8 *)newValue, valueSize, &bytesRead);
            MOP_BREAKIF(status);

            if (bytesRead < valueSize) {
              status = MOP_STATUS_ERR;
              break;
            }

            /* Reinterpreting the raw data from keychain as TCHAR string.
             * This includes adding terminating character. */
            MOP_ASSERT(valueSize % sizeof(TCHAR) == 0);
            valueSize /= sizeof(TCHAR);
            newValue[valueSize] = C('\0');

            value = newValue;
            newValue = NULL;
        } while (FALSE);

        if (newValue) {
            mop_free(newValue);
        }
    }

    MOpKeychain_release(chain);

    if (value) {
        return NewStringUTF8(env, value).pass();
    } else {
        return NULL;
    }
}

void onBreamInitialized()
{
#ifdef MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
    overrideNetworkSettings(MOpSettingsManager_getSingleton());
#endif // MOP_TWEAK_OVERRIDE_NETWORK_SETTINGS_WITH_PLATFORM_VALUE
}

#ifdef MOP_FEATURE_HTTP_PLATFORM_PROXY
TCHAR *getHTTPProxy()
{
    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
        env, (jstring) env->CallStaticObjectMethod(g_platform_class, g_platform_readProxyConfig));
    return mop_fromutf8(str.string(), -1);
}
#endif /* MOP_FEATURE_HTTP_PLATFORM_PROXY */

#ifdef MOP_HAVE_GETSAVEDMOBILEDATACOUNT
UINT32 MOpSystemInfo_getSavedMobileDataCount()
{
    return MAX(getIntValue("savedMobileDataCount"), 0);
}
#endif /* MOP_HAVE_GETSAVEDMOBILEDATACOUNT */

scoped_localref<jobject> getParcelFileDescriptor(const char* filename, const char* mode)
{
    JNIEnv* env = getEnv();
    scoped_localref<jstring> jfilename = NewStringUTF8(env, filename, false);
    scoped_localref<jstring> jmode = NewStringUTF8(env, mode, true);
    return scoped_localref<jobject>(
        env, env->CallStaticObjectMethod(
            g_platform_class, g_platform_getParcelFileDescriptor, *jfilename, *jmode));
}

scoped_localref<jobject> openParcelFileDescriptor(jobject parcelFileDescriptor)
{
    JNIEnv* env = getEnv();
    return scoped_localref<jobject>(
        env, env->CallStaticObjectMethod(
            g_platform_class, g_platform_openParcelFileDescriptor, parcelFileDescriptor));
}

MOP_STATUS MOpSystemInfo_getAdvertisingInfo(SCHAR** advertisingId, BOOL* limitAdTracking, BOOL* freeAdvertisingId)
{
    JNIEnv* env = getEnv();
    jnistring str = GetAndReleaseStringUTF8Chars(
        env, (jstring) env->CallStaticObjectMethod(g_platform_class, g_platform_getAdvertisingId));
    if (!str)
      return MOP_STATUS_OK;
    *advertisingId = mop_fromutf8(str.string(), -1);
    if (*advertisingId == NULL)
      return MOP_STATUS_NOMEM;
    *freeAdvertisingId = TRUE;
    *limitAdTracking = env->CallStaticBooleanMethod(g_platform_class, g_platform_limitAdTracking);
    return MOP_STATUS_OK;
}
