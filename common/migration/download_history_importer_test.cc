// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <string>

#include "base/files/file_util.h"
#include "base/time/time.h"
#include "common/migration/download_history_importer.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/test_path_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class DownloadHistoryImporterTest : public Test {
 public:
  DownloadHistoryImporterTest() : downloads_(NULL) {}

 protected:
  const Downloads* downloads_;

  friend class TestDownloadHistoryReceiver;
};

class TestDownloadHistoryReceiver : public DownloadHistoryReceiver {
 public:
  explicit TestDownloadHistoryReceiver(
      DownloadHistoryImporterTest& download_history_importer_test)
      : download_history_importer_test_(download_history_importer_test) {}

  void OnNewDownloads(const Downloads& downloads,
      scoped_refptr<MigrationResultListener> listener) override {
    download_history_importer_test_.downloads_ = &downloads;
    listener->OnImportFinished(opera::DOWNLOADS, true);
  }

 private:
  ~TestDownloadHistoryReceiver() override {}
  DownloadHistoryImporterTest& download_history_importer_test_;
};

TEST_F(DownloadHistoryImporterTest, ParseGoodDownloadsFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("download.dat");

  std::unique_ptr<std::ifstream> history_file(
      new std::ifstream(history_path.value().c_str(), std::ios_base::binary));

  ASSERT_TRUE(history_file->is_open());
  ASSERT_FALSE(history_file->fail());

  scoped_refptr<DownloadHistoryImporter> importer =
      new DownloadHistoryImporter(make_scoped_refptr(
          new TestDownloadHistoryReceiver(*this)));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::DOWNLOADS, true));
  importer->Import(std::move(history_file), migration_result_listener);
  ASSERT_FALSE(downloads_ == NULL);

  const Downloads& downloads = *downloads_;

  ASSERT_EQ(2u, downloads.size());

  EXPECT_EQ(
      "http://download.nullsoft.com/winamp/client/winamp563_full_emusic-"
                                                        "7plus_pl-pl.exe",
      downloads[0].url_);
  EXPECT_EQ(0x50ab847d, downloads[0].last_visited_);
  EXPECT_FALSE(downloads[0].form_query_result_);

  EXPECT_EQ(0x50ab847d, downloads[0].loaded_time_);
  EXPECT_EQ(2, downloads[0].load_status_);
  EXPECT_EQ(0x00c7a620u, downloads[0].content_size_);
  EXPECT_EQ("application/octet-stream", downloads[0].mime_type_);
  EXPECT_EQ("", downloads[0].character_set_);
  EXPECT_EQ(true, downloads[0].stored_locally_);
  EXPECT_EQ("C:\\Downloads\\winamp563_full_emusic-7plus_pl-pl.exe",
      downloads[0].file_name_);
  EXPECT_FALSE(downloads[0].check_if_modified_);
  EXPECT_EQ(2, downloads[0].network_type_);
  EXPECT_FALSE(downloads[0].mime_initially_empty_);
  EXPECT_EQ("", downloads[0].ftp_modified_date_);
  EXPECT_FALSE(downloads[0].never_flush_);
  EXPECT_EQ(0u, downloads[0].associated_files_);

  EXPECT_EQ(0, downloads[0].segment_start_time_);
  EXPECT_EQ(0, downloads[0].segment_stop_time_);
  EXPECT_EQ(0u, downloads[0].segment_amount_of_bytes_);
  EXPECT_EQ(1u, downloads[0].resume_ability_);

  EXPECT_EQ("http://download.microsoft.com/download/4/A/2/4A25C7D5-EFBE-4182-B6"
                                        "A9-AE6850409A78/GRMWDK_EN_7600_1.ISO",
      downloads[1].url_);
  EXPECT_EQ(0x50ab852c, downloads[1].last_visited_);
  EXPECT_FALSE(downloads[1].form_query_result_);

  EXPECT_EQ(0x50ab852c, downloads[1].loaded_time_);
  EXPECT_EQ(4, downloads[1].load_status_);
  EXPECT_EQ(0x26bc5800u, downloads[1].content_size_);
  EXPECT_EQ("application/octet-stream", downloads[1].mime_type_);
  EXPECT_EQ("", downloads[1].character_set_);
  EXPECT_EQ(true, downloads[1].stored_locally_);
  EXPECT_EQ("C:\\Downloads\\GRMWDK_EN_7600_1.ISO", downloads[1].file_name_);
  EXPECT_FALSE(downloads[1].check_if_modified_);
  EXPECT_EQ(3, downloads[1].network_type_);
  EXPECT_FALSE(downloads[1].mime_initially_empty_);
  EXPECT_EQ("", downloads[1].ftp_modified_date_);
  EXPECT_FALSE(downloads[1].never_flush_);
  EXPECT_EQ(0u, downloads[1].associated_files_);

  EXPECT_EQ(0, downloads[1].segment_start_time_);
  EXPECT_EQ(0, downloads[1].segment_stop_time_);
  EXPECT_EQ(0u, downloads[1].segment_amount_of_bytes_);
  EXPECT_EQ(1u, downloads[1].resume_ability_);
}

TEST_F(DownloadHistoryImporterTest, ParseCorruptedDownloadsFile) {
  base::FilePath history_path =
      ut::GetTestDataDir().AppendASCII("download.dat");

  std::ifstream history_file(
        history_path.value().c_str(), std::ios_base::binary);

  ASSERT_TRUE(history_file.is_open());
  ASSERT_FALSE(history_file.fail());

  scoped_refptr<DownloadHistoryImporter> importer =
      new DownloadHistoryImporter(make_scoped_refptr(
          new TestDownloadHistoryReceiver(*this)));

  std::string input_string_corrupted(
      (std::istreambuf_iterator<char>(history_file)),
      std::istreambuf_iterator<char>());
  ASSERT_EQ(0x50, input_string_corrupted[17]);
  // Inserting a random number for length of the url record.
  input_string_corrupted[17] = 0x36;

  std::unique_ptr<std::istringstream> input(
      new std::istringstream(input_string_corrupted));
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::DOWNLOADS, false));
  importer->Import(std::move(input), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
