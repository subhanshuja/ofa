// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_MIGRATION_OBML_MIGRATOR_H_
#define CHILL_BROWSER_MIGRATION_OBML_MIGRATOR_H_

#include <vector>

#include "base/callback.h"

#include "chill/browser/migration/obml_font_info.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace mobile {
class SavedPage;
}  // namespace mobile

namespace opera {

class OBMLConverterWebUIController;
class OBMLConverterWebUIControllerFactory;

// Performes migration of saved pages which has been saved in OBML format.
// OBML documents are converted into MHTML files which replaces the backing file
// of the associated SavedPage instance.
//
// Conversion is done through a Javascript running inside a WebUI hosted by a
// WebContents which is decoupled from the actual UI. The script populates the
// DOM tree and sends a message when the document has been generated, after
// which the page state is serialized to the resulting MHTML file.
//
// Currently, the JS converter doesn't handle OBML INPUT nodes, and they will be
// left out of the generated document.
class OBMLMigrator {
 public:
  explicit OBMLMigrator(std::vector<OBMLFontInfo> font_info);
  virtual ~OBMLMigrator();

  void Init();

  // Used as a callback to Migrate, and is invoked when the SavedPage has been
  // migrated from OBML to MHTML, or when the conversion failed.
  typedef base::Callback<void(bool /* success */)> MigrationCompleteCallback;

  void Migrate(mobile::SavedPage* saved_page,
               MigrationCompleteCallback callback);

 protected:
  // Override for testing
  virtual base::FilePath GetTargetDirectory();

 private:
  void CreateAndRegisterWebUIFactory();

  void NavigateToWebUI();

  void ReplaceSavedPage(mobile::SavedPage* saved_page,
                        const MigrationCompleteCallback& completion_callback,
                        bool success,
                        const base::FilePath& generated_path);

  // content::WebUIControllerFactory owns the instance
  OBMLConverterWebUIControllerFactory* web_ui_controller_factory_;

  // Sticks to the WebUI, which means that if the associated WebContents
  // navigates away or is destroyed from the web ui this pointer will be borked.
  OBMLConverterWebUIController* migration_controller_;

  std::unique_ptr<content::WebContents> web_contents_;
  std::vector<OBMLFontInfo> font_info_;

  DISALLOW_COPY_AND_ASSIGN(OBMLMigrator);
};

}  // namespace opera

#endif  // CHILL_BROWSER_MIGRATION_OBML_MIGRATOR_H_
