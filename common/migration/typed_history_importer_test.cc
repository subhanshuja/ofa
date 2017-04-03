// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <string>

#include "base/files/file_util.h"
#include "base/time/time.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/test_path_utils.h"
#include "common/migration/typed_history_importer.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;
using ::base::Time;

namespace opera {
namespace common {
namespace migration {

class TypedHistoryImporterTest : public Test {
 public:
  TypedHistoryImporterTest() : history_items_(NULL) {}

 protected:
  const TypedHistoryItems* history_items_;

  friend class TestTypedHistoryReceiver;
};

class TestTypedHistoryReceiver : public TypedHistoryReceiver {
 public:
  explicit TestTypedHistoryReceiver(
      TypedHistoryImporterTest& typed_history_importer_test)
      : typed_history_importer_test_(typed_history_importer_test) {}

  void OnNewHistoryItems(
      const TypedHistoryItems& items,
      scoped_refptr<MigrationResultListener> listener) override {
    typed_history_importer_test_.history_items_ = &items;
    listener->OnImportFinished(opera::TYPED_HISTORY, true);
  }

 private:
  ~TestTypedHistoryReceiver() override {}
  TypedHistoryImporterTest& typed_history_importer_test_;
};

TEST_F(TypedHistoryImporterTest, ParseGoodTypedHistoryFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("typed_history.xml");

  std::unique_ptr<std::ifstream> history_file(
      new std::ifstream(history_path.value().c_str()));

  ASSERT_TRUE(history_file->is_open());
  ASSERT_FALSE(history_file->fail());

  scoped_refptr<TypedHistoryImporter> importer =
    new TypedHistoryImporter(make_scoped_refptr(
        new TestTypedHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::TYPED_HISTORY, true));
  importer->Import(std::move(history_file), migration_result_listener);
  ASSERT_FALSE(history_items_ == NULL);

  const TypedHistoryItems& items = *history_items_;
  Time::Exploded time;

  ASSERT_EQ(4u, items.size());

  Time().FromTimeT(items[0].last_typed_).UTCExplode(&time);
  EXPECT_EQ("www.wp.pl", items[0].url_);
  EXPECT_EQ(TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_TEXT, items[0].type_);
  EXPECT_EQ(2012, time.year);
  EXPECT_EQ(11, time.month);
  EXPECT_EQ(15, time.day_of_month);
  EXPECT_EQ(11, time.hour);
  EXPECT_EQ(55, time.minute);
  EXPECT_EQ(13, time.second);

  Time().FromTimeT(items[1].last_typed_).UTCExplode(&time);
  EXPECT_EQ("http://www.facebook.com/", items[1].url_);
  EXPECT_EQ(TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_SELECTED, items[1].type_);
  EXPECT_EQ(2012, time.year);
  EXPECT_EQ(11, time.month);
  EXPECT_EQ(15, time.day_of_month);
  EXPECT_EQ(10, time.hour);
  EXPECT_EQ(55, time.minute);
  EXPECT_EQ(33, time.second);

  Time().FromTimeT(items[2].last_typed_).UTCExplode(&time);
  EXPECT_EQ("http://pages.ebay.com/topratedplus/index.html", items[2].url_);
  EXPECT_EQ(TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_SELECTED, items[2].type_);
  EXPECT_EQ(2012, time.year);
  EXPECT_EQ(11, time.month);
  EXPECT_EQ(14, time.day_of_month);
  EXPECT_EQ(16, time.hour);
  EXPECT_EQ(10, time.minute);
  EXPECT_EQ(42, time.second);

  Time().FromTimeT(items[3].last_typed_).UTCExplode(&time);
  EXPECT_EQ("ebay.com", items[3].url_);
  EXPECT_EQ(TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_TEXT, items[3].type_);
  EXPECT_EQ(2012, time.year);
  EXPECT_EQ(11, time.month);
  EXPECT_EQ(14, time.day_of_month);
  EXPECT_EQ(15, time.hour);
  EXPECT_EQ(9, time.minute);
  EXPECT_EQ(19, time.second);
}

TEST_F(TypedHistoryImporterTest, ParseCorruptedTypedHistoryInput) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("typed_history.xml");

  std::ifstream history_file(history_path.value().c_str());

  ASSERT_TRUE(history_file.is_open());
  ASSERT_FALSE(history_file.fail());

  std::string input_string(
      (std::istreambuf_iterator<char>(history_file)),
      std::istreambuf_iterator<char>());
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  std::string input_string_corrupted = input_string;

  // Cut away some part of some date.
  size_t searched_pos = input_string_corrupted.find("2012-11-14");
  ASSERT_NE(input_string_corrupted.npos, searched_pos);
  ASSERT_LT(searched_pos + 10, input_string_corrupted.size());
  input_string_corrupted = input_string_corrupted.substr(0, searched_pos)
      + input_string_corrupted.substr(searched_pos + 10);

  std::unique_ptr<std::istringstream> input(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<TypedHistoryImporter> importer1 =
    new TypedHistoryImporter(make_scoped_refptr(
      new TestTypedHistoryReceiver(*this)));
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::TYPED_HISTORY, false));
  importer1->Import(std::move(input), migration_result_listener);

  input_string_corrupted = input_string;
  // Cut away an attribute and its value
  searched_pos = input_string_corrupted.find("content");
  ASSERT_NE(input_string_corrupted.npos, searched_pos);
  ASSERT_LT(searched_pos + 19, input_string_corrupted.size());
  input_string_corrupted = input_string_corrupted.substr(0, searched_pos)
      + input_string_corrupted.substr(searched_pos + 19);

  std::unique_ptr<std::istringstream> input2(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<TypedHistoryImporter> importer2 =
    new TypedHistoryImporter(make_scoped_refptr(
      new TestTypedHistoryReceiver(*this)));
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::TYPED_HISTORY, false));
  importer2->Import(std::move(input2), migration_result_listener);

  input_string_corrupted = input_string;
  // Corrupt xml totally.
  searched_pos = input_string_corrupted.find("typed_history_item");
  ASSERT_NE(input_string_corrupted.npos, searched_pos);
  ASSERT_LT(searched_pos + 5, input_string_corrupted.size());
  input_string_corrupted = input_string_corrupted.substr(0, searched_pos + 5);

  std::unique_ptr<std::istringstream> input3(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<TypedHistoryImporter> importer3 =
    new TypedHistoryImporter(make_scoped_refptr(
      new TestTypedHistoryReceiver(*this)));
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::TYPED_HISTORY, false));
  importer3->Import(std::move(input3), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
