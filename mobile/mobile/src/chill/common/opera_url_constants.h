// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_COMMON_OPERA_URL_CONSTANTS_H_
#define CHILL_COMMON_OPERA_URL_CONSTANTS_H_

#include "base/macros.h"

namespace opera {

extern const char kOperaUIScheme[];
extern const char kOperaAboutScheme[];

// When adding a new webui requiring V8 bindings (i.e. using the 'chrome'
// object), it needs to be added to the return statement in
// OperaWebUIControllerFactory::UseWebUIBindingsForURL(..), as well as the
// kOperaHostUrls array (if it is to show up as a suggestion in the url bar).
extern const char kOperaAboutHost[];
extern const char kOperaCobrandSettingsHost[];
extern const char kOperaDebugZeroRatingHost[];
extern const char kOperaFlagsHost[];
extern const char kOperaLogcatHost[];
extern const char kOperaMemoryHost[];
extern const char kOperaMemoryRedirectHost[];
extern const char kOperaNetInternalsHost[];
extern const char kOperaOBMLMigrationHost[];
extern const char kOperaVersionHost[];

extern const char kOperaMemoryRedirectHostURL[];

struct OperaHostUrl {
  const char* const url;
  const int title;
};

extern const OperaHostUrl kOperaHostUrls[];
extern const size_t kNumberOfOperaHostUrls;

}  // namespace opera

#endif  // CHILL_COMMON_OPERA_URL_CONSTANTS_H_
