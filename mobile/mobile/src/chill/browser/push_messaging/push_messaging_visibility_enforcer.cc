// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software AS

#include "chill/browser/push_messaging/push_messaging_visibility_enforcer.h"

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chill/browser/opera_content_browser_client.h"
#include "chill/browser/push_messaging/push_messaging_constants.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/platform_notification_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "opera_common/grit/product_related_common_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

const int kUserVisibilityDeadlineMs = 10000;

base::DictionaryValue* GetNotificationDebtMap() {
  opera::OperaContentBrowserClient* client =
      opera::OperaContentBrowserClient::Get();
  return client->push_messaging_storage().GetDebtMap();
}

int ChangeNotificationDebt(const GURL& origin, int delta) {
  base::DictionaryValue* map = GetNotificationDebtMap();

  int debt = 0;
  map->GetIntegerWithoutPathExpansion(origin.spec(), &debt);

  if (debt + delta == 0)
    map->RemoveWithoutPathExpansion(origin.spec(), nullptr);
  else
    map->SetIntegerWithoutPathExpansion(origin.spec(), debt + delta);

  opera::OperaContentBrowserClient::Get()->push_messaging_storage().Save();

  return debt + delta;
}

}  // namespace

namespace opera {

PushMessagingVisibilityEnforcer::PushMessagingVisibilityEnforcer(
    content::BrowserContext* context)
    : context_(context), messages_(0), weak_factory_(this) {
  OperaPlatformNotificationService::GetInstance()->AddObserver(this);
}

PushMessagingVisibilityEnforcer::~PushMessagingVisibilityEnforcer() {
  OperaPlatformNotificationService::GetInstance()->RemoveObserver(this);
}

void PushMessagingVisibilityEnforcer::IncMessages() {
  ++messages_;
}

void PushMessagingVisibilityEnforcer::DecMessages() {
  DCHECK_GT(messages_, 0);
  --messages_;
}

PushMessagingVisibilityEnforcer::AutoDecMessages::AutoDecMessages(
    PushMessagingVisibilityEnforcer* enforcer)
    : enforcer_(enforcer) {}

PushMessagingVisibilityEnforcer::AutoDecMessages::~AutoDecMessages() {
  if (enforcer_)
    enforcer_->DecMessages();
}

void PushMessagingVisibilityEnforcer::AutoDecMessages::Cancel() {
  enforcer_ = nullptr;
}

void PushMessagingVisibilityEnforcer::EnforcePolicyAfterDelay(
    const GURL& origin,
    int service_worker_registration_id,
    AutoDecMessages* auto_dec) {
  DCHECK(auto_dec);

  // Cancel caller's auto decrement, since we will decrement in EnforcePolicy.
  auto_dec->Cancel();

  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&PushMessagingVisibilityEnforcer::EnforcePolicy,
                 weak_factory_.GetWeakPtr(), origin,
                 service_worker_registration_id),
      base::TimeDelta::FromMilliseconds(kUserVisibilityDeadlineMs));
}

void PushMessagingVisibilityEnforcer::OnDisplayNotification(
    const GURL& origin,
    const content::PlatformNotificationData& notification_data) {
  if (messages_ <= 0)
    return;

  if (notification_data.tag == kPushMessagingForcedNotificationTag)
    return;

  ChangeNotificationDebt(origin, -1);
}

void PushMessagingVisibilityEnforcer::EnforcePolicy(
    const GURL& origin,
    int service_worker_registration_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DecMessages();

  int debt = ChangeNotificationDebt(origin, 1);

  if (debt <= 0)
    return;

  ChangeNotificationDebt(origin, -debt);

  content::PlatformNotificationData notification_data;
  notification_data.title = url_formatter::FormatUrlForSecurityDisplay(
      origin, url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
  notification_data.direction =
      content::PlatformNotificationData::DIRECTION_LEFT_TO_RIGHT;
  notification_data.body =
      l10n_util::GetStringUTF16(IDS_PUSH_MESSAGING_GENERIC_NOTIFICATION_BODY);
  notification_data.tag = kPushMessagingForcedNotificationTag;
  notification_data.icon = GURL();
  notification_data.silent = true;

  content::NotificationDatabaseData database_data;
  database_data.origin = origin;
  database_data.service_worker_registration_id = service_worker_registration_id;
  database_data.notification_data = notification_data;

  scoped_refptr<content::PlatformNotificationContext> notification_context =
      content::BrowserContext::GetStoragePartitionForSite(context_, origin)
          ->GetPlatformNotificationContext();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &content::PlatformNotificationContext::WriteNotificationData,
          notification_context, origin, database_data,
          base::Bind(&PushMessagingVisibilityEnforcer::DidWriteNotificationData,
                     weak_factory_.GetWeakPtr(), origin,
                     database_data.notification_data)));
}

void PushMessagingVisibilityEnforcer::DidWriteNotificationData(
    const base::WeakPtr<PushMessagingVisibilityEnforcer>& weak_ptr,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    bool success,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&PushMessagingVisibilityEnforcer::DisplayGenericNotification,
                 weak_ptr, origin, notification_data, success,
                 notification_id));
}

void PushMessagingVisibilityEnforcer::DisplayGenericNotification(
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    bool success,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(success);
  OperaPlatformNotificationService::GetInstance()
      ->DisplayPersistentNotification(context_, notification_id,
                                      GURL(), origin, notification_data,
                                      content::NotificationResources());
}

}  // namespace opera
