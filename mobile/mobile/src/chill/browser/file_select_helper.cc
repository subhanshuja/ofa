// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/file_select_helper.cc
// @last-synchronized f2847a0440e8bd0067d48e2e39e988329d719dad

// Changes:
// - Wrap in namespace opera.
// - Remove all ifdef OS_* and FULL_SAFE_BROWSING.
// - Remove unused chrome specific functionality: Profile and Temporary files.
// - Use literal strings instead of translations.
// - Replace ChromeSelectFilePolicy with OperaSelectFilePolicy.
// - New functionality to get the top window.

#include "chill/browser/file_select_helper.h"

#include <stddef.h>

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/worker_pool.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/file_chooser_file_info.h"
#include "content/public/common/file_chooser_params.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "ui/android/view_android.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/selected_file_info.h"

#include "chill/browser/opera_select_file_policy.h"

using content::BrowserThread;
using content::FileChooserParams;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::WebContents;

namespace opera {

namespace {

// There is only one file-selection happening at any given time,
// so we allocate an enumeration ID for that purpose.  All IDs from
// the renderer must start at 0 and increase.
const int kFileSelectEnumerationId = -1;

// Converts a list of FilePaths to a list of ui::SelectedFileInfo.
std::vector<ui::SelectedFileInfo> FilePathListToSelectedFileInfoList(
    const std::vector<base::FilePath>& paths) {
  std::vector<ui::SelectedFileInfo> selected_files;
  for (size_t i = 0; i < paths.size(); ++i) {
    selected_files.push_back(
        ui::SelectedFileInfo(paths[i], paths[i]));
  }
  return selected_files;
}

}  // namespace

struct FileSelectHelper::ActiveDirectoryEnumeration {
  ActiveDirectoryEnumeration() : rvh_(NULL) {}

  std::unique_ptr<DirectoryListerDispatchDelegate> delegate_;
  std::unique_ptr<net::DirectoryLister> lister_;
  RenderViewHost* rvh_;
  std::vector<base::FilePath> results_;
};

FileSelectHelper::FileSelectHelper()
    : render_frame_host_(nullptr),
      web_contents_(nullptr),
      select_file_dialog_(),
      select_file_types_(),
      dialog_type_(ui::SelectFileDialog::SELECT_OPEN_FILE),
      dialog_mode_(FileChooserParams::Open) {}

FileSelectHelper::~FileSelectHelper() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();

  // Stop any pending directory enumeration, prevent a callback, and free
  // allocated memory.
  std::map<int, ActiveDirectoryEnumeration*>::iterator iter;
  for (iter = directory_enumerations_.begin();
       iter != directory_enumerations_.end();
       ++iter) {
    iter->second->lister_.reset();
    delete iter->second;
  }
}

void FileSelectHelper::DirectoryListerDispatchDelegate::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  parent_->OnListFile(id_, data);
}

void FileSelectHelper::DirectoryListerDispatchDelegate::OnListDone(int error) {
  parent_->OnListDone(id_, error);
}

void FileSelectHelper::FileSelected(const base::FilePath& path,
                                    int index, void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void FileSelectHelper::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

  const base::FilePath& path = file.local_path;
  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    StartNewEnumeration(path, kFileSelectEnumerationId,
                        render_frame_host_->GetRenderViewHost());
    return;
  }

  std::vector<ui::SelectedFileInfo> files;
  files.push_back(file);

  NotifyRenderViewHostAndEnd(files);
}

void FileSelectHelper::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(files);

  MultiFilesSelectedWithExtraInfo(selected_files, params);
}

void FileSelectHelper::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {
  NotifyRenderViewHostAndEnd(files);
}

void FileSelectHelper::FileSelectionCanceled(void* params) {
  NotifyRenderViewHostAndEnd(std::vector<ui::SelectedFileInfo>());
}

void FileSelectHelper::StartNewEnumeration(const base::FilePath& path,
                                           int request_id,
                                           RenderViewHost* render_view_host) {
  std::unique_ptr<ActiveDirectoryEnumeration> entry(
      new ActiveDirectoryEnumeration);
  entry->rvh_ = render_view_host;
  entry->delegate_.reset(new DirectoryListerDispatchDelegate(this, request_id));
  entry->lister_.reset(new net::DirectoryLister(
      path, net::DirectoryLister::NO_SORT_RECURSIVE, entry->delegate_.get()));
  if (!entry->lister_->Start(base::WorkerPool::GetTaskRunner(true).get())) {
    if (request_id == kFileSelectEnumerationId)
      FileSelectionCanceled(NULL);
    else
      render_view_host->DirectoryEnumerationFinished(request_id,
                                                     entry->results_);
  } else {
    directory_enumerations_[request_id] = entry.release();
  }
}

void FileSelectHelper::OnListFile(
    int id,
    const net::DirectoryLister::DirectoryListerData& data) {
  ActiveDirectoryEnumeration* entry = directory_enumerations_[id];

  // Directory upload only cares about files.
  if (data.info.IsDirectory())
    return;

  entry->results_.push_back(data.path);
}

void FileSelectHelper::OnListDone(int id, int error) {
  // This entry needs to be cleaned up when this function is done.
  std::unique_ptr<ActiveDirectoryEnumeration> entry(
      directory_enumerations_[id]);
  directory_enumerations_.erase(id);
  if (!entry->rvh_)
    return;
  if (error) {
    FileSelectionCanceled(NULL);
    return;
  }

  std::vector<ui::SelectedFileInfo> selected_files =
      FilePathListToSelectedFileInfoList(entry->results_);

  if (id == kFileSelectEnumerationId) {
    NotifyRenderViewHostAndEnd(selected_files);
  } else {
    entry->rvh_->DirectoryEnumerationFinished(id, entry->results_);
    EnumerateDirectoryEnd();
  }
}

void FileSelectHelper::NotifyRenderViewHostAndEnd(
    const std::vector<ui::SelectedFileInfo>& files) {
  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

  std::vector<content::FileChooserFileInfo> chooser_files;
  for (const auto& file : files) {
    content::FileChooserFileInfo chooser_file;
    chooser_file.file_path = file.local_path;
    chooser_file.display_name = file.display_name;
    chooser_files.push_back(chooser_file);
  }

  NotifyRenderViewHostAndEndAfterConversion(chooser_files);
}

void FileSelectHelper::NotifyRenderViewHostAndEndAfterConversion(
    const std::vector<content::FileChooserFileInfo>& list) {
  if (render_frame_host_)
    render_frame_host_->FilesSelectedInChooser(list, dialog_mode_);

  // No members should be accessed from here on.
  RunFileChooserEnd();
}

void FileSelectHelper::CleanUpOnRenderViewHostChange() {
  // There is no longer any reason to keep this instance around.
  Release();
}

std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
FileSelectHelper::GetFileTypesFromAcceptType(
    const std::vector<base::string16>& accept_types) {
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> base_file_type(
      new ui::SelectFileDialog::FileTypeInfo());
  if (accept_types.empty())
    return base_file_type;

  // Create FileTypeInfo and pre-allocate for the first extension list.
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_type(
      new ui::SelectFileDialog::FileTypeInfo(*base_file_type));
  file_type->include_all_files = true;
  file_type->extensions.resize(1);
  std::vector<base::FilePath::StringType>* extensions =
      &file_type->extensions.back();

  // TODO(christiank): The file type description strings should be translated
  // after deciding on a translation system to use in Opera, see WAM-102. For
  // now English (GB) string literals are used.
  // The strings to translate are: "Image Files", "Audio Files", "Video Files",
  // "Customised Files" and default download filename ("Download").

  // Find the corresponding extensions.
  int valid_type_count = 0;
  base::string16 description;
  for (size_t i = 0; i < accept_types.size(); ++i) {
    std::string ascii_type = base::UTF16ToASCII(accept_types[i]);
    if (!IsAcceptTypeValid(ascii_type))
      continue;

    size_t old_extension_size = extensions->size();
    if (ascii_type[0] == '.') {
      // If the type starts with a period it is assumed to be a file extension
      // so we just have to add it to the list.
      base::FilePath::StringType ext(ascii_type.begin(), ascii_type.end());
      extensions->push_back(ext.substr(1));
    } else {
      if (ascii_type == "image/*")
        description = base::ASCIIToUTF16("Image Files");
      else if (ascii_type == "audio/*")
        description = base::ASCIIToUTF16("Audio Files");
      else if (ascii_type == "video/*")
        description = base::ASCIIToUTF16("Video Files");

      net::GetExtensionsForMimeType(ascii_type, extensions);
    }

    if (extensions->size() > old_extension_size)
      valid_type_count++;
  }

  // If no valid extension is added, bail out.
  if (valid_type_count == 0)
    return base_file_type;

  // Use a generic description "Custom Files" if either of the following is
  // true:
  // 1) There're multiple types specified, like "audio/*,video/*"
  // 2) There're multiple extensions for a MIME type without parameter, like
  //    "ehtml,shtml,htm,html" for "text/html". On Windows, the select file
  //    dialog uses the first extension in the list to form the description,
  //    like "EHTML Files". This is not what we want.
  if (valid_type_count > 1 ||
      (valid_type_count == 1 && description.empty() && extensions->size() > 1))
    description = base::ASCIIToUTF16("Customised Files");

  if (!description.empty()) {
    file_type->extension_description_overrides.push_back(description);
  }

  return file_type;
}

// static
void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const FileChooserParams& params) {
  // FileSelectHelper will keep itself alive until it sends the result message.
  scoped_refptr<FileSelectHelper> file_select_helper(
      new FileSelectHelper());
  file_select_helper->RunFileChooser(
      render_frame_host,
      base::WrapUnique(new content::FileChooserParams(params)));
}

// static
void FileSelectHelper::EnumerateDirectory(content::WebContents* tab,
                                          int request_id,
                                          const base::FilePath& path) {
  // FileSelectHelper will keep itself alive until it sends the result message.
  scoped_refptr<FileSelectHelper> file_select_helper(
      new FileSelectHelper());
  file_select_helper->EnumerateDirectory(
      request_id, tab->GetRenderViewHost(), path);
}

void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    std::unique_ptr<FileChooserParams> params) {
  DCHECK(!render_frame_host_);
  DCHECK(!web_contents_);
  DCHECK(params->default_file_name.empty() ||
         params->mode == FileChooserParams::Save)
      << "The default_file_name parameter should only be specified for Save "
         "file choosers";
  DCHECK(params->default_file_name == params->default_file_name.BaseName())
      << "The default_file_name parameter should not contain path separators";

  render_frame_host_ = render_frame_host;
  web_contents_ = WebContents::FromRenderFrameHost(render_frame_host);
  notification_registrar_.RemoveAll();
  content::WebContentsObserver::Observe(web_contents_);
  notification_registrar_.Add(
      this, content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED,
      content::Source<RenderWidgetHost>(
          render_frame_host_->GetRenderViewHost()->GetWidget()));

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&FileSelectHelper::GetFileTypesOnFileThread, this,
                 base::Passed(&params)));

  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the file dialog.
  // At that point, we must call RunFileChooserEnd().
  AddRef();
}

void FileSelectHelper::GetFileTypesOnFileThread(
    std::unique_ptr<FileChooserParams> params) {
  select_file_types_ = GetFileTypesFromAcceptType(params->accept_types);
  select_file_types_->allowed_paths =
      params->need_local_path ? ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH
                              : ui::SelectFileDialog::FileTypeInfo::ANY_PATH;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&FileSelectHelper::GetSanitizedFilenameOnUIThread, this,
                 base::Passed(&params)));
}

void FileSelectHelper::GetSanitizedFilenameOnUIThread(
    std::unique_ptr<FileChooserParams> params) {
  base::FilePath default_file_path =
      GetSanitizedFileName(params->default_file_name);
  RunFileChooserOnUIThread(default_file_path, std::move(params));
}

void FileSelectHelper::RunFileChooserOnUIThread(
    const base::FilePath& default_file_path,
    std::unique_ptr<FileChooserParams> params) {
  DCHECK(params);
  if (!render_frame_host_ || !web_contents_ ||
      !web_contents_->GetNativeView()) {
    // If the renderer was destroyed before we started, just cancel the
    // operation.
    RunFileChooserEnd();
    return;
  }

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, new OperaSelectFilePolicy());
  if (!select_file_dialog_.get())
    return;

  dialog_mode_ = params->mode;
  switch (params->mode) {
    case FileChooserParams::Open:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      break;
    case FileChooserParams::OpenMultiple:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
      break;
    case FileChooserParams::UploadFolder:
      dialog_type_ = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
      break;
    case FileChooserParams::Save:
      dialog_type_ = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
      break;
    default:
      // Prevent warning.
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      NOTREACHED();
  }

  gfx::NativeWindow owning_window = web_contents_->GetRenderViewHost()
                                        ->GetWidget()
                                        ->GetView()
                                        ->GetNativeView()
                                        ->GetWindowAndroid();

  // Android needs the original MIME types and an additional capture value.
  std::pair<std::vector<base::string16>, bool> accept_types =
      std::make_pair(params->accept_types, params->capture);

  select_file_dialog_->SelectFile(
      dialog_type_, params->title, default_file_path, select_file_types_.get(),
      select_file_types_.get() && !select_file_types_->extensions.empty()
          ? 1
          : 0,  // 1-based index of default extension to show.
      base::FilePath::StringType(),
      owning_window,
      &accept_types);

  select_file_types_.reset();
}

// This method is called when we receive the last callback from the file
// chooser dialog. Perform any cleanup and release the reference we added
// in RunFileChooser().
void FileSelectHelper::RunFileChooserEnd() {
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  Release();
}

void FileSelectHelper::EnumerateDirectory(int request_id,
                                          RenderViewHost* render_view_host,
                                          const base::FilePath& path) {
  // Because this class returns notifications to the RenderViewHost, it is
  // difficult for callers to know how long to keep a reference to this
  // instance. We AddRef() here to keep the instance alive after we return
  // to the caller, until the last callback is received from the enumeration
  // code. At that point, we must call EnumerateDirectoryEnd().
  AddRef();
  StartNewEnumeration(path, request_id, render_view_host);
}

// This method is called when we receive the last callback from the enumeration
// code. Perform any cleanup and release the reference we added in
// EnumerateDirectory().
void FileSelectHelper::EnumerateDirectoryEnd() {
  Release();
}

void FileSelectHelper::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_RENDER_WIDGET_HOST_DESTROYED, type);
  DCHECK_EQ(content::Source<RenderWidgetHost>(source).ptr(),
            render_frame_host_->GetRenderViewHost()->GetWidget());
  render_frame_host_ = nullptr;
}

void FileSelectHelper::RenderViewHostChanged(RenderViewHost* old_host,
                                             RenderViewHost* new_host) {
  CleanUpOnRenderViewHostChange();
}

void FileSelectHelper::WebContentsDestroyed() {
  web_contents_ = nullptr;
  CleanUpOnRenderViewHostChange();
}

// static
bool FileSelectHelper::IsAcceptTypeValid(const std::string& accept_type) {
  // TODO(raymes): This only does some basic checks, extend to test more cases.
  // A 1 character accept type will always be invalid (either a "." in the case
  // of an extension or a "/" in the case of a MIME type).
  std::string unused;
  if (accept_type.length() <= 1 ||
      base::ToLowerASCII(accept_type) != accept_type ||
      base::TrimWhitespaceASCII(accept_type, base::TRIM_ALL, &unused) !=
          base::TRIM_NONE) {
    return false;
  }
  return true;
}

// static
base::FilePath FileSelectHelper::GetSanitizedFileName(
    const base::FilePath& suggested_filename) {
  if (suggested_filename.empty())
    return base::FilePath();
  return net::GenerateFileName(
      GURL(), std::string(), std::string(), suggested_filename.AsUTF8Unsafe(),
      std::string(), std::string("Download"));
}

}  // namespace opera
