// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
// chromium/src/chrome/browser/ui/android/bluetooth_chooser_android.h
// @last-synchronized 3ecef4e6dc7a93987930c1a5a1ad146c3bba58e8

#ifndef CHILL_BROWSER_UI_OPERA_BLUETOOTH_CHOOSER_ANDROID_H_
#define CHILL_BROWSER_UI_OPERA_BLUETOOTH_CHOOSER_ANDROID_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "content/public/browser/bluetooth_chooser.h"
#include "content/public/browser/web_contents.h"

namespace url {
class Origin;
}

namespace opera {

// Represents a way to ask the user to select a Bluetooth device from a list of
// options.
class BluetoothChooserAndroid : public content::BluetoothChooser {
 public:
  // Both frame and event_handler must outlive the BluetoothChooserAndroid.
  BluetoothChooserAndroid(content::RenderFrameHost* frame,
                          const EventHandler& event_handler);
  ~BluetoothChooserAndroid() override;

  // content::BluetoothChooser:
  bool CanAskForScanningPermission() override;
  void SetAdapterPresence(AdapterPresence presence) override;
  void ShowDiscoveryState(DiscoveryState state) override;
  void AddOrUpdateDevice(const std::string& device_id,
                         bool should_update_name,
                         const base::string16& device_name,
                         bool is_gatt_connected,
                         bool is_paired,
                         int signal_strength_level) override;
  void RemoveDevice(const std::string& device_id) override;

  // Report the dialog's result.
  void OnDialogFinished(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        jint event_type,
                        const base::android::JavaParamRef<jstring>& device_id);

  // Notify bluetooth stack that the search needs to be re-issued.
  void RestartSearch(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj);

  void ShowBluetoothAdapterOffLink(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void ShowNeedLocationPermissionLink(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  static bool Register(JNIEnv* env);

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_dialog_;

  content::WebContents* web_contents_;
  BluetoothChooser::EventHandler event_handler_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_UI_OPERA_BLUETOOTH_CHOOSER_ANDROID_H_
