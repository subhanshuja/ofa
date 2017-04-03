// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_IMPL_H_
#define COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_IMPL_H_

#include "common/oauth2/migration/oauth1_migrator.h"

#include <string>
#include <memory>

#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#include "common/oauth2/device_name/device_name_service.h"
#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/migration/oauth1_session_data.h"
#include "common/oauth2/network/network_request_manager.h"
#include "common/oauth2/session/persistent_session.h"

namespace opera {

class SyncLoginDataStore;

namespace oauth2 {

class DiagnosticService;
class MigrationTokenRequest;
class MigrationTokenResponse;
class NetworkRequestManager;
class OAuth1RenewTokenRequest;
class OAuth1RenewTokenResponse;

class OAuth1MigratorImpl : public OAuth1Migrator,
                           public DiagnosticSupplier,
                           public NetworkRequestManager::Consumer {
 public:
  OAuth1MigratorImpl(
      DiagnosticService* diagnostic_service,
      NetworkRequestManager* request_manager,
      DeviceNameService* device_name_service,
      std::unique_ptr<SyncLoginDataStore> oauth1_login_data_store,
      const GURL oauth1_base_url,
      const GURL oauth2_base_url,
      const std::string& oauth1_auth_service_id,
      const std::string& oauth1_client_id,
      const std::string& oauth1_client_secret,
      const std::string& oauth2_client_id);
  ~OAuth1MigratorImpl() override;

  // OAuth1Migrator implementation.
  bool IsMigrationPossible() override;
  void PrepareMigration(PersistentSession* session) override;
  void StartMigration() override;
  void EnsureOAuth1SessionIsCleared() override;
  MigrationResult GetMigrationResult() override;

  // DiagnosticSupplier implementation.
  std::string GetDiagnosticName() const override;
  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override;

  // NetworkRequetManager::Consumer implementation.
  void OnNetworkRequestFinished(scoped_refptr<NetworkRequest> request,
                                NetworkResponseStatus response_status) override;

  std::string GetConsumerName() const override;

 private:
  struct LastFetchTokenInfo {
    LastFetchTokenInfo();
    std::unique_ptr<base::DictionaryValue> AsDiagnosticDict() const;

    int oauth1_auth_error;
    std::string oauth1_error_message;
    OperaAuthError oauth2_auth_error;
    base::Time time;
    NetworkResponseStatus response_status;
  };

  void StartOAuth1TokenRenewalRequest();
  void StartOAuth2MigrationTokenRequest();

  void ProcessOAuth1TokenRenewalResponse(
      scoped_refptr<OAuth1RenewTokenRequest> oauth1_token_renewal_request,
      NetworkResponseStatus response_status);
  void ProcessOAuth2MigrationTokenResponse(
      scoped_refptr<MigrationTokenRequest> migration_token_request,
      NetworkResponseStatus response_status);

  std::unique_ptr<OAuth1SessionData> GetOAuth1SessionData() const;

  MigrationResult OAuth1ErrorToMigrationResult(int oauth1_error) const;
  MigrationResult OAuth2ErrorToMigrationResult(
      OperaAuthError oauth2_error) const;

  void SetMigrationResult(MigrationResult result);

  void NotifyDiagnosticStateMaybeChanged();

  NetworkRequestManager* request_manager_;
  DiagnosticService* diagnostic_service_;
  DeviceNameService* device_name_service_;
  std::unique_ptr<SyncLoginDataStore> oauth1_login_data_store_;
  std::unique_ptr<OAuth1SessionData> oauth1_session_;
  PersistentSession* oauth2_session_;

  GURL oauth1_base_url_;
  GURL oauth2_base_url_;
  std::string oauth1_auth_service_id_;
  std::string oauth1_client_id_;
  std::string oauth1_client_secret_;
  std::string oauth2_client_id_;

  std::unique_ptr<OAuth1TokenFetchStatus> last_renew_oauth1_token_info_;
  std::unique_ptr<OAuth2TokenFetchStatus> last_fetch_refresh_token_info_;

  scoped_refptr<MigrationTokenRequest> migration_token_request_;
  scoped_refptr<OAuth1RenewTokenRequest> oauth1_renew_token_request_;

  OperaAuthError last_get_refresh_token_error_;
  MigrationResult migration_result_;

  base::WeakPtrFactory<OAuth1MigratorImpl> weak_ptr_factory_;
};

}  // namespace oauth2

}  // namespace opera

#endif  // COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_IMPL_H_
