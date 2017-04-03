// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/migration_assistant_factory.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "common/migration/bookmark_importer.h"
#include "common/migration/cookie_importer.h"
#include "common/migration/download_history_importer.h"
#include "common/migration/global_history_importer.h"
#include "common/migration/migration_assistant.h"
#include "common/migration/password_importer.h"
#include "common/migration/prefs_importer.h"
#include "common/migration/profile_folder_finder.h"
#include "common/migration/search_history_importer.h"
#include "common/migration/session_importer.h"
#include "common/migration/speed_dial_importer.h"
#include "common/migration/tools/bulk_file_reader.h"
#include "common/migration/typed_history_importer.h"
#include "common/migration/visited_links_importer.h"
#include "common/migration/web_storage_importer.h"
#include "content/public/browser/browser_thread.h"


namespace opera {
namespace common {
namespace migration {

/* static */
scoped_refptr<MigrationAssistant> MigrationAssistantFactory::Create(
    const ProfileFolderFinder* profile_finder,
    const CookieCallback& cookie_callback,
    const scoped_refptr<BookmarkReceiver>& bookmark_receiver,
    const scoped_refptr<DownloadHistoryReceiver>& download_history_receiver,
    const scoped_refptr<GlobalHistoryReceiver>& global_history_receiver,
    const scoped_refptr<SpeedDialListener>& speed_dial_listener,
    const scoped_refptr<SessionImportListener>& session_listener,
    const scoped_refptr<PasswordImporterListener>& password_listener,
    const scoped_refptr<PrefsListener>& prefs_listener,
    const scoped_refptr<TypedHistoryReceiver>& typed_history_receiver,
    const scoped_refptr<VisitedLinksListener>& visited_link_listener) {
  scoped_refptr<MigrationAssistant> migration_assistant(
      new MigrationAssistant(
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::FILE),
          profile_finder));

  /* Register importers here. */
  RegisterBookmarkImporter(
      migration_assistant,
      bookmark_receiver);
  RegisterCookieImporter(
      migration_assistant,
      cookie_callback);
  RegisterDownloadHistoryImporter(
      migration_assistant,
      download_history_receiver);
  RegisterGlobalHistoryImporter(
      migration_assistant,
      global_history_receiver);
  RegisterPasswordImporter(
      migration_assistant,
      profile_finder,
      password_listener);
  RegisterPrefsImporter(
      migration_assistant,
      prefs_listener);
  RegisterSearchHistoryImporter(
      migration_assistant);
  RegisterSessionImporter(
      migration_assistant,
      profile_finder,
      session_listener);
  RegisterSpeedDialImporter(
      migration_assistant,
      speed_dial_listener);
  RegisterTypedHistoryImporter(
      migration_assistant,
      typed_history_receiver);
  RegisterVisitedLinksImporter(
      migration_assistant,
      visited_link_listener);

  return migration_assistant;
}

/* static */
void MigrationAssistantFactory::RegisterBookmarkImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<BookmarkReceiver>& bookmark_receiver) {
  scoped_refptr<BookmarkImporter> bookmark_importer =
      new BookmarkImporter(bookmark_receiver);

  migration_assistant->RegisterImporter(
      bookmark_importer,
      opera::BOOKMARKS,
      base::FilePath(FILE_PATH_LITERAL("bookmarks.adr")),
      false);
}

/* static */
void MigrationAssistantFactory::RegisterCookieImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const CookieCallback& cookie_callback) {

  scoped_refptr<CookieImporter> cookie_importer =
      new CookieImporter(cookie_callback);

  migration_assistant->RegisterImporter(
      cookie_importer,
      opera::COOKIES,
      base::FilePath(FILE_PATH_LITERAL("cookies4.dat")),
      true);
}

/* static */
void MigrationAssistantFactory::RegisterDownloadHistoryImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<DownloadHistoryReceiver>& download_history_receiver) {

  scoped_refptr<DownloadHistoryImporter> download_importer =
      new DownloadHistoryImporter(download_history_receiver);

  migration_assistant->RegisterImporter(
      download_importer,
      opera::DOWNLOADS,
      base::FilePath(FILE_PATH_LITERAL("download.dat")),
      true);
}

/* static */
void MigrationAssistantFactory::RegisterGlobalHistoryImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<GlobalHistoryReceiver>& global_history_receiver) {

  scoped_refptr<GlobalHistoryImporter> global_history_importer =
      new GlobalHistoryImporter(global_history_receiver);

  migration_assistant->RegisterImporter(
      global_history_importer,
      opera::HISTORY,
      base::FilePath(FILE_PATH_LITERAL("global_history.dat")),
      false);
}

/* static */
void MigrationAssistantFactory::RegisterPasswordImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const ProfileFolderFinder* profile_folder_finder,
    const scoped_refptr<PasswordImporterListener>& password_listener) {

  base::FilePath opcert(FILE_PATH_LITERAL("opcert6.dat"));
  base::FilePath opcert_base_path;
  if (profile_folder_finder->FindFolderFor(opcert, &opcert_base_path)) {

    scoped_refptr<SslPasswordReader> ssl_reader(
        new SslPasswordReader(opcert_base_path.Append(opcert)));
    scoped_refptr<PasswordImporter> importer =
        new PasswordImporter(password_listener, ssl_reader);

    migration_assistant->RegisterImporter(
        importer,
        opera::PASSWORDS,
        base::FilePath(FILE_PATH_LITERAL("wand.dat")),
        true);
  } else {
    LOG(INFO) << "Passwords from Presto not found, not importing";
  }
}

/* static */
void MigrationAssistantFactory::RegisterPrefsImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<PrefsListener>& prefs_listener) {

  scoped_refptr<PrefsImporter> importer = new PrefsImporter(prefs_listener);

  migration_assistant->RegisterImporter(
      importer,
      opera::PREFS,
      base::FilePath(FILE_PATH_LITERAL("operaprefs.ini")),
      false);
}

/* static */
void MigrationAssistantFactory::RegisterSearchHistoryImporter(
    scoped_refptr<MigrationAssistant> migration_assistant) {

  // TODO(pdamek): Get a valid SearchHistoryReceiver pointer.
  scoped_refptr<SearchHistoryReceiver> search_history_receiver = NULL;
  scoped_refptr<SearchHistoryImporter> search_history_importer =
      new SearchHistoryImporter(search_history_receiver);

  migration_assistant->RegisterImporter(
      search_history_importer,
      opera::SEARCH_HISTORY,
      base::FilePath(FILE_PATH_LITERAL("search_field_history.dat")),
      false);
}


/* static */
void MigrationAssistantFactory::RegisterSessionImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const ProfileFolderFinder* profile_folder_finder,
    const scoped_refptr<SessionImportListener>& session_listener) {

  base::FilePath autosave_win = base::FilePath(FILE_PATH_LITERAL("sessions"))
                                .AppendASCII("autosave.win");
  base::FilePath sessions_folder;
  if (!profile_folder_finder->FindFolderFor(
        autosave_win, &sessions_folder)) {
    LOG(ERROR) << "ProfileFolderFinder cannot find folder that contains "
               << autosave_win.value();
    return;
  }
  sessions_folder = sessions_folder.AppendASCII("sessions");
  scoped_refptr<BulkFileReader> bulk_file_reader =
      new BulkFileReader(sessions_folder);
  scoped_refptr<SessionImporter> importer(
      new SessionImporter(bulk_file_reader, session_listener));
  migration_assistant->RegisterImporter(
      importer,
      opera::SESSIONS,
      autosave_win,
      false);
}

/* static */
void MigrationAssistantFactory::RegisterSpeedDialImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<SpeedDialListener>& speed_dial_listener) {

  scoped_refptr<SpeedDialImporter> importer(
        new SpeedDialImporter(speed_dial_listener));
  migration_assistant->RegisterImporter(
      importer,
      opera::SPEED_DIAL,
      base::FilePath(FILE_PATH_LITERAL("speeddial.ini")),
      false);
}

/* static */
void MigrationAssistantFactory::RegisterTypedHistoryImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<TypedHistoryReceiver>& typed_history_receiver) {

  scoped_refptr<TypedHistoryImporter> importer =
      new TypedHistoryImporter(typed_history_receiver);

  migration_assistant->RegisterImporter(
      importer,
      opera::TYPED_HISTORY,
      base::FilePath(FILE_PATH_LITERAL("typed_history.xml")),
      false);
}

/* static */
void MigrationAssistantFactory::RegisterVisitedLinksImporter(
    scoped_refptr<MigrationAssistant> migration_assistant,
    const scoped_refptr<VisitedLinksListener>& visited_link_listener) {

  scoped_refptr<VisitedLinksImporter> importer =
      new VisitedLinksImporter(visited_link_listener);
  migration_assistant->RegisterImporter(
      importer,
      opera::VISITED_LINKS,
      base::FilePath(FILE_PATH_LITERAL("vlink4.dat")),
      true);
}
}  // namespace migration
}  // namespace common
}  // namespace opera
