// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/browser/push_messaging/push_messaging_service_impl.cc
// @final-synchronized

#include "chill/browser/push_messaging/opera_push_messaging_service.h"

#include "base/barrier_closure.h"
#include "base/base64url.h"
#include "base/callback_helpers.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/push_messaging/push_messaging_app_identifier.h"
#include "chill/browser/push_messaging/push_messaging_constants.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_driver_android.h"
#include "components/gcm_driver/gcm_driver_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/push_subscription_options.h"

namespace opera {

namespace {
const int kMaxRegistrations = 1000000;

// Chrome does not yet support silent push messages, and requires websites to
// indicate that they will only send user-visible messages.
const char kSilentPushUnsupportedMessage[] =
    "Chrome currently only supports the Push API for subscriptions that will "
    "result in user-visible messages. You can indicate this by calling "
    "pushManager.subscribe({userVisibleOnly: true}) instead. See "
    "https://goo.gl/yqv4Q4 for more details.";

blink::WebPushPermissionStatus ToPushPermission(
    blink::mojom::PermissionStatus permission_status) {
  switch (permission_status) {
    case blink::mojom::PermissionStatus::GRANTED:
      return blink::WebPushPermissionStatusGranted;
    case blink::mojom::PermissionStatus::DENIED:
      return blink::WebPushPermissionStatusDenied;
    case blink::mojom::PermissionStatus::ASK:
      return blink::WebPushPermissionStatusPrompt;
    default:
      NOTREACHED();
      return blink::WebPushPermissionStatusDenied;
  }
}

void UnregisterCallbackToClosure(const base::Closure& closure,
                                 content::PushUnregistrationStatus status) {
  closure.Run();
}

}  // namespace

// static
void OperaPushMessagingService::InitializeForContext(
    content::BrowserContext* context) {
  // TODO(johnme): Consider whether push should be enabled in incognito.
  if (!context || context->IsOffTheRecord())
    return;

  int count = PushMessagingAppIdentifier::GetCount(nullptr);
  if (count <= 0)
    return;

  OperaPushMessagingService* push_service =
      static_cast<OperaPushMessagingService*>(
          context->GetPushMessagingService());

  push_service->IncreasePushSubscriptionCount(count, false /* is_pending */);

  std::vector<PushMessagingAppIdentifier> all =
      PushMessagingAppIdentifier::GetAll(nullptr);

  for (PushMessagingAppIdentifier app_identifier : all)
    push_service->SubscribePermissionChange(app_identifier.origin());
}

OperaPushMessagingService::OperaPushMessagingService(
    content::BrowserContext* context)
    : context_(context),
      push_subscription_count_(0),
      pending_push_subscription_count_(0),
      weak_factory_(this),
      visibility_enforcer_(context_) {
  DCHECK(context);
}

OperaPushMessagingService::~OperaPushMessagingService() {
  UnsubscribeAllPermissionChangeCallbacks();
}

void OperaPushMessagingService::IncreasePushSubscriptionCount(int add,
                                                              bool is_pending) {
  DCHECK(add > 0);
  if (push_subscription_count_ + pending_push_subscription_count_ == 0) {
    GetGCMDriver()->AddAppHandler(kPushMessagingAppIdentifierPrefix, this);
  }
  if (is_pending) {
    pending_push_subscription_count_ += add;
  } else {
    push_subscription_count_ += add;
  }
}

void OperaPushMessagingService::DecreasePushSubscriptionCount(
    int subtract,
    bool was_pending) {
  DCHECK(subtract > 0);
  if (was_pending) {
    pending_push_subscription_count_ -= subtract;
    DCHECK(pending_push_subscription_count_ >= 0);
  } else {
    push_subscription_count_ -= subtract;
    DCHECK(push_subscription_count_ >= 0);
  }
  if (push_subscription_count_ + pending_push_subscription_count_ == 0) {
    GetGCMDriver()->RemoveAppHandler(kPushMessagingAppIdentifierPrefix);
  }
}

bool OperaPushMessagingService::CanHandle(const std::string& app_id) const {
  return !PushMessagingAppIdentifier::FindByAppId(nullptr, app_id).is_null();
}

void OperaPushMessagingService::ShutdownHandler() {
  // Shutdown() should come before and it removes us from the list of app
  // handlers of gcm::GCMDriver so this shouldn't ever been called.
  NOTREACHED();
}

// OnMessage methods -----------------------------------------------------------

void OperaPushMessagingService::OnMessage(const std::string& app_id,
                                          const gcm::IncomingMessage& message) {
  in_flight_message_deliveries_.insert(app_id);

  base::Closure message_handled_closure = base::Bind(&base::DoNothing);

  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::FindByAppId(nullptr, app_id);
  // Drop message and unregister if app_id was unknown (maybe recently deleted).
  if (app_identifier.is_null()) {
    DeliverMessageCallback(app_id, GURL::EmptyGURL(), -1, message,
                           message_handled_closure,
                           content::PUSH_DELIVERY_STATUS_UNKNOWN_APP_ID);
    return;
  }
  // Drop message and unregister if |origin| has lost push permission.
  if (!IsPermissionSet(app_identifier.origin())) {
    DeliverMessageCallback(app_id, app_identifier.origin(),
                           app_identifier.service_worker_registration_id(),
                           message, message_handled_closure,
                           content::PUSH_DELIVERY_STATUS_PERMISSION_DENIED);
    return;
  }

  // The payload of a push message can be valid with content, valid with empty
  // content, or null. Only set the payload data if it is non-null.
  content::PushEventPayload payload;
  if (message.decrypted)
    payload.setData(message.raw_data);

  visibility_enforcer_.IncMessages();

  // Dispatch the message to the appropriate Service Worker.
  content::BrowserContext::DeliverPushMessage(
      context_, app_identifier.origin(),
      app_identifier.service_worker_registration_id(), payload,
      base::Bind(&OperaPushMessagingService::DeliverMessageCallback,
                 weak_factory_.GetWeakPtr(), app_identifier.app_id(),
                 app_identifier.origin(),
                 app_identifier.service_worker_registration_id(), message,
                 message_handled_closure));
}

void OperaPushMessagingService::DeliverMessageCallback(
    const std::string& app_id,
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const gcm::IncomingMessage& message,
    const base::Closure& message_handled_closure,
    content::PushDeliveryStatus status) {
  DCHECK_GE(in_flight_message_deliveries_.count(app_id), 1u);

  PushMessagingVisibilityEnforcer::AutoDecMessages auto_dec(
      &visibility_enforcer_);

  base::Closure completion_closure =
      base::Bind(&OperaPushMessagingService::DidHandleMessage,
                 weak_factory_.GetWeakPtr(), app_id, message_handled_closure);
  // The completion_closure should run by default at the end of this function,
  // unless it is explicitly passed to another function.
  base::ScopedClosureRunner completion_closure_runner(completion_closure);

  // TODO(mvanouwerkerk): Show a warning in the developer console of the
  // Service Worker corresponding to app_id (and/or on an internals page).
  // See https://crbug.com/508516 for options.
  switch (status) {
    // Call EnforceUserVisibleOnlyRequirements if the message was delivered to
    // the Service Worker JavaScript, even if the website's event handler failed
    // (to prevent sites deliberately failing in order to avoid having to show
    // notifications).
    case content::PUSH_DELIVERY_STATUS_SUCCESS:
    case content::PUSH_DELIVERY_STATUS_EVENT_WAITUNTIL_REJECTED:
      visibility_enforcer_.EnforcePolicyAfterDelay(
          requesting_origin, service_worker_registration_id, &auto_dec);
      break;
    case content::PUSH_DELIVERY_STATUS_SERVICE_WORKER_ERROR:
      break;
    case content::PUSH_DELIVERY_STATUS_UNKNOWN_APP_ID:
    case content::PUSH_DELIVERY_STATUS_PERMISSION_DENIED:
    case content::PUSH_DELIVERY_STATUS_NO_SERVICE_WORKER:
      Unsubscribe(app_id, message.sender_id,
                  base::Bind(&UnregisterCallbackToClosure,
                             completion_closure_runner.Release()));
      break;
  }
}

void OperaPushMessagingService::DidHandleMessage(
    const std::string& app_id,
    const base::Closure& message_handled_closure) {
  auto in_flight_iterator = in_flight_message_deliveries_.find(app_id);
  DCHECK(in_flight_iterator != in_flight_message_deliveries_.end());

  // Remove a single in-flight delivery for |app_id|. This has to be done using
  // an iterator rather than by value, as the latter removes all entries.
  in_flight_message_deliveries_.erase(in_flight_iterator);

  message_handled_closure.Run();
}

// Other gcm::GCMAppHandler methods --------------------------------------------

void OperaPushMessagingService::OnMessagesDeleted(const std::string& app_id) {
  // TODO(mvanouwerkerk): Consider firing an event on the Service Worker
  // corresponding to |app_id| to inform the app about deleted messages.
}

void OperaPushMessagingService::OnSendError(
    const std::string& app_id,
    const gcm::GCMClient::SendErrorDetails& send_error_details) {
  NOTREACHED() << "The Push API shouldn't have sent messages upstream";
}

void OperaPushMessagingService::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  NOTREACHED() << "The Push API shouldn't have sent messages upstream";
}

// GetEndpoint method ------------------------------------------------------

GURL OperaPushMessagingService::GetEndpoint(bool standard_protocol) const {
  return GURL(standard_protocol ? kPushMessagingPushProtocolEndpoint
                                : kPushMessagingGcmEndpoint);
}

// Subscribe and GetPermissionStatus methods -----------------------------------

void OperaPushMessagingService::SubscribeFromDocument(
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    int renderer_id,
    int render_frame_id,
    const content::PushSubscriptionOptions& options,
    const content::PushMessagingService::RegisterCallback& callback) {
  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::Generate(requesting_origin,
                                           service_worker_registration_id);

  if (push_subscription_count_ + pending_push_subscription_count_ >=
      kMaxRegistrations) {
    SubscribeEndWithError(callback,
                          content::PUSH_REGISTRATION_STATUS_LIMIT_REACHED);
    return;
  }

  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(renderer_id, render_frame_id);
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  if (!options.user_visible_only) {
    web_contents->GetMainFrame()->AddMessageToConsole(
        content::CONSOLE_MESSAGE_LEVEL_ERROR, kSilentPushUnsupportedMessage);

    SubscribeEndWithError(callback,
                          content::PUSH_REGISTRATION_STATUS_PERMISSION_DENIED);
    return;
  }

  // Push does not allow permission requests from iframes.
  context_->GetPermissionManager()->RequestPermission(
      content::PermissionType::PUSH_MESSAGING, web_contents->GetMainFrame(),
      requesting_origin, false,
      base::Bind(&OperaPushMessagingService::DidRequestPermission,
                 weak_factory_.GetWeakPtr(), app_identifier, options,
                 callback));
}

void OperaPushMessagingService::SubscribeFromWorker(
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const content::PushSubscriptionOptions& options,
    const content::PushMessagingService::RegisterCallback& register_callback) {
  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::Generate(requesting_origin,
                                           service_worker_registration_id);

  if (push_subscription_count_ + pending_push_subscription_count_ >=
      kMaxRegistrations) {
    SubscribeEndWithError(register_callback,
                          content::PUSH_REGISTRATION_STATUS_LIMIT_REACHED);
    return;
  }

  blink::WebPushPermissionStatus permission_status =
      OperaPushMessagingService::GetPermissionStatus(requesting_origin,
                                                     options.user_visible_only);

  if (permission_status != blink::WebPushPermissionStatusGranted) {
    SubscribeEndWithError(register_callback,
                          content::PUSH_REGISTRATION_STATUS_PERMISSION_DENIED);
    return;
  }

  IncreasePushSubscriptionCount(1, true /* is_pending */);
  std::vector<std::string> sender_ids(1,
                                      NormalizeSenderInfo(options.sender_info));
  GetGCMDriver()->Register(app_identifier.app_id(), sender_ids,
                           base::Bind(&OperaPushMessagingService::DidSubscribe,
                                      weak_factory_.GetWeakPtr(),
                                      app_identifier, register_callback));
}

blink::WebPushPermissionStatus OperaPushMessagingService::GetPermissionStatus(
    const GURL& origin,
    bool user_visible) {
  if (!user_visible)
    return blink::WebPushPermissionStatusDenied;

  // Because the Push API is tied to Service Workers, many usages of the API
  // won't have an embedding origin at all. Only consider the requesting
  // |origin| when checking whether permission to use the API has been granted.
  return ToPushPermission(context_->GetPermissionManager()->GetPermissionStatus(
      content::PermissionType::PUSH_MESSAGING, origin, origin));
}

bool OperaPushMessagingService::SupportNonVisibleMessages() {
  return false;
}

void OperaPushMessagingService::SubscribeEnd(
    const content::PushMessagingService::RegisterCallback& callback,
    const std::string& subscription_id,
    const std::vector<uint8_t>& p256dh,
    const std::vector<uint8_t>& auth,
    content::PushRegistrationStatus status) {
  callback.Run(subscription_id, p256dh, auth, status);
}

void OperaPushMessagingService::SubscribeEndWithError(
    const content::PushMessagingService::RegisterCallback& callback,
    content::PushRegistrationStatus status) {
  SubscribeEnd(callback, std::string() /* subscription_id */,
               std::vector<uint8_t>() /* p256dh */,
               std::vector<uint8_t>() /* auth */, status);
}

void OperaPushMessagingService::DidSubscribe(
    const PushMessagingAppIdentifier& app_identifier,
    const content::PushMessagingService::RegisterCallback& callback,
    const std::string& subscription_id,
    gcm::GCMClient::Result result) {
  DecreasePushSubscriptionCount(1, true /* was_pending */);

  content::PushRegistrationStatus status =
      content::PUSH_REGISTRATION_STATUS_SERVICE_ERROR;

  switch (result) {
    case gcm::GCMClient::SUCCESS:
      // Make sure that this subscription has associated encryption keys prior
      // to returning it to the developer - they'll need this information in
      // order to send payloads to the user.
      GetGCMDriver()->GetEncryptionInfo(
          app_identifier.app_id(),
          base::Bind(&OperaPushMessagingService::DidSubscribeWithEncryptionInfo,
                     weak_factory_.GetWeakPtr(), app_identifier, callback,
                     subscription_id));

      return;
    case gcm::GCMClient::INVALID_PARAMETER:
    case gcm::GCMClient::GCM_DISABLED:
    case gcm::GCMClient::ASYNC_OPERATION_PENDING:
    case gcm::GCMClient::SERVER_ERROR:
    case gcm::GCMClient::UNKNOWN_ERROR:
      status = content::PUSH_REGISTRATION_STATUS_SERVICE_ERROR;
      break;
    case gcm::GCMClient::NETWORK_ERROR:
    case gcm::GCMClient::TTL_EXCEEDED:
      status = content::PUSH_REGISTRATION_STATUS_NETWORK_ERROR;
      break;
  }

  SubscribeEndWithError(callback, status);
}

void OperaPushMessagingService::DidSubscribeWithEncryptionInfo(
    const PushMessagingAppIdentifier& app_identifier,
    const content::PushMessagingService::RegisterCallback& callback,
    const std::string& subscription_id,
    const std::string& p256dh,
    const std::string& auth_secret) {
  if (!p256dh.size()) {
    SubscribeEndWithError(
        callback, content::PUSH_REGISTRATION_STATUS_PUBLIC_KEY_UNAVAILABLE);
    return;
  }

  app_identifier.PersistToPrefs(nullptr);

  SubscribePermissionChange(app_identifier.origin());

  IncreasePushSubscriptionCount(1, false /* is_pending */);

  SubscribeEnd(callback, subscription_id,
               std::vector<uint8_t>(p256dh.begin(), p256dh.end()),
               std::vector<uint8_t>(auth_secret.begin(), auth_secret.end()),
               content::PUSH_REGISTRATION_STATUS_SUCCESS_FROM_PUSH_SERVICE);
}


void OperaPushMessagingService::DidRequestPermission(
    const PushMessagingAppIdentifier& app_identifier,
    const content::PushSubscriptionOptions& options,
    const content::PushMessagingService::RegisterCallback& register_callback,
    blink::mojom::PermissionStatus permission_status) {
  if (permission_status != blink::mojom::PermissionStatus::GRANTED) {
    SubscribeEndWithError(register_callback,
                          content::PUSH_REGISTRATION_STATUS_PERMISSION_DENIED);
    return;
  }

  IncreasePushSubscriptionCount(1, true /* is_pending */);
  std::vector<std::string> sender_ids(1,
                                      NormalizeSenderInfo(options.sender_info));

  GetGCMDriver()->Register(app_identifier.app_id(), sender_ids,
                           base::Bind(&OperaPushMessagingService::DidSubscribe,
                                      weak_factory_.GetWeakPtr(),
                                      app_identifier, register_callback));
}

// GetEncryptionInfo methods ---------------------------------------------------

void OperaPushMessagingService::GetEncryptionInfo(
    const GURL& origin,
    int64_t service_worker_registration_id,
    const std::string& sender_id,
    const PushMessagingService::EncryptionInfoCallback& callback) {
  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::FindByServiceWorker(
          nullptr, origin, service_worker_registration_id);

  DCHECK(!app_identifier.is_null());

  GetGCMDriver()->GetEncryptionInfo(
      app_identifier.app_id(),
      base::Bind(&OperaPushMessagingService::DidGetEncryptionInfo,
                 weak_factory_.GetWeakPtr(), callback));
}

void OperaPushMessagingService::DidGetEncryptionInfo(
    const PushMessagingService::EncryptionInfoCallback& callback,
    const std::string& p256dh,
    const std::string& auth_secret) const {
  // I/O errors might prevent the GCM Driver from retrieving a key-pair.
  const bool success = !!p256dh.size();

  callback.Run(success, std::vector<uint8_t>(p256dh.begin(), p256dh.end()),
               std::vector<uint8_t>(auth_secret.begin(), auth_secret.end()));
}

// Unsubscribe methods ---------------------------------------------------------

void OperaPushMessagingService::Unsubscribe(
    const GURL& requesting_origin,
    int64_t service_worker_registration_id,
    const std::string& sender_id,
    const content::PushMessagingService::UnregisterCallback& callback) {
  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::FindByServiceWorker(
          nullptr, requesting_origin, service_worker_registration_id);
  if (app_identifier.is_null()) {
    if (!callback.is_null()) {
      callback.Run(
          content::PUSH_UNREGISTRATION_STATUS_SUCCESS_WAS_NOT_REGISTERED);
    }
    return;
  }

  Unsubscribe(app_identifier.app_id(), sender_id, callback);
}

void OperaPushMessagingService::Unsubscribe(
    const std::string& app_id,
    const std::string& sender_id,
    const content::PushMessagingService::UnregisterCallback& callback) {
  // Delete the mapping for this app_id, to guarantee that no messages get
  // delivered in future (even if unregistration fails).
  // TODO(johnme): Instead of deleting these app ids, store them elsewhere, and
  // retry unregistration if it fails due to network errors (crbug.com/465399).
  PushMessagingAppIdentifier app_identifier =
      PushMessagingAppIdentifier::FindByAppId(nullptr, app_id);
  bool was_registered = !app_identifier.is_null();
  if (was_registered) {
    app_identifier.DeleteFromPrefs(nullptr);
    UnsubscribePermissionChange(app_identifier.origin());
  }

  const auto& unregister_callback =
      base::Bind(&OperaPushMessagingService::DidUnsubscribe,
                 weak_factory_.GetWeakPtr(), was_registered, callback);

  // On Android the backend is different, and requires the original sender_id.
  // UnsubscribeBecausePermissionRevoked sometimes calls us with an empty one.
  if (sender_id.empty()) {
    unregister_callback.Run(gcm::GCMClient::INVALID_PARAMETER);
  } else {
    GetGCMDriver()->UnregisterWithSenderId(
        app_id, NormalizeSenderInfo(sender_id), unregister_callback);
  }
}

void OperaPushMessagingService::DidUnsubscribe(
    bool was_subscribed,
    const content::PushMessagingService::UnregisterCallback& callback,
    gcm::GCMClient::Result result) {
  if (was_subscribed)
    DecreasePushSubscriptionCount(1, false /* was_pending */);

  // Internal calls pass a null callback.
  if (callback.is_null())
    return;

  if (!was_subscribed) {
    callback.Run(
        content::PUSH_UNREGISTRATION_STATUS_SUCCESS_WAS_NOT_REGISTERED);
    return;
  }
  switch (result) {
    case gcm::GCMClient::SUCCESS:
      callback.Run(content::PUSH_UNREGISTRATION_STATUS_SUCCESS_UNREGISTERED);
      break;
    case gcm::GCMClient::INVALID_PARAMETER:
    case gcm::GCMClient::GCM_DISABLED:
    case gcm::GCMClient::SERVER_ERROR:
    case gcm::GCMClient::UNKNOWN_ERROR:
      callback.Run(content::PUSH_UNREGISTRATION_STATUS_PENDING_SERVICE_ERROR);
      break;
    case gcm::GCMClient::ASYNC_OPERATION_PENDING:
    case gcm::GCMClient::NETWORK_ERROR:
    case gcm::GCMClient::TTL_EXCEEDED:
      callback.Run(content::PUSH_UNREGISTRATION_STATUS_PENDING_NETWORK_ERROR);
      break;
  }
}

void OperaPushMessagingService::UnsubscribeBecausePermissionRevoked(
    const PushMessagingAppIdentifier& app_identifier,
    const base::Closure& closure,
    const std::string& sender_id,
    bool success,
    bool not_found) {
  base::Closure barrier_closure = base::BarrierClosure(2, closure);

  // Unsubscribe the PushMessagingAppIdentifier with the push service.
  // It's possible for GetSenderId to have failed and sender_id to be empty, if
  // cookies (and the SW database) for an origin got cleared before permissions
  // are cleared for the origin. In that case Unsubscribe will just delete the
  // app identifier to block future messages.
  // TODO(johnme): Auto-unregister before SW DB is cleared
  // (https://crbug.com/402458).
  Unsubscribe(app_identifier.app_id(), sender_id,
              base::Bind(&UnregisterCallbackToClosure, barrier_closure));

  // Clear the associated service worker push registration id.
  ClearPushSubscriptionId(context_, app_identifier.origin(),
                          app_identifier.service_worker_registration_id(),
                          barrier_closure);
}

void OperaPushMessagingService::Shutdown() {
  GetGCMDriver()->RemoveAppHandler(kPushMessagingAppIdentifierPrefix);
}

// Helper methods --------------------------------------------------------------

std::string OperaPushMessagingService::NormalizeSenderInfo(
    const std::string& sender_info) const {
  // Only encode the |sender_info| when it is a NIST P-256 public key in
  // uncompressed format, verified through its length and the 0x04 prefix byte.
  if (sender_info.size() != 65 || sender_info[0] != 0x04)
    return sender_info;
  std::string encoded_sender_info;
  base::Base64UrlEncode(sender_info, base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &encoded_sender_info);
  return encoded_sender_info;
}

// Assumes user_visible always since this is just meant to check
// if the permission was previously granted and not revoked.
bool OperaPushMessagingService::IsPermissionSet(const GURL& origin) {
  return GetPermissionStatus(origin, true /* user_visible */) ==
         blink::WebPushPermissionStatusGranted;
}

gcm::GCMDriver* OperaPushMessagingService::GetGCMDriver() {
  if (driver_)
    return driver_.get();

  base::SequencedWorkerPool* worker_pool =
      content::BrowserThread::GetBlockingPool();
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner(
      worker_pool->GetSequencedTaskRunnerWithShutdownBehavior(
          worker_pool->GetSequenceToken(),
          base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));

  driver_.reset(new gcm::GCMDriverAndroid(
      context_->GetPath().Append(gcm_driver::kGCMStoreDirname),
      blocking_task_runner));

  return driver_.get();
}

// OperaPermissionManager methods ----------------------------------------------

void OperaPushMessagingService::OnPermissionChanged(
    const GURL& origin,
    blink::mojom::PermissionStatus status) {

  if (status == blink::mojom::PermissionStatus::GRANTED)
    return;

  std::vector<PushMessagingAppIdentifier> all =
      PushMessagingAppIdentifier::GetAll(nullptr);

  for (const PushMessagingAppIdentifier& app_identifier : all) {
    if (app_identifier.origin() != origin)
      continue;

    GetSenderId(
        context_, app_identifier.origin(),
        app_identifier.service_worker_registration_id(),
        base::Bind(
            &OperaPushMessagingService::UnsubscribeBecausePermissionRevoked,
            weak_factory_.GetWeakPtr(), app_identifier,
            base::Bind(&base::DoNothing)));
  }
}

void OperaPushMessagingService::SubscribePermissionChange(const GURL& origin) {
  if (permission_subscriptions_.find(origin) != permission_subscriptions_.end())
    return;

  content::PermissionManager* permission_manager =
      context_->GetPermissionManager();

  int id = permission_manager->SubscribePermissionStatusChange(
      content::PermissionType::PUSH_MESSAGING, origin, origin,
      base::Bind(&OperaPushMessagingService::OnPermissionChanged,
                 weak_factory_.GetWeakPtr(), origin));

  permission_subscriptions_.insert(std::make_pair(origin, id));
}

void OperaPushMessagingService::UnsubscribePermissionChange(
    const GURL& origin) {
  const auto i = permission_subscriptions_.find(origin);
  if (i == permission_subscriptions_.end())
    return;

  content::PermissionManager* permission_manager =
      context_->GetPermissionManager();
  permission_manager->UnsubscribePermissionStatusChange(i->second);

  permission_subscriptions_.erase(i);
}

void OperaPushMessagingService::UnsubscribeAllPermissionChangeCallbacks() {
  content::PermissionManager* permission_manager =
      context_->GetPermissionManager();
  for (const auto& i : permission_subscriptions_)
    permission_manager->UnsubscribePermissionStatusChange(i.second);
}

}  // namespace opera
