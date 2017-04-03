// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/chrome/browser/media/media_capture_devices_dispatcher.h

#ifndef CHILL_BROWSER_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define CHILL_BROWSER_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include <deque>
#include <map>
#include <string>

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/public/browser/media_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/media_stream_request.h"

namespace opera {

// This singleton is used to receive updates about media events from the content
// layer.
class MediaCaptureDevicesDispatcher : public content::MediaObserver,
                                      public content::NotificationObserver {
 public:
  class Observer {
   public:
    // Handle an information update consisting of a up-to-date audio capture
    // device lists. This happens when a microphone is plugged in or unplugged.
    virtual void OnUpdateAudioDevices(
        const content::MediaStreamDevices& devices) {}

    // Handle an information update consisting of a up-to-date video capture
    // device lists. This happens when a camera is plugged in or unplugged.
    virtual void OnUpdateVideoDevices(
        const content::MediaStreamDevices& devices) {}

    // Handle an information update related to a media stream request.
    virtual void OnRequestUpdate(
        int render_process_id,
        int render_frame_id,
        content::MediaStreamType stream_type,
        const content::MediaRequestState state) {}

    // Handle an information update that a new stream is being created.
    virtual void OnCreatingAudioStream(int render_process_id,
                                       int render_frame_id) {}

    virtual ~Observer() {}
  };

  static MediaCaptureDevicesDispatcher* GetInstance();

  // Methods for observers. Called on UI thread.
  // Observers should add themselves on construction and remove themselves
  // on destruction.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  const content::MediaStreamDevices& GetAudioCaptureDevices();
  const content::MediaStreamDevices& GetVideoCaptureDevices();

  // Method called from WebCapturerDelegate implementations to process access
  // requests.
  void ProcessMediaAccessRequest(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);

  // Overridden from content::MediaObserver:
  void OnAudioCaptureDevicesChanged() override;
  void OnVideoCaptureDevicesChanged() override;
  virtual void OnMediaRequestStateChanged(
      int render_process_id,
      int render_frame_id,
      int page_request_id,
      const GURL& security_origin,
      content::MediaStreamType stream_type,
      content::MediaRequestState state) override;
  virtual void OnCreatingAudioStream(int render_process_id,
                                     int render_frame_id) override;
  virtual void OnSetCapturingLinkSecured(int render_process_id,
                                         int render_frame_id,
                                         int page_request_id,
                                         content::MediaStreamType stream_type,
                                         bool is_secure) override;
  const content::MediaStreamDevice* GetFirstAvailableAudioDevice();
  const content::MediaStreamDevice* GetFirstAvailableVideoDevice();

  // Helpers for picking particular requested devices, identified by raw id.
  // If the device requested is not available it will return NULL.
  const content::MediaStreamDevice* GetRequestedAudioDevice(
      const std::string& requested_audio_device_id);
  const content::MediaStreamDevice* GetRequestedVideoDevice(
      const std::string& requested_video_device_id);

  // Only for testing.
  void SetTestAudioCaptureDevices(const content::MediaStreamDevices& devices);
  void SetTestVideoCaptureDevices(const content::MediaStreamDevices& devices);
 private:
  friend struct base::DefaultSingletonTraits<MediaCaptureDevicesDispatcher>;

  struct PendingAccessRequest {
    PendingAccessRequest(const content::MediaStreamRequest& request,
                         const content::MediaResponseCallback& callback);
    ~PendingAccessRequest() {}

    content::MediaStreamRequest request;
    content::MediaResponseCallback callback;
  };
  typedef std::deque<PendingAccessRequest> RequestsQueue;
  typedef std::map<content::WebContents*, RequestsQueue> RequestsQueues;

  MediaCaptureDevicesDispatcher();
  virtual ~MediaCaptureDevicesDispatcher();

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) override;

  // Helpers for ProcessMediaAccessRequest().
  void ProcessRegularMediaAccessRequest(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
  void ProcessQueuedAccessRequest(content::WebContents* web_contents);
  void OnAccessRequestResponse(content::WebContents* web_contents,
                               const content::MediaStreamDevices& devices,
                               content::MediaStreamRequestResult result,
                               std::unique_ptr<content::MediaStreamUI> ui);

  // Called by the MediaObserver() functions, executed on UI thread.
  void NotifyAudioDevicesChangedOnUIThread();
  void NotifyVideoDevicesChangedOnUIThread();
  void UpdateMediaRequestStateOnUIThread(
      int render_process_id,
      int render_frame_id,
      int page_request_id,
      content::MediaStreamType stream_type,
      content::MediaRequestState state);
  void OnCreatingAudioStreamOnUIThread(int render_process_id,
                                       int render_frame_id);

  // Only for testing, a list of cached audio capture devices.
  content::MediaStreamDevices test_audio_devices_;

  // Only for testing, a list of cached video capture devices.
  content::MediaStreamDevices test_video_devices_;

  // A list of observers for the device update notifications.
  base::ObserverList<Observer> observers_;

  // Flag used by unittests to disable device enumeration.
  bool is_device_enumeration_disabled_;

  RequestsQueues pending_requests_;

  content::NotificationRegistrar notifications_registrar_;

  DISALLOW_COPY_AND_ASSIGN(MediaCaptureDevicesDispatcher);
};

}  // namespace opera

#endif  // CHILL_BROWSER_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
