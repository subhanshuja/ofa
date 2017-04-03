// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/chrome/browser/media/media_capture_devices_dispatcher.cc

#include "chill/browser/media_capture_devices_dispatcher.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"

#include "chill/browser/media_stream_devices_controller.h"

using content::BrowserThread;
using content::MediaCaptureDevices;
using content::MediaStreamDevices;
using content::MediaStreamDevice;

namespace opera {

namespace {

// Finds a device in |devices| that has |device_id|, or NULL if not found.
const MediaStreamDevice* FindDeviceWithId(
    const MediaStreamDevices& devices,
    const std::string& device_id) {
  MediaStreamDevices::const_iterator iter = devices.begin();
  for (; iter != devices.end(); ++iter) {
    if (iter->id == device_id) {
      return &(*iter);
    }
  }
  return NULL;
}

}  // namespace

MediaCaptureDevicesDispatcher::PendingAccessRequest::PendingAccessRequest(
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback)
    : request(request), callback(callback) {}

MediaCaptureDevicesDispatcher* MediaCaptureDevicesDispatcher::GetInstance() {
  return base::Singleton<MediaCaptureDevicesDispatcher>::get();
}

MediaCaptureDevicesDispatcher::MediaCaptureDevicesDispatcher()
    : is_device_enumeration_disabled_(false) {
  // MediaCaptureDevicesDispatcher is a singleton. It should be created on
  // UI thread. Otherwise, it will not receive
  // content::NOTIFICATION_WEB_CONTENTS_DESTROYED, and that will result in
  // possible use after free.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notifications_registrar_.Add(this,
                               content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                               content::NotificationService::AllSources());
}

MediaCaptureDevicesDispatcher::~MediaCaptureDevicesDispatcher() {
}

void MediaCaptureDevicesDispatcher::AddObserver(Observer* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!observers_.HasObserver(observer))
    observers_.AddObserver(observer);
}

void MediaCaptureDevicesDispatcher::RemoveObserver(Observer* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  observers_.RemoveObserver(observer);
}

const MediaStreamDevices&
MediaCaptureDevicesDispatcher::GetAudioCaptureDevices() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (is_device_enumeration_disabled_ || !test_audio_devices_.empty())
    return test_audio_devices_;

  return MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
}

const MediaStreamDevices&
MediaCaptureDevicesDispatcher::GetVideoCaptureDevices() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (is_device_enumeration_disabled_ || !test_video_devices_.empty())
    return test_video_devices_;
  return MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
}

const MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetFirstAvailableVideoDevice() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const MediaStreamDevices& video_devices = GetVideoCaptureDevices();
  if (video_devices.empty())
    return NULL;
  return &(*video_devices.begin());
}

const MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetFirstAvailableAudioDevice() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const MediaStreamDevices& audio_devices = GetAudioCaptureDevices();
  if (audio_devices.empty())
    return NULL;
  return &(*audio_devices.begin());
}

void MediaCaptureDevicesDispatcher::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (type == content::NOTIFICATION_WEB_CONTENTS_DESTROYED) {
    content::WebContents* web_contents =
        content::Source<content::WebContents>(source).ptr();
    pending_requests_.erase(web_contents);
  }
}

void MediaCaptureDevicesDispatcher::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  RequestsQueue& queue = pending_requests_[web_contents];
  queue.push_back(PendingAccessRequest(request, callback));

  // If this is the only request then show the infobar.
  if (queue.size() == 1)
    ProcessQueuedAccessRequest(web_contents);
}

void MediaCaptureDevicesDispatcher::ProcessQueuedAccessRequest(
    content::WebContents* web_contents) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  std::map<content::WebContents*, RequestsQueue>::iterator it =
      pending_requests_.find(web_contents);

  if (it == pending_requests_.end() || it->second.empty()) {
    // Don't do anything if the tab was was closed.
    return;
  }

  DCHECK(!it->second.empty());

  scoped_refptr<MediaStreamDevicesController> controller(
      new MediaStreamDevicesController(
          web_contents,
          it->second.front().request,
          base::Bind(&MediaCaptureDevicesDispatcher::OnAccessRequestResponse,
                     base::Unretained(this),
                     web_contents)));

  // The controller will keep itself alive as long as required.
  controller->RequestPermissions();
}

void MediaCaptureDevicesDispatcher::OnAccessRequestResponse(
    content::WebContents* web_contents,
    const MediaStreamDevices& devices,
    content::MediaStreamRequestResult result,
    std::unique_ptr<content::MediaStreamUI> ui) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  std::map<content::WebContents*, RequestsQueue>::iterator it =
      pending_requests_.find(web_contents);
  if (it == pending_requests_.end()) {
    // WebContents has been destroyed. Don't need to do anything.
    return;
  }

  RequestsQueue& queue(it->second);
  if (queue.empty())
    return;

  content::MediaResponseCallback callback = queue.front().callback;
  queue.pop_front();

  if (!queue.empty()) {
    // Post a task to process next queued request. It has to be done
    // asynchronously to make sure that calling infobar is not destroyed until
    // after this function returns.
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&MediaCaptureDevicesDispatcher::ProcessQueuedAccessRequest,
                   base::Unretained(this),
                   web_contents));
  }

  callback.Run(devices, result, std::move(ui));
}

const MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetRequestedAudioDevice(
    const std::string& requested_audio_device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const MediaStreamDevices& audio_devices = GetAudioCaptureDevices();
  const MediaStreamDevice* const device =
      FindDeviceWithId(audio_devices, requested_audio_device_id);
  return device;
}

const MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetRequestedVideoDevice(
    const std::string& requested_video_device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const MediaStreamDevices& video_devices = GetVideoCaptureDevices();
  const MediaStreamDevice* const device =
      FindDeviceWithId(video_devices, requested_video_device_id);
  return device;
}

void MediaCaptureDevicesDispatcher::OnAudioCaptureDevicesChanged() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
        &MediaCaptureDevicesDispatcher::NotifyAudioDevicesChangedOnUIThread,
        base::Unretained(this)));
}

void MediaCaptureDevicesDispatcher::OnVideoCaptureDevicesChanged() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
        &MediaCaptureDevicesDispatcher::NotifyVideoDevicesChangedOnUIThread,
        base::Unretained(this)));
}

void MediaCaptureDevicesDispatcher::OnMediaRequestStateChanged(
    int render_process_id,
    int render_frame_id,
    int page_request_id,
    const GURL& security_origin,
    content::MediaStreamType stream_type,
    content::MediaRequestState state) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
          &MediaCaptureDevicesDispatcher::UpdateMediaRequestStateOnUIThread,
          base::Unretained(this),
          render_process_id,
          render_frame_id,
          page_request_id,
          stream_type,
          state));
}

void MediaCaptureDevicesDispatcher::OnCreatingAudioStream(
    int render_process_id,
    int render_frame_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
          &MediaCaptureDevicesDispatcher::OnCreatingAudioStreamOnUIThread,
          base::Unretained(this),
          render_process_id,
          render_frame_id));
}

void MediaCaptureDevicesDispatcher::OnSetCapturingLinkSecured(
    int render_process_id,
    int render_frame_id,
    int page_request_id,
    content::MediaStreamType stream_type,
    bool is_secure) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  /* No implementation; ENABLE_EXTENSIONS not enabled. */
}

void MediaCaptureDevicesDispatcher::NotifyAudioDevicesChangedOnUIThread() {
  MediaStreamDevices devices = GetAudioCaptureDevices();
  FOR_EACH_OBSERVER(Observer, observers_, OnUpdateAudioDevices(devices));
}

void MediaCaptureDevicesDispatcher::NotifyVideoDevicesChangedOnUIThread() {
  MediaStreamDevices devices = GetVideoCaptureDevices();
  FOR_EACH_OBSERVER(Observer, observers_, OnUpdateVideoDevices(devices));
}

void MediaCaptureDevicesDispatcher::UpdateMediaRequestStateOnUIThread(
    int render_process_id,
    int render_frame_id,
    int page_request_id,
    content::MediaStreamType stream_type,
    content::MediaRequestState state) {
  // Cancel the request.
  if (state == content::MEDIA_REQUEST_STATE_CLOSING) {
    bool found = false;
    for (RequestsQueues::iterator rqs_it = pending_requests_.begin();
         rqs_it != pending_requests_.end();
         ++rqs_it) {
      RequestsQueue& queue = rqs_it->second;
      for (RequestsQueue::iterator it = queue.begin(); it != queue.end();
           ++it) {
        if (it->request.render_process_id == render_process_id &&
            it->request.render_frame_id == render_frame_id &&
            it->request.page_request_id == page_request_id) {
          queue.erase(it);
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
  }

  FOR_EACH_OBSERVER(
      Observer,
      observers_,
      OnRequestUpdate(render_process_id, render_frame_id, stream_type, state));
}

void MediaCaptureDevicesDispatcher::OnCreatingAudioStreamOnUIThread(
    int render_process_id,
    int render_frame_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  FOR_EACH_OBSERVER(Observer,
                    observers_,
                    OnCreatingAudioStream(render_process_id, render_frame_id));
}

void MediaCaptureDevicesDispatcher::SetTestAudioCaptureDevices(
        const MediaStreamDevices& devices) {
    test_audio_devices_ = devices;
}

void MediaCaptureDevicesDispatcher::SetTestVideoCaptureDevices(
        const MediaStreamDevices& devices) {
    test_video_devices_ = devices;
}

}  // namespace opera
