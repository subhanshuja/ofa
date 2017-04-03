// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_AUTH_SERVICE_IMPL_H_
#define COMMON_OAUTH2_AUTH_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "common/oauth2/auth_service.h"
#include "common/oauth2/device_name/device_name_service.h"
#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/migration/oauth1_migrator.h"
#include "common/oauth2/network/network_request_manager.h"

class GURL;

namespace opera {
namespace oauth2 {

class AccessTokenRequest;
class AuthServiceClient;
class SessionStateObserver;
class InitParams;
class RequestThrottler;
class TokenCache;

class AuthServiceImpl : public AuthService,
                        public DiagnosticSupplier,
                        public NetworkRequestManager::Consumer {
 public:
  AuthServiceImpl();
  ~AuthServiceImpl() override;

  void OnTokenCacheLoaded();

  void Initialize(std::unique_ptr<InitParams> params);
  void SessionStateChangedCallback();

  // KeyedService implementation.
  void Shutdown() override;

  // DiagnosticSupplier implementation.
  std::string GetDiagnosticName() const override;
  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override;

  // NetworkRequestManager::Consumer implementation.
  std::string GetConsumerName() const override;
  void OnNetworkRequestFinished(scoped_refptr<NetworkRequest> request,
                                NetworkResponseStatus response_status) override;

  void StartSessionWithAuthToken(const std::string& username,
                                 const std::string& auth_token) override;

  void EndSession(SessionEndReason reason) override;

  void RequestAccessToken(AuthServiceClient* client, ScopeSet scopes) override;

  void SignalAccessTokenProblem(AuthServiceClient* client,
                                scoped_refptr<const AuthToken> token) override;

  void RegisterClient(AuthServiceClient* client,
                      const std::string& client_name) override;
  void UnregisterClient(AuthServiceClient* client) override;

  void AddSessionStateObserver(SessionStateObserver* observer) override;
  void RemoveSessionStateObserver(SessionStateObserver* observer) override;

  const PersistentSession* GetSession() const override;

 private:
  enum AuthErrorReason { AER_ACCESS_TOKEN, AER_REFRESH_TOKEN };

  void RevokeRefreshToken(const std::string& refresh_token);

  void DoRequestAccessToken(AuthServiceClient* client, ScopeSet scopes);

  void ProcessRefreshTokenRequest(scoped_refptr<AccessTokenRequest> request,
                                  NetworkResponseStatus response_status);
  void ProcessAccessTokenRequest(scoped_refptr<AccessTokenRequest> request,
                                 AuthServiceClient* requesting_client,
                                 NetworkResponseStatus response_status);

  std::string GetClientName(AuthServiceClient* client) const;
  bool IsClientKnown(AuthServiceClient* client) const;

  void NotifyDiagnosticStateMaybeChanged();

  void ProcessRequestTokenCallbacks();

  void EnterAuthError(AuthErrorReason reason, OperaAuthError auth_error);

  std::unique_ptr<NetworkRequestManager> request_manager_;
  std::unique_ptr<PersistentSession> session_;

  scoped_refptr<AccessTokenRequest> refresh_token_for_sso_request_;
  std::unique_ptr<OAuth2TokenFetchStatus> last_fetch_refresh_token_info_;
  std::unique_ptr<OAuth2TokenFetchStatus> last_fetch_access_token_info_;
  std::map<scoped_refptr<AccessTokenRequest>, AuthServiceClient*>
      running_access_token_requests_;

  std::set<std::string> pending_requests_;
  std::unique_ptr<InitParams> params_;
  TokenCache* token_cache_;
  std::unique_ptr<RequestThrottler> request_throttler_;
  base::ObserverList<SessionStateObserver> session_state_observers_;
  std::map<AuthServiceClient*, std::string> client_registrations_;
  DiagnosticService* diagnostic_service_;
  SessionEndReason last_session_end_reason_;

  std::unique_ptr<OAuth1Migrator> oauth1_migrator_;
  DeviceNameService* device_name_service_;

  bool token_cache_loaded_;
  std::vector<base::Callback<void()>> request_token_callbacks_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  base::WeakPtrFactory<AuthServiceImpl> weak_ptr_factory_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_AUTH_SERVICE_IMPL_H_
