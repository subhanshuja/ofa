// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

#include "base/files/file_util.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/data_stream_test_utils.h"
#include "common/migration/tools/test_path_utils.h"
#include "common/migration/visited_links_importer.h"
#include "common/migration/vlinks/vlink4_tags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using opera::common::migration::ut::StreamInserter;
using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class VisitedLinksListenerMock : public VisitedLinksListener {
 public:
  void OnVisitedLinksArrived(
      const std::vector<VisitedLink>& visited_links) override {
    visited_links_ = visited_links;
  }
  std::vector<VisitedLink> visited_links_;

 private:
  ~VisitedLinksListenerMock() override {}
};

class VisitedLinksImporterTest : public Test {
 public:
  VisitedLinksImporterTest() :
      listener_(new VisitedLinksListenerMock()),
      migration_result_listener_(new MigrationResultListenerMock()),
      importer_(new VisitedLinksImporter(listener_)) {
  }

 protected:
  void FillDataStreamWithDefaultSpec(std::iostream* datastream) const {
    DataStreamReader::Spec spec = { 4096, 8192, 1, 2 };
    StreamInserter inserter(true);
    inserter.Insert(spec, datastream);
  }

  void InsertTopLevelVisitedLink(std::string url,
                                 uint32_t last_visited,
                                 std::iostream* datastream) {
    StreamInserter inserter(true);
    inserter.Insert(uint8_t(TAG_URL_NAME), datastream);
    inserter.Insert(uint16_t(url.size()), datastream);
    *datastream << url;
    inserter.Insert(uint8_t(TAG_LAST_VISITED), datastream);
    inserter.Insert(uint16_t(sizeof(last_visited)), datastream);
    inserter.Insert(last_visited, datastream);
  }

  void ImportAndExpectResult(std::unique_ptr<std::istream> input, bool success) {
    EXPECT_CALL(*migration_result_listener_,
                OnImportFinished(opera::VISITED_LINKS, success));
    importer_->Import(std::move(input), migration_result_listener_);
  }

  scoped_refptr<VisitedLinksListenerMock> listener_;
  scoped_refptr<MigrationResultListenerMock> migration_result_listener_;
  scoped_refptr<VisitedLinksImporter> importer_;
};

TEST_F(VisitedLinksImporterTest, EmptyInput) {
  // The input is never really empty, contains at least the datastream header.
  std::unique_ptr<std::stringstream> datastream(new std::stringstream());
  FillDataStreamWithDefaultSpec(datastream.get());

  ImportAndExpectResult(std::move(datastream), true);

  // No links were imported
  EXPECT_EQ(0u, listener_->visited_links_.size());
}

TEST_F(VisitedLinksImporterTest, UnknownTag) {
  std::unique_ptr<std::stringstream> datastream(new std::stringstream());
  FillDataStreamWithDefaultSpec(datastream.get());
  StreamInserter inserter(true);
  inserter.Insert(uint8_t(42), datastream.get());  // Unknown tag

  ImportAndExpectResult(std::move(datastream), false);

  // No links were imported
  EXPECT_EQ(0u, listener_->visited_links_.size());
}

TEST_F(VisitedLinksImporterTest, UnexpectedTag) {
  std::unique_ptr<std::stringstream> datastream(new std::stringstream());
  FillDataStreamWithDefaultSpec(datastream.get());
  StreamInserter inserter(true);
  inserter.Insert(uint8_t(TAG_LAST_VISITED), datastream.get());
  /* TAG_LAST_VISITED is a known tag but we don't expect it yet, there should be
   * a TAG_VISITED_FILE_ENTRY first that starts the record. */

  ImportAndExpectResult(std::move(datastream), false);

  // No links were imported
  EXPECT_EQ(0u, listener_->visited_links_.size());
}

TEST_F(VisitedLinksImporterTest, ParseSingleVlink) {
  std::unique_ptr<std::stringstream> datastream(new std::stringstream());
  FillDataStreamWithDefaultSpec(datastream.get());
  StreamInserter inserter(true);
  inserter.Insert(uint8_t(TAG_VISITED_FILE_ENTRY), datastream.get());
  std::string url = "http://visited.address.com/";
  time_t last_visited = 123456;

  inserter.Insert(uint16_t(url.size() +
                           sizeof(last_visited) +
                           2 * sizeof(int16_t)),  // size of record
                  datastream.get());
  InsertTopLevelVisitedLink(url, last_visited, datastream.get());

  ImportAndExpectResult(std::move(datastream), true);

  // 1 link was imported
  ASSERT_EQ(1u, listener_->visited_links_.size());
  EXPECT_EQ(GURL(url), listener_->visited_links_[0].GetUrl());
  EXPECT_EQ(last_visited, listener_->visited_links_[0].GetLastVisitedTime());

  // This link had no relative paths
  EXPECT_EQ(0u, listener_->visited_links_[0].GetRelativeLinks().size());
}

TEST_F(VisitedLinksImporterTest, ActualData) {
  base::FilePath path =
      ut::GetTestDataDir().AppendASCII("vlink4.dat");
  std::unique_ptr<std::ifstream> input_file(
      new std::ifstream(path.value().c_str(), std::ios_base::binary));
  ASSERT_TRUE(input_file->is_open());
  ASSERT_FALSE(input_file->fail());

  ImportAndExpectResult(std::move(input_file), true);
  // There are 4 visited links in the input file
  ASSERT_EQ(4u, listener_->visited_links_.size());

  // First vlink, a google search, no relative links
  EXPECT_EQ(GURL("http://www.google.pl/search?q=gmock&"
                 "sourceid=opera&ie=utf-8&oe=utf-8&channel=suggest"),
            listener_->visited_links_[0].GetUrl());
  EXPECT_EQ(1352908886, listener_->visited_links_[0].GetLastVisitedTime());
  EXPECT_EQ(0u, listener_->visited_links_[0].GetRelativeLinks().size());

  // Second link, plusone.google.com/..., has relative 2 links
  EXPECT_EQ(GURL(
  "https://plusone.google.com/_/+1/fastbutton?bsv&source=google%3Aprojecthosti"
  "ng&hl=en&origin=http%3A%2F%2Fcode.google.com&url=http%3A%2F%2Fcode.google.c"
  "om%2Fp%2Fgooglemock%2F&jsh=m%3B%2F_%2Fapps-static%2F_%2Fjs%2Fgapi%2F__featu"
  "res__%2Frt%3Dj%2Fver%3DmMrZKyZMZks.pl.%2Fsv%3D1%2Fam%3D!9YrXPIrxx2-ITyEIjA%"
  "2Fd%3D1%2Frs%3DAItRSTMJ59WpXXDcRNFpdPw0eydhw9mscg"),
            listener_->visited_links_[1].GetUrl());
  EXPECT_EQ(1352908890, listener_->visited_links_[1].GetLastVisitedTime());
  ASSERT_EQ(2u, listener_->visited_links_[1].GetRelativeLinks().size());

  // First relative link of plusone.google.com:
  const VisitedLink::RelativeLink& link10 =
      listener_->visited_links_[1].GetRelativeLinks()[0];
  EXPECT_EQ(
        "_methods=onPlusOne%2C_ready%2C_close%2C_open%2C_resizeMe%2C_renderstar"
        "t%2Concircled&id=I1_1352908652397&parent=http%3A%2F%2Fcode.google.com",
        link10.url_);
  EXPECT_EQ(1352908652, link10.last_visited_);

  // Second relative link of plusone.google.com:
  const VisitedLink::RelativeLink& link11 =
      listener_->visited_links_[1].GetRelativeLinks()[1];
  EXPECT_EQ(
        "_methods=onPlusOne%2C_ready%2C_close%2C_open%2C_resizeMe%2C_renderstar"
        "t%2Concircled&id=I1_1352908890431&parent=http%3A%2F%2Fcode.google.com",
        link11.url_);
  EXPECT_EQ(1352908890, link11.last_visited_);

  // Third link, http://www.google.pl/blank.html, no relative links:
  EXPECT_EQ(GURL("http://www.google.pl/blank.html"),
            listener_->visited_links_[2].GetUrl());
  EXPECT_EQ(1352908886, listener_->visited_links_[2].GetLastVisitedTime());
  EXPECT_EQ(0u, listener_->visited_links_[2].GetRelativeLinks().size());

  // Last link, another plusone.google.com, has relative 2 links:
  EXPECT_EQ(GURL(
              "https://plusone.google.com/_/+1/fastbutton?bsv&size=small&annota"
              "tion=inline&width=200&hl=en&origin=http%3A%2F%2Fcode.google.com&"
              "url=http%3A%2F%2Fcode.google.com%2Fp%2Fgooglemock%2F&jsh=m%3B%2F"
              "_%2Fapps-static%2F_%2Fjs%2Fgapi%2F__features__%2Frt%3Dj%2Fver%3D"
              "mMrZKyZMZks.pl.%2Fsv%3D1%2Fam%3D!9YrXPIrxx2-ITyEIjA%2Fd%3D1%2Frs"
              "%3DAItRSTMJ59WpXXDcRNFpdPw0eydhw9mscg"),
            listener_->visited_links_[3].GetUrl());
  EXPECT_EQ(1352908890, listener_->visited_links_[3].GetLastVisitedTime());
  ASSERT_EQ(2u, listener_->visited_links_[3].GetRelativeLinks().size());

  // First relative link:
  const VisitedLink::RelativeLink& link30 =
      listener_->visited_links_[3].GetRelativeLinks()[0];
  EXPECT_EQ(
        "_methods=onPlusOne%2C_ready%2C_close%2C_open%2C_resizeMe%2C_renderstar"
        "t%2Concircled&id=I0_1352908652385&parent=http%3A%2F%2Fcode.google.com",
        link30.url_);
  EXPECT_EQ(1352908652, link30.last_visited_);

  // Second relative link:
  const VisitedLink::RelativeLink& link31 =
      listener_->visited_links_[3].GetRelativeLinks()[1];
  EXPECT_EQ(
        "_methods=onPlusOne%2C_ready%2C_close%2C_open%2C_resizeMe%2C_renderstar"
        "t%2Concircled&id=I0_1352908890424&parent=http%3A%2F%2Fcode.google.com",
        link31.url_);
  EXPECT_EQ(1352908890, link31.last_visited_);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
