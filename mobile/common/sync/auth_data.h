// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_AUTH_DATA_H_
#define MOBILE_COMMON_SYNC_AUTH_DATA_H_

#include <string>

#include "base/observer_list.h"
#include "mobile/common/sync/sync_manager.h"

namespace mobile {

class AuthData {
 public:
  enum Error {
    PASSWORD_INVALID,
    EMAIL_MISSING,
    EMAIL_INVALID,
    EMAIL_INUSE,
    BAD_CREDENTIALS,
    NETWORK_ERROR,
    TIMEDOUT,
  };
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void AuthDataLoggedIn(const std::string& login_name,
                                  const std::string& user_name,
                                  const std::string& password) = 0;
    virtual void AuthDataGotToken(const std::string& token,
                                  const std::string& secret) = 0;
    virtual void AuthDataLogin(const std::string& token,
                               const std::string& secret,
                               const std::string& display_name) = 0;
    virtual void AuthDataError(Error error, const std::string& message) = 0;
   protected:
    Observer() {}
  };
  virtual ~AuthData();

  virtual void Setup(const std::string& application,
                     const std::string& application_key,
                     const std::string& client_key,
                     const std::string& client_secret) = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  virtual void RequestLogin(const std::string& login_name) = 0;
  virtual void RequestToken(const std::string& login_name,
                            const std::string& password) = 0;

 protected:
  AuthData();

  void LoggedIn(const std::string& login_name,
                const std::string& user_name,
                const std::string& password);
  void GotToken(const std::string& token, const std::string& secret);
  void Login(const std::string& token,
             const std::string& secret,
             const std::string& display_name);
  void GotError(Error error, const std::string& message);

 private:
  base::ObserverList<Observer> observers_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_AUTH_DATA_H_
