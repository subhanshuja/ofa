// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_SITE_PATCH_COMPONENT_INSTALLER_H_
#define COMMON_SITEPATCHER_BROWSER_SITE_PATCH_COMPONENT_INSTALLER_H_

#include <string>

#include "base/callback_forward.h"
#include "common/sitepatcher/browser/op_site_patcher.h"
#include "components/prefs/pref_registry_simple.h"
#include "desktop/common/autoupdate/update_component.h"

namespace opera {

class SitePatchComponentInstaller : public FileComponentInstaller {
 public:
  using LoadCallback = base::Callback<bool(bool)>;
  SitePatchComponentInstaller(const std::string& component_name,
                              const std::string& version_pref_path,
                              const LoadCallback& load_callback);

  Status Install(const std::string& version,
                 const std::string& final_checkum,
                 size_t final_size,
                 const AUManifest& manifest,
                 const base::FilePath& path,
                 const std::string& suggested_name,
                 bool is_fallback) override;

  void UpdateVersion(const std::string& new_v);
  bool SupportsBinaryPatching() override;

 private:
  ~SitePatchComponentInstaller() override;
  std::string version_pref_path_;
  std::string component_name_;
  LoadCallback load_callback_;
};

}  // namespace opera

#endif  // COMMON_SITEPATCHER_BROWSER_SITE_PATCH_COMPONENT_INSTALLER_H_
