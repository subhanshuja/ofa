// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_SEARCH_HISTORY_IMPORTER_H_
#define COMMON_MIGRATION_SEARCH_HISTORY_IMPORTER_H_
#include <vector>
#include <string>

#include "common/migration/importer.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

typedef std::vector<std::string> SearchQueries;

/** Interface for receiving parsed search history entries.
*/
class SearchHistoryReceiver
    : public base::RefCountedThreadSafe<SearchHistoryReceiver> {
 public:
  /** Called after successful search history input parsing.
   * NOTES: It is called in the file thread.
   * When the whole import process finishes (either successfully or not), it is
   * expected that some code calls MigrationResultListener::OnImportFinished()
   * passing the result.
   * @param searches vector of search queries (strings). The most recently
   *        typed one is the first.
   * @param listener The listener to notify about the whole import process
   *        result.
   */
  virtual void OnNewSearchQueries(const SearchQueries& searches,
      scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<SearchHistoryReceiver>;
  virtual ~SearchHistoryReceiver() {}
};

/** Importer of search history from Presto Opera.
 *
 * The constructor takes a pointer on SearchHistoryReceiver instance of the
 * class that can be subclassed to make the search queries land in the desired
 * place.
 * @see SearchHistoryReceiver
 */
class SearchHistoryImporter : public Importer {
 public:
  explicit SearchHistoryImporter(
      scoped_refptr<SearchHistoryReceiver> historyReceiver);

   /** Imports search history.
   * First parses the input file into the simple SearchQueries vector,
   * then calls SearchHistoryReceiver::OnNewSearchQueries() method, which
   * allows to handle the imported search queries.
   * @param input should be the content of search_field_history.dat, may be
   *        an open ifstream.
   * @returns whether importing was successfull.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~SearchHistoryImporter() override;

 private:
  /** Parses the input into temporary search queries vector.
   * @returns true if successful, false if the input was corrupted.
   */
  bool Parse(std::istream* input);

  SearchQueries searches_;

  scoped_refptr<SearchHistoryReceiver> history_receiver_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SEARCH_HISTORY_IMPORTER_H_
