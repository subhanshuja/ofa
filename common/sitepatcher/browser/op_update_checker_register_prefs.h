// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_REGISTER_PREFS_H_
#define COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_REGISTER_PREFS_H_

class PrefRegistrySimple;

namespace opera {
namespace SitePatchingUpdateComponent {

void RegisterPrefs(PrefRegistrySimple* prefs);

}  // end namespace SitePatchingUpdateComponent
}  // end namespace opera


#endif  // COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_REGISTER_PREFS_H_
