#include "build/buildflag.h"

#define BUILDFLAG_INTERNAL_ENABLE_GOOGLE_NOW() (0)

#define BUILDFLAG_INTERNAL_ENABLE_ONE_CLICK_SIGNIN() (0)

// This is needed as long as the header file for ChromeHistoryBackendClient doesn't
// match the source file (header files check OS_ANDROID but source check
// BUILDFLAG(ANDROID_JAVA_UI)
#if defined(OS_ANDROID)
#define BUILDFLAG_INTERNAL_ANDROID_JAVA_UI() (1)
#else
#define BUILDFLAG_INTERNAL_ANDROID_JAVA_UI() (0)
#endif

#define BUILDFLAG_INTERNAL_ENABLE_BACKGROUND() (0)
