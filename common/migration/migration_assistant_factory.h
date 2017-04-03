// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_MIGRATION_ASSISTANT_FACTORY_H_
#define COMMON_MIGRATION_MIGRATION_ASSISTANT_FACTORY_H_
#include "base/base_export.h"
#include "base/memory/ref_counted.h"
#include "common/migration/cookies/cookie_callback.h"

namespace net {
class URLRequestContextGetter;
}

class Profile;
namespace opera {
namespace common {
namespace migration {

class BookmarkReceiver;
class DownloadHistoryReceiver;
class GlobalHistoryReceiver;
class MigrationAssistant;
class PasswordImporterListener;
class PrefsListener;
class SessionImportListener;
class SpeedDialListener;
class TypedHistoryReceiver;
class VisitedLinksListener;

class ImporterDataHandlerProvider;
class ProfileFolderFinder;

class MigrationAssistantFactory {
 public:
  /** Returns a MigrationAssistant with importers already registered.
   */
  static scoped_refptr<MigrationAssistant> Create(
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
      const scoped_refptr<VisitedLinksListener>& visited_link_listener);

  static void RegisterBookmarkImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<BookmarkReceiver>& bookmark_receiver);

  static void RegisterCookieImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const CookieCallback& cookie_callback);

  static void RegisterDownloadHistoryImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<DownloadHistoryReceiver>& download_history_receiver);

  static void RegisterGlobalHistoryImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<GlobalHistoryReceiver>& global_history_receiver);

  static void RegisterPasswordImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const ProfileFolderFinder* profile_folder_finder,
      const scoped_refptr<PasswordImporterListener>& password_listener);

  static void RegisterPrefsImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<PrefsListener>& prefs_listener);

  static void RegisterSearchHistoryImporter(
      scoped_refptr<MigrationAssistant> migration_assistant);

  static void RegisterSessionImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const ProfileFolderFinder* profile_folder_finder,
      const scoped_refptr<SessionImportListener>& session_listener);

  static void RegisterSpeedDialImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<SpeedDialListener>& speed_dial_listener);

  static void RegisterTypedHistoryImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<TypedHistoryReceiver>& typed_history_receiver);

  static void RegisterVisitedLinksImporter(
      scoped_refptr<MigrationAssistant> migration_assistant,
      const scoped_refptr<VisitedLinksListener>& visited_link_listener);

 private:
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_MIGRATION_ASSISTANT_FACTORY_H_
