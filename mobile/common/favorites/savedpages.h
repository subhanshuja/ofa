// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_SAVEDPAGES_H_
#define MOBILE_COMMON_FAVORITES_SAVEDPAGES_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/important_file_writer.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

#include "mobile/common/favorites/special_folder_impl.h"

namespace mobile {

class FavoritesImpl;
class SavedPage;
class SavedPagesFolder;

class SavedPagesReader {
 public:
  // All methods in Delegate are called on the UI thread.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Will only be called if there are any saved pages.
    virtual SavedPagesFolder* CreateSavedPagesFolder() = 0;

    // Create a saved page. Will be added to the folder created with
    // CreateSavedPagesFolder().
    virtual SavedPage* CreateSavedPage(const base::string16& title,
                                       const GURL& url,
                                       const std::string& guid,
                                       const std::string& file) = 0;

    // Called when reader is done reading saved pages. The reader will not be
    // used after this call which allows for the delegate to destroy it.
    virtual void DoneReadingSavedPages() = 0;
  };

  SavedPagesReader(
      Delegate& delegate,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
      const base::FilePath& base_path,
      const base::FilePath& saved_pages_path);
  ~SavedPagesReader();

  void ReadSavedPages();

 private:
  void SavedPageRead(const base::string16& title,
                     const GURL& url,
                     const std::string& guid,
                     const std::string& file);
  void DoneReading();

  void ParseSavedPageMHTML(const base::FilePath& filename);
  void ParseSavedPageMHTML(const base::FilePath& filename,
                           const std::string* title);
  void ParseSavedPageWebArchive(const base::FilePath& filename);
  void ParseSavedPageWebArchive(const base::FilePath& filename,
                                const std::string* title);
  void ParseSavedPageWebView(const base::FilePath& filename);
  void ParseSavedPageWebView(const base::FilePath& filename,
                             const std::string* title);
  void ParseSavedPageOBML(const base::FilePath& filename);
  void ParseSavedPageOBML(const base::FilePath& filename,
                          const std::string* title);
  void ParseSavedPageFile(const base::FilePath& filename);
  void ParseSavedPageFile(const base::FilePath& filename,
                          const std::string* title);
  void ParseSavedPage(const base::FilePath& filename);
  void ParseSavedPageNoOBML(const base::FilePath& filename);
  void ParseSavedPage(const base::FilePath& filename, const std::string* title);

  Delegate& delegate_;

  const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner_;

  const base::FilePath base_path_;
  const base::FilePath saved_pages_path_;

  SavedPagesFolder* saved_pages_folder_;
};

class SavedPagesWriter : public base::ImportantFileWriter::DataSerializer {
 public:
  SavedPagesWriter(
      const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
      const base::FilePath& base_path,
      const base::FilePath& saved_pages_path);
  virtual ~SavedPagesWriter();

  bool SerializeData(std::string* data) override;

  void DoScheduledWrite();

  void ScheduleWrite();

  void set_saved_pages_folder(SavedPagesFolder* saved_pages_folder) {
    saved_pages_folder_ = saved_pages_folder;
  }

 private:
  const base::FilePath saved_pages_path_;

  SavedPagesFolder* saved_pages_folder_;

  base::ImportantFileWriter file_writer_;
};

class SavedPagesFolder : public SpecialFolderImpl,
                         public base::SupportsWeakPtr<SavedPagesFolder> {
 public:
  SavedPagesFolder(FavoritesImpl* favorites,
                   int64_t parent,
                   int64_t id,
                   int32_t special_id,
                   const base::string16& title,
                   const std::string& guid,
                   std::unique_ptr<SavedPagesWriter> writer,
                   bool can_be_empty);

  virtual ~SavedPagesFolder();

  bool CanChangeParent() const override;
  bool CanChangeTitle() const override;
  bool CanTakeMoreChildren() const override;
  void Move(int old_position, int new_position) override;
  using FolderImpl::Add;
  void Add(int pos, Favorite* favorite) override;
  void ForgetAbout(Favorite* favorite) override;

  void Remove() override;

  void SetReading(bool reading);
  void SavedPageRead(SavedPage* saved_page);

  void ScheduleWrite();

 protected:
  friend class FavoritesImpl;
  bool Check() override;

  void DoScheduledWrite();

  void RemoveChildren();

 private:
  const std::unique_ptr<SavedPagesWriter> writer_;
  bool reading_;
  bool can_be_empty_;
  bool write_scheduled_while_reading_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_SAVEDPAGES_H_
