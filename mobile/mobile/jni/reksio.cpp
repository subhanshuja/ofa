/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
 *
 * Copyright (C) 2014 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <jni.h>

#include "bream.h"
#include "platform.h"
#include "reksio.h"
#include "resolve.h"
#include "socket.h"

#include "jni_obfuscation.h"
#include "com_opera_android_browser_obml_Platform.h"

extern "C" {
#include "mini/c/shared/core/protocols/obsp/ObspConnection.h"
#include "mini/c/shared/core/settings/SettingsManager.h"
#include "mini/c/shared/core/softcore/softcore.h"
}

static jmethodID g_platform_requestSlice;

jint Java_com_opera_android_browser_obml_Platform_runSlice(JNIEnv *, jclass) {
  Socket_slice();
  Resolve_slice();
  return Core_slice();
}

static void CoreSliceCallback(unsigned int delay, void*) {
  getEnv()->CallStaticVoidMethod(
      g_platform_class, g_platform_requestSlice, static_cast<jint>(delay));
}

static void NetworkSliceCallback(JNIEnv *env, void *userdata UNUSED)
{
  env->CallStaticVoidMethod(g_platform_class, g_platform_requestSlice, 0);
}

void initReksio(JNIEnv *env) {
  g_platform_requestSlice = env->GetStaticMethodID(
      g_platform_class,
      M_com_opera_android_browser_obml_Platform_requestSlice_ID);

  /* TODO: Move to Platform */
  Socket_setup(NetworkSliceCallback, NULL);
  Resolve_setup(NetworkSliceCallback, NULL);

  MOP_STATUS status = Core_initialize();
  MOP_ASSERT(status == MOP_STATUS_OK);

  Core_registerRequestSliceCallback(CoreSliceCallback, NULL);

  ObspConnection *conn = ObspConnection_getInstance();
  if (conn) {
    UINT32 features =
        OBSP_REQUEST_FEATURE_EXT2_EXTENDED_SEARCHENGINE_DATA |
        OBSP_REQUEST_FEATURE_EXT2_SEARCHENGINE_LARGE_ICON |
        OBSP_REQUEST_FEATURE_EXT2_DISABLE_RSS |
        OBSP_REQUEST_FEATURE_EXT2_TEXT_SIZE_ADJUST |
        OBSP_REQUEST_FEATURE_EXT2_HQ_THUMBNAIL |
        OBSP_REQUEST_FEATURE_EXT2_HQ_THUMBNAILS_NO_CROPPING |
        OBSP_REQUEST_FEATURE_EXT2_SEARCHENGINE_SIMPLESEARCH |
        OBSP_REQUEST_FEATURE_EXT2_NATIVE_UI;
    ObspConnection_useFeatures(conn, OBSPCONNECTION_EXTENDED_FEATURES_2,
                               features, TRUE);
    ObspConnection_prepareNetworkTests(conn, FALSE);
    ObspConnection_release(conn);
  }

  MOpReachability_monitorStatus(
      MOpReachability_get(), onReachabilityChanged, NULL);
  MOpSettingsManager_registerCallback(
      MOpSettingsManager_getSingleton(), onSettingChanged, NULL);
}
