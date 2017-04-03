// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TYPED_HISTORY_IMPORTER_H_
#define COMMON_MIGRATION_TYPED_HISTORY_IMPORTER_H_

#include <vector>
#include <string>

#include "common/migration/importer.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

struct TypedHistoryItem;

typedef std::vector<TypedHistoryItem> TypedHistoryItems;

struct TypedHistoryItem {
  enum TypedHistoryItemType {
    TYPED_HISTORY_ITEM_TYPE_TEXT,  /// Typed by the user.
    /// Selected in an address bar from previous global history entries.
    TYPED_HISTORY_ITEM_TYPE_SELECTED,
    TYPED_HISTORY_ITEM_TYPE_SEARCH  /// Search engine query, ex. "g Top Gear"
  };

  std::string url_;
  TypedHistoryItemType type_;
  time_t last_typed_;
};

/** Interface for receiving parsed history entries.
*/
class TypedHistoryReceiver
    : public base::RefCountedThreadSafe<TypedHistoryReceiver> {
 public:
  /** Called after successful typed history input parsing.
   * NOTES: It is called in the file thread.
   * When the whole import process finishes (either successfully or not),
   * it is expected that some code calls
   * MigrationResultListener::OnImportFinished() passing the result.
   * @param items vector of history items. The most recently typed one is
   *        the first.
   * @param listener The listener to notify about the whole import process
   *        result.
   */
  virtual void OnNewHistoryItems(
      const TypedHistoryItems& items,
      scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<TypedHistoryReceiver>;
  virtual ~TypedHistoryReceiver() {}
};

/** Importer of typed history items from Presto Opera.
 *
 * The constructor takes a pointer on TypedHistoryReceiver instance of
 * the class that can be subclassed to make the history items land in
 * the desired place.
 * @see TypedHistoryReceiver
 */
class TypedHistoryImporter : public Importer {
 public:
  explicit TypedHistoryImporter(
      scoped_refptr<TypedHistoryReceiver> historyReceiver);

   /** Imports typed history.
   * First parses the input file into the simple TypedHistoryItem structures,
   * then calls TypedHistoryReceiver::OnNewHistoryItems() method, which allows
   * to handle the imported history items.
   * @param input should be the content of typed_history.xml, may be an open
   *        ifstream.
   * @returns whether importing was successfull.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~TypedHistoryImporter() override;

 private:
  /** Parses the input into temporary history storage.
   * @returns true if successful, false if the input was corrupted.
   */
  bool Parse(std::istream* input);

  TypedHistoryItems items_;

  scoped_refptr<TypedHistoryReceiver> history_receiver_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TYPED_HISTORY_IMPORTER_H_
