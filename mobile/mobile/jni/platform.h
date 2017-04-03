#ifndef PLATFORM_H
#define PLATFORM_H

#include <jni.h>
#include "bream/vm/c/graphics/bream_graphics.h"

#ifdef __cplusplus
#include "jniutils.h"

extern "C" {
#endif

#include "mini/c/shared/core/pi/Reachability.h"

/* Get the JNI Environment for the current Java thread.
 * Returns NULL if the current thread is not a Java thread. */
JNIEnv* getEnv();

#ifdef __cplusplus
}

#ifdef BREAM_ENTRY_THREAD_CHECK
extern "C" BOOL breamEntryThreadCheck();
# define ASSERT_BREAM_THREAD() MOP_ASSERT(breamEntryThreadCheck())
#else
# define ASSERT_BREAM_THREAD()
#endif

extern jclass g_platform_class;
extern bgPlatform g_bg_platform;
extern const char *g_user_agent;
extern unsigned int g_screen_width;
extern unsigned int g_screen_height;
extern unsigned int g_screen_dpi_x;
extern unsigned int g_screen_dpi_y;
extern BOOL g_is_tablet;

void onReachabilityChanged(void *userData, MOpReachabilityStatus status);
void onSettingChanged(void *userData, UINT32 param, void *change);

void onBreamInitialized();

/** Returns android.os.ParcelFileDescriptor */
scoped_localref<jobject> getParcelFileDescriptor(const char* filename, const char* mode);
/** Returns java.nio.channels.FileChannel */
scoped_localref<jobject> openParcelFileDescriptor(jobject parcelFileDescriptor);
#endif

#ifdef MOP_FEATURE_HTTP_PLATFORM_PROXY
/**
 * Get platform http proxy in format <host:port>.
 * Return "" if no proxy.
 *
 * Note:
 * The returned string should be freed by mop_free
 * NULL may be returned in case of OOM.
 */
OP_EXTERN_C TCHAR * getHTTPProxy();
#endif /* MOP_FEATURE_HTTP_PLATFORM_PROXY */
#endif /* PLATFORM_H */
