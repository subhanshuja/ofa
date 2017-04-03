/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2014 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"
#include <jni.h>
#include <math.h>

#include <android/bitmap.h>

#include "canvascontext.h"
#include "jniutils.h"
#include "platform.h"

OP_EXTERN_C_BEGIN

#include "bream/vm/c/graphics/bream_graphics_backend.h"

// android.graphics.Bitmap
static jclass g_bitmap_class;
static jmethodID g_bitmap_createBitmap;

// android.graphics.Bitmap.Config
static jobject g_bitmapconfig_A_8;
static jobject g_bitmapconfig_RGB_565;
static jobject g_bitmapconfig_ARGB_8888;

// android.graphics.Canvas
static jclass g_canvas_class;
static jmethodID g_canvas_clipRect;
static jmethodID g_canvas_drawBitmap;
static jmethodID g_canvas_drawBitmap_FF;
static jmethodID g_canvas_drawRect;
static jmethodID g_canvas_save;
static jmethodID g_canvas_save_I;
static jmethodID g_canvas_restore;
#define CLIP_SAVE_FLAG 2

// android.graphics.Paint
static jclass g_paint_class;
static jmethodID g_paint_setARGB;

// android.graphics.Rect
static jclass g_rect_class;
static jfieldID g_rect_left;
static jfieldID g_rect_top;
static jfieldID g_rect_right;
static jfieldID g_rect_bottom;

// android.graphics.RectF
static jclass g_rectf_class;
static jfieldID g_rectf_left;
static jfieldID g_rectf_top;
static jfieldID g_rectf_right;
static jfieldID g_rectf_bottom;


struct CanvasImage
{
    bgImage image;
    jobject bitmap;
    BG_FORMAT format;
};


static void canvasDisposeImage(bgImage *image)
{
    BREAM_ASSERT(image->type == BG_TYPE_CANVAS);
    if (bgImage_decRef(image))
    {
        getEnv()->DeleteGlobalRef(((CanvasImage*)image)->bitmap);
        bream_free(image);
    }
}

static unsigned int canvasGetImageSize(bgImage *image)
{
    return bgPixelSize(((CanvasImage*)image)->format) *
        image->width * image->height;
}

static BG_ALPHA canvasGetImageAlpha(bgImage *image)
{
    return bgGetAlpha(((CanvasImage*)image)->format);
}

static bgImage *canvasCreateImage(unsigned int width, unsigned int height,
                                  BG_FORMAT format, jobject canvasFormat)
{
    JNIEnv *env = getEnv();
    CanvasImage *image = (CanvasImage*)bream_malloc(sizeof(CanvasImage));
    if (!image)
    {
        return NULL;
    }
    fallbackInitializeImage(&image->image, BG_TYPE_CANVAS, width, height);

    scoped_localref<jobject> bitmap(env, env->CallStaticObjectMethod(g_bitmap_class, g_bitmap_createBitmap, width, height, canvasFormat));
    image->bitmap = env->NewGlobalRef(*bitmap);
    if (!image->bitmap)
    {
        bream_free(image);
        return NULL;
    }

    image->format = format;
    image->image.dispose = canvasDisposeImage;
    image->image.getSize = canvasGetImageSize;
    image->image.getAlpha = canvasGetImageAlpha;
    return &image->image;
}


static bgImage *canvasConvertImage(bgContext *context UNUSED, bgImage *image, BOOL copy UNUSED, BOOL eager UNUSED)
{
    JNIEnv *env = getEnv();
    int *pixels;
    swImage tmp;
    swContext swcontext;
    jobject canvasFormat = g_bitmapconfig_ARGB_8888;
    BG_FORMAT format = BG_RGBA_8888;
    if (image->type == BG_TYPE_SW)
    {
        BG_FORMAT srcFormat = ((swImage*)image)->format;
        if (srcFormat == BG_A_8)
        {
            canvasFormat = g_bitmapconfig_A_8;
            format = BG_A_8;
        }
        else if (bgGetAlpha(srcFormat) == BG_ALPHA_NONE)
        {
            canvasFormat = g_bitmapconfig_RGB_565;
            format = BG_RGB_565_BS;
        }
    }
    CanvasImage *result = (CanvasImage*)canvasCreateImage(
        image->width, image->height, format, canvasFormat);
    if (!result)
    {
        return NULL;
    }
    AndroidBitmap_lockPixels(env, result->bitmap, (void**)&pixels);
    swWrapImageData(&tmp, pixels, image->width, image->height, image->width * bgPixelSize(format), format);
    swInitializeContext(context->platform, &swcontext, &tmp, format);
    swcontext.context.drawImage(&swcontext.context, image, 0, 0, BG_BLEND_COPY);
    swDisposeContext(&swcontext);
    AndroidBitmap_unlockPixels(env, result->bitmap);
    return &result->image;
}

static BOOL canvasCompatibleImage(bgContext *context UNUSED, bgImage *image)
{
    switch (image->type)
    {
    case BG_TYPE_SW:
    case BG_TYPE_COMPOUND:
    case BG_TYPE_CANVAS:
        return TRUE;
    case BG_TYPE_STRETCH:
        return canvasCompatibleImage(context, ((bgStretchedImage*)image)->source);
    case BG_TYPE_PALETTE:
    case BG_TYPE_GLES1:
    case BG_TYPE_CG:
    case BG_TYPE_D2D:
    case BG_TYPE_D2D_WORKERBUF:
        break;
    }
    return FALSE;
}

static void canvasFillRect(bgContext *context UNUSED, int x UNUSED, int y UNUSED,
                           unsigned int width UNUSED, unsigned int height UNUSED, UINT32 color UNUSED)
{
    JNIEnv *env = getEnv();
    CanvasContext *cc = (CanvasContext*)context;

    BREAM_ASSERT(context->type == BG_TYPE_CANVAS);

    if (!bgClipDestination(context, &x, &y, &width, &height))
    {
        return;
    }

    color = bgBlendColor(context->blendColor, color);
    env->CallVoidMethod(cc->paint, g_paint_setARGB, (color >> 24) & 0xff, (color >> 16) & 0xff,
                        (color >> 8) & 0xff, color & 0xff);
    env->CallVoidMethod(cc->canvas, g_canvas_drawRect,
                        (float)x, (float)y, (float)(x + width), (float)(y + height), cc->paint);
}

static void canvasDrawRegion(bgContext *context UNUSED, bgImage *image UNUSED,
                             unsigned int srcx UNUSED, unsigned int srcy UNUSED,
                             unsigned int width UNUSED, unsigned int height UNUSED,
                             int dstx UNUSED, int dsty UNUSED, BG_BLEND blend UNUSED)
{
    JNIEnv *env = getEnv();
    CanvasContext *cc = (CanvasContext*)context;
    int cx, cy;
    unsigned int cw, ch;

    BREAM_ASSERT(context->type == BG_TYPE_CANVAS);

    if (image->type == BG_TYPE_STRETCH || image->type == BG_TYPE_COMPOUND)
    {
        fallbackDrawRegion(context, image, srcx, srcy, width, height,
                           dstx, dsty, blend);
        return;
    }

    if (image->type == BG_TYPE_SW)
    {
        image = canvasConvertImage(context, image, TRUE, TRUE);
        if (!image)
        {
            MOpDebug("Failed to create temporary image");
            return;
        }
        canvasDrawRegion(context, image, srcx, srcy, width, height, dstx, dsty, blend);
        image->dispose(image);
        return;
    }

    BREAM_ASSERT(srcx <= image->width);
    BREAM_ASSERT(srcx + width <= image->width);
    BREAM_ASSERT(srcy <= image->height);
    BREAM_ASSERT(srcy + height <= image->height);

    BREAM_ASSERT(image->type == BG_TYPE_CANVAS);

    cx = dstx;
    cy = dsty;
    cw = width;
    ch = height;
    if (!bgClipDestination(context, &cx, &cy, &cw, &ch))
    {
        return;
    }

    env->CallIntMethod(cc->canvas, g_canvas_save_I, CLIP_SAVE_FLAG);
    env->CallBooleanMethod(cc->canvas, g_canvas_clipRect,
                           cx, cy, cx + cw, cy + ch);

    int color = context->blendColor;
    env->CallVoidMethod(cc->paint, g_paint_setARGB,
                        (color >> 24) & 0xff, (color >> 16) & 0xff,
                        (color >> 8) & 0xff, color & 0xff);

    env->CallVoidMethod(cc->canvas, g_canvas_drawBitmap_FF,
                        ((CanvasImage*)image)->bitmap,
                        (float)(dstx - (int)srcx), (float)(dsty - (int)srcy),
                        cc->paint);

    env->CallVoidMethod(cc->canvas, g_canvas_restore);
}

static void canvasDrawRegionStretched(bgContext *context UNUSED, bgImage *image UNUSED,
                                      unsigned int srcx UNUSED, unsigned int srcy UNUSED,
                                      unsigned int srcw UNUSED, unsigned int srch UNUSED,
                                      int dstx UNUSED, int dsty UNUSED,
                                      unsigned int dstw UNUSED, unsigned int dsth UNUSED,
                                      BG_BLEND blend UNUSED, BG_STRETCH stretch UNUSED)
{
    JNIEnv *env = getEnv();
    CanvasContext *cc = (CanvasContext*)context;
    int cx, cy;
    unsigned int cw, ch;

    BREAM_ASSERT(context->type == BG_TYPE_CANVAS);

    if (image->type == BG_TYPE_STRETCH || image->type == BG_TYPE_COMPOUND)
    {
        fallbackDrawRegionStretched(context, image, srcx, srcy, srcw, srch,
                                    dstx, dsty, dstw, dsth, blend, stretch);
        return;
    }

    if (image->type == BG_TYPE_SW)
    {
        image = canvasConvertImage(context, image, TRUE, TRUE);
        if (!image)
        {
            MOpDebug("Failed to create temporary image");
            return;
        }
        canvasDrawRegionStretched(context, image, srcx, srcy, srcw, srch,
                                  dstx, dsty, dstw, dsth, blend, stretch);
        image->dispose(image);
        return;
    }

    BREAM_ASSERT(srcx <= image->width);
    BREAM_ASSERT(srcx + srcw <= image->width);
    BREAM_ASSERT(srcy <= image->height);
    BREAM_ASSERT(srcy + srch <= image->height);

    BREAM_ASSERT(image->type == BG_TYPE_CANVAS);

    cx = dstx;
    cy = dsty;
    cw = dstw;
    ch = dsth;
    if (!bgClipDestination(context, &cx, &cy, &cw, &ch))
    {
        return;
    }

    env->CallIntMethod(cc->canvas, g_canvas_save_I, CLIP_SAVE_FLAG);
    env->CallBooleanMethod(cc->canvas, g_canvas_clipRect,
                           cx, cy, cx + cw, cy + ch);

    int color = context->blendColor;
    env->CallVoidMethod(cc->paint, g_paint_setARGB,
                        (color >> 24) & 0xff, (color >> 16) & 0xff,
                        (color >> 8) & 0xff, color & 0xff);

    env->SetIntField(cc->rect, g_rect_left, srcx);
    env->SetIntField(cc->rect, g_rect_top, srcy);
    env->SetIntField(cc->rect, g_rect_right, srcx + srcw);
    env->SetIntField(cc->rect, g_rect_bottom, srcy + srch);
    env->SetFloatField(cc->rectf, g_rectf_left, dstx);
    env->SetFloatField(cc->rectf, g_rectf_top, dsty);
    env->SetFloatField(cc->rectf, g_rectf_right, ceilf(dstx + dstw));
    env->SetFloatField(cc->rectf, g_rectf_bottom, ceilf(dsty + dsth));

    env->CallVoidMethod(cc->canvas, g_canvas_drawBitmap,
                        ((CanvasImage*)image)->bitmap,
                        cc->rect, cc->rectf,
                        cc->paint);

    env->CallVoidMethod(cc->canvas, g_canvas_restore);
}

static void canvasDrawRegionStretchedf(bgContext *context UNUSED, bgImage *image UNUSED,
                                       float srcx1, float srcy1,
                                       float srcx2, float srcy2,
                                       float dstx1, float dsty1,
                                       float dstx2, float dsty2,
                                       BG_BLEND blend UNUSED, BG_STRETCH stretch UNUSED)
{
    JNIEnv *env = getEnv();
    CanvasContext *cc = (CanvasContext*)context;

    BREAM_ASSERT(context->type == BG_TYPE_CANVAS);

    if (image->type == BG_TYPE_STRETCH || image->type == BG_TYPE_COMPOUND)
    {
        fallbackDrawRegionStretchedf(context, image, srcx1, srcy1, srcx2, srcy2,
                                     dstx1, dsty1, dstx2, dsty2, blend, stretch);
        return;
    }

    if (image->type == BG_TYPE_SW)
    {
        image = canvasConvertImage(context, image, TRUE, TRUE);
        if (!image)
        {
            MOpDebug("Failed to create temporary image");
            return;
        }
        canvasDrawRegionStretchedf(context, image, srcx1, srcy1, srcx2, srcy2,
                                   dstx1, dsty1, dstx2, dsty2, blend, stretch);
        image->dispose(image);
        return;
    }

    BREAM_ASSERT(srcx1 <= image->width);
    BREAM_ASSERT(srcx2 <= image->width);
    BREAM_ASSERT(srcy1 <= image->height);
    BREAM_ASSERT(srcy2 <= image->height);

    BREAM_ASSERT(image->type == BG_TYPE_CANVAS);

    float origdstx1, origdsty1, origdstx2, origdsty2;
    origdstx1 = dstx1;
    origdsty1 = dsty1;
    origdstx2 = dstx2;
    origdsty2 = dsty2;
    if (!bgClipDestinationf(context, &dstx1, &dsty1, &dstx2, &dsty2))
    {
        return;
    }
    bgStretchSourceCoordinatesf(&srcx1, &srcx2,
                                srcx1, srcx2, origdstx1, origdstx2,
                                context->cx, context->cw);
    bgStretchSourceCoordinatesf(&srcy1, &srcy2,
                                srcy1, srcy2, origdsty1, origdsty2,
                                context->cy, context->ch);

    env->CallIntMethod(cc->canvas, g_canvas_save_I, CLIP_SAVE_FLAG);

    int color = context->blendColor;
    env->CallVoidMethod(cc->paint, g_paint_setARGB,
                        (color >> 24) & 0xff, (color >> 16) & 0xff,
                        (color >> 8) & 0xff, color & 0xff);

    env->SetIntField(cc->rect, g_rect_left, srcx1);
    env->SetIntField(cc->rect, g_rect_top, srcy1);
    env->SetIntField(cc->rect, g_rect_right, srcx2);
    env->SetIntField(cc->rect, g_rect_bottom, srcy2);
    env->SetFloatField(cc->rectf, g_rectf_left, dstx1);
    env->SetFloatField(cc->rectf, g_rectf_top, dsty1);
    env->SetFloatField(cc->rectf, g_rectf_right, ceilf(dstx2));
    env->SetFloatField(cc->rectf, g_rectf_bottom, ceilf(dsty2));

    env->CallVoidMethod(cc->canvas, g_canvas_drawBitmap,
                        ((CanvasImage*)image)->bitmap,
                        cc->rect, cc->rectf,
                        cc->paint);

    env->CallVoidMethod(cc->canvas, g_canvas_restore);
}

OP_EXTERN_C_END

void canvasContextStaticInit(JNIEnv *env)
{
    {
        scoped_localref<jclass> paint_class(env, env->FindClass("android/graphics/Paint"));
        g_paint_class = static_cast<jclass>(env->NewGlobalRef(*paint_class));
        g_paint_setARGB = env->GetMethodID(*paint_class, "setARGB", "(IIII)V");
    }

    {
        scoped_localref<jclass> bitmap_class(env, env->FindClass("android/graphics/Bitmap"));
        g_bitmap_class = static_cast<jclass>(env->NewGlobalRef(*bitmap_class));
        g_bitmap_createBitmap = env->GetStaticMethodID(
            g_bitmap_class, "createBitmap",
            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    }

    {
        scoped_localref<jclass> bitmapconfig_class(env, env->FindClass("android/graphics/Bitmap$Config"));
        jfieldID a_8 = env->GetStaticFieldID(*bitmapconfig_class, "ALPHA_8", "Landroid/graphics/Bitmap$Config;");
        g_bitmapconfig_A_8 = env->NewGlobalRef(env->GetStaticObjectField(*bitmapconfig_class, a_8));
        jfieldID argb_8888 = env->GetStaticFieldID(*bitmapconfig_class, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        g_bitmapconfig_ARGB_8888 = env->NewGlobalRef(env->GetStaticObjectField(*bitmapconfig_class, argb_8888));
        jfieldID rgb_565 = env->GetStaticFieldID(*bitmapconfig_class, "RGB_565", "Landroid/graphics/Bitmap$Config;");
        g_bitmapconfig_RGB_565 = env->NewGlobalRef(env->GetStaticObjectField(*bitmapconfig_class, rgb_565));
    }

    {
        scoped_localref<jclass> canvas_class(env, env->FindClass("android/graphics/Canvas"));
        g_canvas_class = static_cast<jclass>(env->NewGlobalRef(*canvas_class));
        g_canvas_clipRect = env->GetMethodID(g_canvas_class, "clipRect", "(IIII)Z");
        g_canvas_drawBitmap = env->GetMethodID(g_canvas_class, "drawBitmap", "(Landroid/graphics/Bitmap;Landroid/graphics/Rect;Landroid/graphics/RectF;Landroid/graphics/Paint;)V");
        g_canvas_drawBitmap_FF = env->GetMethodID(g_canvas_class, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
        g_canvas_drawRect = env->GetMethodID(g_canvas_class, "drawRect", "(FFFFLandroid/graphics/Paint;)V");
        g_canvas_save = env->GetMethodID(g_canvas_class, "save", "()I");
        g_canvas_save_I = env->GetMethodID(g_canvas_class, "save", "(I)I");
        g_canvas_restore = env->GetMethodID(g_canvas_class, "restore", "()V");
    }

    {
        scoped_localref<jclass> rect_class(env, env->FindClass("android/graphics/Rect"));
        g_rect_class  = static_cast<jclass>(env->NewGlobalRef(*rect_class));
        g_rect_left   = env->GetFieldID(*rect_class, "left", "I");
        g_rect_top    = env->GetFieldID(*rect_class, "top", "I");
        g_rect_right  = env->GetFieldID(*rect_class, "right", "I");
        g_rect_bottom = env->GetFieldID(*rect_class, "bottom", "I");
    }

    {
        scoped_localref<jclass> rectf_class(env, env->FindClass("android/graphics/RectF"));
        g_rectf_class  = static_cast<jclass>(env->NewGlobalRef(*rectf_class));
        g_rectf_left   = env->GetFieldID(*rectf_class, "left", "F");
        g_rectf_top    = env->GetFieldID(*rectf_class, "top", "F");
        g_rectf_right  = env->GetFieldID(*rectf_class, "right", "F");
        g_rectf_bottom = env->GetFieldID(*rectf_class, "bottom", "F");
    }
}

void initCanvasContext(JNIEnv *env, bgPlatform *platform, CanvasContext *context)
{
    fallbackInitializeContext(platform, &context->context, BG_TYPE_CANVAS, BG_RGB_565);

    context->canvas = NULL;
    scoped_localref<jobject> paint(env, env->NewObject(g_paint_class, env->GetMethodID(g_paint_class, "<init>", "()V")));
    context->paint = env->NewGlobalRef(*paint);

    scoped_localref<jobject> rect(env, env->NewObject(g_rect_class, env->GetMethodID(g_rect_class, "<init>", "()V")));
    context->rect = env->NewGlobalRef(*rect);

    scoped_localref<jobject> rectf(env, env->NewObject(g_rectf_class, env->GetMethodID(g_rectf_class, "<init>", "()V")));
    context->rectf = env->NewGlobalRef(*rectf);

    context->context.dispose = bgNoopDisposeContext;
    context->context.convertImage = canvasConvertImage;
    context->context.compatibleImage = canvasCompatibleImage;
    context->context.fillRect = canvasFillRect;
    context->context.drawRegion = canvasDrawRegion;
    context->context.drawRegionStretched = canvasDrawRegionStretched;
    context->context.drawRegionStretchedf = canvasDrawRegionStretchedf;
}

void setCanvasContextTarget(CanvasContext *context, jobject canvas,
                            unsigned int width, unsigned int height)
{
    context->canvas = canvas;
    context->context.cx = 0;
    context->context.cy = 0;
    context->context.cw = width;
    context->context.ch = height;
    context->context.width = width;
    context->context.height = height;
}

jobject createAndroidCanvasFromBitmap(JNIEnv *env, jobject bitmap)
{
    return env->NewObject(g_canvas_class, env->GetMethodID(g_canvas_class, "<init>", "(Landroid/graphics/Bitmap;)V"), bitmap);
}
