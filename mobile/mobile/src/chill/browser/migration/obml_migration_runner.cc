// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/migration/obml_migration_runner.h"

#include "base/files/file_path.h"
#include "content/public/browser/browser_thread.h"

#include "mobile/common/favorites/favorites.h"
#include "mobile/common/favorites/folder.h"

#include "chill/browser/migration/obml_migration_callback_arguments.h"

namespace opera {

/* static */
void OBMLMigrationRunner::StartMigration(
    std::vector<OBMLFontInfo> font_info_vector,
    base::Callback<void(const OpArguments&)> callback) {
  mobile::Favorites* favorites = mobile::Favorites::instance();
  DCHECK(favorites->IsLoaded());

  scoped_refptr<OBMLMigrationRunner> migration_runner(
      new OBMLMigrationRunner(font_info_vector, callback));

  mobile::Folder* saved_pages_folder = favorites->saved_pages();
  const std::vector<int64_t>& saved_pages = saved_pages_folder->Children();
  for (auto it = saved_pages.begin(); it != saved_pages.end(); it++) {
    mobile::SavedPage* saved_page =
        static_cast<mobile::SavedPage*>(favorites->GetFavorite(*it));

    if (base::FilePath(saved_page->file()).Extension() == "") {
      migration_runner->Migrate(saved_page);
    }
  }
}

OBMLMigrationRunner::OBMLMigrationRunner(
    std::vector<OBMLFontInfo> font_info_vector,
    base::Callback<void(const OpArguments&)> callback)
    : migrator_(new OBMLMigrator(font_info_vector)),
      callback_(callback),
      successful_(true) {
  migrator_->Init();
}

OBMLMigrationRunner::~OBMLMigrationRunner() {
  OBMLMigrationCallbackArguments args;
  args.success = successful_;
  callback_.Run(args);

  // Allow the migrator to finish up before removing it
  content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                     migrator_);
}

void OBMLMigrationRunner::Migrate(mobile::SavedPage* saved_page) {
  migrator_->Migrate(saved_page,
                     base::Bind(&OBMLMigrationRunner::FinishMigration,
                                make_scoped_refptr(this), saved_page));
}

void OBMLMigrationRunner::FinishMigration(mobile::SavedPage* saved_page,
                                          bool success) {
  successful_ &= success;
}

}  // namespace opera
