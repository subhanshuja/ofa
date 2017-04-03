// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/auth_data.h"

namespace mobile {

AuthData::AuthData() {
}

AuthData::~AuthData() {
}

void AuthData::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AuthData::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AuthData::LoggedIn(const std::string& login_name,
                        const std::string& user_name,
                        const std::string& password) {
  FOR_EACH_OBSERVER(
      Observer, observers_, AuthDataLoggedIn(login_name, user_name, password));
}

void AuthData::GotToken(const std::string& token, const std::string& secret) {
  FOR_EACH_OBSERVER(Observer, observers_, AuthDataGotToken(token, secret));
}

void AuthData::Login(const std::string& token,
                     const std::string& secret,
                     const std::string& display_name) {
  FOR_EACH_OBSERVER(Observer, observers_,
                    AuthDataLogin(token, secret, display_name));
}

void AuthData::GotError(Error error, const std::string& message) {
  FOR_EACH_OBSERVER(Observer, observers_, AuthDataError(error, message));
}

}  // namespace mobile
