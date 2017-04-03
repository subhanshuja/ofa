// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_download_manager_delegate.h
// @final-synchronized

#ifndef CHILL_BROWSER_DOWNLOAD_OPERA_DOWNLOAD_MANAGER_DELEGATE_H_
#define CHILL_BROWSER_DOWNLOAD_OPERA_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>

#include "base/android/jni_android.h"
#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"

#include "chill/browser/download/download_storage.h"
#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"

namespace content {
class DownloadManager;
}

namespace opera {

class OperaDownloadManagerDelegate
    : public content::DownloadManagerDelegate,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  explicit OperaDownloadManagerDelegate(bool store_downloads);
  virtual ~OperaDownloadManagerDelegate();

  void SetDownloadManager(content::DownloadManager* manager);

  void Shutdown() override;
  virtual void GetNextId(const content::DownloadIdCallback& callback);
  virtual bool DetermineDownloadTarget(
      content::DownloadItem* download,
      const content::DownloadTargetCallback& callback) override;
  content::WebContents* GetWebContentsForResumableDownload() override;

  void ReadDownloadMetadataFromDisk(DownloadReadFromDiskObserver* observer);

  void MigrateDownload(std::string full_path, std::string url,
                       std::string mimetype, int downloaded, int total,
                       bool completed);

  virtual void OnShowDownloadPathDialog(unsigned int download_id,
      OpRunnable callback, const base::string16& suggested_filename);

 private:
  friend class base::RefCountedThreadSafe<OperaDownloadManagerDelegate>;

  void ShowDownloadPathDialogCallback(
      const content::DownloadTargetCallback& callback,
      const OpArguments& args);

  content::DownloadManager* download_manager_;
  base::WeakPtrFactory<OperaDownloadManagerDelegate> weak_ptr_factory_;
  std::unique_ptr<content::WebContents> resume_downloads_web_contents_;

  std::unique_ptr<DownloadStorage> storage_;

  DISALLOW_COPY_AND_ASSIGN(OperaDownloadManagerDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_OPERA_DOWNLOAD_MANAGER_DELEGATE_H_
