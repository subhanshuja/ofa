// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/migration/obml_migrator.h"

#include <string>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/path_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "url/gurl.h"

#include "mobile/common/favorites/savedpage.h"

#include "chill/browser/migration/obml_converter_web_ui_controller.h"
#include "chill/browser/migration/obml_converter_web_ui_controller_factory.h"
#include "chill/browser/migration/obml_font_info.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/common/opera_url_constants.h"

namespace {

GURL GetWebUIUrl() {
  return GURL(std::string(opera::kOperaUIScheme) + std::string("://") +
              std::string(opera::kOperaOBMLMigrationHost));
}

const char* FindSubstring(const char* haystack,
                          size_t haystack_length,
                          const char* needle,
                          size_t needle_length) {
  DCHECK_GE(haystack_length, needle_length);

  for (size_t i = 0; i + needle_length < haystack_length; i++) {
    if (strncmp(&haystack[i], needle, needle_length) == 0)
      return &haystack[i];
  }

  return nullptr;
}

// There's no way to spoof the location in a running WebContents, and the web ui
// url will be written as the Content-Location header in the generated MHTML
// file, which is subsequently read at upstart.
void RewriteLocationHeaderOnFileThread(mobile::SavedPage* saved_page) {
  static const char kContentLocationHead[] = "Content-Location: ";
  static const char kEOL[] = "\r\n";

  base::FilePath file_path(saved_page->file());

  base::FilePath tmp_file_path;
  if (!base::CreateTemporaryFile(&tmp_file_path)) {
    LOG(ERROR) << "Couldn't rewrite Content-Location";
    return;
  }

  base::File tmp_file(tmp_file_path,
                      base::File::FLAG_OPEN_TRUNCATED | base::File::FLAG_WRITE);
  DCHECK(tmp_file.IsValid());

  base::MemoryMappedFile saved_file;
  saved_file.Initialize(file_path);
  DCHECK(saved_file.IsValid());

  const char* saved_file_data =
      reinterpret_cast<const char*>(saved_file.data());

  std::string original_location_header = std::string(kContentLocationHead) +
                                         GetWebUIUrl().spec() +
                                         std::string(kEOL);

  std::string new_location_header = std::string(kContentLocationHead) +
                                    saved_page->url().spec() +
                                    std::string(kEOL);

  // The only interesting Content-Location header is that of the main document,
  // which comes first in the MHTML file.
  const char* header_ptr = FindSubstring(saved_file_data, saved_file.length(),
                                         original_location_header.c_str(),
                                         original_location_header.size());

  if (!header_ptr) {
    LOG(ERROR) << "Couldn't rewrite Content-Location header: not found";
    return;
  }

  tmp_file.WriteAtCurrentPos(saved_file_data, header_ptr - saved_file_data);
  tmp_file.WriteAtCurrentPos(new_location_header.c_str(),
                             new_location_header.size());

  size_t remaining_length = (saved_file_data + saved_file.length()) -
                            (header_ptr + original_location_header.size());
  tmp_file.WriteAtCurrentPos(header_ptr + original_location_header.size(),
                             remaining_length);

  tmp_file.Close();

  base::Move(tmp_file_path, file_path);
}

}  // namespace

namespace opera {

OBMLMigrator::OBMLMigrator(std::vector<OBMLFontInfo> font_info_vector)
    : font_info_(font_info_vector) {
}

OBMLMigrator::~OBMLMigrator() {
  // As there's no proper way to unregister a controller factory, we'll simply
  // disable and leak it.
  web_ui_controller_factory_->Disable();
}

void OBMLMigrator::Init() {
  content::WebContents::CreateParams create_params(
      OperaBrowserContext::GetPrivateBrowserContext());
  create_params.initially_hidden = true;
  create_params.opener_suppressed = true;
  web_contents_.reset(content::WebContents::Create(create_params));

  CreateAndRegisterWebUIFactory();
  NavigateToWebUI();

  content::WebUI* web_ui = web_contents_->GetWebUI();
  DCHECK(web_ui);
  migration_controller_ =
      static_cast<OBMLConverterWebUIController*>(web_ui->GetController());
}

base::FilePath OBMLMigrator::GetTargetDirectory() {
  static const char* kSavedPagesDirectoryName = "saved_pages";

  base::FilePath data_dir;
  bool got_data_dir = PathService::Get(base::DIR_ANDROID_APP_DATA, &data_dir);
  DCHECK(got_data_dir);
  return data_dir.Append(std::string(kSavedPagesDirectoryName));
}

void OBMLMigrator::Migrate(mobile::SavedPage* saved_page,
                           MigrationCompleteCallback callback) {
  DCHECK(migration_controller_);
  migration_controller_->ConvertSavedPage(
      base::FilePath(saved_page->file()),
      base::Bind(&OBMLMigrator::ReplaceSavedPage, base::Unretained(this),
                 saved_page, callback));
}

void OBMLMigrator::CreateAndRegisterWebUIFactory() {
  base::FilePath saved_pages_directory = GetTargetDirectory();

  web_ui_controller_factory_ = new OBMLConverterWebUIControllerFactory(
      GetWebUIUrl(), web_contents_.get(), &font_info_, saved_pages_directory);
  content::WebUIControllerFactory::RegisterFactory(web_ui_controller_factory_);
}

void OBMLMigrator::NavigateToWebUI() {
  content::NavigationController::LoadURLParams load_params(GetWebUIUrl());
  web_contents_->GetController().LoadURLWithParams(load_params);
}

void OBMLMigrator::ReplaceSavedPage(
    mobile::SavedPage* saved_page,
    const MigrationCompleteCallback& completion_callback,
    bool success,
    const base::FilePath& generated_path) {
  if (!success) {
    completion_callback.Run(success);
    return;
  }

  base::FilePath old_file_path(saved_page->file());
  saved_page->SetFile(generated_path.value());

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(base::IgnoreResult(&base::DeleteFile), old_file_path, false));

  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&RewriteLocationHeaderOnFileThread, saved_page),
      base::Bind(completion_callback, success));
}

}  // namespace opera
