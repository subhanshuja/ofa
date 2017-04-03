// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_SESSIONS_SESSION_IMPORT_LISTENER_H_
#define COMMON_MIGRATION_SESSIONS_SESSION_IMPORT_LISTENER_H_
#include <vector>

#include "common/migration/sessions/parent_window.h"
#include "base/memory/ref_counted.h"

namespace opera {
namespace common {
namespace migration {
class MigrationResultListener;

/** Triggered when SessionImporter finishes parsing session files.
 */
class SessionImportListener
    : public base::RefCountedThreadSafe<SessionImportListener> {
 public:
  /** Called when the last used session was read.
   *
   * @note This method will be called on the thread that does file input
   * and parsing. If you plan to do long-running work in response to this call,
   * delegate it to an appropriate thread. The caller assumes this method
   * is fast.
   *
   * @param session Contains all windows (and all tabs within these windows)
   * that were stored in Presto's session file.
   * @param listener Call its OnImportFinished upon finishing importing.
   */
  virtual void OnLastSessionRead(
      const std::vector<ParentWindow>& session,
      scoped_refptr<MigrationResultListener> listener) = 0;

  /** Called when stored a session other than the last one used was read. May
   * be called multiple times if the user has stored several sessions.
   * Most users only have a single session (the autosaved last used) - for them,
   * this method will not get called at all.
   *
   * The same threading considerations as for OnLastSessionRead() apply.
   *
   * @param name Name of the session, as given by the user.
   * @param session Contains all windows (and all tabs within these windows)
   * that were stored in Presto's session file.
   */
  virtual void OnStoredSessionRead(
      const base::string16& name,
      const std::vector<ParentWindow>& session) = 0;

 protected:
  friend class base::RefCountedThreadSafe<SessionImportListener>;
  virtual ~SessionImportListener() {}
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SESSIONS_SESSION_IMPORT_LISTENER_H_
