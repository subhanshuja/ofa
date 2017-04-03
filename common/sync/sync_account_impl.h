// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_ACCOUNT_IMPL_H_
#define COMMON_SYNC_SYNC_ACCOUNT_IMPL_H_

#include <memory>
#include <string>

#include "base/callback.h"

#include "common/sync/sync_account.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_error_data.h"

namespace net {
class URLRequestContextGetter;
}

namespace opera {

class TimeSkewResolver;

/**
 * Represents an AccountService-based user account used by Sync.
 */
class SyncAccountImpl : public SyncAccount {
 public:
  /**
   * @param service_factory an AccountService factory
   * @param time_skew_resolver obtains time skew from the server
   * @param login_data_store a (presumably persistent) store of log-in data
   * @param auth_server_url the URL of the authorization server
   * @param request_context the request context to use for communication with
   *    the authorization server
   * @param auth_data_updater_factory a factory of AccountAuthDataFetcher
   *    objects that can be used to refresh expired authorization token data
   * @param stop_syncing_callback stops the Sync mechanism
   */
  SyncAccountImpl(const ServiceFactory& service_factory,
                  std::unique_ptr<TimeSkewResolver> time_skew_resolver,
                  std::unique_ptr<SyncLoginDataStore> login_data_store,
                  net::URLRequestContextGetter* request_context_getter,
                  const AuthDataUpdaterFactory& auth_data_updater_factory,
                  const base::Closure& stop_syncing_callback,
                  const opera::ExternalCredentials& external_credentials);
  ~SyncAccountImpl() override;

  // SyncAccount overrides:
  base::WeakPtr<AccountService> service() const override;
  void SetLoginData(const SyncLoginData& login_data) override;
  const SyncLoginData& login_data() const override;
  void SetLoginErrorData(const SyncLoginErrorData& login_error_data) override;
  const SyncLoginErrorData& login_error_data() const override;
  bool HasAuthData() const override;
  bool HasValidAuthData() const override;
  void SyncEnabled() override;

 private:
  class ServiceDelegate;
  friend class ServiceDelegate;
  class CredentialsBasedServiceDelegate;
  friend class CredentialsBasedServiceDelegate;

  void LoggedOut();

  SyncLoginData login_data_;
  std::unique_ptr<SyncLoginDataStore> login_data_store_;

  SyncLoginErrorData login_error_data_;

  bool auth_data_valid_;

  std::unique_ptr<AccountServiceDelegate> account_service_delegate_;
  std::unique_ptr<AccountService> account_service_;

  DISALLOW_COPY_AND_ASSIGN(SyncAccountImpl);
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_ACCOUNT_IMPL_H_
