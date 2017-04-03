// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_update_checker.h"

#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

#include "common/sitepatcher/browser/op_site_patcher.h"
#include "common/sitepatcher/browser/site_patch_component_installer.h"
#include "desktop/common/autoupdate/autoupdate_manager.h"
#include "desktop/common/autoupdate/update_component.h"
#include "desktop/common/browser_process_impl.h"
#include "desktop/common/opera_pref_names.h"
#include "desktop/common/paths/opera_paths.h"

using content::BrowserThread;

namespace {

const char* kBrowserJSComponentName = "sitepatcher_opautoupdatechecker";
const char* kSitePrefsComponentName =
    "sitepatcher_opautoupdatechecker_siteprefs";
const char* kWebBtBlacklistComponentName = "sitepatcher_opwebbtblacklist";
const char* kBrowserJSVersionPref = opera::prefs::kBrowserJSVersion;
const char* kSitePrefsVersionPref = opera::prefs::kSitePrefsVersion;
const char* kWebBtBlacklistVersionPref = opera::prefs::kWebBtBlacklistVersion;

}  // namespace

class OpUpdateCheckerImpl {
 public:
  OpUpdateCheckerImpl(OpSitePatcher* parent) : is_started_(false) {
    struct InstallerInfo {
      const char* name;
      const char* version_pref;
      base::Callback<bool(bool)> load_function;
    };
    const InstallerInfo kInstallersInfo[] = {
        {kBrowserJSComponentName, kBrowserJSVersionPref,
         base::Bind(&OpSitePatcher::LoadBrowserJS, parent)},
        {kSitePrefsComponentName, kSitePrefsVersionPref,
         base::Bind(&OpSitePatcher::LoadSitePrefs, parent)},
    };
    for (size_t iter = 0; iter < arraysize(kInstallersInfo); ++iter) {
      InstallerDetails details;
      details.component_name = kInstallersInfo[iter].name;
      details.version_pref_path = kInstallersInfo[iter].version_pref;
      details.installer = new opera::SitePatchComponentInstaller(
          details.component_name, details.version_pref_path,
          kInstallersInfo[iter].load_function);
      details_.push_back(details);
    }
  }

  ~OpUpdateCheckerImpl() {}

  void Start(bool force_check);
  void Stop();

 private:
  struct InstallerDetails {
    scoped_refptr<opera::SitePatchComponentInstaller> installer;
    std::string component_name;
    std::string version_pref_path;
  };

  std::vector<InstallerDetails> details_;
  bool is_started_;
};

OpUpdateChecker::OpUpdateChecker(
    scoped_refptr<net::URLRequestContextGetter> request_context,
    const OpSitepatcherConfig& config,
    OpSitePatcher* parent)
    : impl_(new OpUpdateCheckerImpl(parent)) {}

OpUpdateChecker::~OpUpdateChecker() {
  impl_->Stop();
  delete impl_;
}

void OpUpdateChecker::SetPrefs(PersistentPrefStore* prefs) {}

void OpUpdateChecker::Start(bool force_check) {
  impl_->Start(force_check);
}

namespace {

bool CompareVersions(const std::string& old_v, const std::string& new_v) {
  int64_t new_number = -1;
  int64_t old_number = -1;
  base::StringToInt64(new_v, &new_number);
  base::StringToInt64(old_v, &old_number);
  return old_number < new_number;
}

}  // namespace

void OpUpdateCheckerImpl::Start(bool force_check) {
  if (is_started_)
    return;
  opera::AutoUpdateService* auto_updater =
      opera::AutoUpdateService::FromBrowserProcess(g_browser_process);
  // Sometimes in tests there is no auto updater.
  if (auto_updater == NULL)
    return;

  for (const auto detail : details_) {
    opera::FileUpdateComponent fuc;
    fuc.info.name = detail.component_name;
    fuc.info.version = base::IntToString(
        g_browser_process->local_state()->GetInt64(detail.version_pref_path));
    fuc.info.version_comparator = base::Bind(CompareVersions);
    fuc.installer = detail.installer;

    auto_updater->RegisterFileUpdateComponent(fuc);
  }
  is_started_ = true;
}

void OpUpdateCheckerImpl::Stop() {
  for (auto detail : details_) {
    opera::AutoUpdateService* auto_updater =
        opera::AutoUpdateService::FromBrowserProcess(g_browser_process);
    // Sometimes in tests there is no auto updater.
    if (auto_updater != NULL) {
      auto_updater->UnRegisterFileUpdateComponent(detail.component_name);
    }
  }
  is_started_ = false;
}
