// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_BOOKMARK_IMPORTER_H_
#define COMMON_MIGRATION_BOOKMARK_IMPORTER_H_

#include <vector>
#include <string>

#include "common/migration/importer.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

struct BookmarkEntry;

typedef std::vector<BookmarkEntry> BookmarkEntries;

/** Simple data structure that represents a single bookmark entry.
*/
struct BookmarkEntry {
  enum Type {
    BOOKMARK_ENTRY_SEPARATOR,
    BOOKMARK_ENTRY_FOLDER,
    BOOKMARK_ENTRY_URL,
    BOOKMARK_ENTRY_DELETED
  };

  BookmarkEntry();
  BookmarkEntry(const BookmarkEntry&);
  ~BookmarkEntry();

  Type type;

  // Fields used by url and folder entries.
  std::string guid;
  std::string name;
  time_t create_time;
  std::string description;
  std::string short_name;
  int personal_bar_pos;  // -1 means not on personal bar

  // Fields used by a url entry.
  std::string url;
  std::string display_url;
  int panel_pos;  // -1 means not on panel

  // Fields used by a folder entry.
  bool trash_folder;
  BookmarkEntries entries;  /// Entries in the folder.

  // Fields used by a url and deleted entry.
  std::string partner_id;
};

/** Interface for receiving parsed bookmarks.
*/
class BookmarkReceiver : public base::RefCountedThreadSafe<BookmarkReceiver> {
 public:
  /** Called after successful bookmarks input parsing.
   * NOTES: It might be called multiple times, passing the same entries vector.
   * The implementation must handle that.
   * It is called in the file thread.
   * When the whole import process finishes (either successfully or not), it is
   * expected that some code calls MigrationResultListener::OnImportFinished()
   * passing the result.
   * @param entries The vector of root bookmark entries.
   * @param listener The listener to notify about the whole import process
   *        result.
   */
  virtual void OnNewBookmarks(
    const BookmarkEntries& entries,
    scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<BookmarkReceiver>;
  virtual ~BookmarkReceiver() {}
};

/** Importer of bookmarks from Presto Opera.
 *
 * Presto bookmarks are more advanced than Chromium's. This importer stores
 * all the contents of the input bookmark stream in the BookmarkEntries
 * vector, which represents root bookmark entries.
 * The constructor takes a pointer on BookmarkReceiver instance of the class
 * that can be subclassed to make the bookmarks land in the desired place.
 * @see BookmarkReceiver
 */
class BookmarkImporter : public Importer {
 public:
  explicit BookmarkImporter(
      scoped_refptr<BookmarkReceiver> bookmarkReceiver);

   /** Imports bookmarks.
   * First parses the input file into the simple BookmarkEntry structures,
   * then calls BookmarkReceiver::OnNewBookmarks() method, which allows to
   * handle the imported bookmarks.
   * @param input should be the content of bookmarks.adr, may be an open
   *        ifstream.
   * @returns whether importing was successfull.
   */

 void Import(std::unique_ptr<std::istream> input,
             scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~BookmarkImporter() override;

 private:
  /** Parses the input into temporary bookmark storage.
   * @returns true if successful, false if the input was corrupted.
   */
  bool Parse(std::istream* input);

  /** Recursive method used by parse().
  */
  static bool ParseFolder(BookmarkEntries* entries, std::istream* input);

  BookmarkEntries entries_;  /// Entries in the bookmarks root.

  /// Not owned by BookmarkImporter.
  scoped_refptr<BookmarkReceiver> bookmark_receiver_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_BOOKMARK_IMPORTER_H_
