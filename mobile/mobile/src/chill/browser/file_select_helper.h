// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/file_select_helper.h
// @last-synchronized f2847a0440e8bd0067d48e2e39e988329d719dad

#ifndef CHILL_BROWSER_FILE_SELECT_HELPER_H_
#define CHILL_BROWSER_FILE_SELECT_HELPER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/file_chooser_params.h"
#include "net/base/directory_lister.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
struct FileChooserFileInfo;
class RenderViewHost;
class WebContents;
}

namespace ui {
struct SelectedFileInfo;
}

namespace opera {

// This class handles file-selection requests coming from WebUI elements
// (via the extensions::ExtensionHost class). It implements both the
// initialisation and listener functions for file-selection dialogs.
//
// Since FileSelectHelper has-a NotificationRegistrar, it needs to live on and
// be destroyed on the UI thread. References to FileSelectHelper may be passed
// on to other threads.
class FileSelectHelper : public base::RefCountedThreadSafe<
                             FileSelectHelper,
                             content::BrowserThread::DeleteOnUIThread>,
                         public ui::SelectFileDialog::Listener,
                         public content::WebContentsObserver,
                         public content::NotificationObserver {
 public:
  // Show the file chooser dialog.
  static void RunFileChooser(content::RenderFrameHost* render_frame_host,
                             const content::FileChooserParams& params);

  // Enumerates all the files in directory.
  static void EnumerateDirectory(content::WebContents* tab,
                                 int request_id,
                                 const base::FilePath& path);

 private:
  friend class base::RefCountedThreadSafe<FileSelectHelper>;
  friend class base::DeleteHelper<FileSelectHelper>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;

  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, IsAcceptTypeValid);
  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, ZipPackage);
  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, GetSanitizedFileName);
  FileSelectHelper();
  ~FileSelectHelper() override;

  // Utility class which can listen for directory lister events and relay
  // them to the main object with the correct tracking id.
  class DirectoryListerDispatchDelegate
      : public net::DirectoryLister::DirectoryListerDelegate {
   public:
    DirectoryListerDispatchDelegate(FileSelectHelper* parent, int id)
        : parent_(parent),
          id_(id) {}
    ~DirectoryListerDispatchDelegate() override {}
    void OnListFile(
        const net::DirectoryLister::DirectoryListerData& data) override;
    void OnListDone(int error) override;

   private:
    // This FileSelectHelper owns this object.
    FileSelectHelper* parent_;
    int id_;

    DISALLOW_COPY_AND_ASSIGN(DirectoryListerDispatchDelegate);
  };

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileChooserParams> params);
  void GetFileTypesOnFileThread(
      std::unique_ptr<content::FileChooserParams> params);
  void GetSanitizedFilenameOnUIThread(
      std::unique_ptr<content::FileChooserParams> params);
  void RunFileChooserOnUIThread(
      const base::FilePath& default_path,
      std::unique_ptr<content::FileChooserParams> params);

  // Cleans up and releases this instance. This must be called after the last
  // callback is received from the file chooser dialog.
  void RunFileChooserEnd();

  // SelectFileDialog::Listener overrides.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file,
                                 int index,
                                 void* params) override;
  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override;
  void MultiFilesSelectedWithExtraInfo(
      const std::vector<ui::SelectedFileInfo>& files,
      void* params) override;
  void FileSelectionCanceled(void* params) override;

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // content::WebContentsObserver overrides.
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;

  void EnumerateDirectory(int request_id,
                          content::RenderViewHost* render_view_host,
                          const base::FilePath& path);

  // Kicks off a new directory enumeration.
  void StartNewEnumeration(const base::FilePath& path,
                           int request_id,
                           content::RenderViewHost* render_view_host);

  // Callbacks from directory enumeration.
  virtual void OnListFile(
      int id,
      const net::DirectoryLister::DirectoryListerData& data);
  virtual void OnListDone(int id, int error);

  // Cleans up and releases this instance. This must be called after the last
  // callback is received from the enumeration code.
  void EnumerateDirectoryEnd();

  // Utility method that passes |files| to the render view host, and ends the
  // file chooser.
  void NotifyRenderViewHostAndEnd(
      const std::vector<ui::SelectedFileInfo>& files);

  // Sends the result to the render process, and call |RunFileChooserEnd|.
  void NotifyRenderViewHostAndEndAfterConversion(
      const std::vector<content::FileChooserFileInfo>& list);

  // Cleans up when the RenderViewHost of our WebContents changes.
  void CleanUpOnRenderViewHostChange();

  // Helper method to get allowed extensions for select file dialog from
  // the specified accept types as defined in the spec:
  //   http://whatwg.org/html/number-state.html#attr-input-accept
  // |accept_types| contains only valid lowercased MIME types or file extensions
  // beginning with a period (.).
  static std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
  GetFileTypesFromAcceptType(const std::vector<base::string16>& accept_types);

  // Check the accept type is valid. It is expected to be all lower case with
  // no whitespace.
  static bool IsAcceptTypeValid(const std::string& accept_type);

  // Get a sanitized filename suitable for use as a default filename. The
  // suggested filename coming over the IPC may contain invalid characters or
  // may result in a filename that's reserved on the current platform.
  //
  // If |suggested_path| is empty, the return value is also empty.
  //
  // If |suggested_path| is non-empty, but can't be safely converted to UTF-8,
  // or is entirely lost during the sanitization process (e.g. because it
  // consists entirely of invalid characters), it's replaced with a default
  // filename.
  //
  // Otherwise, returns |suggested_path| with any invalid characters will be
  // replaced with a suitable replacement character.
  static base::FilePath GetSanitizedFileName(
      const base::FilePath& suggested_path);

  // The RenderFrameHost and WebContents for the page showing a file dialog
  // (may only be one such dialog).
  content::RenderFrameHost* render_frame_host_;
  content::WebContents* web_contents_;

  // Dialog box used for choosing files to upload from file form fields.
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> select_file_types_;

  // The type of file dialog last shown.
  ui::SelectFileDialog::Type dialog_type_;

  // The mode of file dialog last shown.
  content::FileChooserParams::Mode dialog_mode_;

  // Maintain a list of active directory enumerations.  These could come from
  // the file select dialog or from drag-and-drop of directories, so there could
  // be more than one going on at a time.
  struct ActiveDirectoryEnumeration;
  std::map<int, ActiveDirectoryEnumeration*> directory_enumerations_;

  // Registrar for notifications regarding our RenderViewHost.
  content::NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(FileSelectHelper);
};

}  // namespace opera

#endif  // CHILL_BROWSER_FILE_SELECT_HELPER_H_
