// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <vector>

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "common/migration/session_importer.h"
#include "common/migration/sessions/session_import_listener.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/mock_bulk_file_reader.h"
#include "common/migration/tools/test_path_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using testing::Return;

namespace opera {
namespace common {
namespace migration {

class SessionImportListenerTestImpl : public SessionImportListener {
 public:
  SessionImportListenerTestImpl()
    : last_session_read_called_(false),
      stored_session_read_called_(false) {
  }

  void OnLastSessionRead(
      const std::vector<ParentWindow>& session,
      scoped_refptr<MigrationResultListener> listener) override {
    last_session_read_called_ = true;
    last_session_ = session;
  }

  void OnStoredSessionRead(
      const base::string16& name,
      const std::vector<ParentWindow>& session) override {
    stored_session_read_called_ = true;
    stored_name_ = name;
    stored_session_ = session;
  }

  bool last_session_read_called_;
  bool stored_session_read_called_;
  std::vector<ParentWindow> last_session_;
  std::vector<ParentWindow> stored_session_;
  base::string16 stored_name_;

 private:
  ~SessionImportListenerTestImpl() override {}
};

class SessionImporterTest : public ::testing::Test {
 public:
  SessionImporterTest()
    : reader_(new MockBulkFileReader()),
      session_listener_(new SessionImportListenerTestImpl()),
      migration_listener_(new MigrationResultListenerMock()),
      importer_(new SessionImporter(reader_, session_listener_)) {
  }

  void Import(std::unique_ptr<std::istream> input) {
    importer_->Import(std::move(input), migration_listener_);
  }

 protected:
  scoped_refptr<MockBulkFileReader> reader_;
  scoped_refptr<SessionImportListenerTestImpl> session_listener_;
  scoped_refptr<MigrationResultListenerMock> migration_listener_;
  scoped_refptr<SessionImporter> importer_;
};

TEST_F(SessionImporterTest, EmptyInput) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());

  // The importer will try reading the backup of the session.
  base::FilePath backup_session(FILE_PATH_LITERAL("autosave.win.bak"));
  EXPECT_CALL(*reader_, ReadFileContent(backup_session)).WillOnce(Return(""));

  EXPECT_CALL(*reader_, GetNextFile()).WillOnce(Return(base::FilePath()));
  EXPECT_CALL(*migration_listener_, OnImportFinished(opera::SESSIONS, false));
  Import(std::move(input));
  EXPECT_FALSE(session_listener_->last_session_read_called_);
}

void AssertSessionMatchesOneTabWin(const std::vector<ParentWindow>& session) {

  // There's one window
  ASSERT_EQ(1u, session.size());
  const ParentWindow& window = session[0];
  EXPECT_EQ(1940, window.x_);
  EXPECT_EQ(45, window.y_);
  EXPECT_EQ(1873, window.width_);
  EXPECT_EQ(1031, window.height_);
  EXPECT_EQ(ParentWindow::STATE_MAXIMIZED, window.state_);

  // The window has one tab
  ASSERT_EQ(1u, window.owned_tabs_.size());
  const TabSession& tab1 = window.owned_tabs_[0];
  EXPECT_EQ(0, tab1.x_);
  EXPECT_EQ(0, tab1.y_);
  EXPECT_EQ(1873, tab1.width_);
  EXPECT_EQ(982, tab1.height_);
  EXPECT_TRUE(tab1.active_);
  EXPECT_EQ("", tab1.encoding_);
  EXPECT_EQ(0, tab1.group_id_);
  EXPECT_FALSE(tab1.group_active_);
  EXPECT_EQ(100, tab1.scale_);
  EXPECT_FALSE(tab1.hidden_);
  EXPECT_TRUE(tab1.show_scrollbars_);
  EXPECT_EQ(0, tab1.tab_index_);
  EXPECT_EQ(0, tab1.stack_index_);

  // The tab has two entries in history
  ASSERT_EQ(2u, tab1.history_.size());
  EXPECT_EQ(GURL("opera:speeddial"), tab1.history_[0].url_);
  EXPECT_EQ(base::UTF8ToUTF16("Speed Dial"), tab1.history_[0].title_);
  EXPECT_EQ(0, tab1.history_[0].scroll_position_);

  EXPECT_EQ(GURL("http://portal.opera.com/"), tab1.history_[1].url_);
  EXPECT_EQ(base::UTF8ToUTF16("News Portal"), tab1.history_[1].title_);
  EXPECT_EQ(0, tab1.history_[1].scroll_position_);
}

/* Simple case with one open tab, two-element history (speed dial -> opera news).
 */
TEST_F(SessionImporterTest, OneTab) {
  base::FilePath path =
      ut::GetTestDataDir().AppendASCII("sessions").AppendASCII("one_tab.win");
  std::unique_ptr<std::ifstream> input(
      new std::ifstream(path.value().c_str(), std::ios_base::binary));
  ASSERT_TRUE(input->is_open());
  ASSERT_FALSE(input->fail());

  EXPECT_CALL(*reader_, GetNextFile()).WillOnce(Return(base::FilePath()));
  Import(std::move(input));
  EXPECT_FALSE(session_listener_->stored_session_read_called_);
  EXPECT_TRUE(session_listener_->last_session_read_called_);

  AssertSessionMatchesOneTabWin(session_listener_->last_session_);
}

/* Complex case: two windows, each with several open tabs. Two tab groups,
 * utf-8 encoded site title (aljazeera.net)
 */
// Some helper functions for the complex case
void CheckWindow1Tab1(const TabSession& tab) {
  EXPECT_EQ(0, tab.x_);
  EXPECT_EQ(0, tab.y_);
  EXPECT_EQ(1873, tab.width_);
  EXPECT_EQ(982, tab.height_);
  EXPECT_FALSE(tab.active_);
  EXPECT_EQ("iso-8859-1", tab.encoding_);  // Custom encoding
  EXPECT_EQ(1057, tab.group_id_);  // Is member of a group
  EXPECT_TRUE(tab.group_active_);
  EXPECT_EQ(100, tab.scale_);
  EXPECT_FALSE(tab.hidden_);
  EXPECT_TRUE(tab.show_scrollbars_);
  EXPECT_EQ(1, tab.tab_index_);
  EXPECT_EQ(1, tab.stack_index_);

  // Tab 1 has two entries in history
  ASSERT_EQ(2u, tab.history_.size());
  EXPECT_EQ(GURL("opera:speeddial"), tab.history_[0].url_);
  EXPECT_EQ(base::UTF8ToUTF16("Speed Dial"), tab.history_[0].title_);
  EXPECT_EQ(0, tab.history_[0].scroll_position_);

  EXPECT_EQ(GURL("http://my.opera.com/community/?utm_source=DesktopBrowser"),
            tab.history_[1].url_);
  EXPECT_EQ(base::UTF8ToUTF16("My Opera - Blogs and photos"), tab.history_[1].title_);
  EXPECT_EQ(0, tab.history_[1].scroll_position_);

  EXPECT_EQ(1u, tab.current_history_);
}

void CheckWindow1Tab2(const TabSession& tab) {
  EXPECT_EQ(0, tab.x_);
  EXPECT_EQ(0, tab.y_);
  EXPECT_EQ(1873, tab.width_);
  EXPECT_EQ(982, tab.height_);
  EXPECT_FALSE(tab.active_);
  EXPECT_EQ("", tab.encoding_);
  EXPECT_EQ(1057, tab.group_id_);  // Is member of a group
  EXPECT_FALSE(tab.group_active_);
  EXPECT_EQ(100, tab.scale_);
  EXPECT_TRUE(tab.hidden_);
  EXPECT_TRUE(tab.show_scrollbars_);
  EXPECT_EQ(1, tab.tab_index_);
  EXPECT_EQ(2, tab.stack_index_);

  ASSERT_EQ(2u, tab.history_.size());
  EXPECT_EQ(GURL("opera:speeddial"), tab.history_[0].url_);
  EXPECT_EQ(base::UTF8ToUTF16("Speed Dial"), tab.history_[0].title_);
  EXPECT_EQ(0, tab.history_[0].scroll_position_);

  EXPECT_EQ(GURL("http://portal.opera.com/"),
            tab.history_[1].url_);
  EXPECT_EQ(base::UTF8ToUTF16("News Portal"), tab.history_[1].title_);
  EXPECT_EQ(0, tab.history_[1].scroll_position_);

  EXPECT_EQ(1u, tab.current_history_);
}

void CheckWindow1Tab3(const TabSession& tab) {
  EXPECT_EQ(0, tab.x_);
  EXPECT_EQ(0, tab.y_);
  EXPECT_EQ(1873, tab.width_);
  EXPECT_EQ(982, tab.height_);
  EXPECT_TRUE(tab.active_);
  EXPECT_EQ("", tab.encoding_);
  EXPECT_EQ(0, tab.group_id_);
  EXPECT_FALSE(tab.group_active_);
  EXPECT_EQ(70, tab.scale_);
  EXPECT_FALSE(tab.hidden_);
  EXPECT_TRUE(tab.show_scrollbars_);
  EXPECT_EQ(1, tab.tab_index_);
  EXPECT_EQ(0, tab.stack_index_);

  ASSERT_EQ(3u, tab.history_.size());
  EXPECT_EQ(GURL("opera:speeddial"), tab.history_[0].url_);
  EXPECT_EQ(base::UTF8ToUTF16("Speed Dial"), tab.history_[0].title_);
  EXPECT_EQ(0, tab.history_[0].scroll_position_);

  EXPECT_EQ(GURL("http://www.cnn.com/?refresh=1"),
            tab.history_[1].url_);
  EXPECT_EQ(base::UTF8ToUTF16("CNN.com - Breaking News"), tab.history_[1].title_);
  EXPECT_EQ(378, tab.history_[1].scroll_position_);  // scroll position non-0

  EXPECT_EQ(GURL("http://www.cnn.com/WORLD/"),
            tab.history_[2].url_);
  EXPECT_EQ(base::UTF8ToUTF16("World News - International Headlines"),
            tab.history_[2].title_);
  EXPECT_EQ(896, tab.history_[2].scroll_position_);  // scroll position non-0

  EXPECT_EQ(1u, tab.current_history_);  // The last history entry is not current
}

void CheckWindow2Tab1(const TabSession& tab) {
  EXPECT_EQ(0, tab.x_);
  EXPECT_EQ(0, tab.y_);
  EXPECT_EQ(1873, tab.width_);
  EXPECT_EQ(982, tab.height_);
  EXPECT_TRUE(tab.active_);
  EXPECT_EQ("", tab.encoding_);
  EXPECT_EQ(0, tab.group_id_);
  EXPECT_FALSE(tab.group_active_);
  EXPECT_EQ(100, tab.scale_);
  EXPECT_FALSE(tab.hidden_);
  EXPECT_TRUE(tab.show_scrollbars_);
  EXPECT_EQ(0, tab.tab_index_);
  EXPECT_EQ(0, tab.stack_index_);

  // Tab 1 has two entries in history
  ASSERT_EQ(2u, tab.history_.size());
  EXPECT_EQ(GURL("opera:speeddial"), tab.history_[0].url_);
  EXPECT_EQ(base::UTF8ToUTF16("Speed Dial"), tab.history_[0].title_);
  EXPECT_EQ(0, tab.history_[0].scroll_position_);

  EXPECT_EQ(GURL("http://www.aljazeera.net/portal"),
            tab.history_[1].url_);
  /* Al Jazeera's title, not representable in ASCII */
  EXPECT_EQ(base::UTF8ToUTF16("\xD8\xA7\xD9\x84\xD8\xAC\xD8\xB2\xD9"
                              "\x8A\xD8\xB1\xD8\xA9\x2E\xD9\x86\xD8\xAA"),
            tab.history_[1].title_);
  EXPECT_EQ(0, tab.history_[1].scroll_position_);

  EXPECT_EQ(1u, tab.current_history_);
}

void AssertSessionMatchesAutosaveWin(const std::vector<ParentWindow>& session) {
  // There are two windows
  ASSERT_EQ(2u, session.size());

  const ParentWindow& window1 = session[0];
  EXPECT_EQ(1940, window1.x_);
  EXPECT_EQ(45, window1.y_);
  EXPECT_EQ(1873, window1.width_);
  EXPECT_EQ(1031, window1.height_);
  EXPECT_EQ(ParentWindow::STATE_MAXIMIZED, window1.state_);

  // The first window has 3 tabs
  ASSERT_EQ(3u, window1.owned_tabs_.size());
  CheckWindow1Tab1(window1.owned_tabs_[0]);
  CheckWindow1Tab2(window1.owned_tabs_[1]);
  CheckWindow1Tab3(window1.owned_tabs_[2]);

  const ParentWindow& window2 = session[1];
  EXPECT_EQ(1960, window2.x_);
  EXPECT_EQ(65, window2.y_);
  EXPECT_EQ(1873, window2.width_);
  EXPECT_EQ(1031, window2.height_);

  // The second window has one tab
  ASSERT_EQ(1u, window2.owned_tabs_.size());
  CheckWindow2Tab1(window2.owned_tabs_[0]);
}

TEST_F(SessionImporterTest, TwoWindows) {
  base::FilePath path =
      ut::GetTestDataDir().AppendASCII("sessions").AppendASCII("autosave.win");
  std::unique_ptr<std::ifstream> input(
      new std::ifstream(path.value().c_str(), std::ios_base::binary));
  ASSERT_TRUE(input->is_open());
  ASSERT_FALSE(input->fail());

  EXPECT_CALL(*reader_, GetNextFile()).WillOnce(Return(base::FilePath()));
  Import(std::move(input));
  EXPECT_FALSE(session_listener_->stored_session_read_called_);
  EXPECT_TRUE(session_listener_->last_session_read_called_);
  AssertSessionMatchesAutosaveWin(session_listener_->last_session_);
}

TEST_F(SessionImporterTest, StoredSessions) {
  base::FilePath path =
      ut::GetTestDataDir().AppendASCII("sessions").AppendASCII("autosave.win");
  std::unique_ptr<std::ifstream> input(
      new std::ifstream(path.value().c_str(), std::ios_base::binary));
  ASSERT_TRUE(input->is_open());
  ASSERT_FALSE(input->fail());

  base::FilePath stored_session_path =
      ut::GetTestDataDir().AppendASCII("sessions").AppendASCII("one_tab.win");
  EXPECT_CALL(*reader_, GetNextFile())
      .WillOnce(Return(stored_session_path))
      .WillOnce(Return(base::FilePath()));
  std::string one_tab_content;
  ASSERT_TRUE(base::ReadFileToString(stored_session_path,
                                     &one_tab_content));
  EXPECT_CALL(*reader_, ReadFileContent(stored_session_path))
      .WillOnce(Return(one_tab_content));
  Import(std::move(input));
  EXPECT_TRUE(session_listener_->stored_session_read_called_);
  EXPECT_TRUE(session_listener_->last_session_read_called_);

  AssertSessionMatchesAutosaveWin(session_listener_->last_session_);
  AssertSessionMatchesOneTabWin(session_listener_->stored_session_);
}

TEST_F(SessionImporterTest, StoredSessions_CorruptAutosaveWin) {
  std::unique_ptr<std::stringstream> corrupt_autosave_content(
      new std::stringstream());
  // We declare 7 windows but none are actually there
  *corrupt_autosave_content
      << "Opera Preferences version 2.1\n"
      << "; Do not edit this file while Opera is running\n"
      << "; This file is stored in UTF-8 encoding\n"
      << "\n"
      << "[session]\n"
      << "version=7000\n"
      << "window count=7\n";

  // The importer will try reading the backup of the session.
  // It's no good either.
  base::FilePath backup_session(FILE_PATH_LITERAL("autosave.win.bak"));
  EXPECT_CALL(*reader_, ReadFileContent(backup_session)).WillOnce(Return(""));

  base::FilePath stored_session_path =
      ut::GetTestDataDir().AppendASCII("sessions").AppendASCII("one_tab.win");
  EXPECT_CALL(*reader_, GetNextFile())
      .WillOnce(Return(stored_session_path))
      .WillOnce(Return(base::FilePath()));
  std::string one_tab_content;
  ASSERT_TRUE(base::ReadFileToString(stored_session_path,
                                     &one_tab_content));
  EXPECT_CALL(*reader_, ReadFileContent(stored_session_path))
      .WillOnce(Return(one_tab_content));
  EXPECT_CALL(*migration_listener_, OnImportFinished(opera::SESSIONS, false));
  Import(std::move(corrupt_autosave_content));
  EXPECT_TRUE(session_listener_->stored_session_read_called_);
  EXPECT_FALSE(session_listener_->last_session_read_called_);

  AssertSessionMatchesOneTabWin(session_listener_->stored_session_);
}
}  // namespace migration
}  // namespace common
}  // namespace opera
