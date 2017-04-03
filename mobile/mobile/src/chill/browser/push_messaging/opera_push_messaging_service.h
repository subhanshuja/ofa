// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/browser/push_messaging/push_messaging_service_impl.h
// @final-synchronized

#ifndef CHILL_BROWSER_PUSH_MESSAGING_OPERA_PUSH_MESSAGING_SERVICE_H_
#define CHILL_BROWSER_PUSH_MESSAGING_OPERA_PUSH_MESSAGING_SERVICE_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "chill/browser/push_messaging/push_messaging_visibility_enforcer.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/push_messaging_service.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"
#include "third_party/WebKit/public/platform/modules/push_messaging/WebPushPermissionStatus.h"
#include "url/gurl.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace gcm {
class GCMDriver;
}

namespace content {
struct PushSubscriptionOptions;
}

namespace opera {

class PushMessagingAppIdentifier;

class OperaPushMessagingService : public content::PushMessagingService,
                                  public gcm::GCMAppHandler {
 public:
  // If any Service Workers are using push, starts GCM and adds an app handler.
  static void InitializeForContext(content::BrowserContext* context);

  explicit OperaPushMessagingService(content::BrowserContext* context);
  ~OperaPushMessagingService() override;

  // gcm::GCMAppHandler implementation.
  void ShutdownHandler() override;
  void OnMessage(const std::string& app_id,
                 const gcm::IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(
      const std::string& app_id,
      const gcm::GCMClient::SendErrorDetails& send_error_details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;
  bool CanHandle(const std::string& app_id) const override;

  // content::PushMessagingService implementation:
  GURL GetEndpoint(bool standard_protocol) const override;
  void SubscribeFromDocument(
      const GURL& requesting_origin,
      int64_t service_worker_registration_id,
      int renderer_id,
      int render_frame_id,
      const content::PushSubscriptionOptions& options,
      const content::PushMessagingService::RegisterCallback& callback) override;
  void SubscribeFromWorker(
      const GURL& requesting_origin,
      int64_t service_worker_registration_id,
      const content::PushSubscriptionOptions& options,
      const content::PushMessagingService::RegisterCallback& callback) override;
  void GetEncryptionInfo(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const std::string& sender_id,
      const content::PushMessagingService::EncryptionInfoCallback& callback)
      override;
  void Unsubscribe(
      const GURL& requesting_origin,
      int64_t service_worker_registration_id,
      const std::string& sender_id,
      const content::PushMessagingService::UnregisterCallback&) override;
  blink::WebPushPermissionStatus GetPermissionStatus(
      const GURL& origin,
      bool user_visible) override;
  bool SupportNonVisibleMessages() override;

  void Shutdown();

 private:
  // A subscription is pending until it has succeeded or failed.
  void IncreasePushSubscriptionCount(int add, bool is_pending);
  void DecreasePushSubscriptionCount(int subtract, bool was_pending);

  // OnMessage methods ---------------------------------------------------------

  void DeliverMessageCallback(const std::string& app_id,
                              const GURL& requesting_origin,
                              int64_t service_worker_registration_id,
                              const gcm::IncomingMessage& message,
                              const base::Closure& message_handled_closure,
                              content::PushDeliveryStatus status);

  void DidHandleMessage(const std::string& app_id,
                        const base::Closure& completion_closure);

  // Subscribe methods ---------------------------------------------------------

  void SubscribeEnd(
      const content::PushMessagingService::RegisterCallback& callback,
      const std::string& subscription_id,
      const std::vector<uint8_t>& p256dh,
      const std::vector<uint8_t>& auth,
      content::PushRegistrationStatus status);

  void SubscribeEndWithError(
      const content::PushMessagingService::RegisterCallback& callback,
      content::PushRegistrationStatus status);

  void DidSubscribe(
      const PushMessagingAppIdentifier& app_identifier,
      const content::PushMessagingService::RegisterCallback& callback,
      const std::string& subscription_id,
      gcm::GCMClient::Result result);

  void DidSubscribeWithEncryptionInfo(
      const PushMessagingAppIdentifier& app_identifier,
      const content::PushMessagingService::RegisterCallback& callback,
      const std::string& subscription_id,
      const std::string& p256dh,
      const std::string& auth_secret);

  void DidRequestPermission(
      const PushMessagingAppIdentifier& app_identifier,
      const content::PushSubscriptionOptions& options,
      const content::PushMessagingService::RegisterCallback& callback,
      blink::mojom::PermissionStatus permission_status);

  // GetEncryptionInfo method --------------------------------------------------

  void DidGetEncryptionInfo(
      const PushMessagingService::EncryptionInfoCallback& callback,
      const std::string& p256dh,
      const std::string& auth_secret) const;

  // Unsubscribe methods -------------------------------------------------------

  void Unsubscribe(const std::string& app_id,
                   const std::string& sender_id,
                   const content::PushMessagingService::UnregisterCallback&);

  void DidUnsubscribe(bool was_subscribed,
                      const content::PushMessagingService::UnregisterCallback&,
                      gcm::GCMClient::Result result);

  void UnsubscribeBecausePermissionRevoked(
      const PushMessagingAppIdentifier& app_identifier,
      const base::Closure& closure,
      const std::string& sender_id,
      bool success,
      bool not_found);

  // Helper methods ------------------------------------------------------------

  // Normalizes the |sender_info|. In most cases the |sender_info| will be
  // passed through to the GCM Driver as-is, but NIST P-256 application server
  // keys have to be encoded using the URL-safe variant of the base64 encoding.
  std::string NormalizeSenderInfo(const std::string& sender_info) const;

  // Checks if a given origin is allowed to use Push.
  bool IsPermissionSet(const GURL& origin);
  gcm::GCMDriver* GetGCMDriver();

  // OperaPermissionManager methods --------------------------------------------

  void OnPermissionChanged(const GURL& origin,
                           blink::mojom::PermissionStatus status);

  void SubscribePermissionChange(const GURL& origin);
  void UnsubscribePermissionChange(const GURL& origin);
  void UnsubscribeAllPermissionChangeCallbacks();

  std::map<GURL, int> permission_subscriptions_;

  content::BrowserContext* context_;

  int push_subscription_count_;
  int pending_push_subscription_count_;

  // A multiset containing one entry for each in-flight push message delivery,
  // keyed by the receiver's app id.
  std::multiset<std::string> in_flight_message_deliveries_;

  base::WeakPtrFactory<OperaPushMessagingService> weak_factory_;

  std::unique_ptr<gcm::GCMDriver> driver_;

  PushMessagingVisibilityEnforcer visibility_enforcer_;

  DISALLOW_COPY_AND_ASSIGN(OperaPushMessagingService);
};

}  // namespace opera

#endif  // CHILL_BROWSER_PUSH_MESSAGING_OPERA_PUSH_MESSAGING_SERVICE_H_
