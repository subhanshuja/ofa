/*
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <android/bitmap.h>
#include <string.h>
#include <jni.h>
#include <math.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "com_opera_android_browser_obml_Font.h"
#include "font.h"
#include "platform.h"

#include <android_graphics_Typeface.h>

OP_EXTERN_C_BEGIN
#include "bream/vm/c/graphics/bream_graphics.h"
#include "bream/vm/c/graphics/obmlpainter.h"
#include "bream/vm/c/graphics/thread.h"
#include "bream/vm/c/graphics/utils.h"
#include "bream/vm/c/inc/bream_host.h"
#include "bream/vm/c/inc/bream_types.h"
#include "mini/c/shared/core/encoding/tstring.h"
#include "mini/c/shared/core/font/Font.h"
#include "mini/c/shared/core/font/FontEx.h"
#include "mini/c/shared/core/font/FontsCollection.h"
#include "mini/c/shared/core/pi/SystemInfo.h"
#include "mini/c/shared/core/protocols/obsp/ObspFontInfo.h"
OP_EXTERN_C_END

#if SYSTEM_ENCODING != ENCODING_UTF8
# error Text rendering and font functions currently assume UTF-8.
#endif

static const int MIN_FONT_SCALE = 30;
// font_scale=100 will result in single words no longer being separate obml
// nodes, thus letting char measurment errors to accumulate (see
// ANDMO-4044). This should no longer be an issue when OMS-7495 is
// implemented.
static const int MAX_FONT_SCALE = 90;
static const float SIZE_SETTING_FACTOR = 5.0f / 4.0f;

// These values will be adjusted with regards to font scale, e.g. for
// font_scale = 50 we double every size value.
static int g_font_sizes[] = { 10, 11, 12, 14, 16, 18, 20, 23, 26, 30, 34, 39, 44, 50, 57, 65 };

static const size_t kMaxNumFonts = ARRAY_SIZE(g_font_sizes) * 4 * 3;
static AndroidFont* g_fonts[kMaxNumFonts];
static size_t g_num_fonts;

static int g_font_scale = MAX_FONT_SCALE;
static bgContext* g_temp_context;

static ThreadMutex* g_text_mutex;
static jcharArray g_text_array;
static jsize g_text_array_size;
static jfloatArray g_float_array;
static jsize g_float_array_size;

static jclass g_font_clazz;
static jfieldID g_id_text_bitmap;
static jmethodID g_id_get_font_metrics_int;
static jmethodID g_id_draw_text;
static jmethodID g_id_get_text_width;
static jmethodID g_id_get_characters_widths;
static jmethodID g_id_get_font_height;
static jmethodID g_id_get_font;

static void InitFontSizeInfo() {
  int dpi = MIN(g_screen_dpi_x, g_screen_dpi_y);

  int w = g_screen_width;
  int h = g_screen_height;

  int diagonal_px = sqrt(w*w + h*h);

  // 3-variable equation solution for 3 chosen devices.
  // A / dpi + B / sqrt(diagonal_px) + C = desired font scale
  // Devices:
  // Sony Xperia ZL (desired font scale=30)
  // Asus Transformer Pad Infinity (font scale=38)
  // Samsung Galaxy Y (font scale=84)
  const float FONT_SCALE_FACTOR_DPI = 3683.7211f;
  const float FONT_SCALE_FACTOR_DIAGONAL = 1083.331f;
  const float FONT_SCALE_FACTOR_C = -1.9228f;

  float font_scale = (FONT_SCALE_FACTOR_DPI / dpi
      + FONT_SCALE_FACTOR_DIAGONAL / sqrt(diagonal_px)
      + FONT_SCALE_FACTOR_C);
  g_font_scale = MAX(MIN_FONT_SCALE, MIN(MAX_FONT_SCALE, (int) font_scale));
  // Round to the nearest ten to avoid having too many font size sets.
  g_font_scale = round(g_font_scale / 10.0f) * 10;

  for (unsigned int i = 0; i < ARRAY_SIZE(g_font_sizes); ++i)
    g_font_sizes[i] = g_font_sizes[i] * 100 / g_font_scale;
}

jbyteArray Java_com_opera_android_browser_obml_Font_nativeGetFontHash(JNIEnv* env, jclass) {
  ObspFontInfo* font_info = ObspFontInfo_get(FALSE);
  if (!ObspFontInfo_isInfoReady(font_info, FALSE)) {
    return NULL;
  }
  void* data = NULL;
  UINT32 dataSize = 0;
  MOP_STATUS status = ObspFontInfo_getInfo(font_info, FALSE, &data, &dataSize);
  if (status != MOP_STATUS_OK || dataSize == 0) {
    return NULL;
  }
  jbyteArray arr = env->NewByteArray(32);
  if (arr != NULL) {
    env->SetByteArrayRegion(arr, (jsize)0, (jsize)32, (jbyte*)data);
  }
  return arr;
}

#ifdef MOP_FEATURE_ODP
JNIEXPORT void JNICALL Java_com_opera_android_browser_obml_Font_nativeSetFontGenerationEnabled(
        JNIEnv *, jclass, jboolean enable) {
  ObspFontInfo_setCanGenerate(ObspFontInfo_get(FALSE), enable);
}
#endif

bool AndroidFont::InitGlobals() {
  JNIEnv* env = getEnv();

  if (!g_text_mutex) {
    g_text_mutex = ThreadMutex_create();
    if (!g_text_mutex)
      return false;
  }

  InitFontSizeInfo();

  if (!g_font_clazz) {
    scoped_localref<jclass> clazz(
        env, env->FindClass(C_com_opera_android_browser_obml_Font));
    MOP_ASSERT(clazz);
    if (!clazz)
      return false;

    g_font_clazz = reinterpret_cast<jclass>(env->NewGlobalRef(*clazz));

    g_id_text_bitmap = env->GetStaticFieldID(*clazz,
        F_com_opera_android_browser_obml_Font_textBitmap_ID);
#define M(id,m) g_id_##id = env->GetStaticMethodID(*clazz, M_com_opera_android_browser_obml_Font_##m##_ID)
    M(get_font_metrics_int, getFontMetricsInt);
    M(draw_text, drawText);
    M(get_characters_widths, getCharactersWidths);
    M(get_text_width, getTextWidth);
    M(get_font_height, getFontHeight);
    M(get_font, getFont);
  }

  if (g_num_fonts == 0) {
    if (!CreateFonts())
      return false;
  }

  if (!g_temp_context) {
    /* Format is unused, this context is only used by getTextWidth that
     * really only use the platform */
    g_temp_context = reinterpret_cast<bgContext *>(swCreateEmptyContext(&g_bg_platform, BG_A_8));
  }

  return true;
}

bool AndroidFont::CreateFonts() {
  bool res =
      // Proportional
      CreateFontForAllSizes(false, false, false, false) &&
      CreateFontForAllSizes(false, false, false, true) &&
      CreateFontForAllSizes(false, false, true, false) &&
      CreateFontForAllSizes(false, false, true, true) &&
      // Serif
      CreateFontForAllSizes(true, false, false, false) &&
      CreateFontForAllSizes(true, false, false, true) &&
      CreateFontForAllSizes(true, false, true, false) &&
      CreateFontForAllSizes(true, false, true, true) &&
      // Monospace
      CreateFontForAllSizes(false, true, false, false) &&
      CreateFontForAllSizes(false, true, false, true) &&
      CreateFontForAllSizes(false, true, true, false) &&
      CreateFontForAllSizes(false, true, true, true);

  if (!res) {
    for (size_t i = 0; i < g_num_fonts; i++) {
      delete g_fonts[i];
      g_fonts[i] = NULL;
    }
    g_num_fonts = 0;
  }

  return res;
}

bool AndroidFont::CreateFontForAllSizes(bool serif, bool monospace, bool bold,
                                        bool italic) {
  for (size_t i = 0; i < ARRAY_SIZE(g_font_sizes); i++) {
    AndroidFont* font = new AndroidFont();

    if (!font->Init(serif, monospace, bold, italic, g_font_sizes[i])) {
      delete font;
      return false;
    }

    MOP_ASSERT(g_num_fonts < kMaxNumFonts);
    g_fonts[g_num_fonts++] = font;
  }

  return true;
}

int AndroidFont::GetDefaultFontIndex(unsigned int textSize)
{
  return textSize * 2;
}

int AndroidFont::GetFontScale(unsigned int textSize) {
  int targetFontScale = g_font_scale * SIZE_SETTING_FACTOR;
  switch (textSize)   {
    case 0:
      targetFontScale *= SIZE_SETTING_FACTOR;
      break;
    case 2:
      targetFontScale /= SIZE_SETTING_FACTOR;
      break;
  }
  // Avoid server snapping to font scale = 50. To be removed
  // when OMS-7952 is implemented.
  if (targetFontScale > 48 && targetFontScale < 52)
    targetFontScale = targetFontScale <= 50 ? 48 : 52;
  return MIN(MAX_FONT_SCALE, targetFontScale);
}

int AndroidFont::GetFontSize(unsigned int index) {
  MOP_ASSERT(index < ARRAY_SIZE(g_font_sizes));
  return g_font_sizes[index];
}

size_t AndroidFont::GetFontCount() {
  return ARRAY_SIZE(g_font_sizes);
}

AndroidFont::AndroidFont()
    : paint_(NULL),
      serif_(false),
      monospace_(false),
      bold_(false),
      italic_(false),
      ascent_(0),
      descent_(0) {
}

AndroidFont::~AndroidFont() {
  if (paint_)
    getEnv()->DeleteGlobalRef(paint_);
}

bool AndroidFont::Init(bool serif,
                       bool monospace,
                       bool bold,
                       bool italic,
                       unsigned int size) {
  MOP_ASSERT(!serif || !monospace);

  if (!InitPaint(serif, monospace, bold, italic, size))
    return false;

  serif_ = serif;
  monospace_ = monospace;
  bold_ = bold;
  italic_ = italic;

  if (!InitMetrics())
    return false;

  return true;
}

bool AndroidFont::InitPaint(bool serif,
                            bool monospace,
                            bool bold,
                            bool italic,
                            unsigned int size) {
  MOP_ASSERT(!paint_);

  JNIEnv* env = getEnv();
  if (ThreadMutex_lock(g_text_mutex)) {
    scoped_localref<jobject> font(
        env, env->CallStaticObjectMethod(g_font_clazz,
                                         g_id_get_font,
                                         serif,
                                         monospace,
                                         bold,
                                         italic,
                                         size));
    ThreadMutex_unlock(g_text_mutex);
    paint_ = env->NewGlobalRef(*font);
  }

  return paint_ != NULL;
}

bool AndroidFont::InitMetrics() {
  JNIEnv* env = getEnv();

  scoped_localref<jclass> metrics_clazz(
      env, env->FindClass("android/graphics/Paint$FontMetricsInt"));
  if (!metrics_clazz)
    return false;
  scoped_localref<jobject> metrics(
      env, env->CallStaticObjectMethod(g_font_clazz, g_id_get_font_metrics_int, paint_));

  jfieldID top_id = env->GetFieldID(*metrics_clazz, "top", "I");
  jfieldID bottom_id = env->GetFieldID(*metrics_clazz, "bottom", "I");

  ascent_ = -env->GetIntField(*metrics, top_id);
  descent_ = env->GetIntField(*metrics, bottom_id);

  return true;
}

AndroidFont* AndroidFont::GetFont(UINT16 obml_font) {
  return obml_font < g_num_fonts ? g_fonts[obml_font] : NULL;
}

const char* AndroidFont::GetName() const {
  // Lies, it's all lies. But there is no way to get face name in Android (?)
  // and we do not really use it for anything important. This behavior matches
  // what Android Mini Java does.
  if (monospace())
    return "monospace";

  return serif() ? "serif" : "sans";
}

unsigned int AndroidFont::GetHeight(int unscaled, int scaled) const {
  if (unscaled == scaled)
    return ascent() + descent();

  unsigned int h = 0;
  if (ThreadMutex_lock(g_text_mutex)) {
    float scale = static_cast<float>(scaled) / unscaled;
    h = getEnv()->CallStaticIntMethod(g_font_clazz,
                                      g_id_get_font_height,
                                      scale,
                                      paint_);
    ThreadMutex_unlock(g_text_mutex);
  }
  return h;
}

static void GrowTextArrayIfNeeded(size_t need) {
  MOP_ASSERT(g_text_array_size >= 0);
  size_t text_array_size = static_cast<size_t>(g_text_array_size);
  if (g_text_array && need <= text_array_size)
    return;

  MOP_ASSERT(need <= INT_MAX);
  g_text_array_size = MAX(text_array_size * 2, need);
  JNIEnv* env = getEnv();

  if (g_text_array)
    env->DeleteGlobalRef(g_text_array);

  scoped_localref<jcharArray> tmp(env, env->NewCharArray(g_text_array_size));
  g_text_array = static_cast<jcharArray>(env->NewGlobalRef(*tmp));
}

static jint WriteToTextArray(const char* text, size_t length) {
  bream_string_t str;
  str.length = length;
  str.data = text;
  size_t need = bream_string_bufsize_utf16(str) + 1;

  GrowTextArrayIfNeeded(need);

  JNIEnv* env = getEnv();
  jchar* buf = static_cast<jchar*>(env->GetPrimitiveArrayCritical(
      g_text_array, NULL));
  bream_string_to_utf16(str, buf, need, NULL);
  env->ReleasePrimitiveArrayCritical(g_text_array, buf, 0);

  return need - 1;
}

static jint WriteToTextArray(const WCHAR* text, size_t length) {
  GrowTextArrayIfNeeded(length);

  JNIEnv* env = getEnv();
  jchar* buf = static_cast<jchar*>(env->GetPrimitiveArrayCritical(
      g_text_array, NULL));
  memcpy(buf, text, sizeof(WCHAR) * length);
  env->ReleasePrimitiveArrayCritical(g_text_array, buf, 0);

  return length;
}

static void GrowFloatArrayIfNeeded(size_t need) {
  MOP_ASSERT(g_float_array_size >= 0);
  size_t float_array_size = static_cast<size_t>(g_float_array_size);
  if (g_float_array && need <= float_array_size)
    return;

  MOP_ASSERT(need <= INT_MAX);
  g_float_array_size = MAX(float_array_size * 2, need);
  JNIEnv* env = getEnv();

  if (g_float_array)
    env->DeleteGlobalRef(g_float_array);

  scoped_localref<jfloatArray> tmp(env, env->NewFloatArray(g_float_array_size));
  g_float_array = static_cast<jfloatArray>(env->NewGlobalRef(*tmp));
}

unsigned int AndroidFont::StringWidth(const char* text,
                                      size_t length,
                                      int unscaled,
                                      int scaled) const {
  unsigned int w = 0;
  if (ThreadMutex_lock(g_text_mutex)) {
    jint len = WriteToTextArray(text, length);
    float scale = static_cast<float>(scaled) / unscaled;
    w = getEnv()->CallStaticIntMethod(g_font_clazz,
                                      g_id_get_text_width,
                                      g_text_array,
                                      0,
                                      len,
                                      scale,
                                      paint_);
    ThreadMutex_unlock(g_text_mutex);
  }
  return w;
}

unsigned int AndroidFont::StringWidth(const WCHAR* text,
                                      size_t length,
                                      int unscaled,
                                      int scaled) const {
  unsigned int w = 0;
  if (ThreadMutex_lock(g_text_mutex)) {
    jint len = WriteToTextArray(text, length);
    float scale = static_cast<float>(scaled) / unscaled;
    w = getEnv()->CallStaticIntMethod(g_font_clazz,
                                      g_id_get_text_width,
                                      g_text_array,
                                      0,
                                      len,
                                      scale,
                                      paint_);
    ThreadMutex_unlock(g_text_mutex);
  }
  return w;
}

void AndroidFont::CharactersWidth(const WCHAR* text,
                                  size_t length,
                                  UINT16* widths,
                                  int unscaled,
                                  int scaled) const {
  if (ThreadMutex_lock(g_text_mutex)) {
    jint len = WriteToTextArray(text, length);
    GrowFloatArrayIfNeeded(length);
    float scale = static_cast<float>(scaled) / unscaled;
    getEnv()->CallStaticVoidMethod(g_font_clazz,
                                  g_id_get_characters_widths,
                                  g_text_array,
                                  len,
                                  scale,
                                  paint_,
                                  g_float_array);
    ThreadMutex_unlock(g_text_mutex);

    JNIEnv* env = getEnv();
    jfloat* buf = static_cast<jfloat*>(env->GetPrimitiveArrayCritical(
        g_float_array, NULL));
    for (size_t i = 0; i < length; i++) {
      // Mini is not supporting negative character advances - see
      //   REKS5-727, ANDMO-3940.
      if (buf[i] < 0)
        widths[i] = 0;
      // ANDMO-4087 - samsung galaxy S3 returns width >
      //   UINT16 max value for some characters.
      else if (buf[i] > 1024)
        widths[i] = 1024;
      // Android returns non-integer character widths, when
      // Paint.SUBPIXEL_TEXT_FLAG is set.
      else
        widths[i] = (UINT16) round(buf[i]);
    }
    env->ReleasePrimitiveArrayCritical(g_float_array, buf, 0);
  }
}

void AndroidFont::DrawString(bgContext* context,
                             const char* text,
                             size_t length,
                             int x,
                             int y,
                             int unscaled,
                             int scaled,
                             BG_BLEND blend) const {
  if (x >= 0 && static_cast<unsigned int>(x) >= context->cx + context->cw)
    return;
  if (y >= 0 && static_cast<unsigned int>(y) >= context->cy + context->ch)
    return;
  if (y < static_cast<int>(context->cy) &&
      y + static_cast<int>(GetHeight(unscaled, scaled)) <= static_cast<int>(context->cy))
    return;

  unsigned int left = MAX(static_cast<int>(context->cx) - x, 0);
  unsigned int cw = context->cw;
  if (x > 0 && static_cast<unsigned int>(x) > context->cx)
    cw = context->cx + context->cw - static_cast<unsigned int>(x);

  if (ThreadMutex_lock(g_text_mutex)) {
    jint len = WriteToTextArray(text, length);
    float scale = static_cast<float>(scaled) / unscaled;
    JNIEnv* env = getEnv();
    jlong size = env->CallStaticLongMethod(g_font_clazz, g_id_draw_text,
                                           g_text_array, 0, len,
                                           left, cw,
                                           scale, paint_);
    unsigned int width = size >> 32;
    unsigned int height = size & 0xffffffff;

    if (width && height) {
      AndroidBitmapInfo info;
      scoped_localref<jobject> bitmap(
          env, env->GetStaticObjectField(g_font_clazz, g_id_text_bitmap));
      if (AndroidBitmap_getInfo(env, *bitmap, &info)) {
        ThreadMutex_unlock(g_text_mutex);
        return;
      }
      void* pixels;
      if (AndroidBitmap_lockPixels(env, *bitmap, &pixels)) {
        ThreadMutex_unlock(g_text_mutex);
        return;
      }
      swImage tmp;
      swWrapImageData(&tmp, pixels, width, height, info.stride, BG_A_8);
      context->drawRegion(context, &tmp.image, 0, 0, width, height,
                          x + left, y, blend);
      AndroidBitmap_unlockPixels(env, *bitmap);
    }
    ThreadMutex_unlock(g_text_mutex);
  }
}

MOP_CLASS_EXTENDS(MOpAndroidFont, MOpFont)
  AndroidFont* font;
  UINT32 id;
MOP_CLASS_EXTENDS_END(MOpAndroidFont);

MOP_CLASS_EXTENDS(MOpAndroidFontMetrics, MOpFontMetrics)
  AndroidFont* font;
MOP_CLASS_EXTENDS_END(MOpAndroidFontMetrics);

static MOP_STATUS MOpAndroidFontMetrics_create(MOpAndroidFontMetrics** metrics,
                                               AndroidFont* font);


static void MOpAndroidFont_dtor(MOpAndroidFont* _this) {
  MOpBase_dtor(reinterpret_cast<MOpBase*>(_this));
}

/* static */ AndroidFont* AndroidFont::GetFont(MOpFont* font) {
  return ((MOpAndroidFont*)font)->font;
}

static MOP_STATUS MOpAndroidFont_queryInterface(MOpAndroidFont* _this,
                                                UINT32 id,
                                                MOpBaseQueryRet* out) {
  switch (id) {
    case MOP_INTERFACE_ID_FONT:
      out->iface = reinterpret_cast<MOpIFace*>(_this);
      MOpFont_addRef(_this);
      break;

    case MOP_INTERFACE_ID_FONTMETRICS:
      MOpAndroidFontMetrics* metrics;
      MOP_RETURNIF(MOpAndroidFontMetrics_create(&metrics, _this->font));
      out->iface = reinterpret_cast<MOpIFace*>(metrics);
      break;

    default:
      return MOP_STATUS_NOT_IMPLEMENTED;
  }

  return MOP_STATUS_OK;
}

static BOOL MOpAndroidFont_getProperty(MOpAndroidFont* _this,
                                       MOpFontPropertyItem item,
                                       MOpFontPropertyValue* param) {
  switch (item) {
    case MOP_FONT_PROPERTY_FACE_NAME:
      param->str = _this->font->GetName();
      break;

    default:
      MOP_ASSERT(FALSE);
      return FALSE;
  }

  return TRUE;
}

static BOOL MOpAndroidFont_setProperty(MOpAndroidFont* _this UNUSED,
                                       MOpFontPropertyItem item UNUSED,
                                       MOpFontPropertyValue* param UNUSED) {
  MOP_ASSERT(FALSE);

  return FALSE;
}

static UINT32 MOpAndroidFont_getFontId(MOpAndroidFont* _this) {
  return _this->id;
}

static MOP_STATUS MOpAndroidFont_getTextWidth(MOpAndroidFont* _this,
                                              const TCHAR* text,
                                              INT32 len,
                                              UINT32 width,
                                              UINT32* char_fits,
                                              UINT32* text_width) {
  unsigned int used_pixels, used_chars;
  if (len < 0)
    len = mop_tstrlen(text);
  if (width == MOP_FONT_MAX_WIDTH) {
    used_chars = len;
    used_pixels = _this->font->StringWidth(text, len);
  } else {
    used_chars = fitStringUTF8(g_temp_context,
                               reinterpret_cast<bgFont *>(_this->font),
                               text, len, width, &used_pixels);
  }

 if (char_fits)
   *char_fits = used_chars;
 if (text_width)
   *text_width = used_pixels;

  return MOP_STATUS_OK;
}

static MOP_STATUS MOpAndroidFont_drawString(
    MOpAndroidFont* _this UNUSED,
    struct MOpGraphicsContext* context UNUSED,
    UINT32 ctx_opts UNUSED,
    const TCHAR* text UNUSED,
    INT32 len UNUSED,
    const struct _MOpRect* rect UNUSED,
    UINT32 color UNUSED) {

  MOP_ASSERT(FALSE);

  return MOP_STATUS_NOT_IMPLEMENTED;
}

static MOP_STATUS MOpAndroidFont_drawStringEx(
    MOpAndroidFont* _this UNUSED,
    UINT32 font_opts UNUSED,
    struct MOpGraphicsContext* context UNUSED,
    UINT32 ctx_opts UNUSED,
    const TCHAR* text UNUSED,
    INT32 len UNUSED,
    const struct _MOpRect* rect UNUSED,
    UINT32 color UNUSED,
    struct MOpFwdIterator* selection UNUSED) {

    MOP_ASSERT(FALSE);

    return MOP_STATUS_NOT_IMPLEMENTED;
}

static MOP_STATUS MOpAndroidFont_createWithScale(MOpAndroidFont* _this UNUSED,
                                                 UINT32 scale_p UNUSED,
                                                 UINT32 scale_q UNUSED,
                                                 MOpFont** out UNUSED) {
  MOP_ASSERT(FALSE);

  return MOP_STATUS_NOT_IMPLEMENTED;
}

static MOP_STATUS MOpAndroidFont_ctor(MOpAndroidFont* _this,
                                      AndroidFont* font) {
  MOP_RETURNIF(MOpBase_ctor(reinterpret_cast<MOpBase*>(_this)));

  _this->font = font;

  MOP_ASSERT(font->GetHeight() <= MOP_FT_FONT_MASK);
  _this->id = font->GetHeight() & MOP_FT_FONT_MASK;
  if (font->bold())
    _this->id |= MOP_FT_BOLD;
  if (font->italic())
    _this->id |= MOP_FT_ITALIC;
  if (font->monospace())
    _this->id |= MOP_FT_FACE_MONOSPACE;
  if (font->serif())
    _this->id |= MOP_FT_FACE_SERIF;

  return MOP_STATUS_OK;
}

static MOP_STATUS MOpAndroidFont_create(MOpAndroidFont** font,
                                        AndroidFont* android_font) {
  *font = static_cast<MOpAndroidFont*>(MOpClass_new(MOpAndroidFont));
  MOP_RETURNIF_NOMEM(*font);

  MOP_STATUS status = MOpAndroidFont_ctor(*font, android_font);
  if (status != MOP_STATUS_OK) {
    MOpFont_release(*font);
    *font = NULL;
    return status;
  }

  return MOP_STATUS_OK;
}

static void MOpAndroidFontMetrics_dtor(MOpAndroidFontMetrics* _this) {
  MOpBase_dtor(reinterpret_cast<MOpBase*>(_this));
}

static UINT32 MOpAndroidFontMetrics_getAscent(MOpAndroidFontMetrics* _this) {
  return _this->font->ascent();
}

static UINT32 MOpAndroidFontMetrics_getDescent(MOpAndroidFontMetrics* _this) {
  return _this->font->descent();
}

static UINT32 MOpAndroidFontMetrics_getHeight(MOpAndroidFontMetrics* _this) {
  return _this->font->GetHeight();
}

static UINT32 MOpAndroidFontMetrics_getMaxWidth(
    MOpAndroidFontMetrics* _this UNUSED) {

  MOP_ASSERT(FALSE);

  return 0;
}

static MOP_STATUS MOpAndroidFontMetrics_getWidth(
    MOpAndroidFontMetrics* _this,
    const TCHAR* text,
    INT32 len,
    UINT32 width UNUSED,
    UINT32* char_fits NDEBUG_UNUSED,
    UINT32* text_width) {

  MOP_ASSERT(!char_fits);

  *text_width = _this->font->StringWidth(text, len);

  return MOP_STATUS_OK;
}

static BOOL MOpAndroidFontMetrics_getNextCodePoint(
    MOpAndroidFontMetrics* _this UNUSED,
    WCHAR* codepoint UNUSED) {

  if (*codepoint >= 0xffff)
    return FALSE;

  (*codepoint)++;

  return TRUE;
}

static MOP_STATUS MOpAndroidFontMetrics_getCharactersWidths(
    MOpAndroidFontMetrics* _this,
    const WCHAR* chars,
    UINT16* widths,
    UINT32 count) {

  _this->font->CharactersWidth(chars, count, widths, 1, 1);
  return MOP_STATUS_OK;
}

static void MOpAndroidFontMetrics_prepareForInfoGeneration(
    MOpAndroidFontMetrics* _this UNUSED) {
}

static MOP_STATUS MOpAndroidFontMetrics_ctor(MOpAndroidFontMetrics* _this,
                                             AndroidFont* font) {
  MOP_RETURNIF(MOpBase_ctor(reinterpret_cast<MOpBase*>(_this)));

  _this->font = font;

  return MOP_STATUS_OK;
}

static MOP_STATUS MOpAndroidFontMetrics_create(MOpAndroidFontMetrics** metrics,
                                               AndroidFont* font) {
  *metrics = static_cast<MOpAndroidFontMetrics*>(MOpClass_new(MOpAndroidFontMetrics));
  MOP_RETURNIF_NOMEM(*metrics);

  MOP_STATUS status = MOpAndroidFontMetrics_ctor(*metrics, font);
  if (status != MOP_STATUS_OK) {
    MOpFontMetrics_release(*metrics);
    *metrics = NULL;
    return status;
  }

  return MOP_STATUS_OK;
}

MOP_CLASS_VTABLE_BEGIN(MOpAndroidFont, MOpBase)
  MOP_CLASS_VTABLE_SETMETHOD(MOpIFace, dtor) =
      (MOpIFace_Dtor*) MOpAndroidFont_dtor;
  MOP_CLASS_VTABLE_SETMETHOD(MOpBaseRef, queryInterface) =
      (MOpBaseRef_QueryInterface*) MOpAndroidFont_queryInterface;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, getProperty) =
      (MOpFont_GetProperty*) MOpAndroidFont_getProperty;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, setProperty) =
      (MOpFont_SetProperty*) MOpAndroidFont_setProperty;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, getFontId) =
      (MOpFont_GetFontId*) MOpAndroidFont_getFontId;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, getTextWidth) =
      (MOpFont_GetTextWidth*) MOpAndroidFont_getTextWidth;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, drawString) =
      (MOpFont_DrawString*) MOpAndroidFont_drawString;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, drawStringEx) =
      (MOpFont_DrawStringEx*) MOpAndroidFont_drawStringEx;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFont, createWithScale) =
      (MOpFont_CreateWithScale*) MOpAndroidFont_createWithScale;
MOP_CLASS_VTABLE_END

MOP_CLASS_VTABLE_BEGIN(MOpAndroidFontMetrics, MOpBase)
  MOP_CLASS_VTABLE_SETMETHOD(MOpIFace, dtor) =
      (MOpIFace_Dtor*) MOpAndroidFontMetrics_dtor;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getAscent) =
      (MOpFontMetrics_GetAscent*) MOpAndroidFontMetrics_getAscent;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getDescent) =
      (MOpFontMetrics_GetDescent*) MOpAndroidFontMetrics_getDescent;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getHeight) =
      (MOpFontMetrics_GetHeight*) MOpAndroidFontMetrics_getHeight;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getMaxWidth) =
      (MOpFontMetrics_GetMaxWidth*) MOpAndroidFontMetrics_getMaxWidth;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getWidth) =
      (MOpFontMetrics_GetWidth*) MOpAndroidFontMetrics_getWidth;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getNextCodePoint) =
      (MOpFontMetrics_GetNextCodePoint*) MOpAndroidFontMetrics_getNextCodePoint;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, writeUniqueData) =
      (MOpFontMetrics_WriteUniqueData*) MOpFontMetricsEx_writeUniqueData;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, getCharactersWidths) =
      (MOpFontMetrics_GetCharactersWidths*) MOpAndroidFontMetrics_getCharactersWidths;
  MOP_CLASS_VTABLE_SETMETHOD(MOpFontMetrics, prepareForInfoGeneration) =
      (MOpFontMetrics_PrepareForInfoGeneration*) MOpAndroidFontMetrics_prepareForInfoGeneration;
MOP_CLASS_VTABLE_END;

MOP_STATUS AndroidFont::CreateCollection(MOpFontsCollection** collection) {
  MOP_RETURNIF(MOpFontsCollectionFactory_create(
      collection, MOP_FONTS_COLLECTION_FACTORY_PREFERRED));

  MOP_STATUS status = MOP_STATUS_OK;

  for (UINT16 obml_font = 0; obml_font < g_num_fonts; obml_font++) {
    MOpAndroidFont* font;
    MOP_BREAKIF(status = MOpAndroidFont_create(
        &font,
        AndroidFont::GetFont(obml_font)));

    status = MOpFontsCollection_addFont(*collection,
                                        reinterpret_cast<MOpFont*>(font));
    MOpFont_release(font);
    MOP_BREAKIF(status);
  }

  if (status != MOP_STATUS_OK) {
    MOpFontsCollection_release(*collection);
    *collection = NULL;
    return status;
  }

  return MOP_STATUS_OK;
}

#ifdef MOP_HAVE_FONT_SCALE_RUNTIME_DETECT
UINT16 MOpSystemInfo_getFontScale(UINT32 size) {
  return AndroidFont::GetFontScale(size);
}
#endif // MOP_HAVE_FONT_SCALE_RUNTIME_DETECT

bgFont* MOpFont_getBGFont(MOpFont* font)
{
  return font ? (bgFont*)((MOpAndroidFont*)font)->font : NULL;
}
