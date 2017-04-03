// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/ticl_invalidation_service.h"

#include <stddef.h>
#include <utility>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/invalidation/impl/gcm_invalidation_bridge.h"
#include "components/invalidation/impl/invalidation_service_util.h"
#include "components/invalidation/impl/invalidator.h"
#include "components/invalidation/impl/non_blocking_invalidator.h"
#include "components/invalidation/public/invalidation_util.h"
#include "components/invalidation/public/invalidator_state.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OPERA_SYNC)
#include "base/task_runner_util.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/browser/browser_thread.h"

#include "common/account/account_service.h"
#include "common/oauth2/auth_service.h"
#include "common/oauth2/session/persistent_session.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/scopes.h"
#include "common/sync/sync_account.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#endif  // OPERA_SYNC

#if !defined(OPERA_SYNC)
static const char* kOAuth2Scopes[] = {
  GaiaConstants::kGoogleTalkOAuth2Scope
};
#endif  // !OPERA_SYNC

static const net::BackoffEntry::Policy kRequestAccessTokenBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  2000,

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.2, // 20%

  // Maximum amount of time we are willing to delay our request in ms.
  // TODO(pavely): crbug.com/246686 ProfileSyncService should retry
  // RequestAccessToken on connection state change after backoff
  1000 * 3600 * 4, // 4 hours.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

namespace invalidation {

#if defined(OPERA_SYNC)
namespace {

const char* kFakeXmppJidSuffix = "@fake.com";
const char* kOperaXmppServerMechanism = "X-OPERA-TOKEN";
opera::AccountService::HttpMethod kOperaXmppServerHttpMethod =
  opera::AccountService::HTTP_METHOD_POST;
const char* kOperaXmppAuthURL = "http://sync.opera.com/";

// Used to communicate with push server when the email is missing.
const char* kFakeXmppSuffix = "@syncpush.opera.com";

std::string GetOAuthHeader(
    base::WeakPtr<opera::AccountService> account_service) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  if (!account_service.get()) {
    // This can only happen during application shutdown.
    return std::string();
  }
  GURL request_url(kOperaXmppAuthURL);
  opera::AccountService::HttpMethod method(kOperaXmppServerHttpMethod);
  std::string realm(opera::SyncConfig::AuthServerURL().host());
  std::string header =
      account_service->GetSignedAuthHeader(request_url, method, realm);
  VLOG(4) << "XMPP: " << header;
  return header;
}

// Posts a task to UI thread to generate an oauth header (via GetOAuthHeader)
// and responds with the result to the given callback. This function will be
// called when an entity on IO thread wants to get a fresh token.
void TokenGenerator(base::WeakPtr<opera::AccountService> account_service,
                    const opera::TokenReceiver& receiver_callback) {
  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&GetOAuthHeader, account_service),
      receiver_callback);
}

}  // namespace
#endif  // OPERA_SYNC

TiclInvalidationService::TiclInvalidationService(
    const std::string& user_agent,
#if !defined(OPERA_SYNC)
    std::unique_ptr<IdentityProvider> identity_provider,
#else
    // TODO(mzajaczkowski): Need to pass the auth data to invalidation
    // service somehow.
    opera::SyncAccount* account,
    opera::oauth2::AuthService* auth_service,
#endif  // !OPERA_SYNC
#if !defined(OPERA_SYNC)
    std::unique_ptr<TiclSettingsProvider> settings_provider,
    gcm::GCMDriver* gcm_driver,
#endif  // !OPERA_SYNC
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
#if !defined(OPERA_SYNC)
    : OAuth2TokenService::Consumer("ticl_invalidation"),
#else
    :
#endif  // !OPERA_SYNC
      user_agent_(user_agent),
#if !defined(OPERA_SYNC)
      identity_provider_(std::move(identity_provider)),
      settings_provider_(std::move(settings_provider)),
#else
      account_(account),
      auth_service_(auth_service),
#endif  // !OPERA_SYNC
      invalidator_registrar_(new syncer::InvalidatorRegistrar()),
      request_access_token_backoff_(&kRequestAccessTokenBackoffPolicy),
#if !defined(OPERA_SYNC)
      network_channel_type_(GCM_NETWORK_CHANNEL),
#else
      network_channel_type_(PUSH_CLIENT_CHANNEL),  // Opera doesn't have GCM
#endif  // !OPERA_SYNC
#if !defined(OPERA_SYNC)
      gcm_driver_(gcm_driver),
#endif  // !OPERA_SYNC
      request_context_(request_context),
      logger_() {}

TiclInvalidationService::~TiclInvalidationService() {
  DCHECK(CalledOnValidThread());
  invalidator_registrar_->UpdateInvalidatorState(
#if defined(OPERA_SYNC)
      syncer::OperaInvalidatorState(syncer::INVALIDATOR_SHUTTING_DOWN));
#else
      syncer::INVALIDATOR_SHUTTING_DOWN);
#endif  // OPERA_SYNC
#if !defined(OPERA_SYNC)
  settings_provider_->RemoveObserver(this);
  identity_provider_->RemoveActiveAccountRefreshTokenObserver(this);
  identity_provider_->RemoveObserver(this);
#else
  if (!opera::SyncConfig::UsesOAuth2()) {
    DCHECK(account());
    account()->service()->RemoveObserver(this);
  } else {
    DCHECK(auth_service_);
    auth_service_->UnregisterClient(this);
    auth_service_->RemoveSessionStateObserver(this);
  }
#endif  // !OPERA_SYNC
  if (IsStarted()) {
    StopInvalidator();
  }
}

void TiclInvalidationService::Init(
    std::unique_ptr<syncer::InvalidationStateTracker>
        invalidation_state_tracker) {
  DCHECK(CalledOnValidThread());
#if defined(OPERA_SYNC)
  if (!opera::SyncConfig::UsesOAuth2()) {
    DCHECK(account());
    account()->service()->AddObserver(this);
  } else {
    DCHECK(auth_service_);
    auth_service_->RegisterClient(this, "ticl_invalidator");
    auth_service_->AddSessionStateObserver(this);
  }
#endif  // OPERA_SYNC
  invalidation_state_tracker_ = std::move(invalidation_state_tracker);

  if (invalidation_state_tracker_->GetInvalidatorClientId().empty()) {
    invalidation_state_tracker_->ClearAndSetNewClientId(
        GenerateInvalidatorClientId());
  }

  UpdateInvalidationNetworkChannel();
  if (IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  }
#if !defined(OPERA_SYNC)
  identity_provider_->AddObserver(this);
  identity_provider_->AddActiveAccountRefreshTokenObserver(this);
  settings_provider_->AddObserver(this);
#endif  // !OPERA_SYNC
}

void TiclInvalidationService::InitForTest(
#if defined(OPERA_SYNC)
  opera::SyncAccount* sync_account,
#endif  // OPERA_SYNC
    std::unique_ptr<syncer::InvalidationStateTracker>
        invalidation_state_tracker,
    syncer::Invalidator* invalidator) {
#if defined(OPERA_SYNC)
  // Here we initialize account_ to avoid obtaining it inside account() method
  // since that would require mocking Profile.
  account_ = sync_account;
#endif  // OPERA_SYNC
  // Here we perform the equivalent of Init() and StartInvalidator(), but with
  // some minor changes to account for the fact that we're injecting the
  // invalidator.
  invalidation_state_tracker_ = std::move(invalidation_state_tracker);
  invalidator_.reset(invalidator);

  invalidator_->RegisterHandler(this);
  CHECK(invalidator_->UpdateRegisteredIds(
      this, invalidator_registrar_->GetAllRegisteredIds()));
}

void TiclInvalidationService::RegisterInvalidationHandler(
    syncer::InvalidationHandler* handler) {
  DCHECK(CalledOnValidThread());
  DVLOG(2) << "Registering an invalidation handler";
  invalidator_registrar_->RegisterHandler(handler);
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
syncer::OperaInvalidatorState
    TiclInvalidationService::GetInvalidatorState() const {
#else
syncer::InvalidatorState TiclInvalidationService::GetInvalidatorState() const {
#endif  // OPERA_SYNC
  DCHECK(CalledOnValidThread());
  if (invalidator_) {
    DVLOG(2) << "GetInvalidatorState returning "
#if defined(OPERA_SYNC)
        << invalidator_->GetInvalidatorState().state;
#else
        << invalidator_->GetInvalidatorState();
#endif  // OPERA_SYNC
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
#if !defined(OPERA_SYNC)
  return identity_provider_.get();
#else
  NOTREACHED();
  return NULL;
#endif  // !OPERA_SYNC
}

void TiclInvalidationService::RequestDetailedStatus(
    base::Callback<void(const base::DictionaryValue&)> return_callback) const {
  if (IsStarted()) {
    return_callback.Run(network_channel_options_);
    invalidator_->RequestDetailedStatus(return_callback);
  }
}

void TiclInvalidationService::RequestAccessToken() {
#if !defined(OPERA_SYNC)
  // Only one active request at a time.
  if (access_token_request_ != NULL)
    return;
  request_access_token_retry_timer_.Stop();
  OAuth2TokenService::ScopeSet oauth2_scopes;
  for (size_t i = 0; i < arraysize(kOAuth2Scopes); i++)
    oauth2_scopes.insert(kOAuth2Scopes[i]);
  // Invalidate previous token, otherwise token service will return the same
  // token again.
  const std::string& account_id = identity_provider_->GetActiveAccountId();
  OAuth2TokenService* token_service = identity_provider_->GetTokenService();
  token_service->InvalidateAccessToken(account_id, oauth2_scopes,
                                       access_token_);
  access_token_.clear();
  access_token_request_ =
      token_service->StartRequest(account_id, oauth2_scopes, this);
#else
  DCHECK(opera::SyncConfig::UsesOAuth2());
  // TODO(mzajaczkowski): Original code does request cache invalidation for the
  // current token, and so do we.
  DCHECK(auth_service_);
  if (opera_access_token_) {
    auth_service_->SignalAccessTokenProblem(this, opera_access_token_);
    opera_access_token_ = nullptr;
  }
  auth_service_->RequestAccessToken(this,
                                    opera::oauth2::ScopeSet::FromEncoded(
                                        opera::SyncConfig::OAuth2ScopeXmpp()));
#endif  // !OPERA_SYNC
}

#if !defined(OPERA_SYNC)
void TiclInvalidationService::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(access_token_request_.get(), request);
  access_token_request_.reset();
  // Reset backoff time after successful response.
  request_access_token_backoff_.Reset();
  access_token_ = access_token;
  if (!IsStarted() && IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  } else {
    UpdateInvalidatorCredentials();
  }
}

void TiclInvalidationService::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(access_token_request_.get(), request);
  DCHECK_NE(error.state(), GoogleServiceAuthError::NONE);
  access_token_request_.reset();
  switch (error.state()) {
    case GoogleServiceAuthError::CONNECTION_FAILED:
    case GoogleServiceAuthError::SERVICE_UNAVAILABLE: {
      // Transient error. Retry after some time.
      request_access_token_backoff_.InformOfRequest(false);
      request_access_token_retry_timer_.Start(
            FROM_HERE,
            request_access_token_backoff_.GetTimeUntilRelease(),
            base::Bind(&TiclInvalidationService::RequestAccessToken,
                       base::Unretained(this)));
      break;
    }
    case GoogleServiceAuthError::SERVICE_ERROR:
    case GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS: {
      invalidator_registrar_->UpdateInvalidatorState(
          syncer::INVALIDATION_CREDENTIALS_REJECTED);
      break;
    }
    default: {
      // We have no way to notify the user of this.  Do nothing.
    }
  }
}

void TiclInvalidationService::OnRefreshTokenAvailable(
    const std::string& account_id) {
  if (!IsStarted() && IsReadyToStart())
    StartInvalidator(network_channel_type_);
}

void TiclInvalidationService::OnRefreshTokenRevoked(
    const std::string& account_id) {
  access_token_.clear();
  if (IsStarted())
    UpdateInvalidatorCredentials();
}

void TiclInvalidationService::OnActiveAccountLogout() {
  access_token_request_.reset();
  request_access_token_retry_timer_.Stop();

  if (gcm_invalidation_bridge_)
    gcm_invalidation_bridge_->Unregister();

  if (IsStarted()) {
    StopInvalidator();
  }

  // This service always expects to have a valid invalidation state. Thus, we
  // must generate a new client ID to replace the existing one. Setting a new
  // client ID also clears all other state.
  invalidation_state_tracker_->
      ClearAndSetNewClientId(GenerateInvalidatorClientId());
}

#else
void TiclInvalidationService::OnSessionStateChanged() {
  DCHECK(opera::SyncConfig::UsesOAuth2());
  DCHECK(auth_service_->GetSession());

  switch (auth_service_->GetSession()->GetState()) {
  case opera::oauth2::OA2SS_IN_PROGRESS:
    DCHECK(!opera_access_token_);
    if (!IsStarted() && IsReadyToStart())
      StartInvalidator(network_channel_type_);
    break;
  case opera::oauth2::OA2SS_INACTIVE :
    VLOG(4) << "Logged out";
    opera_access_token_ = nullptr;
    if (IsStarted()) {
      StopInvalidator();
    }
    break;
  case opera::oauth2::OA2SS_AUTH_ERROR:
    VLOG(4) << "Auth error";
    opera_access_token_ = nullptr;
    if (IsStarted()) {
      StopInvalidator();
    }
    break;
  default:
    // Empty.
    break;
  }
}

void TiclInvalidationService::OnAccessTokenRequestCompleted(
    opera::oauth2::AuthService* service,
    opera::oauth2::RequestAccessTokenResponse response) {
  DCHECK(opera::SyncConfig::UsesOAuth2());
  DCHECK_EQ(service, auth_service_);
  DCHECK_EQ(response.scopes_requested().encode(),
            opera::SyncConfig::OAuth2ScopeXmpp());

  switch (response.auth_error()) {
    case opera::oauth2::OAE_OK:
      DCHECK(response.access_token());
      DCHECK_EQ(response.access_token()->scopes().encode(),
                opera::SyncConfig::OAuth2ScopeXmpp());
      opera_access_token_ = response.access_token();
      if (!IsStarted() && IsReadyToStart()) {
        StartInvalidator(network_channel_type_);
      } else {
        UpdateInvalidatorCredentials();
      }
    default:
      invalidator_registrar_->UpdateInvalidatorState(
          syncer::OperaInvalidatorState(
              syncer::INVALIDATION_CREDENTIALS_REJECTED));
      break;
  }
}

void TiclInvalidationService::OnAccessTokenRequestDenied(
    opera::oauth2::AuthService* service,
    opera::oauth2::ScopeSet requested_scopes) {
  NOTREACHED();
}

void TiclInvalidationService::OnLoggedIn(opera::AccountService*,
                                         opera_sync::OperaAuthProblem problem) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  if (!IsStarted() && IsReadyToStart()) {
    StartInvalidator(network_channel_type_);
  } else {
    UpdateInvalidatorCredentials();
  }
}

void TiclInvalidationService::OnLoginError(opera::AccountService*,
    opera::account_util::AuthDataUpdaterError,
    opera::account_util::AuthOperaComError,
    const std::string&,
    opera_sync::OperaAuthProblem) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  VLOG(4) << "Login error";
  // This is a result of a token refresh request, compare
  // OnInvalidatorStateChange().
  invalidator_registrar_->UpdateInvalidatorState(
      syncer::OperaInvalidatorState(syncer::INVALIDATION_CREDENTIALS_REJECTED));
}

void TiclInvalidationService::OnLoggedOut(opera::AccountService* account,
    opera::account_util::LogoutReason logout_reason) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  VLOG(4) << "Logged out";
  if (IsStarted()) {
    StopInvalidator();
  }
}

void TiclInvalidationService::OnAuthDataExpired(opera::AccountService* account,
    opera_sync::OperaAuthProblem problem) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
}

opera::SyncAccount* TiclInvalidationService::account() const {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  DCHECK(account_);
  return account_;
}
#endif  // !OPERA_SYNC

#if !defined(OPERA_SYNC)
void TiclInvalidationService::OnUseGCMChannelChanged() {
  UpdateInvalidationNetworkChannel();
}
#endif  // !OPERA_SYNC

void TiclInvalidationService::OnInvalidatorStateChange(
#if defined(OPERA_SYNC)
    syncer::OperaInvalidatorState state) {
  state.problem.set_source(opera_sync::OperaAuthProblem::SOURCE_XMPP);
#else
    syncer::InvalidatorState state) {
#endif  // OPERA_SYNC
  VLOG(4) << "XMPP: " << syncer::InvalidatorStateToString(state);

#if defined(OPERA_SYNC)
  if (state.state == syncer::INVALIDATION_CREDENTIALS_REJECTED) {
#else
  if (state == syncer::INVALIDATION_CREDENTIALS_REJECTED) {
#endif  // OPERA_SYNC
    // This may be due to normal OAuth access token expiration.  If so, we must
    // fetch a new one using our refresh token.  Resetting the invalidator's
    // access token will not reset the invalidator's exponential backoff, so
    // it's safe to try to update the token every time we receive this signal.
    //
    // We won't be receiving any invalidations while the refresh is in progress,
    // we set our state to TRANSIENT_INVALIDATION_ERROR.  If the credentials
    // really are invalid, the refresh request should fail and
    // OnGetTokenFailure() will put us into a INVALIDATION_CREDENTIALS_REJECTED
    // state.
    invalidator_registrar_->UpdateInvalidatorState(
#if defined(OPERA_SYNC)
        syncer::OperaInvalidatorState(syncer::TRANSIENT_INVALIDATION_ERROR));
#else
        syncer::TRANSIENT_INVALIDATION_ERROR);
#endif  // OPERA_SYNC

#if !defined(OPERA_SYNC)
    RequestAccessToken();
#else
    // The invalidation server rejected the auth header, that might be a result
    // of:
    // * token expiration;
    // * auth data becoming invalid (i.e. password changed on the server);
    // In response to the AuthDataExpired() call we will get either the
    // OnLoggedIn() or OnLoginError() callback, that will update the
    // credentials or put us into the INVALIDATION_CREDENTIALS_REJECTED
    // state as required in the original comment above.
    if (opera::SyncConfig::UsesOAuth2()) {
#if defined(OPERA_DESKTOP)
      RequestAccessToken();
#else
      NOTREACHED();
#endif  // OPERA_DESKTOP
    } else {
      account_->AuthDataExpired(state.problem);
    }
#endif  // !OPERA_SYNC
  } else {
    // Once a network error occurs after invalidations had been enabled
    // refresh credentials so that a new header (new nonce) is used when
    // network is back. Otherwise auth will reject the credentials due to
    // timestamp/once being wrong and we'll end up trying to renew a valid
    // token - which will also fail causing user logout. Credentials are
    // refreshed by the call to OnLoggedIn().
#if defined(OPERA_SYNC)
    if (!opera::SyncConfig::UsesOAuth2()) {
      if (state.state == syncer::TRANSIENT_INVALIDATION_ERROR &&
          invalidator_registrar_->GetInvalidatorState().state ==
              syncer::INVALIDATIONS_ENABLED) {
        VLOG(4) << "Trigger logged in event";
        OnLoggedIn(account()->service().get(), state.problem);
      }
    }
#endif  // OPERA_SYNC
    invalidator_registrar_->UpdateInvalidatorState(state);
  }
#if defined(OPERA_SYNC)
  logger_.OnStateChange(state.state);
#else
  logger_.OnStateChange(state);
#endif  // OPERA_SYNC
}

void TiclInvalidationService::OnIncomingInvalidation(
    const syncer::ObjectIdInvalidationMap& invalidation_map) {
  invalidator_registrar_->DispatchInvalidationsToHandlers(invalidation_map);

  logger_.OnInvalidation(invalidation_map);
}

std::string TiclInvalidationService::GetOwnerName() const { return "TICL"; }

bool TiclInvalidationService::IsReadyToStart() {
#if !defined(OPERA_SYNC)
  if (identity_provider_->GetActiveAccountId().empty()) {
    DVLOG(2) << "Not starting TiclInvalidationService: User is not signed in.";
    return false;
  }

  OAuth2TokenService* token_service = identity_provider_->GetTokenService();
  if (!token_service) {
    DVLOG(2)
        << "Not starting TiclInvalidationService: "
        << "OAuth2TokenService unavailable.";
    return false;
  }

  if (!token_service->RefreshTokenIsAvailable(
          identity_provider_->GetActiveAccountId())) {
    DVLOG(2)
        << "Not starting TiclInvalidationServce: Waiting for refresh token.";
    return false;
  }
#else
  if (opera::SyncConfig::UsesOAuth2()) {
    DCHECK(auth_service_);
    return auth_service_->GetSession() &&
           auth_service_->GetSession()->IsInProgress();
  } else {
    if (!account()->IsLoggedIn()) {
      DVLOG(2)
          << "Not starting TiclInvalidationService: User is not signed in.";
      return false;
    }
  }
#endif  // !OPERA_SYNC
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

#if defined(OPERA_SYNC)
  if (opera::SyncConfig::UsesOAuth2()) {
    DCHECK_EQ(network_channel, PUSH_CLIENT_CHANNEL);
    if (!opera_access_token_) {
      DVLOG(1)
        << "TiclInvalidationService: "
        << "Deferring start until we have an access token.";
      RequestAccessToken();
      return;
    }
  }  else {
    DCHECK(account()->IsLoggedIn());
  }
#else
  // Request access token for PushClientChannel. GCMNetworkChannel will request
  // access token before sending message to server.
  if (network_channel == PUSH_CLIENT_CHANNEL && access_token_.empty()) {
    DVLOG(1)
        << "TiclInvalidationService: "
        << "Deferring start until we have an access token.";
    RequestAccessToken();
    return;
  }

#endif  // OPERA_SYNC

  syncer::NetworkChannelCreator network_channel_creator;

  switch (network_channel) {
    case PUSH_CLIENT_CHANNEL: {
      notifier::NotifierOptions options =
          ParseNotifierOptions(*base::CommandLine::ForCurrentProcess());

      options.request_context_getter = request_context_;

#if defined(OPERA_SYNC)
      if (!opera::SyncConfig::UsesOAuth2()) {
        options.auth_mechanism = kOperaXmppServerMechanism;
        // The invalidator that uses token_generator will die before this, so
        // it's safe to give it a function bound with a raw pointer to our
        // member.
        options.token_generator =
            base::Bind(&TokenGenerator, account()->service());
      } else {
        options.auth_mechanism = "X-OAUTH2";
      }
#else
      options.auth_mechanism = "X-OAUTH2";
#endif  // OPERA_SYNC

      network_channel_options_.SetString("Options.HostPort",
                                         options.xmpp_host_port.ToString());
      network_channel_options_.SetString("Options.AuthMechanism",
                                         options.auth_mechanism);
      DCHECK_EQ(notifier::NOTIFICATION_SERVER, options.notification_method);
      network_channel_creator =
          syncer::NonBlockingInvalidator::MakePushClientChannelCreator(options);
      break;
    }
    case GCM_NETWORK_CHANNEL: {
#if !defined(OPERA_SYNC)
      gcm_invalidation_bridge_.reset(new GCMInvalidationBridge(
          gcm_driver_, identity_provider_.get()));
      network_channel_creator =
          syncer::NonBlockingInvalidator::MakeGCMNetworkChannelCreator(
              request_context_, gcm_invalidation_bridge_->CreateDelegate());
      break;
#endif  // !OPERA_SYNC
    }
    default: {
      NOTREACHED();
      return;
    }
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Invalidations.NetworkChannel", network_channel, NETWORK_CHANNELS_COUNT);
  invalidator_.reset(new syncer::NonBlockingInvalidator(
          network_channel_creator,
          invalidation_state_tracker_->GetInvalidatorClientId(),
          invalidation_state_tracker_->GetSavedInvalidations(),
          invalidation_state_tracker_->GetBootstrapData(),
          invalidation_state_tracker_.get(),
          user_agent_,
          request_context_));

  UpdateInvalidatorCredentials();

  invalidator_->RegisterHandler(this);
  CHECK(invalidator_->UpdateRegisteredIds(
      this, invalidator_registrar_->GetAllRegisteredIds()));
}

void TiclInvalidationService::UpdateInvalidationNetworkChannel() {
  const InvalidationNetworkChannel network_channel_type =
#if !defined(OPERA_SYNC)
      settings_provider_->UseGCMChannel() ? GCM_NETWORK_CHANNEL
                                          : PUSH_CLIENT_CHANNEL;
#else
      PUSH_CLIENT_CHANNEL;
#endif  // !OPERA_SYNC
  if (network_channel_type_ == network_channel_type)
    return;
  network_channel_type_ = network_channel_type;
  if (IsStarted()) {
    StopInvalidator();
    StartInvalidator(network_channel_type_);
  }
}

void TiclInvalidationService::UpdateInvalidatorCredentials() {
#if !defined(OPERA_SYNC)
  std::string email = identity_provider_->GetActiveAccountId();
  DVLOG(2) << "UpdateCredentials: " << email;
  invalidator_->UpdateCredentials(email, access_token_);
  DCHECK(!email.empty()) << "Expected user to be signed in.";
#else
  if (opera::SyncConfig::UsesOAuth2()) {
    DCHECK(auth_service_);
    DCHECK(auth_service_->GetSession());
    DCHECK(auth_service_->GetSession()->IsInProgress());
    std::string email = auth_service_->GetSession()->GetUsername();
    if (email.find('@') == std::string::npos) {
      email += kFakeXmppJidSuffix;
    }
    DVLOG(2) << "UpdateCredentials: " << email;
    DCHECK(opera_access_token_);
    invalidator_->UpdateCredentials(email, opera_access_token_->token());
    DCHECK(!email.empty()) << "Expected user to be signed in.";
  } else {
    DCHECK(account());
    std::string email = account()->login_data().user_email;
    // XMPP expects email. In case account lacks email, construct a fake one
    // from the username.
    if (email.empty())
      email = account()->login_data().user_name + kFakeXmppSuffix;

    VLOG(2) << "UpdateCredentials: " << email;
    invalidator_->UpdateCredentials(email,
                                    GetOAuthHeader(account()->service()));
    DCHECK(!email.empty()) << "Expected user to be signed in.";
  }
#endif  // !OPERA_SYNC
}

void TiclInvalidationService::StopInvalidator() {
  DCHECK(invalidator_);
#if !defined(OPERA_SYNC)
  gcm_invalidation_bridge_.reset();
#endif  // !OPERA_SYNC
  invalidator_->UnregisterHandler(this);
  invalidator_.reset();
}

}  // namespace invalidation
