// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sync_internals_message_handler.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "components/sync/base/weak_handle.h"
#include "components/sync/driver/about_sync_util.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/cycle/commit_counters.h"
#include "components/sync/engine/cycle/status_counters.h"
#include "components/sync/engine/cycle/update_counters.h"
#include "components/sync/engine/events/protocol_event.h"
#include "components/sync/js/js_event_details.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"

#if defined(OPERA_SYNC)
#include "common/sync/sync_config.h"
#endif  // OPERA_SYNC

using browser_sync::ProfileSyncService;
using syncer::JsEventDetails;
using syncer::ModelTypeSet;
using syncer::WeakHandle;

namespace {
class UtilAboutSyncDataExtractor : public AboutSyncDataExtractor {
 public:
  std::unique_ptr<base::DictionaryValue> ConstructAboutInformation(
      syncer::SyncService* service,
      SigninManagerBase* signin) override {
    return syncer::sync_ui_util::ConstructAboutInformation(
        service, signin, chrome::GetChannel());
  }
};
}  //  namespace

SyncInternalsMessageHandler::SyncInternalsMessageHandler()
    : SyncInternalsMessageHandler(
          base::MakeUnique<UtilAboutSyncDataExtractor>()) {
#if defined(OPERA_SYNC)
  tracker_is_registered_ = false;
#endif  // OPERA_SYNC
}

SyncInternalsMessageHandler::SyncInternalsMessageHandler(
    std::unique_ptr<AboutSyncDataExtractor> about_sync_data_extractor)
    : about_sync_data_extractor_(std::move(about_sync_data_extractor)),
      weak_ptr_factory_(this) {
#if defined(OPERA_SYNC)
  tracker_is_registered_ = false;
#endif  // OPERA_SYNC
}

SyncInternalsMessageHandler::~SyncInternalsMessageHandler() {
  if (js_controller_)
    js_controller_->RemoveJsEventHandler(this);

  ProfileSyncService* service = GetProfileSyncService();
  if (service && service->HasObserver(this)) {
    service->RemoveObserver(this);
    service->RemoveProtocolEventObserver(this);
  }

#if defined(OPERA_SYNC)
  if (opera::SyncConfig::ShouldDeduplicateBookmarks()) {
    if (tracker_is_registered_) {
      opera::DuplicateTrackerFactory::GetForProfile(
          Profile::FromWebUI(web_ui())->GetOriginalProfile())
          ->RemoveObserver(this);
    }
  }
#endif  // OPERA_SYNC

  if (service && is_registered_for_counters_) {
    service->RemoveTypeDebugInfoObserver(this);
  }
}

void SyncInternalsMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRegisterForEvents,
      base::Bind(&SyncInternalsMessageHandler::HandleRegisterForEvents,
                 base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRegisterForPerTypeCounters,
      base::Bind(&SyncInternalsMessageHandler::HandleRegisterForPerTypeCounters,
                 base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRequestUpdatedAboutInfo,
      base::Bind(&SyncInternalsMessageHandler::HandleRequestUpdatedAboutInfo,
                 base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRequestListOfTypes,
      base::Bind(&SyncInternalsMessageHandler::HandleRequestListOfTypes,
                 base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kGetAllNodes,
      base::Bind(&SyncInternalsMessageHandler::HandleGetAllNodes,
                 base::Unretained(this)));

#if defined(OPERA_SYNC)
  web_ui()->RegisterMessageCallback(
      sync_driver::sync_ui_util::kExitDuplicationBackoff,
      base::Bind(&SyncInternalsMessageHandler::HandleExitDuplicationBackoff,
                 base::Unretained(this)));
#endif  // OPERA_SYNC
}

void SyncInternalsMessageHandler::HandleRegisterForEvents(
    const base::ListValue* args) {
  DCHECK(args->empty());

  // is_registered_ flag protects us from double-registering.  This could
  // happen on a page refresh, where the JavaScript gets re-run but the
  // message handler remains unchanged.
  ProfileSyncService* service = GetProfileSyncService();
  if (service && !is_registered_) {
    service->AddObserver(this);
    service->AddProtocolEventObserver(this);
    js_controller_ = service->GetJsController();
    js_controller_->AddJsEventHandler(this);
    is_registered_ = true;
  }

#if defined(OPERA_SYNC)
  if (!tracker_is_registered_) {
    if (opera::SyncConfig::ShouldDeduplicateBookmarks()) {
      tracker_is_registered_ = true;
      opera::DuplicateTrackerFactory::GetForProfile(
          Profile::FromWebUI(web_ui())->GetOriginalProfile())
          ->AddObserver(this);
    }
  }
#endif  // OPERA_SYNC
}

void SyncInternalsMessageHandler::HandleRegisterForPerTypeCounters(
    const base::ListValue* args) {
  DCHECK(args->empty());

  if (ProfileSyncService* service = GetProfileSyncService()) {
    if (!is_registered_for_counters_) {
      service->AddTypeDebugInfoObserver(this);
      is_registered_for_counters_ = true;
    } else {
      // Re-register to ensure counters get re-emitted.
      service->RemoveTypeDebugInfoObserver(this);
      service->AddTypeDebugInfoObserver(this);
    }
  }
}

void SyncInternalsMessageHandler::HandleRequestUpdatedAboutInfo(
    const base::ListValue* args) {
  DCHECK(args->empty());
  SendAboutInfo();
}

void SyncInternalsMessageHandler::HandleRequestListOfTypes(
    const base::ListValue* args) {
  DCHECK(args->empty());
  base::DictionaryValue event_details;
  std::unique_ptr<base::ListValue> type_list(new base::ListValue());
  ModelTypeSet protocol_types = syncer::ProtocolTypes();
  for (ModelTypeSet::Iterator it = protocol_types.First();
       it.Good(); it.Inc()) {
    type_list->AppendString(ModelTypeToString(it.Get()));
  }
  event_details.Set(syncer::sync_ui_util::kTypes, type_list.release());
  web_ui()->CallJavascriptFunctionUnsafe(
      syncer::sync_ui_util::kDispatchEvent,
      base::StringValue(syncer::sync_ui_util::kOnReceivedListOfTypes),
      event_details);
}

void SyncInternalsMessageHandler::HandleGetAllNodes(
    const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  int request_id = 0;
  bool success = args->GetInteger(0, &request_id);
  DCHECK(success);

  ProfileSyncService* service = GetProfileSyncService();
  if (service) {
    service->GetAllNodes(
        base::Bind(&SyncInternalsMessageHandler::OnReceivedAllNodes,
                   weak_ptr_factory_.GetWeakPtr(), request_id));
  }
}

#if defined(OPERA_SYNC)
void SyncInternalsMessageHandler::HandleExitDuplicationBackoff(
    const base::ListValue* args) {
  opera::DuplicateTracker* tracker = GetDuplicateTracker();
  if (tracker && tracker->IsWithinBackoff())
    tracker->ExitBackoff();
}
#endif  // OPERA_SYNC

void SyncInternalsMessageHandler::OnReceivedAllNodes(
    int request_id,
    std::unique_ptr<base::ListValue> nodes) {
  base::FundamentalValue id(request_id);
  web_ui()->CallJavascriptFunctionUnsafe(
      syncer::sync_ui_util::kGetAllNodesCallback, id, *nodes);
}

void SyncInternalsMessageHandler::OnStateChanged() {
  SendAboutInfo();
}

void SyncInternalsMessageHandler::OnProtocolEvent(
    const syncer::ProtocolEvent& event) {
  std::unique_ptr<base::DictionaryValue> value(
      syncer::ProtocolEvent::ToValue(event));
  web_ui()->CallJavascriptFunctionUnsafe(
      syncer::sync_ui_util::kDispatchEvent,
      base::StringValue(syncer::sync_ui_util::kOnProtocolEvent), *value);
}

void SyncInternalsMessageHandler::OnCommitCountersUpdated(
    syncer::ModelType type,
    const syncer::CommitCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kCommit, counters.ToValue());
}

void SyncInternalsMessageHandler::OnUpdateCountersUpdated(
    syncer::ModelType type,
    const syncer::UpdateCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kUpdate, counters.ToValue());
}

void SyncInternalsMessageHandler::OnStatusCountersUpdated(
    syncer::ModelType type,
    const syncer::StatusCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kStatus, counters.ToValue());
}

#if defined(OPERA_SYNC)
void SyncInternalsMessageHandler::OnTrackerStateChanged(
    opera::DuplicateTracker* tracker) {
}

void SyncInternalsMessageHandler::OnTrackerStatisticsUpdated(
    opera::DuplicateTracker* tracker) {
}

void SyncInternalsMessageHandler::OnDuplicateAppeared(
    opera::DuplicateTracker* tracker,
    opera::FlawId id) {
}

void SyncInternalsMessageHandler::OnFlawAppeared(
    opera::DuplicateTracker* tracker,
    opera::FlawId id) {
}

void SyncInternalsMessageHandler::OnDuplicateDisappeared(
    opera::DuplicateTracker* tracker,
    opera::FlawId id) {
}

void SyncInternalsMessageHandler::OnFlawDisappeared(
    opera::DuplicateTracker* tracker,
    opera::FlawId id) {
}

void SyncInternalsMessageHandler::OnDebugStatsUpdated(
    opera::DuplicateTracker * tracker) {
  DCHECK(opera::SyncConfig::ShouldDeduplicateBookmarks());
  SendAboutInfo();
}
#endif  // OPERA_SYNC

void SyncInternalsMessageHandler::EmitCounterUpdate(
    syncer::ModelType type,
    const std::string& counter_type,
    std::unique_ptr<base::DictionaryValue> value) {
  std::unique_ptr<base::DictionaryValue> details(new base::DictionaryValue());
  details->SetString(syncer::sync_ui_util::kModelType, ModelTypeToString(type));
  details->SetString(syncer::sync_ui_util::kCounterType, counter_type);
  details->Set(syncer::sync_ui_util::kCounters, value.release());
  web_ui()->CallJavascriptFunctionUnsafe(
      syncer::sync_ui_util::kDispatchEvent,
      base::StringValue(syncer::sync_ui_util::kOnCountersUpdated), *details);
}

void SyncInternalsMessageHandler::HandleJsEvent(
    const std::string& name,
    const JsEventDetails& details) {
  DVLOG(1) << "Handling event: " << name
           << " with details " << details.ToString();
  web_ui()->CallJavascriptFunctionUnsafe(syncer::sync_ui_util::kDispatchEvent,
                                         base::StringValue(name),
                                         details.Get());
}

void SyncInternalsMessageHandler::SendAboutInfo() {
  ProfileSyncService* sync_service = GetProfileSyncService();
#if !defined(OPERA_SYNC)
  SigninManagerBase* signin = sync_service ? sync_service->signin() : nullptr;
#else
  SigninManagerBase* signin = nullptr;
#endif  // ! OPERA_SYNC
  std::unique_ptr<base::DictionaryValue> value =
      about_sync_data_extractor_->ConstructAboutInformation(sync_service,
                                                            signin);
  web_ui()->CallJavascriptFunctionUnsafe(
      syncer::sync_ui_util::kDispatchEvent,
      base::StringValue(syncer::sync_ui_util::kOnAboutInfoUpdated), *value);
}

// Gets the ProfileSyncService of the underlying original profile.
// May return NULL (e.g., if sync is disabled on the command line).
ProfileSyncService* SyncInternalsMessageHandler::GetProfileSyncService() {
  Profile* profile = Profile::FromWebUI(web_ui());
  return ProfileSyncServiceFactory::GetForProfile(
      profile->GetOriginalProfile());
}

#if defined(OPERA_SYNC)
opera::DuplicateTracker* SyncInternalsMessageHandler::GetDuplicateTracker() {
  Profile* profile = Profile::FromWebUI(web_ui());
  return opera::DuplicateTrackerFactory::GetForProfile(
      profile->GetOriginalProfile());
}
#endif  // OPERA_SYNC
