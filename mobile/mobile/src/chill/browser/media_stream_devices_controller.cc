// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/chrome/browser/media/media_stream_devices_controller.cc

#include "chill/browser/media_stream_devices_controller.h"

#include <vector>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "chill/browser/media_capture_devices_dispatcher.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/permissions/opera_permission_manager.h"

namespace opera {

namespace {

struct NameTranslation {
  const char* device_id;  ///< Device ID (see MediaStreamDevice).
  int translation;        ///< String translation ID.
};

// Known microphone IDs.
// See media/base/android/java/src/org/chromium/media/AudioManagerAndroid.java
// and media/audio/android/audio_manager_android.cc
const std::string kDefaultMicrophoneId = "default";
const NameTranslation kMicrophoneTranslations[] = {
  {"default", IDS_USER_MEDIA_DEFAULT_MICROPHONE},
  {"0", IDS_USER_MEDIA_MICROPHONE_SPEAKERPHONE},
  {"1", IDS_USER_MEDIA_MICROPHONE_WIRED_HEADSET},
  {"2", IDS_USER_MEDIA_MICROPHONE_EARPIECE},
  {"3", IDS_USER_MEDIA_MICROPHONE_BLUETOOTH_HEADSET}};
const int kNumMicrophoneTranslations =
    sizeof(kMicrophoneTranslations) / sizeof(NameTranslation);

std::string GetMicrophoneName(const content::MediaStreamDevice* device) {
  for (int i = 0; i < kNumMicrophoneTranslations; ++i) {
    if (device->id == kMicrophoneTranslations[i].device_id) {
      return l10n_util::GetStringUTF8(kMicrophoneTranslations[i].translation);
    }
  }
  return device->name;
}

std::string GetCameraName(const content::MediaStreamDevice* device) {
  // Select an apropriate name based on the way the camera is facing.
  switch (device->video_facing) {
    case content::MEDIA_VIDEO_FACING_USER:
      return l10n_util::GetStringUTF8(IDS_USER_MEDIA_FRONT_CAMERA);
      break;
    case content::MEDIA_VIDEO_FACING_ENVIRONMENT:
      return l10n_util::GetStringUTF8(IDS_USER_MEDIA_BACK_CAMERA);
      break;
    default:
      return device->name;
  }
}

class CaptureDeviceDialogDelegate
    : public MultipleChoiceDialogDelegate {
 public:
  CaptureDeviceDialogDelegate(
      std::vector<OpMultipleChoiceEntry> values,
      base::Callback<void(const std::string& id)> device_selected_callback)
      : MultipleChoiceDialogDelegate(values),
        device_selected_callback_(device_selected_callback) {}

  void Selected(ChromiumTab* tab, const std::string& id) {
    device_selected_callback_.Run(id);
  }
  void Cancel(ChromiumTab* tab) { NOTREACHED(); }

 private:
  base::Callback<void(const std::string& id)> device_selected_callback_;
};

}  // namespace

class OperaMediaStreamUI : public content::MediaStreamUI {
 public:
  explicit OperaMediaStreamUI(content::WebContents* web_contents) {
    delegate_ = ChromiumTabDelegate::FromWebContents(web_contents);
  }

  gfx::NativeViewId OnStarted(const base::Closure& stop) override {
    delegate_->MediaStreamState(true);
    return 0;
  }

 private:
  ChromiumTabDelegate* delegate_;
};

MediaStreamDevicesController::MediaStreamDevicesController(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : web_contents_(web_contents),
      request_(request),
      callback_(callback),
      microphone_requested_(
          request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE ||
          request.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY),
      webcam_requested_(
          request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE ||
          request.request_type == content::MEDIA_OPEN_DEVICE_PEPPER_ONLY) {
  waiting_for_audio_ = microphone_requested_;
  waiting_for_video_ = webcam_requested_;
}

void MediaStreamDevicesController::RequestDeviceDialog(
    ChromiumTabDelegate::MultipleChoiceDialogType type,
    content::MediaStreamDevices devices) {
  std::vector<OpMultipleChoiceEntry> values;
  for (content::MediaStreamDevices::const_iterator device = devices.begin();
       device != devices.end();
       ++device) {
    // Get a translated device name if possible.
    std::string name = device->name;
    if (type == ChromiumTabDelegate::VideoSource) {
      name = GetCameraName(&(*device));
    } else if (type == ChromiumTabDelegate::AudioSource) {
      // Currently AudioManagerAndroid::GetAudioInputDeviceNames inserts a
      // default microphone entry, which is an alias for another item in the
      // list. We just drop it if we find it, which has the nice side effect
      // of skipping the dialog if there's only a single microphone.
      if (device->id == kDefaultMicrophoneId && device == devices.begin() &&
          devices.size() > 1) {
        continue;
      }
      name = GetMicrophoneName(&(*device));
    }

    values.push_back(OpMultipleChoiceEntry(device->id, name));
  }
  if (values.size() > 1) {
    base::Callback<void(const std::string& value)> device_selected_callback =
        base::Bind(&MediaStreamDevicesController::DeviceSelected,
                   this,
                   type);
    ChromiumTabDelegate::FromWebContents(web_contents_)
        ->RequestMultipleChoiceDialog(
            type, web_contents_->GetURL().host(),
            new CaptureDeviceDialogDelegate(values, device_selected_callback));
  } else if (values.size() == 1) {
    DeviceSelected(type, values.front().id);
  }
}

MediaStreamDevicesController::~MediaStreamDevicesController() {
  if (!callback_.is_null()) {
    callback_.Run(content::MediaStreamDevices(),
                  content::MEDIA_DEVICE_INVALID_STATE,
                  std::unique_ptr<content::MediaStreamUI>());
  }
}

void MediaStreamDevicesController::RequestPermissions() {
  std::vector<content::PermissionType> permissions;

  if (webcam_requested_) {
    permissions.push_back(content::PermissionType::VIDEO_CAPTURE);
  }
  if (microphone_requested_) {
    permissions.push_back(content::PermissionType::AUDIO_CAPTURE);
  }

  OperaPermissionManager* opm = static_cast<OperaPermissionManager*>(
      web_contents_->GetBrowserContext()->GetPermissionManager());

  opm->RequestPermissions(
      permissions,
      web_contents_,
      web_contents_->GetURL(),
      base::Bind(&MediaStreamDevicesController::PermissionsCallback, this));
}

void MediaStreamDevicesController::PermissionsCallback(
    const std::vector<blink::mojom::PermissionStatus>& result) {
  int idx = 0;
  if (webcam_requested_) {
    if (result[idx] != blink::mojom::PermissionStatus::GRANTED) {
      webcam_requested_ = false;
      waiting_for_video_ = false;
    }
    idx++;
  }
  if (microphone_requested_) {
    if (result[idx] != blink::mojom::PermissionStatus::GRANTED) {
      microphone_requested_ = false;
      waiting_for_audio_ = false;
    }
  }

  if (webcam_requested_ || microphone_requested_) {
    RequestDevices();
    return;
  }

  Deny(content::MEDIA_DEVICE_PERMISSION_DENIED);
}

void MediaStreamDevicesController::RequestDevices() {
  switch (request_.request_type) {
    case content::MEDIA_GENERATE_STREAM: {
      // Register default audio/video device
      if (microphone_requested_) {
        const content::MediaStreamDevices& devices =
            MediaCaptureDevicesDispatcher::GetInstance()
                ->GetAudioCaptureDevices();
        RequestDeviceDialog(ChromiumTabDelegate::AudioSource, devices);
      }
      if (webcam_requested_) {
        const content::MediaStreamDevices& devices =
            MediaCaptureDevicesDispatcher::GetInstance()
                ->GetVideoCaptureDevices();
        RequestDeviceDialog(ChromiumTabDelegate::VideoSource, devices);
      }
      break;
    }
    default: {
      Accept();
      break;
    }
  }
}

void MediaStreamDevicesController::DeviceSelected(
    ChromiumTabDelegate::MultipleChoiceDialogType type,
    const std::string& value) {
  if (type == ChromiumTabDelegate::AudioSource) {
    audio_device_selected_ = value;
    waiting_for_audio_ = false;
  } else if (type == ChromiumTabDelegate::VideoSource) {
    video_device_selected_ = value;
    waiting_for_video_ = false;
  }
  if (!waiting_for_audio_ && !waiting_for_video_)
    Accept();
}

void MediaStreamDevicesController::Accept() {
  // Get the default devices for the request.
  content::MediaStreamDevices devices;
  if (microphone_requested_ || webcam_requested_) {
    switch (request_.request_type) {
      case content::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
        const content::MediaStreamDevice* device = NULL;
        // For open device request pick the desired device or fall back to the
        // first available of the given type.
        if (request_.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
          device = MediaCaptureDevicesDispatcher::GetInstance()
              ->GetRequestedAudioDevice(request_.requested_audio_device_id);
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                ->GetFirstAvailableAudioDevice();
          }
        } else if (request_.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
          // Pepper API opens only one device at a time.
          device = MediaCaptureDevicesDispatcher::GetInstance()
              ->GetRequestedVideoDevice(request_.requested_video_device_id);
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                ->GetFirstAvailableVideoDevice();
          }
        }
        if (device)
          devices.push_back(*device);
        break;
      }
      case content::MEDIA_GENERATE_STREAM: {
        // Get the exact audio or video device if an id is specified.
        MediaCaptureDevicesDispatcher* dispatcher =
            MediaCaptureDevicesDispatcher::GetInstance();
        const content::MediaStreamDevice* audio_device =
            dispatcher->GetRequestedAudioDevice(audio_device_selected_);
        if (audio_device) {
          devices.push_back(*audio_device);
        }
        const content::MediaStreamDevice* video_device =
            dispatcher->GetRequestedVideoDevice(video_device_selected_);
        if (video_device) {
          devices.push_back(*video_device);
        }
        break;
      }
      case content::MEDIA_DEVICE_ACCESS: {
        MediaCaptureDevicesDispatcher* dispatcher =
            MediaCaptureDevicesDispatcher::GetInstance();
        if (microphone_requested_) {
          devices.push_back(*dispatcher->GetFirstAvailableAudioDevice());
        }
        if (webcam_requested_) {
          devices.push_back(*dispatcher->GetFirstAvailableVideoDevice());
        }
        break;
      }
      case content::MEDIA_ENUMERATE_DEVICES:
        // Do nothing.
        NOTREACHED();
        break;
    }
  }
  content::MediaResponseCallback cb = callback_;
  callback_.Reset();
  cb.Run(
      devices,
      content::MEDIA_DEVICE_OK,
      std::unique_ptr<content::MediaStreamUI>(
          new OperaMediaStreamUI(web_contents_)));
}

void MediaStreamDevicesController::Deny(
    content::MediaStreamRequestResult result) {
  content::MediaResponseCallback cb = callback_;
  callback_.Reset();
  cb.Run(content::MediaStreamDevices(),
         result,
         std::unique_ptr<content::MediaStreamUI>());
}

}  // namespace opera
