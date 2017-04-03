#ifndef CANVASCONTEXT_H
#define CANVASCONTEXT_H

#include "bream/vm/c/graphics/bream_graphics.h"

typedef struct CanvasContext CanvasContext;
struct CanvasContext
{
    bgContext context;
    jobject canvas;
    jobject paint;
    jobject rect;
    jobject rectf;
};

extern void canvasContextStaticInit(JNIEnv *env);
extern void initCanvasContext(JNIEnv *env, bgPlatform *platform, CanvasContext *context);
extern void setCanvasContextTarget(CanvasContext *context, jobject canvas,
                                   unsigned int width, unsigned int height);
extern jobject createAndroidCanvasFromBitmap(JNIEnv *env, jobject bitmap);

#endif /* CANVASCONTEXT_H */
