// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
// chromium/src/chrome/browser/ui/android/bluetooth_chooser_android.cc
// @last-synchronized 3ecef4e6dc7a93987930c1a5a1ad146c3bba58e8

#include <string>

#include "chill/browser/ui/opera_bluetooth_chooser_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/render_frame_host.h"
#include "jni/BluetoothChooserDialog_jni.h"
#include "ui/android/window_android.h"
#include "url/origin.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaLocalRef;

namespace opera {

BluetoothChooserAndroid::BluetoothChooserAndroid(
    content::RenderFrameHost* frame,
    const EventHandler& event_handler)
    : web_contents_(content::WebContents::FromRenderFrameHost(frame)),
      event_handler_(event_handler) {
  const url::Origin origin = frame->GetLastCommittedOrigin();
  DCHECK(!origin.unique());

  base::android::ScopedJavaLocalRef<jobject> window_android =
      content::ContentViewCore::FromWebContents(web_contents_)
          ->GetWindowAndroid()
          ->GetJavaObject();

  // Create (and show) the BluetoothChooser dialog.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> origin_string =
      ConvertUTF8ToJavaString(env, origin.Serialize());
  java_dialog_.Reset(Java_BluetoothChooserDialog_create(
      env, window_android.obj(), origin_string.obj(),
      reinterpret_cast<intptr_t>(this)));
}

BluetoothChooserAndroid::~BluetoothChooserAndroid() {
  if (!java_dialog_.is_null()) {
    Java_BluetoothChooserDialog_closeDialog(AttachCurrentThread(),
                                            java_dialog_);
  }
}

bool BluetoothChooserAndroid::CanAskForScanningPermission() {
  // Creating the dialog returns null if Chromium can't ask for permission to
  // scan for BT devices.
  return !java_dialog_.is_null();
}

void BluetoothChooserAndroid::SetAdapterPresence(AdapterPresence presence) {
  JNIEnv* env = AttachCurrentThread();
  if (presence != AdapterPresence::POWERED_ON) {
    Java_BluetoothChooserDialog_notifyAdapterTurnedOff(env, java_dialog_);
  } else {
    Java_BluetoothChooserDialog_notifyAdapterTurnedOn(env, java_dialog_);
  }
}

void BluetoothChooserAndroid::ShowDiscoveryState(DiscoveryState state) {
  // These constants are used in BluetoothChooserDialog.notifyDiscoveryState.
  int java_state = -1;
  switch (state) {
    case DiscoveryState::FAILED_TO_START:
      java_state = 0;
      break;
    case DiscoveryState::DISCOVERING:
      java_state = 1;
      break;
    case DiscoveryState::IDLE:
      java_state = 2;
      break;
  }
  Java_BluetoothChooserDialog_notifyDiscoveryState(AttachCurrentThread(),
                                                   java_dialog_, java_state);
}

void BluetoothChooserAndroid::AddOrUpdateDevice(
    const std::string& device_id,
    bool should_update_name,
    const base::string16& device_name,
    bool is_gatt_connected,
    bool is_paired,
    int signal_strength_level) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_device_id =
      ConvertUTF8ToJavaString(env, device_id);
  ScopedJavaLocalRef<jstring> java_device_name =
      ConvertUTF16ToJavaString(env, device_name);
  Java_BluetoothChooserDialog_addOrUpdateDevice(
      env, java_dialog_, java_device_id, java_device_name);
}

void BluetoothChooserAndroid::RemoveDevice(const std::string& device_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_device_id =
      ConvertUTF16ToJavaString(env, base::UTF8ToUTF16(device_id));
  Java_BluetoothChooserDialog_removeDevice(env, java_dialog_, java_device_id);
}

void BluetoothChooserAndroid::OnDialogFinished(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jint event_type,
    const base::android::JavaParamRef<jstring>& device_id) {
  // Values are defined in BluetoothChooserDialog as DIALOG_FINISHED constants.
  switch (event_type) {
    case 0:
      event_handler_.Run(Event::DENIED_PERMISSION, "");
      return;
    case 1:
      event_handler_.Run(Event::CANCELLED, "");
      return;
    case 2:
      event_handler_.Run(
          Event::SELECTED,
          base::android::ConvertJavaStringToUTF8(env, device_id));
      return;
  }
  NOTREACHED();
}

void BluetoothChooserAndroid::RestartSearch(
    JNIEnv*,
    const base::android::JavaParamRef<jobject>&) {
  event_handler_.Run(Event::RESCAN, "");
}

// static
bool BluetoothChooserAndroid::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace opera
