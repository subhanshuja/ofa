// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_SPEED_DIAL_IMPORTER_H_
#define COMMON_MIGRATION_SPEED_DIAL_IMPORTER_H_

#include <string>
#include <vector>

#include "common/migration/importer.h"

#include "base/compiler_specific.h"
#include "url/gurl.h"

namespace opera {
namespace common {
namespace migration {

/** Gets called when all speed dial entries are parsed from the ini file and
 * ready to apply.
 */
class SpeedDialListener : public base::RefCountedThreadSafe<SpeedDialListener> {
 public:
  struct BackgroundInfo {
    BackgroundInfo();
    ~BackgroundInfo();
    bool enabled_;  ///< If true, read image from @a file_path_.
    std::string file_path_;  ///< OS specific slashes probably.
    std::string layout_;  ///< ex. "Crop" or "Tile".
  };

  struct SpeedDialEntry {
    SpeedDialEntry();
    SpeedDialEntry(const SpeedDialEntry&);
    ~SpeedDialEntry();
    std::string title_;
    bool title_is_custom_;  ///< If true, title has been changed by user.
    GURL url_;  ///< ex.: http://redir.opera.com/speeddials/facebook/
    GURL display_url_;  ///< ex.: http://www.facebook.com/
    std::string partner_id_;  ///< ex.: "opera-social"
    bool enable_reload_;  ///< If false, ignore other reload_* attributes
    int reload_interval_;  ///< In seconds.
    bool reload_only_if_expired_;  ///< Only if server says page has expired.
    std::string extension_id_;  ///< Empty if this is not an extension.
  };

  /** May be called from a file thread, if you're doing time consuming work
   * within this method, delegate it to a suitable thread.
   */
  virtual void OnSpeedDialParsed(const std::vector<SpeedDialEntry>& entries,
                                 const BackgroundInfo& background) = 0;

 protected:
  friend class base::RefCountedThreadSafe<SpeedDialListener>;
  virtual ~SpeedDialListener() {}
};

/** Importer for seed dial.
 *
 * Presto stored speed dial in speeddial.ini, a simple ini file.
 */
class SpeedDialImporter : public Importer {
 public:
  /** @param listener Receives the parsed ini file upon completing Import()
   */
  explicit SpeedDialImporter(scoped_refptr<SpeedDialListener> listener);

  /** Parses input, which is considered content of speeddial.ini, and
   * notifies listener.
   * @param input Content of speeddial.ini.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~SpeedDialImporter() override;
  bool ImportImplementation(std::istream* input);
  scoped_refptr<SpeedDialListener> listener_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SPEED_DIAL_IMPORTER_H_
