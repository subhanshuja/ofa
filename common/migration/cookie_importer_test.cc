// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <stdint.h>
#include <sstream>
#include <fstream>
#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "net/cookies/cookie_store.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "common/migration/cookie_importer.h"
#include "common/migration/cookies/cookie_import_tags.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/tools/data_stream_test_utils.h"
#include "common/migration/tools/test_path_utils.h"

using opera::common::migration::ut::MigrationResultListenerMock;
using opera::common::migration::ut::StreamInserter;
using ::testing::Test;
using ::testing::_;

namespace opera {
namespace common {
namespace migration {

class CookieCallbackMock {
 public:
  MOCK_METHOD2(SetCookie, void(
                 const GURL& url,
                 const std::string& cookie_line));
};

namespace {

DataStreamReader::Spec CreateDefaultSpec() {
  // Values taken from http://www.opera.com/docs/operafiles/#cookies
  DataStreamReader::Spec spec = { 4096, 8192, 1, 2 };
  return spec;
}
}  // anonymous namespace

class CookieImporterTest : public ::testing::Test {
 protected:
  CookieCallback GetCallback() {
    return base::Bind(&CookieCallbackMock::SetCookie,
                      base::Unretained(&callback_mock_));
  }
  void ExpectResult(bool result, std::istream* input) {
    scoped_refptr<CookieImporter> importer =
        new CookieImporter(GetCallback());
    scoped_refptr<MigrationResultListenerMock> migration_result_listener =
        new MigrationResultListenerMock();
    EXPECT_CALL(*migration_result_listener,
                OnImportFinished(opera::COOKIES, result));
    importer->Import(base::WrapUnique(input), migration_result_listener);
  }

  void ExpectCookie(const std::string& url, const std::string& line) {
    EXPECT_CALL(callback_mock_, SetCookie(GURL(url), line));
  }

  CookieCallbackMock callback_mock_;
};

TEST_F(CookieImporterTest, InvalidSpec_BadFileVersion) {
  DataStreamReader::Spec spec = CreateDefaultSpec();
  spec.file_version = 4095;  // Should be 4096

  std::stringstream* datastream = new std::stringstream();

  StreamInserter inserter(true /* reverse endianness */);
  inserter.Insert(spec, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, InvalidSpec_BadAppVersion) {
  DataStreamReader::Spec spec = CreateDefaultSpec();
  spec.app_version = 0x1003;  // Major ver. 1, minor ver. 3, should be 2.x

  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  inserter.Insert(spec, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, InvalidSpec_BadTagLength) {
  DataStreamReader::Spec spec = CreateDefaultSpec();
  spec.tag_length = 2;  // Should be 1

  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  inserter.Insert(spec, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, InvalidSpec_BadLenLength) {
  DataStreamReader::Spec spec = CreateDefaultSpec();
  spec.len_length = 1;  // Should be 2

  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  inserter.Insert(spec, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, InvalidSpec_InputTooShort) {
  int32_t dummy_number = 5;  // Input will contain only one int, instead of spec
  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  inserter.Insert(dummy_number, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, InvalidTagAfterSpec) {
  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  DataStreamReader::Spec spec = CreateDefaultSpec();
  inserter.Insert(spec, datastream);
  uint8_t tag = TAG_COOKIE_ACCEPTED_AS_THIRDPARTY;
  inserter.Insert(tag, datastream);

  ExpectResult(false, datastream);
}

TEST_F(CookieImporterTest, NoDomains) {
  std::stringstream* datastream = new std::stringstream();
  StreamInserter inserter(true /* reverse endianness */);
  DataStreamReader::Spec spec = CreateDefaultSpec();
  inserter.Insert(spec, datastream);

  // Even an empty cookie file has a domain end tag (right after the header)
  uint8_t tag = TAG_COOKIE_DOMAIN_END;
  inserter.Insert(tag, datastream);

  ExpectResult(true, datastream);
}

TEST_F(CookieImporterTest, ActualData) {
  base::FilePath cookie_path = ut::GetTestDataDir().AppendASCII("cookies4.dat");
  std::unique_ptr<std::ifstream> cookie_file(
      new std::ifstream(cookie_path.value().c_str(), std::ios_base::binary));
  ASSERT_TRUE(cookie_file->is_open());
  ASSERT_FALSE(cookie_file->fail());

  ExpectCookie("http://tradedoubler.com",
      "TD_POOL=1t9h**RjsM*PjXs**2UfEO*1;"
      " Expires=Thu, 24-Oct-2013 13:46:47 GMT; Domain=.tradedoubler.com");
  ExpectCookie("http://tradedoubler.com",
      "TD_PIC=1283607*tTu*5tHr*1FKdC*1t9h***1*1GXzKd*1GYdWd*();"
      " Expires=Fri, 26-Oct-2012 13:46:47 GMT; Domain=.tradedoubler.com");
  ExpectCookie("http://tradedoubler.com",
      "BT=z11zz1Fiz2RlfETzzzz9yW1n3BWa; Expires=Thu, 24-Oct-2013 13:46:47 GMT; "
      "Domain=.tradedoubler.com");
  ExpectCookie("http://tradedoubler.com",
      "TD_UNIQUE_IMP=227192a6783286b220907a6740116;"
      " Expires=Thu, 24-Oct-2013 13:46:48 GMT; Domain=.tradedoubler.com");

  ExpectCookie("http://my2.adocean.pl",
      "GAD=KlxNuGoGvaGpI8Reo6V8sZMGG8WDG8m8EMQpazMj8718EeamXvwm1MGGQaGGYSg78JFc"
      "8e5GsG..; Expires=Wed, 30-Aug-2017 00:00:00 GMT; "
      "Domain=.my2.adocean.pl");

  ExpectCookie("http://dot.adtotal.pl",
      "statid=91.241.2.4.4882:1351086407:1059032027:v1;"
      " Expires=Sat, 24-Oct-2015 13:46:47 GMT");

  ExpectCookie("http://hit.gemius.pl",
      "Gdyn=KlSxgQsGQMGGMnWm4jmGoeQGazMj8iMoFRxSG7BLMSyGuFaCYlMMGSoswOx1qSFnSG8"
      ".; Expires=Wed, 30-Aug-2017 00:00:00 GMT; Domain=.hit.gemius.pl");

  ExpectCookie("http://wp.pl",
      "statid=91.241.2.4.16954:1351086406:3300266106:v1;"
      " Expires=Sat, 24-Oct-2015 13:46:46 GMT; Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "gusid=b5314b3b7140085fc63f505cc051f659;"
      " Expires=Sat, 24-Oct-2015 13:46:47 GMT; Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "rgr=none; Expires=Wed, 24-Oct-2012 19:46:47 GMT; Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "wpg=616%3B7%3B41483; Expires=Thu, 25-Oct-2012 13:46:47 GMT; "
      "Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "rekticket=1351086407; Expires=Fri, 26-Oct-2012 13:46:47 GMT; "
      "Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "OAX=W/ECBFCH8UcACd/N; Expires=Thu, 31-Dec-2020 23:59:59 GMT; "
      "Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "__vrf=13510864090886OTwvpGrMTqE3fBITwJ538XYnM9rpW2O;"
      " Expires=Wed, 24-Oct-2012 14:46:49 GMT; Domain=.wp.pl");
  ExpectCookie("http://wp.pl",
      "__vru=http%3A//www.wp.pl/; Expires=Wed, 24-Oct-2012 14:16:49 GMT; "
      "Domain=.wp.pl");
  ExpectCookie("http://www.wp.pl",
      "rekticket=1351086407; Expires=Thu, 25-Oct-2012 13:46:47 GMT; "
      "Domain=.www.wp.pl");
  ExpectCookie("http://www.wp.pl",
      "camps=v1j78FAAEAx0KJUOC%2FBQABAMcrkVA%3D;"
      " Expires=Sat, 19-Oct-2013 13:46:47 GMT; Domain=.www.wp.pl");
  ExpectCookie("http://www.wp.pl",
      "reksticket=1351086407; Expires=Fri, 26-Oct-2012 13:46:47 GMT; "
      "Domain=.www.wp.pl");
  ExpectCookie("http://x.wpimg.pl",
      "statid=91.241.2.4.30442:1351086406:2788917950:v1;"
      " Expires=Sat, 24-Oct-2015 13:46:46 GMT");

  ExpectResult(true, cookie_file.release());
}

}  // namespace migration
}  // namespace common
}  // namespace opera
