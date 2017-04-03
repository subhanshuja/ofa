// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/bookmark_importer.h"

#include <fstream>

#include "base/files/file_util.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/test_path_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class TestBookmarkReceiver : public BookmarkReceiver {
 public:
  explicit TestBookmarkReceiver() {}

  void OnNewBookmarks(
    const BookmarkEntries& entries,
    scoped_refptr<MigrationResultListener> listener) override {
    bookmark_entries_ = entries;
    listener->OnImportFinished(opera::BOOKMARKS, true);
  }

  BookmarkEntries bookmark_entries_;
 protected:
  ~TestBookmarkReceiver() override {}
};

class BookmarkImporterTest : public Test {
 public:
  BookmarkImporterTest() : test_receiver_(new TestBookmarkReceiver()) {}

 protected:
  static const char* ref_good_input_string;
  scoped_refptr<TestBookmarkReceiver> test_receiver_;
};


const char* BookmarkImporterTest::ref_good_input_string =
"#FOLDER\n"
"NAME=Trash\n"
"CREATED=1351608158\n"
"TRASH FOLDER=YES\n"
"\n"
"#URL\n"
"NAME=Deleted Boomark\n"
"URL=http://someurl\n"
"CREATED=1351610186\n"
"\n"
"-\n"
"\n"
"#FOLDER\n"
"NAME=Folder1\n"
"CREATED=1351608443\n"
"DESCRIPTION=This is a folder in root.\n"
"SHORT NAME=F1\n"
"\n"
"#URL\n"
"NAME=Opera1\n"
"URL=http://www.opera.com\n"
"CREATED=1351608465\n"
"DESCRIPTION=This is a bookmark to Opera. It has a very long description."
" It has a very long description. It has a very long description."
" It has a very long description. It has a very long description."
" It has a very long description. It has a very long description."
" It has a very long description. It has a very long description."
" It has a very long description. It has a very long description.\n"
"SHORT NAME=Op\n"
"PERSONALBAR_POS=0\n"
"PANEL_POS=0\n"
"\n"
"#SEPARATOR\n"
"\n"
"#URL\n"
"NAME=Ebay\n"
"URL=http://ebay.com\n"
"CREATED=1351608852\n"
"\n"
"#FOLDER\n"
"NAME=Folder2\n"
"CREATED=1351610348\n"
"PERSONALBAR_POS=0\n"
"\n"
"-\n"
"\n"
"-\n"
"\n"
"#URL\n"
"NAME=Facebook\n"
"URL=http://facebook.com\n"
"CREATED=1351608222\n\n";

std::ostream& operator << (std::ostream& out, // NOLINT
                         const BookmarkEntries& bookmarkEntries) {
  for (size_t i = 0; i < bookmarkEntries.size(); i++) {
    bool enter_folder = false;
    const BookmarkEntry& entry = bookmarkEntries[i];

    switch (entry.type) {
    case BookmarkEntry::BOOKMARK_ENTRY_URL:
      out << "#URL\n";
      break;
    case BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR:
      out << "#SEPARATOR\n";
      break;
    case BookmarkEntry::BOOKMARK_ENTRY_FOLDER:
      out << "#FOLDER\n";
      enter_folder = true;
      break;
    case BookmarkEntry::BOOKMARK_ENTRY_DELETED:
      out << "#DELETED\n";
      break;
    }

    if (entry.type != BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR)
      out << "NAME=" << entry.name << std::endl;
    if (entry.type == BookmarkEntry::BOOKMARK_ENTRY_URL)
      out << "URL=" << entry.url << std::endl;
    if (entry.type != BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR)
      out << "CREATED=" << entry.create_time << std::endl;
    if (entry.type == BookmarkEntry::BOOKMARK_ENTRY_FOLDER &&
        entry.trash_folder)
      out << "TRASH FOLDER=YES" << std::endl;

    if (entry.type != BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR) {
      if (entry.description.size() > 0)
        out << "DESCRIPTION=" << entry.description << std::endl;
      if (entry.short_name.size() > 0)
        out << "SHORT NAME=" << entry.short_name << std::endl;
      if (entry.personal_bar_pos >= 0)
        out << "PERSONALBAR_POS=" << entry.personal_bar_pos << std::endl;
      if (entry.partner_id.size() > 0)
        out << "PARTNERID" << entry.partner_id << std::endl;
    }

    if (entry.type == BookmarkEntry::BOOKMARK_ENTRY_URL &&
        entry.panel_pos >= 0)
      out << "PANEL_POS=" << entry.panel_pos << std::endl;
    out << std::endl;

    if (enter_folder) {
      out << (entry.entries) << "-\n" << std::endl;
    }
  }

  return out;
}

TEST_F(BookmarkImporterTest, ImportSimple) {
  std::string input_string =
    "#URL\n\tNAME=bookmark\n\tURL=url\n\tCREATED=1351610186\n\n";
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<BookmarkImporter> importer =
      new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, true));
  importer->Import(std::move(input), migration_result_listener);

  const BookmarkEntries& bookmarks = test_receiver_->bookmark_entries_;

  ASSERT_EQ(1u, bookmarks.size());

  BookmarkEntry entry = bookmarks[0];

  EXPECT_EQ(BookmarkEntry::BOOKMARK_ENTRY_URL, entry.type);
  EXPECT_EQ("bookmark", entry.name);
  EXPECT_EQ("url", entry.url);
  EXPECT_EQ(1351610186, entry.create_time);
  EXPECT_EQ(-1, entry.panel_pos);
  EXPECT_EQ(-1, entry.personal_bar_pos);
  EXPECT_EQ("", entry.short_name);
  EXPECT_EQ("", entry.description);
  EXPECT_FALSE(entry.trash_folder);
  EXPECT_EQ(0u, entry.entries.size());
}

TEST_F(BookmarkImporterTest, ImportTwice) {
  std::string input_string1 =
      "#FOLDER\n\tNAME=folder\n\tCREATED=1351610187\n\n#SEPARATOR\n\n-\n\n";
  std::unique_ptr<std::istringstream> input1(new std::istringstream(input_string1));
  std::string input_string2 =
      "#URL\n\tNAME=bookmark\n\tURL=url\n\tCREATED=1351610186\n\n";
  std::unique_ptr<std::istringstream> input2(new std::istringstream(input_string2));

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, true)).Times(2);
  importer->Import(std::move(input1), migration_result_listener);
  importer->Import(std::move(input2), migration_result_listener);

  const BookmarkEntries& bookmarks = test_receiver_->bookmark_entries_;

  ASSERT_EQ(2u, bookmarks.size());

  const BookmarkEntry& entry = bookmarks[0];

  EXPECT_EQ(BookmarkEntry::BOOKMARK_ENTRY_FOLDER, entry.type);
  EXPECT_EQ("folder", entry.name);
  EXPECT_EQ("", entry.url);
  EXPECT_EQ(1351610187, entry.create_time);
  EXPECT_EQ(-1, entry.panel_pos);
  EXPECT_EQ(-1, entry.personal_bar_pos);
  EXPECT_EQ("", entry.short_name);
  EXPECT_EQ("", entry.description);
  EXPECT_FALSE(entry.trash_folder);
  EXPECT_EQ(1u, entry.entries.size());

  const BookmarkEntry& child_entry = entry.entries[0];

  EXPECT_EQ(BookmarkEntry::BOOKMARK_ENTRY_SEPARATOR, child_entry.type);
  EXPECT_EQ("", child_entry.name);
  EXPECT_EQ("", child_entry.url);
  EXPECT_EQ(0, child_entry.create_time);
  EXPECT_EQ(-1, child_entry.panel_pos);
  EXPECT_EQ(-1, child_entry.personal_bar_pos);
  EXPECT_EQ("", child_entry.short_name);
  EXPECT_EQ("", child_entry.description);
  EXPECT_FALSE(child_entry.trash_folder);
  EXPECT_EQ(0u, child_entry.entries.size());

  const BookmarkEntry& second_entry = bookmarks[1];

  EXPECT_EQ(BookmarkEntry::BOOKMARK_ENTRY_URL, second_entry.type);
  EXPECT_EQ("bookmark", second_entry.name);
  EXPECT_EQ("url", second_entry.url);
  EXPECT_EQ(1351610186, second_entry.create_time);
  EXPECT_EQ(-1, second_entry.panel_pos);
  EXPECT_EQ(-1, second_entry.personal_bar_pos);
  EXPECT_EQ("", second_entry.short_name);
  EXPECT_EQ("", second_entry.description);
  EXPECT_FALSE(second_entry.trash_folder);
  EXPECT_EQ(0u, second_entry.entries.size());
}

TEST_F(BookmarkImporterTest, ParseGoodBookmarksFile) {
  base::FilePath bookmarks_path =
      ut::GetTestDataDir().AppendASCII("bookmarks.txt");

  std::unique_ptr<std::ifstream> bookmarks_file(
      new std::ifstream(bookmarks_path.value().c_str()));

  ASSERT_TRUE(bookmarks_file->is_open());
  ASSERT_FALSE(bookmarks_file->fail());

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, true));
  importer->Import(std::move(bookmarks_file), migration_result_listener);

  std::ostringstream out_string_stream;
  std::string ref_string(ref_good_input_string);
  std::string output_string;

  out_string_stream << (test_receiver_->bookmark_entries_);
  output_string = out_string_stream.str();

  EXPECT_EQ(ref_string, output_string);
}

TEST_F(BookmarkImporterTest, ParseBookmarkFileWithNoEmptyLineAtTheEnd) {
  std::string input_string("#FOLDER\n\tNAME=folder\n\n#URL\n\tNAME=url\n");
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, true));
  importer->Import(std::move(input), migration_result_listener);
}

TEST_F(BookmarkImporterTest, ParseCorruptedBookmarksFile1) {
  std::string input_string =
    "Some nonsense.\nSome nonsense.\nSome nonsense.\nSome nonsense.\n";
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, false));
  importer->Import(std::move(input), migration_result_listener);
}

TEST_F(BookmarkImporterTest, ParseCorruptedBookmarksFile2) {
  std::string input_string =
    "#FOLDER\n\tNAME=folder\n\n#URL\n\tNAME=url\nSome Garbage.\n\n-\n\n";
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, false));
  importer->Import(std::move(input), migration_result_listener);
}

TEST_F(BookmarkImporterTest, ParseCorruptedBookmarksFile3) {
  std::string input_string =
    "#FOLDER\n\tNAME=folder\n\n#URL\n\tNAME=url\n\tURLurl\n\n-\n\n";
  std::unique_ptr<std::istringstream> input(new std::istringstream(input_string));

  scoped_refptr<BookmarkImporter> importer =
    new BookmarkImporter(test_receiver_);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::BOOKMARKS, false));
  importer->Import(std::move(input), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
