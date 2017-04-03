// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_download_manager_delegate.cc
// @final-synchronized

#include "chill/browser/download/opera_download_manager_delegate.h"

#include <string>

#include "base/android/path_utils.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"

#include "chill/browser/download/download_target_arguments.h"
#include "chill/browser/opera_browser_context.h"

using base::FilePath;
using content::BrowserThread;

namespace opera {

OperaDownloadManagerDelegate::OperaDownloadManagerDelegate(bool store_downloads)
    : download_manager_(NULL),
      weak_ptr_factory_(this) {
  if (store_downloads) {
    storage_.reset(new DownloadStorage());
  }
}

OperaDownloadManagerDelegate::~OperaDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(NULL);
    download_manager_ = NULL;
  }
}

void OperaDownloadManagerDelegate::SetDownloadManager(
    content::DownloadManager* download_manager) {

  download_manager_ = download_manager;
  if (storage_.get() != NULL) {
    storage_->SetDownloadManager(download_manager);
  }
}

void OperaDownloadManagerDelegate::ReadDownloadMetadataFromDisk(
    DownloadReadFromDiskObserver* observer) {
  if (storage_.get() != NULL) {
    storage_->Load(observer);
  }
}

void OperaDownloadManagerDelegate::MigrateDownload(std::string full_path,
                                                   std::string url,
                                                   std::string mimetype,
                                                   int downloaded, int total,
                                                   bool completed) {
  if (storage_.get() != NULL) {
    storage_->MigrateDownload(full_path, url, mimetype, downloaded, total,
                              completed);
  }
}

void OperaDownloadManagerDelegate::OnShowDownloadPathDialog(
    unsigned int download_id,
    OpRunnable callback,
    const base::string16& suggested_path) {
  // Implemented in Java
  NOTREACHED();
}

void OperaDownloadManagerDelegate::Shutdown() {
  // Revoke any pending callbacks. download_manager_ et. al. are no longer safe
  // to access after this point.
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = NULL;
}

void OperaDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  if (storage_.get() != NULL) {
    storage_->GetNextId(callback);
  } else {
    static uint32_t download_id = 0;
    callback.Run(++download_id);
  }
}

bool OperaDownloadManagerDelegate::DetermineDownloadTarget(
    content::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!download->GetTargetFilePath().empty()) {
    callback.Run(download->GetTargetFilePath(),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetTargetFilePath());
    return true;
  }

  if (!download->GetForcedFilePath().empty()) {
    callback.Run(download->GetForcedFilePath(),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 download->GetForcedFilePath());
    return true;
  }

  base::string16 suggested_name = net::GetSuggestedFilename(download->GetURL(),
      download->GetContentDisposition(), std::string(),
      download->GetSuggestedFilename(), download->GetMimeType(), "download");

  OpRunnable opcallback =
      base::Bind(&OperaDownloadManagerDelegate::ShowDownloadPathDialogCallback,
                 base::Unretained(this),
                 callback);

  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&OperaDownloadManagerDelegate::OnShowDownloadPathDialog,
                 weak_ptr_factory_.GetWeakPtr(),
                 download->GetId(),
                 opcallback,
                 suggested_name));

  return true;
}

void OperaDownloadManagerDelegate::ShowDownloadPathDialogCallback(
    const content::DownloadTargetCallback& callback,
    const OpArguments& args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const DownloadTargetArguments& target_args =
    static_cast<const DownloadTargetArguments&>(args);
  FilePath target_path(target_args.targetPath);
  callback.Run(target_path,
               content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
               target_path);
}

content::WebContents*
OperaDownloadManagerDelegate::GetWebContentsForResumableDownload() {
  if (resume_downloads_web_contents_.get() == NULL) {
    resume_downloads_web_contents_.reset(
        content::WebContents::Create(
            content::WebContents::CreateParams(
                OperaBrowserContext::GetDefaultBrowserContext())));
  }
  return resume_downloads_web_contents_.get();
}

}  // namespace opera
