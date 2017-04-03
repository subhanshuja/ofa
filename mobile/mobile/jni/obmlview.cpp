/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <assert.h>
#include <android/bitmap.h>
#include <math.h>
#include <time.h>

#include "jni_obfuscation.h"
#include "jniutils.h"
#include "com_opera_android_bream_Bream.h"
#include "com_opera_android_browser_obml_OBMLView.h"

#include "bream.h"
#include "canvascontext.h"
#include "font.h"
#include "obmlview.h"
#include "platform.h"
#include "vmentry_utils.h"
#include "vminstance_ext.h"
#include "workerthread.h"

#include "bream/vm/c/inc/bream_host.h"
#include "bream/vm/c/inc/bream_const.h"

OP_EXTERN_C_BEGIN

#include "bream/vm/c/graphics/thread.h"
#include "bream/vm/c/graphics/deferredpainter.h"
#include "bream/vm/c/graphics/focus.h"

#include "mini/c/shared/core/components/ImageCache.h"
#include "mini/c/shared/core/graphics/BreamGraphicsContext.h"
#include "mini/c/shared/core/graphics/GraphicsContext.h"
#include "mini/c/shared/core/hardcore/GlobalMemory.h"
#include "mini/c/shared/core/obml/ObmlData.h"
#include "mini/c/shared/core/obml/ObmlDocument.h"
#include "mini/c/shared/core/pi/ScreenGraphicsContext.h"
#include "mini/c/shared/core/pi/SystemInfo.h"
#include "mini/c/shared/core/renderer/FoldData.h"
#include "mini/c/shared/core/renderer/GuiResources.h"
#include "mini/c/shared/core/renderer/ObmlRenderer.h"
#include "mini/c/shared/core/renderer/RendererTiling.h"
#include "mini/c/shared/generic/pi/WorkerThread.h"
#include "mini/c/shared/generic/vm_integ/ObjectWrapper.h"
#include "mini/c/shared/generic/vm_integ/TilesContainer.h"
#include "mini/c/shared/generic/vm_integ/SavedPages.h"
#include "mini/c/shared/generic/vm_integ/VMEntries.h"
#include "mini/c/shared/generic/vm_integ/VMFunctions.h"
#include "mini/c/shared/generic/vm_integ/VMInstance.h"

#ifdef MOP_FEATURE_ODP
#include "com_opera_android_portal_PortalView.h"
#include "mini/c/shared/core/components/Portal.h"
#endif /* MOP_FEATURE_ODP */

struct bgFont* painter_getFontBogus(struct bgContext* context,
                                    obmlDocument* doc,
                                    UINT16 obmlFont);

static CanvasContext canvasContext;

#define SCROLLBAR_THICKNESS 5
#define SCROLLBAR_MARGIN 6
#define SCROLLBAR_COLOR 0xff909090
#define SCROLLBAR_CORNER_COLOR 0x84909090

static bgImage *fsbDU, *fsbDP, *fsbUU, *fsbUP;
static int fsbPadding;

OP_EXTERN_C_END

#define OMEXPORT_ONLOAD JNIEXPORT

static jclass obmlViewClass;

static void initObmlViewClass(JNIEnv* env, jclass clazz)
{
    if (!obmlViewClass)
    {
        obmlViewClass = (jclass)env->NewGlobalRef(clazz);
        assert(obmlViewClass);
    }
}

struct OBMLView
{
    static gles1Context* gles1;
    static bgContext* context;

    jobject javaInstance;

    bream_handle_t breamBrowser;

    int* focusArea;
    size_t focusAreas;
    jint focusAreaPaintWidth;
    int* linkCandidateArea;
    size_t linkCandidateAreas;
    jint linkCandidateAreaPaintWidth;

    OBMLView(JNIEnv* env, jobject o, jobject portalListener, BOOL priv):
        javaInstance(NULL),
        focusArea(NULL), focusAreas(0),
        linkCandidateArea(NULL), linkCandidateAreas(0)
    {
        ASSERT_BREAM_THREAD();
        // Static init must be done before we try to create a new browser.
        MOP_ASSERT(obmlViewClass);
        MOP_ASSERT(context && gles1);

        bream_init_handle(&breamBrowser);

        MOpObjectWrapper* wrapper = get_object_wrapper(env, o);
        bream_register_handle(g_bream_vm, &breamBrowser);
        if (portalListener) {
            bream_ptr_t portalWrapper;
            if (wrap_object(env, g_bream_vm, portalListener, &portalWrapper)) {
                MOpDebug("Failed to init portal listener");
            }
            MOpVMEntry_nativeui_api_createPortalBrowser(
                &g_vm_instance.base, wrapper, portalWrapper, &breamBrowser.ptr);
        } else {
            MOpVMEntry_nativeui_api_createBrowser(
                &g_vm_instance.base, wrapper, priv, &breamBrowser.ptr);
        }
        MOpVMEntry_nativeui_api_NativeUIBrowser_setViewportSize(
            &g_vm_instance.base, breamBrowser.ptr,
            context->width, context->height, context->height);

        javaInstance = env->NewGlobalRef(o);
    }


    virtual ~OBMLView()
    {
        if (breamBrowser.ptr) {
            MOpVMEntry_nativeui_api_NativeUIBrowser_dispose(
                &g_vm_instance.base, breamBrowser.ptr);
        }
        bream_unregister_handle(g_bream_vm, &breamBrowser);
        if (javaInstance) {
            JNIEnv *env = getEnv();
            env->DeleteGlobalRef(javaInstance);
        }
        free(focusArea);
        free(linkCandidateArea);
    }

    static void staticInit(JNIEnv *env, MOpVMInstance* inst, jint selectionColor) {
        ASSERT_BREAM_THREAD();

        gles1 = gles1CreateContext(&g_bg_platform);
        initCanvasContext(env, &g_bg_platform, &canvasContext);
        ((bgContext*)gles1)->width = g_screen_width;
        ((bgContext*)gles1)->height = g_screen_height;
        context = (bgContext*)dpCreateContext((bgContext*)gles1, DP_FLAGS_MULTIPLE_THREADS | DP_FLAGS_SPLIT_FRAME);

        // We shouldn't be called if we don't have a size.
        MOP_ASSERT(g_screen_width > 0 && g_screen_height > 0);

        if (context->width != g_screen_width) {
            context->cw = g_screen_width;
            context->ch = g_screen_height;
            context->width = g_screen_width;
            context->height = g_screen_height;
        }

        GlobalMemory* gm = GlobalMemory_get();
        if (!gm->guiResources) {
            BG_FORMAT format =
                ((bgContext*)gles1)->optimalImageFormat((bgContext*)gles1, BG_ALPHA_FULL);
            MOpGuiResources* res;
            MOpGraphicsContext* resctx;
            bgContext* empty;
            /* Cannot use real context here as that would mean the resources
             * would be optimized for drawing using GL when they in fact will be
             * drawn using software on tiles. */
            empty = (bgContext*)swCreateEmptyContext(&g_bg_platform, format);
            MOpBreamGraphicsContext_create((MOpBreamGraphicsContext**)&resctx,
                                           empty);
            if (MOpGuiResources_create(&res, resctx) != MOP_STATUS_OK) {
                MOpDebug("Failed to init GUI resources");
            }
            MOpGraphicsContext_release(resctx);
            empty->dispose(empty);
            gm->guiResources = res;
        }
        if (!gm->tilesContainer) {
            MOpTilesContainer* container;
            MOpTilesContainer_create(&container, (bgContext*)gles1, NULL, updateContext, invalidateArea);
            MOpTilesContainer_setVMInstance(container, inst);
            if (selectionColor) {
                MOpTilesContainer_setSelectionColor(container, selectionColor);
            }
            gm->tilesContainer = container;
        }
    }

    void setVisibleArea(jint x, jint y, jint paintWidth,
                        ENUM_BREAM_UTIL_DIRECTION scrollDirection,
                        jboolean dragging, jboolean panning,
                        jboolean zooming,
                        jint targetX, jint targetY, jint targetZoom) {
        ASSERT_BREAM_THREAD();
        MOpVMEntry_nativeui_api_NativeUIBrowser_setVisibleArea(
                &g_vm_instance.base, breamBrowser.ptr,
                x, y, context->width, context->ch, paintWidth,
                scrollDirection,
                dragging, panning, zooming,
                targetX, targetY, context->width, context->ch, targetZoom);
    }

    static void glPaint() {
        dpPaintFrame((dpContext*)context);
    }

    void swPaint(jobject canvas, int x, int y, int docWidth, int paintWidth, int topOverScroll) {
        setCanvasContextTarget(&canvasContext, canvas, context->width, context->height);
        canvasContext.context.cx = context->cx;
        canvasContext.context.cy = context->cy;
        canvasContext.context.cw = context->cw;
        canvasContext.context.ch = context->ch;
        paintFrame(&canvasContext.context, x, y, docWidth, paintWidth,
                topOverScroll, 0, 0, FALSE, FALSE);
    }

    static void surfaceCreated(unsigned int width, unsigned int height) {
        gles1SurfaceCreated(gles1);
        revalidateTiles(getTilesContainer());
        surfaceChanged(width, height);
    }

    static void revalidateTiles(MOpTilesContainer* container) {
        MOP_ASSERT(container);
        MOpTilesContainer_revalidateImages(container);
    }

    static void surfaceChanged(unsigned int width, unsigned int height) {
        MOpDebug("glSurfaceChanged %ux%u", width, height);
        gles1SurfaceChanged(gles1, width, height);
        invalidate();
    }

    void setViewportSize(unsigned int width, unsigned int height,
                         unsigned int verticalViewportOffset, unsigned int topOverScroll) {
        ASSERT_BREAM_THREAD();
        // Resize and repaint the deferred painter
        context->width = width;
        context->height = height;
        context->cy = verticalViewportOffset;
        context->cw = width;
        context->ch = height - verticalViewportOffset;
        MOpVMEntry_nativeui_api_NativeUIBrowser_setViewportSize(
            &g_vm_instance.base, breamBrowser.ptr,
            width, height - verticalViewportOffset, height - topOverScroll);
    }

    bream_ptr_t getBreamBrowser() {
        return breamBrowser.ptr;
    }

    bream_t* getVM() {
        return g_bream_vm;
    }

    static MOpTilesContainer* getTilesContainer()
    {
        return (MOpTilesContainer*) GlobalMemory_get()->tilesContainer;
    }

    MOpDocumentLite *getDocument() {
        bream_ptr_t p = 0;
        MOpVMEntry_nativeui_api_NativeUIBrowser_getCurrentDocument(
            &g_vm_instance.base, breamBrowser.ptr, &p);
        return (MOpDocumentLite*)MOpObjectWrapper_get(MOpVMFunctions_loadObject(g_bream_vm, p));
    }

    static void drawScrollBar(bgContext *context, int x, int y, int width, int height) {
        context->fillRect(context, x, y, width, 1, SCROLLBAR_CORNER_COLOR);
        context->fillRect(context, x, y + height - 1, width, 1, SCROLLBAR_CORNER_COLOR);
        context->fillRect(context, x + 1, y, width - 2, height, SCROLLBAR_COLOR);
        context->fillRect(context, x, y + 1, width, height - 2, SCROLLBAR_COLOR);
    }

    void paintFrame(bgContext *context, int x, int y, int docWidth, int paintWidth,
            int topOverScroll, int scrollBarAlpha, int fastScrollButtonAlpha,
            BOOL fsbUp, BOOL fsbPressed) {
        ASSERT_BREAM_THREAD();

        context->beginFrame(context);
        MOpTilesContainer_beginPaint(getTilesContainer());
        bream_ptr_t node = 0;
        MOpVMEntry_nativeui_api_NativeUIBrowser_getPaintNode(
            &g_vm_instance.base, breamBrowser.ptr, paintWidth, &node);
        if (node) {
            bream_put_field(g_bream_vm, node, NATIVE_BREAM_GRAPHICS_PAINTNODE_X, -x);
            bream_put_field(g_bream_vm, node, NATIVE_BREAM_GRAPHICS_PAINTNODE_Y, -y);
            context->paintNode(context, g_bream_vm, node);
        }
        MOpTilesContainer_endPaint(getTilesContainer());

        if (linkCandidateArea) {
            paintFrameFocus(context, x, y, docWidth, paintWidth,
                    linkCandidateArea, linkCandidateAreas, &linkCandidateAreaPaintWidth,
                    0x29807028, 0xffc0b890);
        } else if (focusArea) {
            paintFrameFocus(context, x, y, docWidth, paintWidth,
                    focusArea, focusAreas, &focusAreaPaintWidth,
                    0x293078f0, 0xff90a8d0);
        }

        MOpDocumentLite* doc = getDocument();
        // TODO Might need to MOpObjectWrapper_addRef or something, to make
        // sure Bream doesn't delete the document while we're working on it.
        if (doc) {
            if (node && scrollBarAlpha) {
                bream_ptr_t folddata = bream_get_field_ptr(
                    g_bream_vm, node, NATIVE_COMPONENT_WEB_OBML_API_PAINTOBMLNODE_FOLDDATA);
                bream_intptr_t data = folddata ?
                    bream_get_field_ptr(g_bream_vm, folddata, NATIVE_BREAM_LANG_WEAKREFERENCE_O) : 0;

                if (data) {
                    MOpFoldData foldData;
                    MOpFoldData_init(&foldData, bream_intarray_get_ptr(g_bream_vm, data, 0), bream_intarray_size(g_bream_vm, data));
                    MOpObmlDocument *obmlDoc = MOpDocumentLite_getObmlDocument(doc);
                    UINT32 height = MOpObmlDocument_getFoldedPageHeight(obmlDoc, &foldData);
                    float scale = paintWidth / (float)MOpObmlDocument_getPageWidth(obmlDoc);
                    float scaledHeight = ceilf(height * scale);
                    BOOL vertVisible = topOverScroll + scaledHeight > context->height;
                    BOOL horzVisible = (unsigned int)paintWidth > context->cw;

                    int bc = context->blendColor;
                    context->setBlendColor(context, (scrollBarAlpha << 24) | 0xffffff);
                    if (vertVisible) {
                        int vertHeight = MAX(
                            (int)roundf((context->height - topOverScroll - 2 * SCROLLBAR_MARGIN) *
                                        context->height / (topOverScroll + scaledHeight)),
                            SCROLLBAR_THICKNESS);
                        int vertY = SCROLLBAR_MARGIN + topOverScroll +
                            (int)roundf((topOverScroll + y) /
                                        (topOverScroll + scaledHeight - context->height) *
                                        (context->height - topOverScroll - vertHeight -
                                         2 * SCROLLBAR_MARGIN));

                        drawScrollBar(context,
                                      context->width - SCROLLBAR_THICKNESS - SCROLLBAR_MARGIN,
                                      vertY,
                                      SCROLLBAR_THICKNESS, vertHeight);
                    }
                    if (horzVisible) {
                        int vertWidth = vertVisible ?
                            2 * SCROLLBAR_MARGIN + SCROLLBAR_THICKNESS : 0;
                        int horzWidth = MAX(
                            (int)roundf((context->width - vertWidth - 2 * SCROLLBAR_MARGIN) *
                                        context->width / (float)paintWidth),
                            SCROLLBAR_THICKNESS);
                        int horzX = SCROLLBAR_MARGIN +
                            (int)roundf((float)x / (paintWidth - context->width) *
                                        (context->width - horzWidth -
                                         vertWidth - SCROLLBAR_MARGIN));

                        drawScrollBar(context, horzX,
                                      context->height - SCROLLBAR_THICKNESS - SCROLLBAR_MARGIN,
                                      horzWidth, SCROLLBAR_THICKNESS);
                    }
                    context->setBlendColor(context, bc);
                }
            }
            if (fastScrollButtonAlpha)
            {
                bgImage *fsb = fsbUp ? fsbPressed ? fsbUP : fsbUU : fsbPressed ? fsbDP : fsbDU;
                if (fsb)
                {
                    int bc = context->blendColor;
                    context->setBlendColor(context, (fastScrollButtonAlpha << 24) | 0xffffff);
                    context->drawImage(context, fsb, context->width - fsb->width - fsbPadding,
                                       (context->height - fsb->height) >> 1, BG_BLEND_ALPHA);
                    context->setBlendColor(context, bc);
                }
            }
        }

        context->finishFrame(context);
    }

    void paintFrameFocus(bgContext *context, int x, int y, int docWidth, int paintWidth,
            int *area, size_t areas, jint *areaPaintWidth,
            UINT32 color, UINT32 colorBorder) {
        bgContextState state;
        context->saveState(context, &state);
        if (paintWidth != docWidth) {
            int *src = area;
            int *dst = area + areas;
            area = dst;
            if (paintWidth != *areaPaintWidth) {
                *areaPaintWidth = paintWidth;
                for (const int *stop = dst; src < stop;) {
                    int count = *src++;
                    *dst++ = count;
                    scaleAreas(src, dst, count, paintWidth);
                    src += count * 4;
                    dst += count * 4;
                }
            }
        }
        for (const int *stop = area + areas; area < stop;) {
            int count = *area++;
            paintFocus(context, -x, -y, area, count, color, colorBorder);
            area += count * 4;
        }
        context->restoreState(context, &state);
    }

    void repaintOverlay() {
        bream_ptr_t rects;
        do {
            if (MOpVMEntry_nativeui_api_NativeUIBrowser_getLinkCandidateRects(&g_vm_instance.base, breamBrowser.ptr, &rects) == MOP_STATUS_OK &&
                rects != 0) {
                linkCandidateAreas = bream_intarray_size(g_bream_vm, rects);
                linkCandidateArea = (int*)realloc(linkCandidateArea, linkCandidateAreas * 2 * sizeof(int));
                if (linkCandidateArea) {
                    memcpy(linkCandidateArea, bream_intarray_get_ptr(g_bream_vm, rects, 0), linkCandidateAreas * sizeof(int));
                    linkCandidateAreaPaintWidth = 0;
                    break;
                }
            }
            free(linkCandidateArea);
            linkCandidateArea = NULL;
            linkCandidateAreas = 0;
            linkCandidateAreaPaintWidth = 0;
        } while (false);
        do {
            if (linkCandidateArea == NULL &&
                MOpVMEntry_nativeui_api_NativeUIBrowser_getFocusedLinkRects(&g_vm_instance.base, breamBrowser.ptr, &rects) == MOP_STATUS_OK &&
                rects != 0) {
                focusAreas = bream_intarray_size(g_bream_vm, rects) + 1;
                focusArea = (int*)realloc(focusArea, focusAreas * 2 * sizeof(int));
                if (focusArea) {
                    focusArea[0] = focusAreas / 4;
                    memcpy(focusArea + 1, bream_intarray_get_ptr(g_bream_vm, rects, 0), (focusAreas - 1) * sizeof(int));
                    focusAreaPaintWidth = 0;
                    break;
                }
            }
            free(focusArea);
            focusArea = NULL;
            focusAreas = 0;
            focusAreaPaintWidth = 0;
        } while (false);
    }

    static void updateContext(void*, BOOL, BOOL)
    {
        ASSERT_BREAM_THREAD();
    }

    static void invalidateArea(void* userdata UNUSED, struct MOpObmlDocument* context UNUSED, int x UNUSED, int y UNUSED, int w UNUSED, int h UNUSED) {
        invalidate();
    }

    static void invalidate() {
    }

public:
    static void syncTextures() {
        ((bgContext*)gles1)->syncTextures((bgContext*)gles1);

        // Currently safe because when owner == NULL, this just calls
        // TilesWorker_wakeUp
        MOpTilesContainer_inputEvent(getTilesContainer(), NULL);
    }

    void cancelFindText() {
        MOpDocumentLite *doc = getDocument();
        if (doc)
            MOpDocumentLite_findTextReset(doc);
    }

private:
    void scaleAreas(const int *sourceArea, int *targetArea, size_t areas,
                    jint zoom) {
        const int *src = sourceArea;
        int *tgt = targetArea;

        while (areas-- > 0) {
            // X
            MOpVMEntry_nativeui_api_NativeUIBrowser_getScaledX(
                &g_vm_instance.base, breamBrowser.ptr,
                src[0], src[1], zoom, tgt + 0);
            // W
            MOpVMEntry_nativeui_api_NativeUIBrowser_getScaledX(
                &g_vm_instance.base, breamBrowser.ptr,
                src[0] + src[2], src[1], zoom, tgt + 2);
            tgt[2] -= tgt[0];
            // Y
            MOpVMEntry_nativeui_api_NativeUIBrowser_rescaleY(
                &g_vm_instance.base, breamBrowser.ptr,
                src[1], zoom, tgt + 1);
            // H
            MOpVMEntry_nativeui_api_NativeUIBrowser_rescaleY(
                &g_vm_instance.base, breamBrowser.ptr,
                src[1] + src[3], zoom, tgt + 3);
            tgt[3] -= tgt[1];
            src += 4;
            tgt += 4;
        }
    }
};
gles1Context* OBMLView::gles1 = NULL;
bgContext* OBMLView::context = NULL;

bgContext *MOpScreenGraphicsContext_getForDocument(BOOL portal)
{
    return portal ? &canvasContext.context : (bgContext*)OBMLView::gles1;
}

BOOL MOpSystemInfo_willRepaint()
{
    // Actually, since invalidateArea checks the flag itself, we can just
    // keep returning FALSE here.
    return FALSE;
}

static int getSkinResolution() {
    int res = MIN(g_screen_width, g_screen_height);
    if (res < 300) {
        return 0;   // QVGA (HQVGA, etc...)
    } else if (res < 440) {
        return 1;   // HVGA
    } else if (res < 600) {
        return 2;  // VGA
    } else if (res < 1000) {
        return 3;  // SVGA
    } else {
        return 4;  // QXGA
    }
}

static void setDefaultFontSizes(MOpVMInstance* vm, int skinResolution UNUSED) {
    static const MOpFMFSizes mopsizes[] = {
        MOP_FMFS_SMALL,
        MOP_FMFS_MEDIUM,
        MOP_FMFS_LARGE
    };

    for (size_t i = 0; i < sizeof(mopsizes)/sizeof(mopsizes[0]); i++) {
        MOpVMInstance_setFontSizeInfo(vm,
                mopsizes[i], AndroidFont::GetFontSize(i * 2), i * 2);
    }
    for (size_t i = 0; i < sizeof(mopsizes)/sizeof(mopsizes[0]); i++) {
        int defaultFontIdx = AndroidFont::GetDefaultFontIndex(i);
        MOpVMInstance_setFontSizeInfo(vm,
                mopsizes[i], AndroidFont::GetFontSize(defaultFontIdx), defaultFontIdx);
    }
}

void ObmlView_requestCallInMain()
{
}

static OBMLView* get(JNIEnv*, jobject object)
{
    MOP_ASSERT(object);
    return NULL;
}

void Java_com_opera_android_browser_obml_OBMLView_nativeStaticInit(
    JNIEnv *env, jclass clazz, jint selectionColor)
{
    initObmlViewClass(env, clazz);
    MOpVMInstance* vm = &g_vm_instance.base;
    OBMLView::staticInit(env, vm, selectionColor);
    setDefaultFontSizes(vm, getSkinResolution());
}

static bgImage *imageFromPixelArray(JNIEnv *env, bgContext *context, jintArray pixels, jint width, jint height)
{
    jint *data = env->GetIntArrayElements(pixels, NULL);
    if (!data)
    {
        return NULL;
    }
    swImage i;
    swWrapImageData(&i, data, width, height, width * bgPixelSize(BG_BGRA_8888), BG_BGRA_8888);
    bgImage *image = context->convertImage(context, &i.image, true, true);
    env->ReleaseIntArrayElements(pixels, data, JNI_ABORT);
    return image;
}

#ifdef MOP_FEATURE_ODP
jboolean Java_com_opera_android_portal_PortalView_canSwipeLeft(JNIEnv *env, jobject o)
{
    OBMLView *view = get(env, o);
    int result;
    MOpVMEntry_nativeui_api_NativePortalBrowser_canSwipeLeft(
        &g_vm_instance.base, view->breamBrowser.ptr, &result);
    return result;
}

jboolean Java_com_opera_android_portal_PortalView_canSwipeRight(JNIEnv *env, jobject o)
{
    OBMLView *view = get(env, o);
    int result;
    MOpVMEntry_nativeui_api_NativePortalBrowser_canSwipeRight(
        &g_vm_instance.base, view->breamBrowser.ptr, &result);
    return result;
}

jboolean Java_com_opera_android_portal_PortalView_slideFold(JNIEnv *env, jobject o, jint dx)
{
    OBMLView *view = get(env, o);
    int result;
    MOpVMEntry_nativeui_api_NativePortalBrowser_slideFold(
        &g_vm_instance.base, view->breamBrowser.ptr, dx, &result);
    return result;
}

jboolean Java_com_opera_android_portal_PortalView_stepAnimation(JNIEnv*, jclass)
{
    int result;
    MOpVMEntry_nativeui_api_stepAnimation(&g_vm_instance.base, &result);
    return result;
}
#endif /* MOP_FEATURE_ODP */

#include "mini/c/shared/generic/vm_integ/NativeUIBrowser.cpp"
