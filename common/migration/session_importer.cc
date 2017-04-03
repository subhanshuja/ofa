// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/session_importer.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "common/ini_parser/ini_parser.h"
#include "common/migration/sessions/parent_window.h"
#include "common/migration/sessions/session_import_listener.h"
#include "common/migration/sessions/tab_session.h"
#include "common/migration/tools/bulk_file_reader.h"

namespace opera {
namespace common {
namespace migration {
using opera::common::IniParser;

SessionImporter::SessionImporter(
    scoped_refptr<BulkFileReader> reader,
    scoped_refptr<SessionImportListener> listener)
    : reader_(reader),
      session_listener_(listener) {
}

SessionImporter::~SessionImporter() {
}

void SessionImporter::Import(std::unique_ptr<std::istream> input,
                             scoped_refptr<MigrationResultListener> listener) {
  std::vector<ParentWindow> session_windows;
  // Main session
  bool main_session_parsed =
      session_listener_ && ParseIni(input.get(), &session_windows);
  if (!main_session_parsed) {
    LOG(INFO) << "autosave.win corrupted, trying to read from autosave.win.bak";
    std::istringstream backup_content(
        reader_->ReadFileContent(
            base::FilePath(FILE_PATH_LITERAL("autosave.win.bak"))));
    main_session_parsed = ParseIni(&backup_content, &session_windows);
  }
  if (main_session_parsed)
    session_listener_->OnLastSessionRead(session_windows, listener);
  // Other saved sessions
  base::FilePath file_in_sessions_dir = reader_->GetNextFile();
  while (!file_in_sessions_dir.empty()) {
    base::FilePath::StringType extension = file_in_sessions_dir.Extension();
    base::string16 name =
        file_in_sessions_dir.BaseName().RemoveExtension().LossyDisplayName();
    if (extension == FILE_PATH_LITERAL(".win") &&
        name != base::ASCIIToUTF16("autosave")) {
      session_windows.clear();
      std::string content = reader_->ReadFileContent(file_in_sessions_dir);
      std::istringstream iss(content);
      if (session_listener_ && ParseIni(&iss, &session_windows)) {
        session_listener_->OnStoredSessionRead(name, session_windows);
      }
    }
    file_in_sessions_dir = reader_->GetNextFile();
  }
  if (!main_session_parsed)
    listener->OnImportFinished(opera::SESSIONS, false);
}

bool SessionImporter::ParseIni(
    std::istream* input,
    std::vector<ParentWindow>* output) const {
  IniParser parser;
  if (!parser.Parse(input)) {
    LOG(ERROR) << "Could not parse input as ini";
    return false;
  }
  // window count is the number of both parent windows and tabs
  int window_count = parser.GetAs<int>("session", "window count", -1);
  if (window_count == -1) {
    LOG(ERROR) << "Session file invalid, no 'window count' in [session]";
    return false;
  }
  std::vector<ParentWindow>& parent_windows = *output;
  for (int i = 1; i <= window_count; ++i) {
    int parent = parser.GetAs<int>(base::IntToString(i), "parent", -1);
    if (parent == -1) {
      LOG(ERROR) << "Entry " << i << " is malformed, no parent";
      return false;
    }
    if (parent == 0) {
      // This is a parent window
      ParentWindow current_parent_window;
      if (!current_parent_window.SetFromIni(parser, i)) {
        LOG(ERROR) << "Entry " << i << " identifies as parent window, couldn't "
                   << "parse as such";
        return false;
      }
      parent_windows.push_back(current_parent_window);
    } else {
      // This is a tab session
      if (parent_windows.empty()) {
        LOG(ERROR) << "There's no parent window to hold tab session " << i;
        return false;
      }
      TabSession current_session;
      if (!current_session.SetFromIni(parser, i)) {
        LOG(ERROR) << "Entry " << i << " identifies as tab session, couldn't "
                   << "parse as such";
        return false;
      }
      ParentWindow& last_window = parent_windows.back();
      last_window.owned_tabs_.push_back(current_session);
    }
  }
  return true;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
