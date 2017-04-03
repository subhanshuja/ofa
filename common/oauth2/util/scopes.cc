// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/util/scopes.h"

namespace opera {
namespace oauth2 {
namespace scope {

// Resolves to all scopes currently assigned to the client
const char kAll[] = "ALL";

// General sync scope
const char kSync[] = "https://sync.opera.com";

// Content broker for thumbnails
const char kContentBroker[] = "https://cdnbroker.opera.com";

// Push notifications
const char kPushNotifications[] = "https://push.opera.com";

// News api, for article comments etc
const char kNewsApi[] = "https://news.opera-api.com/";
}  // namespace scope
}  // namespace oauth2
}  // namespace opera
