// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/chrome/browser/media/media_stream_devices_controller.h

#ifndef CHILL_BROWSER_MEDIA_STREAM_DEVICES_CONTROLLER_H_
#define CHILL_BROWSER_MEDIA_STREAM_DEVICES_CONTROLLER_H_

#include <string>
#include <vector>

#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

#include "chill/browser/chromium_tab_delegate.h"

namespace content {
class WebContents;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace opera {

class MediaStreamDevicesController
    : public base::RefCounted<MediaStreamDevicesController> {
 public:
  MediaStreamDevicesController(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
  virtual ~MediaStreamDevicesController();

  void RequestPermissions();

 private:
  void PermissionsCallback(
      const std::vector<blink::mojom::PermissionStatus>& result);

  void RequestDevices();
  void Accept();
  void Deny(content::MediaStreamRequestResult result);

  void RequestDeviceDialog(ChromiumTabDelegate::MultipleChoiceDialogType type,
                           content::MediaStreamDevices devices);
  void DeviceSelected(ChromiumTabDelegate::MultipleChoiceDialogType type,
                      const std::string& value);

  content::WebContents* web_contents_;

  // The original request for access to devices.
  const content::MediaStreamRequest request_;

  // The callback that needs to be Run to notify WebRTC of whether access to
  // audio/video devices was granted or not.
  content::MediaResponseCallback callback_;

  std::string video_device_selected_;
  std::string audio_device_selected_;
  bool microphone_requested_;
  bool webcam_requested_;
  bool waiting_for_audio_;
  bool waiting_for_video_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDevicesController);
};

}  // namespace opera

#endif  // CHILL_BROWSER_MEDIA_STREAM_DEVICES_CONTROLLER_H_
