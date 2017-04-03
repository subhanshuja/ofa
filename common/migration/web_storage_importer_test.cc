// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <sstream>
#include <fstream>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/bulk_file_reader.h"
#include "common/migration/tools/mock_bulk_file_reader.h"
#include "common/migration/tools/test_path_utils.h"
#include "common/migration/web_storage_listener.h"
#include "common/migration/web_storage_importer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"


using opera::common::migration::ut::MigrationResultListenerMock;
using ::testing::Test;
using ::testing::Return;
using ::testing::Contains;
using ::testing::_;

namespace opera {
namespace common {
namespace migration {

class WebStorageListenerMock : public WebStorageListener {
 public:
  WebStorageListenerMock() {}
  MOCK_METHOD2(OnUrlImported, bool(
      const std::string& url,
      const std::vector<KeyValuePair>& kayValuePairs));
 private:
  virtual ~WebStorageListenerMock() {}
};

TEST(WebStorageImporterTest, EmptyPsindex) {
  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, NULL, false);

  std::unique_ptr<std::stringstream> input(new std::stringstream());
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, false));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, OneItemEmptyFile) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  *input << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  *input << "<preferences>\n";
  *input << "  <section id=\"C37F702C324684E0A1D726FB3CF6250152476FEE\">\n";
  *input << "    <value id=\"Type\" xml:space=\"preserve\">";
  *input << "localstorage</value>\n";
  *input << "    <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "http://www.cnn.com</value>\n";
  *input << "    <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/00/15/00000000</value>\n";
  *input << "  </section>\n";
  *input << "</preferences>";

  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/00/15/00000000")))).
      WillOnce(Return(""));  // Empty content
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, NULL, false);

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, false));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, Unicode) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  *input << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  *input << "<preferences>\n";
  *input << "  <section id=\"C37F702C324684E0A1D726FB3CF6250152476FEE\">\n";
  *input << "    <value id=\"Type\" xml:space=\"preserve\">";
  *input << "localstorage</value>\n";
  *input << "    <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "http://www.cnn.com</value>\n";
  *input << "    <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/00/12/00000000</value>\n";
  *input << "  </section>\n";
  *input << "</preferences>\n";

  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/00/12/00000000")))).
      WillOnce(Return("<ws>\n"
                      "<e><k>BAE=</k>\n"
                      "<v>5wA=</v></e>\n"
                      "</ws>"));

  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, listener, false);

  WebStorageListener::KeyValuePair expected_key_value =
      std::make_pair(base::string16(1, 0x104), base::string16(1, 0xE7));

  EXPECT_CALL(*listener, OnUrlImported("http://www.cnn.com",
                                       Contains(expected_key_value))).
      WillOnce(Return(true));

  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, true));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, TwoItems) {
  // Base64 encoded input
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  *input << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  *input << "<preferences>\n";
  *input << "  <section id=\"C37F702C324684E0A1D726FB3CF6250152476FEE\">\n";
  *input << "    <value id=\"Type\" xml:space=\"preserve\">";
  *input << "localstorage</value>\n";
  *input << "    <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "http://www.cnn.com</value>\n";
  *input << "    <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/00/15/00000000</value>\n";
  *input << "  </section>";
  *input << "  <section id=\"9B9DAFA97CF8F0596408E0B9BC44F285DAF6C6CB\">\n";
  *input << "    <value id=\"Type\" xml:space=\"preserve\">";
  *input << "localstorage</value>\n";
  *input << "    <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "http://stackoverflow.com</value>\n";
  *input << "    <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/00/02/00000000</value>\n";
  *input << "   </section>\n";
  *input << "</preferences>";

  std::stringstream content02;
  content02 << "<ws>\n";
  content02 << "<e><k>bgB1AEMAbwB1AG4AdABlAHIA</k>\n";
  // This long string is some JSON encoded in base 64
  content02 <<
      "<v>ewAiAG4AcwBnAFQAZQBzAHQAUwB0AG8AcgBhAGcAZQAiADoAdAByAHUAZQAsACI"
      "AbgBzAGcARABlAGYAYQB1AGwAdABCAG8AeABlAHMATwByAGQAZQByACIAOgB7ACIAb"
      "gBlAHcAcwBCAG8AeAAxACIAOgBbACIAYgBvAHgATgBlAHcAcwAiACwAIgBiAG8AeAB"
      "TAHAAbwByAHQAIgAsACIAYgBvAHgARQBjAG8AbgBvAG0AeQAiACwAIgBiAG8AeABMA"
      "G8AYwBhAGwATgBlAHcAcwAiACwAIgBiAG8AeABUAGUAYwBoACIALAAiAGIAbwB4AE0"
      "AbwB0AG8AIgBdAH0ALAAiAG4AcwBnAEwAbwBjAGEAbABOAGUAdwBzAEMAcwByAEkAZ"
      "AAiADoAIgB3AGEAcgBzAHoAYQB3AGEAIgB9AA==</v></e>\n";
  content02 << "</ws>";

  std::stringstream content15;
  content15 << "<ws>\n";
  content15 << "<e><k>XwBjAGIAXwBjAHAA</k>\n";
  content15 << "<v></v></e>\n";
  content15 << "</ws>";

  // Decoded input
  std::string content02key("nuCounter");
  std::string content02value(
      "{\"nsgTestStorage\":true,\"nsgDefaultBoxesOrder\""
      ":{\"newsBox1\":[\"boxNews\",\"boxSport\",\"boxEconomy\""
      ",\"boxLocalNews\",\"boxTech\",\"boxMoto\"]}"
      ",\"nsgLocalNewsCsrId\":\"warszawa\"}");
  std::string content15key("_cb_cp");
  std::string content15value("");

  WebStorageListener::KeyValuePair content02kv =
      std::make_pair(base::ASCIIToUTF16(content02key),
                     base::ASCIIToUTF16(content02value));
  WebStorageListener::KeyValuePair content15kv =
      std::make_pair(base::ASCIIToUTF16(content15key),
                     base::ASCIIToUTF16(content15value));

  // The reader will be asked to provide contents of the two DataFiles mentioned
  // in the index file
  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/00/15/00000000")))).
      WillOnce(Return(content15.str()));
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/00/02/00000000")))).
      WillOnce(Return(content02.str()));

  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, listener, false);

  // The listener will be notified of these two new origin URLs along with
  // key-value pairs
  EXPECT_CALL(*listener, OnUrlImported("http://www.cnn.com",
                                       Contains(content15kv))).
      WillOnce(Return(true));
  EXPECT_CALL(*listener, OnUrlImported("http://stackoverflow.com",
                                       Contains(content02kv))).
      WillOnce(Return(true));
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, true));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, RealData) {
  // Using a real reader
  base::FilePath base_dir = ut::GetTestDataDir();
  ASSERT_TRUE(base::PathExists(base_dir));
  scoped_refptr<BulkFileReader> reader = new BulkFileReader(base::FilePath(base_dir));
  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();
  EXPECT_CALL(*listener, OnUrlImported("http://www.cnn.com", _)).
      WillOnce(Return(true));
  EXPECT_CALL(*listener, OnUrlImported("http://stackoverflow.com", _)).
      WillOnce(Return(true));

  scoped_refptr<WebStorageImporter> importer
      = new WebStorageImporter(reader, listener, false);

  base::FilePath index_file =
      base_dir.AppendASCII("pstorage").AppendASCII("psindex.dat");
  std::unique_ptr<std::ifstream> input(new std::ifstream(index_file.value().c_str()));
  ASSERT_TRUE(input->is_open());
  ASSERT_TRUE(input->good());
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, true));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, ExtensionMissingType) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  *input << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  *input << "<preferences>\n";
  *input << " <section id=\"8AA12B3D611A067EFD8B6E7DDD6B143C8FDA93A8\">\n";
  *input << "  <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "widget://wuid-3f77ae75-5dea-6f41-8b6b-f3c4a72b5d33</value>\n";
  *input << "  <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage\\00\\01\\00000000</value>\n";
  *input << " </section>\n";
  *input << "</preferences>\n";

  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, listener, true);
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, false));
  importer->Import(std::move(input), migration_result_listener);
}


TEST(WebStorageImporterTest, ExtensionWidgetPrefsAndLocalStorage) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  *input << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  *input << "<preferences>\n";
  *input << " <section id=\"8AA12B3D611A067EFD8B6E7DDD6B143C8FDA93A8\">\n";
  *input << "  <value id=\"Type\" xml:space=\"preserve\">localstorage</value>";
  *input << "  <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "widget://wuid-3f77ae75-5dea-6f41-8b6b-f3c4a72b5d33</value>\n";
  *input << "  <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/00/01/00000000</value>\n";
  *input << " </section>\n";
  *input << " <section id=\"84DE05EAA1665044A315F935F55B1B38B6ADDE52\">\n";
  *input << "  <value id=\"Type\" xml:space=\"preserve\">";
  *input << "widgetpreferences</value>\n";
  *input << "  <value id=\"Origin\" xml:space=\"preserve\">";
  *input << "widget://wuid-3f77ae75-5dea-6f41-8b6b-f3c4a72b5d33</value>\n";
  *input << "  <value id=\"DataFile\" xml:space=\"preserve\">";
  *input << "pstorage/03/01/00000000</value>\n";
  *input << "  </section>\n";
  *input << "</preferences>\n";

  std::stringstream content_widget;
  content_widget << "<ws>" << "<e>";
  content_widget << "<k>dwBpAGQAZwBlAHQASwBlAHkA</k>";
  content_widget << "<v>dwBpAGQAZwBlAHQAVgBhAGwAdQBlAA==</v>";
  content_widget << "</e>" << "</ws>";

  std::stringstream content_local;
  content_local << "<ws>" << "<e>";
  content_local << "<k>bABvAGMAYQBsAEsAZQB5AA==</k>";
  content_local << "<v>bABvAGMAYQBsAFYAYQBsAHUAZQA=</v>";
  content_local << "</e>" << "</ws>";

  WebStorageListener::KeyValuePair contentLocalPair =
      std::make_pair(base::ASCIIToUTF16("localKey"),
                     base::ASCIIToUTF16("localValue"));
  WebStorageListener::KeyValuePair contentWidgetPair =
      std::make_pair(base::ASCIIToUTF16("widgetKey"),
                     base::ASCIIToUTF16("widgetValue"));

  // The reader will be asked to provide contents of the two DataFiles mentioned
  // in the index file
  scoped_refptr<MockBulkFileReader> reader = new MockBulkFileReader();
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/00/01/00000000")))).
      WillOnce(Return(content_local.str()));
  EXPECT_CALL(*reader, ReadFileContent(
                base::FilePath(FILE_PATH_LITERAL("pstorage/03/01/00000000")))).
      WillOnce(Return(content_widget.str()));

  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();
  scoped_refptr<WebStorageImporter> importer =
      new WebStorageImporter(reader, listener, true);

  // The listener will be notified of these two new origin URLs along with
  // key-value pairs
  EXPECT_CALL(*listener, OnUrlImported("localstorage",
                                       Contains(contentLocalPair))).
      WillOnce(Return(true));
  EXPECT_CALL(*listener, OnUrlImported("widgetpreferences",
                                       Contains(contentWidgetPair))).
      WillOnce(Return(true));
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, true));
  importer->Import(std::move(input), migration_result_listener);
}

TEST(WebStorageImporterTest, ExtensionsRealData) {
  // Using a real reader
  base::FilePath base_dir = ut::GetTestDataDir().
      AppendASCII("extensions").
      AppendASCII("test_extension_id");
  ASSERT_TRUE(base::PathExists(base_dir));
  scoped_refptr<BulkFileReader> reader = new BulkFileReader(base::FilePath(base_dir));
  scoped_refptr<WebStorageListenerMock> listener =
      new WebStorageListenerMock();

  WebStorageListener::KeyValuePair contentLocalPair =
      std::make_pair(base::ASCIIToUTF16("localKey"),
                     base::ASCIIToUTF16("localValue"));
  WebStorageListener::KeyValuePair contentWidgetPair =
      std::make_pair(base::ASCIIToUTF16("widgetKey"),
                     base::ASCIIToUTF16("widgetValue"));

  EXPECT_CALL(*listener, OnUrlImported("widgetpreferences", Contains(contentWidgetPair))).
      WillOnce(Return(true));
  EXPECT_CALL(*listener, OnUrlImported("localstorage", Contains(contentLocalPair))).
      WillOnce(Return(true));

  scoped_refptr<WebStorageImporter> importer
      = new WebStorageImporter(reader, listener, true);

  base::FilePath index_file =
      base_dir.AppendASCII("pstorage").AppendASCII("psindex.dat");
  std::unique_ptr<std::ifstream> input(new std::ifstream(index_file.value().c_str()));
  ASSERT_TRUE(input->is_open());
  ASSERT_TRUE(input->good());
  scoped_refptr<MigrationResultListenerMock> migration_result_listener =
      new MigrationResultListenerMock();
  EXPECT_CALL(*migration_result_listener,
              OnImportFinished(opera::WEB_STORAGE, true));
  importer->Import(std::move(input), migration_result_listener);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
