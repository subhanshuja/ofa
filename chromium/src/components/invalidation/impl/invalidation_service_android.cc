// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidation_service_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/callback.h"
#include "components/invalidation/impl/invalidation_service_util.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "google/cacheinvalidation/types.pb.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaRef;
using base::android::JavaParamRef;

namespace {
std::string g_invalidator_id;
}  // namespace

namespace invalidation {

InvalidationServiceAndroid::InvalidationServiceAndroid(
    const JavaRef<jobject>& context)
    : invalidator_state_(syncer::INVALIDATIONS_ENABLED), logger_() {
  if (g_invalidator_id.empty()) {
    g_invalidator_id = GenerateInvalidatorClientId();
  }
}

InvalidationServiceAndroid::~InvalidationServiceAndroid() { }

void InvalidationServiceAndroid::RegisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK(CalledOnValidThread());
  invalidator_registrar_.RegisterHandler(handler);
  logger_.OnRegistration(handler->GetOwnerName());
}

bool InvalidationServiceAndroid::UpdateRegisteredInvalidationIds(
    syncer::InvalidationHandler* handler,
    const syncer::ObjectIdSet& ids) {
  DCHECK(CalledOnValidThread());

  if (!invalidator_registrar_.UpdateRegisteredIds(handler, ids))
    return false;

  logger_.OnUpdateIds(invalidator_registrar_.GetSanitizedHandlersIdsMap());
  return true;
}

void InvalidationServiceAndroid::UnregisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK(CalledOnValidThread());
  invalidator_registrar_.UnregisterHandler(handler);
  logger_.OnUnregistration(handler->GetOwnerName());
}

#if defined(OPERA_SYNC)
syncer::OperaInvalidatorState
#else
syncer::InvalidatorState
#endif  // OPERA_SYNC
InvalidationServiceAndroid::GetInvalidatorState() const {
  DCHECK(CalledOnValidThread());
  return invalidator_state_;
}

std::string InvalidationServiceAndroid::GetInvalidatorClientId() const {
  DCHECK(CalledOnValidThread());
  return g_invalidator_id;
}

InvalidationLogger* InvalidationServiceAndroid::GetInvalidationLogger() {
  return &logger_;
}

void InvalidationServiceAndroid::RequestDetailedStatus(
    base::Callback<void(const base::DictionaryValue&)> return_callback) const {
}

IdentityProvider* InvalidationServiceAndroid::GetIdentityProvider() {
  return NULL;
}

void InvalidationServiceAndroid::TriggerStateChangeForTest(
    syncer::InvalidatorState state) {
  DCHECK(CalledOnValidThread());
  //invalidator_state_ = state;
  invalidator_registrar_.UpdateInvalidatorState(invalidator_state_);
}

void InvalidationServiceAndroid::Invalidate(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint object_source,
    const JavaParamRef<jstring>& java_object_id,
    jlong version,
    const JavaParamRef<jstring>& java_payload) {
  syncer::ObjectIdInvalidationMap object_invalidation_map;
  if (!java_object_id) {
    syncer::ObjectIdSet sync_ids;
    if (object_source == 0) {
      sync_ids = invalidator_registrar_.GetAllRegisteredIds();
    } else {
      for (const auto& id : invalidator_registrar_.GetAllRegisteredIds()) {
        if (id.source() == object_source)
          sync_ids.insert(id);
      }
    }
    object_invalidation_map =
        syncer::ObjectIdInvalidationMap::InvalidateAll(sync_ids);
  } else {
    invalidation::ObjectId object_id(
        object_source, ConvertJavaStringToUTF8(env, java_object_id));

    if (version == ipc::invalidation::Constants::UNKNOWN) {
      object_invalidation_map.Insert(
          syncer::Invalidation::InitUnknownVersion(object_id));
    } else {
      ObjectIdVersionMap::iterator it =
          max_invalidation_versions_.find(object_id);
      if ((it != max_invalidation_versions_.end()) && (version <= it->second)) {
        DVLOG(1) << "Dropping redundant invalidation with version " << version;
        return;
      }
      max_invalidation_versions_[object_id] = version;
      std::string payload;
      if (!java_payload.is_null())
        ConvertJavaStringToUTF8(env, java_payload, &payload);

      object_invalidation_map.Insert(
          syncer::Invalidation::Init(object_id, version, payload));
    }
  }

  invalidator_registrar_.DispatchInvalidationsToHandlers(
      object_invalidation_map);
  logger_.OnInvalidation(object_invalidation_map);
}

}  // namespace invalidation
