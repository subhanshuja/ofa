// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <fstream>
#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "common/migration/migration_result_listener_mock.h"
#include "common/migration/password_importer.h"
#include "common/migration/tools/test_path_utils.h"

using ::testing::Test;
using ::testing::_;
using ::testing::SafeMatcherCast;
using ::testing::Invoke;
using ::testing::WithArg;
using opera::common::migration::ut::MigrationResultListenerMock;

namespace opera {
namespace common {
namespace migration {

class PasswordImporterListenerMock : public PasswordImporterListener {
 public:
  MOCK_METHOD2(OnMasterPasswordNeeded,
               void(uint32_t failed_attempts_count,
                    PasswordImporterListener::MasterPasswordContinuation));
  MOCK_METHOD2(OnWandParsed,
               void(const WandManager&,
                    scoped_refptr<MigrationResultListener>));

 private:
  virtual ~PasswordImporterListenerMock() {}
};

class PasswordImporterTest : public Test {
 public:
  PasswordImporterTest() :
      password_listener_(new PasswordImporterListenerMock()),
      migration_result_listener_(new MigrationResultListenerMock()) {
    base_path_ = ut::GetTestDataDir().AppendASCII("passwords");
  }

  // Retuns a WandManager expected for tests that use wand-single-login{-mp}.dat
  WandManager WandManagerForSingleLogin() const {
    WandManager manager;
    manager.current_profile_ = 0;
    WandLogin login;
    login.password_ = base::ASCIIToUTF16("test_password");
    login.username_ = base::ASCIIToUTF16("test_account");
    login.url_ = base::ASCIIToUTF16("*https://critic.oslo.osa/");
    manager.logins_.push_back(login);
    manager.log_profile_.name_ = base::ASCIIToUTF16("Log profile");
    WandProfile personal_profile;
    personal_profile.name_ = base::ASCIIToUTF16("Personal profile");
    WandPage page;
    page.document_x_ = 0;
    page.document_y_ = 0;
    page.flags_ = 0;
    page.form_number_ = 0;
    page.offset_x_ = 0;
    page.offset_y_ = 0;
    page.objects_.push_back(WandObject());
    page.objects_[0].is_changed_ = true;
    page.objects_[0].is_guessed_username_ = false;
    page.objects_[0].is_password_ = false;
    page.objects_[0].is_textfield_for_sure_ = false;
    page.objects_[0].name_ = base::ASCIIToUTF16("Ecom_SchemaVersion");
    page.objects_[0].value_ = base::ASCIIToUTF16(
        "http://www.ecml.org/version/1.1");
    personal_profile.pages_.push_back(page);
    manager.profiles_.push_back(personal_profile);
    return manager;
  }

  // Retuns a WandManager expected for tests that use wand-two-logins{-mp}.dat
  WandManager WandManagerForTwoLogins() const {
    WandManager manager = WandManagerForSingleLogin();
    WandLogin login;
    /* This login has unicode symbols embedded in the password and username */
    login.password_ = base::UTF8ToUTF16(
        "test_password\xE9\xB4\x89\xE7\x89\x87");
    login.username_ = base::UTF8ToUTF16("test_account\xE9\xB4\x89\xE7\x89\x87");
    login.url_ = base::ASCIIToUTF16("*https://critic.oslo.osa/");
    manager.logins_.push_back(login);
    return manager;
  }

  // Retuns a WandManager expected for tests that use wand-multiple{-mp}.dat
  WandManager WandManagerForMultiple() const {
    WandManager manager = WandManagerForTwoLogins();
    manager.log_profile_.pages_.push_back(WandPage());
    WandPage& facebook = manager.log_profile_.pages_.back();
    facebook.document_x_ = 1389;
    facebook.document_y_ = 42;
    facebook.flags_ = 256;
    facebook.form_number_ = 1;
    facebook.offset_x_ = 15;
    facebook.offset_y_ = 7;
    facebook.url_ = base::ASCIIToUTF16("http://www.facebook.com/");
    facebook.url_action_ = base::ASCIIToUTF16("https://www.facebook.com");
    {
      facebook.objects_.push_back(WandObject());
      WandObject& username = facebook.objects_.back();
      username.is_changed_ = true;
      username.is_guessed_username_ = true;
      username.is_password_ = false;
      username.is_textfield_for_sure_ = true;
      username.name_ = base::ASCIIToUTF16("email");
      username.value_ = base::UTF8ToUTF16(
          "test_account\xE9\xB4\x89\xE7\x89\x87");
      facebook.objects_.push_back(WandObject());
      WandObject& password = facebook.objects_.back();
      password.is_changed_ = true;
      password.is_guessed_username_ = false;
      password.is_password_ = true;
      password.is_textfield_for_sure_ = false;
      password.name_ = base::ASCIIToUTF16("pass");
      password.password_ = base::UTF8ToUTF16(
          "test_password\xE9\xB4\x89\xE7\x89\x87");
    }
    manager.log_profile_.pages_.push_back(WandPage());
    WandPage& google = manager.log_profile_.pages_.back();
    google.document_x_ = 1176;
    google.document_y_ = 326;
    google.flags_ = 256;
    google.form_number_ = 1;
    google.offset_x_ = 40;
    google.offset_y_ = 22;
    google.submitname_ = base::ASCIIToUTF16("signIn");
    google.url_ = base::ASCIIToUTF16(
        "https://accounts.google.com/ServiceLogin");
    google.url_action_ = base::ASCIIToUTF16("https://accounts.google.com");
    {
      google.objects_.push_back(WandObject());
      WandObject& username = google.objects_.back();
      username.is_changed_ = true;
      username.is_guessed_username_ = true;
      username.is_password_ = false;
      username.is_textfield_for_sure_ = true;
      username.name_ = base::ASCIIToUTF16("Email");
      username.value_ = base::UTF8ToUTF16(
          "test_account\xE9\xB4\x89\xE7\x89\x87");
      google.objects_.push_back(WandObject());
      WandObject& password = google.objects_.back();
      password.is_changed_ = true;
      password.is_guessed_username_ = false;
      password.is_password_ = true;
      password.is_textfield_for_sure_ = false;
      password.name_ = base::ASCIIToUTF16("Passwd");
      password.password_ = base::UTF8ToUTF16(
          "test_password\xE9\xB4\x89\xE7\x89\x87");
    }
    return manager;
  }

  void Import(const char wand_file_name[], const char cert_file_name[]) {
    std::unique_ptr<std::ifstream> input_file(new std::ifstream(
          base_path_.AppendASCII(wand_file_name).value().c_str(),
          std::ios_base::binary));
    ASSERT_TRUE(input_file->is_open());
    ASSERT_FALSE(input_file->fail());
    ASSERT_TRUE(base::PathExists(base_path_.AppendASCII(cert_file_name)));

    scoped_refptr<SslPasswordReader> ssl_reader(
          new SslPasswordReader(base_path_.AppendASCII(cert_file_name)));
    scoped_refptr<PasswordImporter> importer(
          new PasswordImporter(password_listener_, ssl_reader));
    importer->Import(std::move(input_file), migration_result_listener_);
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

  struct SupplyMasterPassword {
    SupplyMasterPassword(const char master_password_utf8[], bool cancel) :
        password_(base::UTF8ToUTF16(master_password_utf8)),
        cancel_(cancel) {
    }

    typedef PasswordImporterListener::MasterPasswordContinuation Continuation;
    void operator()(Continuation continuation_callback) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
            FROM_HERE,
            base::Bind(continuation_callback, password_, cancel_));
    }
    base::string16 password_;
    bool cancel_;
  };

  base::FilePath base_path_;
  scoped_refptr<PasswordImporterListenerMock> password_listener_;
  scoped_refptr<MigrationResultListenerMock> migration_result_listener_;
  base::MessageLoop message_loop_;
};

TEST_F(PasswordImporterTest, NoData) {
  std::unique_ptr<std::stringstream> input(new std::stringstream());
  scoped_refptr<PasswordImporter> importer(
        new PasswordImporter(password_listener_, NULL));
  /* Note, we've passed NULL as the SslPasswordReader. This is legal but means
   * we won't be able to decode wand data encrypted using a master password. */

  // We expect to receive a false result, the file should never be empty.
  EXPECT_CALL(*migration_result_listener_,
              OnImportFinished(opera::PASSWORDS, false));
  importer->Import(std::move(input), migration_result_listener_);
}

TEST_F(PasswordImporterTest, SingleLogin) {
  /* A simple case, only one login, not guarded by a master password. */
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForSingleLogin(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-single-login.dat", "opcert6-basic.dat");
}

TEST_F(PasswordImporterTest, SingleLoginWithMasterPassword) {
  /* The OnMasterPasswordNeeded method should be called with a continuation
   * functor. We'll invoke the functor and supply it with our master password,
   * 'cancel' is set to false
   *
   * 'secret_master_password' is the password encoded in opcert6-basic.dat.
   * wand-single-login-mp.dat is encrypted using this key.
   */
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(0, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword("secret_master_password", false))));
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForSingleLogin(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-single-login-mp.dat", "opcert6-basic.dat");
}

TEST_F(PasswordImporterTest, TwoLogins) {
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForTwoLogins(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-two-logins.dat", "opcert6-unicode.dat");
}

TEST_F(PasswordImporterTest, TwoLoginsWithMasterPassword) {
  /* The master password in opcert6-unicode.dat contains Chinese characters and
   * it stored in UTF16. The wand file wand-two-logins-mp.dat is encrypted using
   * this key. Additionally, the second login in wand-two-logins-mp.dat
   * also has Chinese characters, so we're checking unicode all around.
   */
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(0, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword(
              "\xE9\xB4\x89\xE7\x89\x87\xE9\xB4\x89\xE7"
              "\x89\x87\xE9\xB4\x89\xE7\x89\x87\x31\x34",
              false))));
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForTwoLogins(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-two-logins-mp.dat", "opcert6-unicode.dat");
}

TEST_F(PasswordImporterTest, MasterPasswordOkAfterThreeTries) {
  /* Several attempts at coming up with the correct password. */
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(0, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword("wrong password", false))));
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(1, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword("can't remember the right one", false))));

  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(2, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword("is it '12345'?", false))));

  // This is the correct one
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(3, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword(
              "\xE9\xB4\x89\xE7\x89\x87\xE9\xB4\x89\xE7"
              "\x89\x87\xE9\xB4\x89\xE7\x89\x87\x31\x34",
              false))));
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForTwoLogins(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-two-logins-mp.dat", "opcert6-unicode.dat");
}

TEST_F(PasswordImporterTest, CancelMasterPassword) {
  /* The user choses not to give his master password, import aborted. */
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(0, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword("", true))));  // 'cancel' is true
  EXPECT_CALL(*migration_result_listener_,
              OnImportFinished(opera::PASSWORDS, false));  // Abort
  Import("wand-two-logins-mp.dat", "opcert6-unicode.dat");
}

TEST_F(PasswordImporterTest, MultipleLoginsAndObjects) {
  /* Two logins, two objects (facebook and google login forms) */
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForMultiple(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-multiple.dat", "opcert6-unicode.dat");
}

TEST_F(PasswordImporterTest, MultipleLoginsAndObjectsWithMasterPassword) {
  EXPECT_CALL(*password_listener_, OnMasterPasswordNeeded(0, _)).
      WillOnce(
        WithArg<1>(
          Invoke(
            SupplyMasterPassword(
              "\xE9\xB4\x89\xE7\x89\x87\xE9\xB4\x89\xE7"
              "\x89\x87\xE9\xB4\x89\xE7\x89\x87\x31\x34",
              false))));
  EXPECT_CALL(*password_listener_, OnWandParsed(
                WandManagerForMultiple(),
                SafeMatcherCast<scoped_refptr<MigrationResultListener> >(
                  migration_result_listener_)));
  Import("wand-multiple-mp.dat", "opcert6-unicode.dat");
}

}  // namespace migration
}  // namespace common
}  // namespace opera
