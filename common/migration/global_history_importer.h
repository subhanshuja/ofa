// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_GLOBAL_HISTORY_IMPORTER_H_
#define COMMON_MIGRATION_GLOBAL_HISTORY_IMPORTER_H_
#include <vector>
#include <string>

#include "common/migration/importer.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

struct GlobalHistoryItem;

typedef std::vector<GlobalHistoryItem> GlobalHistoryItems;

struct GlobalHistoryItem {
  std::string title_;
  std::string url_;
  time_t visit_time_;

  /** This is a pseudo average time between visits as used by Presto.
   * -1 means only one visit in history so far.
   * It is computed the following way when re-visiting a history item
   * (taken from Presto):
   * If item has no average interval set the method will return with a monthly
   * visit (2419200 = 28*24*60*60), otherwise if the average interval was
   * greater than the new interval the new interval is weighted at 20%, if the
   * average interval was less than the new interval the new interval is weighted
   * at 10%. Meaning an item will increase in popularity faster than it will
   * lose it.
   */
  int32_t average_visit_interval_;
};

/** Interface for receiving parsed history entries.
*/
class GlobalHistoryReceiver
    : public base::RefCountedThreadSafe<GlobalHistoryReceiver> {
 public:
  /** Called after successful global history input parsing.
   * NOTES: It is called in the file thread.
   * When the whole import process finishes (either successfully or not), it is
   * expected that some code calls MigrationResultListener::OnImportFinished()
   * passing the result.
   * @param items vector of history items. The most recently visited one is
   *        the last.
   * @param listener The listener to notify about the whole import process
   *        result.
   */
  virtual void OnNewHistoryItems(const GlobalHistoryItems& items,
      scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<GlobalHistoryReceiver>;
  virtual ~GlobalHistoryReceiver() {}
};

/** Importer of global history items from Presto Opera.
 *
 * The constructor takes a pointer on GlobalHistoryReceiver instance of the class
 * that can be subclassed to make the history items land in the desired place.
 * @see GlobalHistoryReceiver
 */
class GlobalHistoryImporter : public Importer {
 public:
  explicit GlobalHistoryImporter(
      scoped_refptr<GlobalHistoryReceiver> historyReceiver);

   /** Imports global history.
   * First parses the input file into the simple GlobalHistoryItem structures,
   * then calls GlobalHistoryReceiver::OnNewHistoryItems() method, which allows
   * to handle the imported history items.
   * @param input should be the content of global_history.dat, may be an open
   *        ifstream.
   * @returns whether importing was successfull.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~GlobalHistoryImporter() override;

 private:
  /** Parses the input into temporary history storage.
   * @returns true if successful, false if the input was corrupted.
   */
  bool Parse(std::istream* input);

  GlobalHistoryItems items_;

  scoped_refptr<GlobalHistoryReceiver> history_receiver_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_GLOBAL_HISTORY_IMPORTER_H_
