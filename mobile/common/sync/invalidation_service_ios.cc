// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "components/invalidation/impl/ticl_invalidation_service.h"

#include "components/invalidation/impl/invalidation_service_util.h"
#include "components/invalidation/impl/invalidation_state_tracker.h"
#include "components/invalidation/impl/invalidator.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "net/url_request/url_request_context_getter.h"

#include "common/account/account_service.h"
#include "common/account/account_util.h"
#include "common/sync/sync_account.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"

#include "mobile/common/sync/fake_invalidator.h"

namespace {

static const net::BackoffEntry::Policy kRequestAccessTokenBackoffPolicy = { 0 };

}

namespace invalidation {

TiclInvalidationService::TiclInvalidationService(
    const std::string& user_agent,
    opera::SyncAccount* account,
    opera::oauth2::AuthService* auth_service,
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
    : user_agent_(user_agent),
      account_(account),
      invalidator_registrar_(new syncer::InvalidatorRegistrar()),
      request_access_token_backoff_(&kRequestAccessTokenBackoffPolicy),
      request_context_(request_context) {
}

TiclInvalidationService::~TiclInvalidationService() {
  if (IsStarted()) {
    StopInvalidator();
  }
}

void TiclInvalidationService::Init(
    std::unique_ptr<syncer::InvalidationStateTracker> invalidation_state_tracker) {
  DCHECK(CalledOnValidThread());
  invalidation_state_tracker_ = std::move(invalidation_state_tracker);

  if (invalidation_state_tracker_->GetInvalidatorClientId().empty()) {
    invalidation_state_tracker_->ClearAndSetNewClientId(
        GenerateInvalidatorClientId());
  }

  UpdateInvalidationNetworkChannel();
  if (IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  }
}

void TiclInvalidationService::RegisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK(CalledOnValidThread());
  DVLOG(2) << "Registering an invalidation handler";
  invalidator_registrar_->RegisterHandler(handler);
  if (invalidator_) {
    CHECK(invalidator_->UpdateRegisteredIds(
        this, invalidator_registrar_->GetAllRegisteredIds()));
  }
  logger_.OnRegistration(handler->GetOwnerName());
}

bool TiclInvalidationService::UpdateRegisteredInvalidationIds(
    syncer::InvalidationHandler* handler,
    const syncer::ObjectIdSet& ids) {
  DCHECK(CalledOnValidThread());
  DVLOG(2) << "Registering ids: " << ids.size();
  if (!invalidator_registrar_->UpdateRegisteredIds(handler, ids))
    return false;
  if (invalidator_) {
    CHECK(invalidator_->UpdateRegisteredIds(
        this, invalidator_registrar_->GetAllRegisteredIds()));
  }
  logger_.OnUpdateIds(invalidator_registrar_->GetSanitizedHandlersIdsMap());
  return true;
}

void TiclInvalidationService::UnregisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK(CalledOnValidThread());
  DVLOG(2) << "Unregistering";
  invalidator_registrar_->UnregisterHandler(handler);
  if (invalidator_) {
    CHECK(invalidator_->UpdateRegisteredIds(
        this, invalidator_registrar_->GetAllRegisteredIds()));
  }
  logger_.OnUnregistration(handler->GetOwnerName());
}

#if defined(OPERA_SYNC)
syncer::OperaInvalidatorState TiclInvalidationService::GetInvalidatorState() const {
#else
syncer::InvalidatorState TiclInvalidationService::GetInvalidatorState() const {
#endif  // OPERA_SYNC
  DCHECK(CalledOnValidThread());
  if (invalidator_) {
    DVLOG(2) << "GetInvalidatorState returning "

        << invalidator_->GetInvalidatorState().state;
    return invalidator_->GetInvalidatorState();
  } else {
    DVLOG(2) << "Invalidator currently stopped";
#if defined(OPERA_SYNC)
    return syncer::OperaInvalidatorState(syncer::TRANSIENT_INVALIDATION_ERROR);
#else
    return syncer::TRANSIENT_INVALIDATION_ERROR;
#endif  // OPERA_SYNC
  }
}

std::string TiclInvalidationService::GetInvalidatorClientId() const {
  DCHECK(CalledOnValidThread());
  return invalidation_state_tracker_->GetInvalidatorClientId();
}

InvalidationLogger* TiclInvalidationService::GetInvalidationLogger() {
  return &logger_;
}

IdentityProvider* TiclInvalidationService::GetIdentityProvider() {
  NOTREACHED();
  return NULL;
}

void TiclInvalidationService::RequestDetailedStatus(
    base::Callback<void(const base::DictionaryValue&)> return_callback) const {
  if (IsStarted()) {
    return_callback.Run(network_channel_options_);
    invalidator_->RequestDetailedStatus(return_callback);
  }
}

void TiclInvalidationService::RequestAccessToken() {
}

void TiclInvalidationService::OnSessionStateChanged() {
}

void TiclInvalidationService::OnAccessTokenRequestCompleted(
      opera::oauth2::AuthService* service,
      opera::oauth2::RequestAccessTokenResponse response) {
}

void TiclInvalidationService::OnLoggedIn(opera::AccountService*, opera_sync::OperaAuthProblem) {
}

void TiclInvalidationService::OnLoggedOut(opera::AccountService*,
    opera::account_util::LogoutReason) {
}

void TiclInvalidationService::OnLoginError(opera::AccountService*,
                                           opera::account_util::AuthDataUpdaterError,
                                           opera::account_util::AuthOperaComError,
                                           const std::string&,
                                           opera_sync::OperaAuthProblem) {
}

void TiclInvalidationService::OnAuthDataExpired(opera::AccountService*,
                                                opera_sync::OperaAuthProblem) {
}

opera::SyncAccount* TiclInvalidationService::account() const {
  return NULL;
}

void TiclInvalidationService::OnInvalidatorStateChange(
#if defined(OPERA_SYNC)
    syncer::OperaInvalidatorState state) {
#else
    syncer::InvalidatorState state) {
#endif  // OPERA_SYNC
  invalidator_registrar_->UpdateInvalidatorState(state);
#if defined(OPERA_SYNC)
  logger_.OnStateChange(state.state);
#else
  logger_.OnStateChange(state);
#endif  // OPERA_SYNC
}

void TiclInvalidationService::OnIncomingInvalidation(
    const syncer::ObjectIdInvalidationMap& invalidation_map) {

  // An empty map implies that we should invalidate all.
  const syncer::ObjectIdInvalidationMap& effective_invalidation_map =
      invalidation_map.Empty() ?
      syncer::ObjectIdInvalidationMap::InvalidateAll(
          invalidator_registrar_->GetAllRegisteredIds()) :
      invalidation_map;

  invalidator_registrar_->DispatchInvalidationsToHandlers(
      effective_invalidation_map);

  logger_.OnInvalidation(effective_invalidation_map);
}

std::string TiclInvalidationService::GetOwnerName() const { return "TICL"; }

bool TiclInvalidationService::IsReadyToStart() {
  return true;
}

bool TiclInvalidationService::IsStarted() const {
  return invalidator_.get() != NULL;
}

void TiclInvalidationService::StartInvalidator(
    InvalidationNetworkChannel network_channel) {
  DCHECK(CalledOnValidThread());
  DCHECK(!invalidator_);
  DCHECK(invalidation_state_tracker_);
  DCHECK(!invalidation_state_tracker_->GetInvalidatorClientId().empty());
  DCHECK_EQ(network_channel, PUSH_CLIENT_CHANNEL);

  invalidator_.reset(new mobile::FakeInvalidator(
      invalidation_state_tracker_->GetSavedInvalidations(),
      invalidation_state_tracker_->GetBootstrapData(),
      invalidation_state_tracker_.get()));

  invalidator_->RegisterHandler(this);
  CHECK(invalidator_->UpdateRegisteredIds(
      this, invalidator_registrar_->GetAllRegisteredIds()));
}

void TiclInvalidationService::UpdateInvalidationNetworkChannel() {
  if (network_channel_type_ == PUSH_CLIENT_CHANNEL)
    return;
  network_channel_type_ = PUSH_CLIENT_CHANNEL;
  if (IsStarted()) {
    StopInvalidator();
    StartInvalidator(network_channel_type_);
  }
}

void TiclInvalidationService::UpdateInvalidatorCredentials() {
}

void TiclInvalidationService::StopInvalidator() {
  DCHECK(invalidator_);
  invalidator_->UnregisterHandler(this);
  invalidator_.reset();
}

}  // namespace invalidation
