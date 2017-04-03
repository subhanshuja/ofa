// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_DOWNLOAD_HISTORY_IMPORTER_H_
#define COMMON_MIGRATION_DOWNLOAD_HISTORY_IMPORTER_H_
#include <vector>

#include "common/migration/importer.h"
#include "common/migration/download/download_history_data.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

typedef std::vector<DownloadInfo> Downloads;

/** Interface for receiving download history data.
*/
class DownloadHistoryReceiver
    : public base::RefCountedThreadSafe<DownloadHistoryReceiver> {
 public:
  /** Called after successful download history input reading.
   * NOTES: It is called in the file thread.
   * When the whole import process finishes (either successfully or not), it is
   * expected that some code calls MigrationResultListener::OnImportFinished()
   * passing the result.
   * @param downloads vector of DownloadInfo structures - each one representing
   *        one download.
   * @param listener The listener to notify about the whole import process
   *        result.
   */
  virtual void OnNewDownloads(
      const Downloads& downloads,
      scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<DownloadHistoryReceiver>;
  virtual ~DownloadHistoryReceiver() {}
};

/** Importer of download history from Presto Opera.
 *
 * The constructor takes a pointer on DownloadHistoryReceiver instance of
 * the class that can be subclassed to make the download history data land in
 * the desired place.
 * @see TypedHistoryReceiver
 */
class DownloadHistoryImporter : public Importer {
 public:
  explicit DownloadHistoryImporter(
      scoped_refptr<DownloadHistoryReceiver> downloadReceiver);

   /** Imports download history.
   * First reads the input file into the vector of DownloadInfo structures,
   * then calls DownloadHistoryReceiver::OnNewDownloads() method, which allows
   * to handle the imported downloads.
   * @param input should be the content of download.dat, may be an open ifstream.
   * @returns whether importing was successfull.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~DownloadHistoryImporter() override;

 private:
  /** Parses the input into temporary Downloads vector.
   * @returns true if successful, false if the input was corrupted.
   */
  bool Parse(std::istream* input);

  bool IsSpecOk(DataStreamReader* input);

  Downloads downloads_;

  scoped_refptr<DownloadHistoryReceiver> download_receiver_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_DOWNLOAD_HISTORY_IMPORTER_H_
