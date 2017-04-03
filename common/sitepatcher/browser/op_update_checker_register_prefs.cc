// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_update_checker_register_prefs.h"

#include "components/prefs/pref_registry_simple.h"
#include "desktop/common/opera_pref_names.h"

namespace opera {
namespace SitePatchingUpdateComponent {

void RegisterPrefs(PrefRegistrySimple* prefs) {
    prefs->RegisterInt64Pref(opera::prefs::kBrowserJSVersion, -1);
    prefs->RegisterInt64Pref(opera::prefs::kSitePrefsVersion, -1);
}

}  // end namespace SitePatchingUpdateComponent
}  // end namespace opera
