// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_H_
#define CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_H_

#include <string>
#include <memory>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "common/auth/auth_token_consumer.h"
#include "common/oauth2/session/session_state_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace net {
class URLRequestContextGetter;
}

namespace opera {
class AuthTokenFetcher;
class OAuth2AccountServiceObserver;

namespace oauth2 {
class AuthService;
}

// This is a collation of error events as encountered from OAuth2AccountService
// and its dependents.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: com.opera.android.oauth2
// GENERATED_JAVA_CLASS_NAME_OVERRIDE: AccountError
// GENERATED_JAVA_PREFIX_TO_STRIP: ERR_
enum OAuth2AccountError {
  ERR_NONE,
  // There was an error when connecting to any of the OAuth2 services
  ERR_NETWORK_ERROR,
  // The supplied user credentials were invalid
  ERR_INVALID_CREDENTIALS,
  // An error occurred either within a OAuth2 service, or there was an issue
  // with its response
  ERR_SERVICE_ERROR,
};

class OAuth2AccountService : public KeyedService,
                             public AuthTokenConsumer,
                             public oauth2::SessionStateObserver {
 public:
  struct SessionStartResult {
    OAuth2AccountError error;

    // Only set if error == ERR_NONE
    base::Optional<std::string> username;
  };

  using SessionStartCallback = base::Callback<void(const SessionStartResult&)>;

  OAuth2AccountService(oauth2::AuthService* auth_service,
                       net::URLRequestContextGetter* getter);
  ~OAuth2AccountService() override;

  // TODO(ingemara): Replace base::Callback with base::OnceCallback once
  // available
  void StartSessionWithCredentials(const std::string& username,
                                   const std::string& password,
                                   const SessionStartCallback& callback);
  void StartSessionWithAuthToken(const std::string& username,
                                 const std::string& token,
                                 const SessionStartCallback& callback);
  void EndSession();

  // Returns whether there is an active session and it is logged in
  bool IsLoggedIn() const;

  void AddObserver(OAuth2AccountServiceObserver* observer);
  void RemoveObserver(OAuth2AccountServiceObserver* observer);

  // AuthTokenConsumer implementation
  void OnAuthSuccess(const std::string& auth_token,
                     std::unique_ptr<UserData> user_data) override;
  void OnAuthError(AuthServiceError error,
                   const std::string& error_msg) override;

  // SessionStateObserver implementation
  void OnSessionStateChanged() override;

 private:
  void DeleteAuthTokenFetcherSoon();
  void CallPendingStartCallback(const SessionStartResult& result);

  void NotifyLoggedIn();
  void NotifyLoggedOut();

  // Keeps track of session state change
  bool was_logged_in_;
  SessionStartCallback pending_session_start_callback_;

  oauth2::AuthService* const auth_service_;
  net::URLRequestContextGetter* const request_context_getter_;
  std::unique_ptr<AuthTokenFetcher> token_fetcher_;  // Only set during login

  base::ObserverList<OAuth2AccountServiceObserver> service_observers_;
  base::WeakPtrFactory<AuthTokenConsumer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OAuth2AccountService);
};

}  // namespace opera

#endif  // CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_H_
