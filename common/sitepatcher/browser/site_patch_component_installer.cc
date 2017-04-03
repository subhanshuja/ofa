// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/site_patch_component_installer.h"

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

#include "desktop/common/autoupdate/autoupdate_manager.h"
#include "desktop/common/browser_process_impl.h"

using content::BrowserThread;

namespace opera {

SitePatchComponentInstaller::SitePatchComponentInstaller(
    const std::string& component_name,
    const std::string& version_pref_path,
    const LoadCallback& load_callback)
    : version_pref_path_(version_pref_path),
      component_name_(component_name),
      load_callback_(load_callback) {
}

SitePatchComponentInstaller::~SitePatchComponentInstaller() {
}

void SitePatchComponentInstaller::UpdateVersion(const std::string& new_v) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  int64_t new_number = -1;
  base::StringToInt64(new_v, &new_number);
  g_browser_process->local_state()->SetInt64(version_pref_path_, new_number);
}

bool SitePatchComponentInstaller::SupportsBinaryPatching() {
  return false;
}

FileComponentInstaller::Status SitePatchComponentInstaller::Install(
    const std::string& version,
    const std::string& final_checkum,
    size_t final_size,
    const AUManifest& manifest,
    const base::FilePath& path,
    const std::string& suggested_name,
    bool is_fallback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  const bool load_new = true;

  // Oh boy. This is a hack for corner case when the OpSitePatcher dies on
  // the UI thread and this gets called on FILE thread. It's possible since
  // the OpSitePatcher in desktop evolved from a gobal outliving everything
  // (same as it still is for tv and mobile) into a BCKS and this is ref
  // counted and travelling between threads. If the OpSitePatcher is no more
  // it obviously can't be used and it is below. This is why if this has only
  // one ref it bails out as it means it got kept alive by posting it to
  // the FILE thread and the OpSitePatcher died on the UI thread in
  // the meanwhile. If it didn't this would have at least 3 references since
  // it's stored in the component updater and in the OpUpdateCheckerImpl
  // (which is an object inside the OpSitePatcher and when it dies it removes
  // this from the component updater which means its death removes 2
  // references).
  if (this->HasOneRef())
    return FileComponentInstaller::kGenericError;

  base::FilePath sp_path;
  PathService::Get(chrome::DIR_USER_DATA, &sp_path);
  sp_path = sp_path.AppendASCII(suggested_name);
  if (load_new)
    sp_path = sp_path.AddExtension(FILE_PATH_LITERAL(".new"));
  if (!base::CopyFile(path, sp_path))
    return FileComponentInstaller::kGenericError;

  if (!load_callback_.is_null() && load_callback_.Run(load_new)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&SitePatchComponentInstaller::UpdateVersion, this, version));

    return FileComponentInstaller::kOk;
  } else {
    return FileComponentInstaller::kGenericError;
  }
}

}  // end namespace opera
