// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/passwords_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"


#if defined(OPERA_SYNC)
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "components/sync/base/nigori.h"
#endif  // OPERA_SYNC

using passwords_helper::AddLogin;
using passwords_helper::CreateTestPasswordForm;
using passwords_helper::GetPasswordCount;
using passwords_helper::GetPasswordStore;
using passwords_helper::GetVerifierPasswordCount;
using passwords_helper::GetVerifierPasswordStore;
using passwords_helper::ProfileContainsSamePasswordFormsAsVerifier;

using autofill::PasswordForm;

class SingleClientPasswordsSyncTest : public SyncTest {
 public:
  SingleClientPasswordsSyncTest() : SyncTest(SINGLE_CLIENT) {}
  ~SingleClientPasswordsSyncTest() override {}

#if defined(OPERA_SYNC)
  void OperaGetKeyNameAndBlobFromSyncEntity(
      const sync_pb::SyncEntity& sync_entity,
      std::string* key_name, std::string* blob) {
    ASSERT_TRUE(key_name && blob);
    sync_pb::EntitySpecifics entity_specifics;
    sync_pb::PasswordSpecifics password_specifics;
    sync_pb::EncryptedData encrypted_data;
    sync_pb::PasswordSpecificsData password_sd;

    ASSERT_TRUE(sync_entity.has_specifics());
    entity_specifics = sync_entity.specifics();
    ASSERT_TRUE(entity_specifics.has_password());
    password_specifics = entity_specifics.password();

    ASSERT_FALSE(password_specifics.has_client_only_encrypted_data());

    ASSERT_TRUE(password_specifics.has_encrypted());
    encrypted_data = password_specifics.encrypted();
    ASSERT_TRUE(encrypted_data.has_key_name());
    ASSERT_TRUE(encrypted_data.has_blob());
    *key_name = encrypted_data.key_name();
    *blob = encrypted_data.blob();
  }

  void OperaDecryptAndVerify(const sync_pb::SyncEntity& sync_entity,
      const PasswordForm& password_form,
      const std::string& passphrase) {
    // We derive a Nigori key from the given passphrase.
    // The hostname (localhost) and username (dummy) are
    // hardcoded in the Chromium implementation.
    // Also the type (Nigori::Password) and name (nigori-key)
    // are fixed.
    std::string key_name, blob;
    OperaGetKeyNameAndBlobFromSyncEntity(sync_entity, &key_name, &blob);
    ASSERT_NE(key_name, std::string());
    ASSERT_NE(blob, std::string());

    syncer::Nigori passphrase_nigori;
    std::string passphrase_key_name;
    ASSERT_TRUE(passphrase_nigori.InitByDerivation("localhost", "dummy",
        passphrase));
    ASSERT_TRUE(passphrase_nigori.Permute(syncer::Nigori::Password,
        "nigori-key", &passphrase_key_name));

    ASSERT_EQ(key_name, passphrase_key_name);
    std::string decrypted;
    ASSERT_TRUE(passphrase_nigori.Decrypt(blob, &decrypted));
    sync_pb::PasswordSpecificsData password_sd;
    ASSERT_TRUE(password_sd.ParseFromString(decrypted));

    ASSERT_EQ(password_form.origin.spec(), password_sd.origin());
    ASSERT_EQ(password_form.signon_realm, password_sd.signon_realm());
    const base::string16& form_username_value =
        password_form.username_value;
    const base::string16& form_password_value =
        password_form.password_value;
    const base::string16& server_username_value =
        base::ASCIIToUTF16(password_sd.username_value());
    const base::string16& server_password_value =
        base::ASCIIToUTF16(password_sd.password_value());
    ASSERT_EQ(form_username_value, server_username_value);
    ASSERT_EQ(form_password_value, server_password_value);
  }
#endif  // OPERA_SYNC

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientPasswordsSyncTest);
};

IN_PROC_BROWSER_TEST_F(SingleClientPasswordsSyncTest, Sanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  ASSERT_EQ(1, GetVerifierPasswordCount());
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  ASSERT_TRUE(ProfileContainsSamePasswordFormsAsVerifier(0));
  ASSERT_EQ(1, GetPasswordCount(0));
}

#if defined(OPERA_SYNC)
IN_PROC_BROWSER_TEST_F(SingleClientPasswordsSyncTest, OperaServerEncrypted) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  PasswordForm form = CreateTestPasswordForm(0);
  AddLogin(GetVerifierPasswordStore(), form);
  ASSERT_EQ(1, GetVerifierPasswordCount());
  AddLogin(GetPasswordStore(0), form);
  ASSERT_EQ(1, GetPasswordCount(0));

  ASSERT_TRUE(AwaitCommitActivityCompletion(GetSyncService((0))));
  ASSERT_TRUE(ProfileContainsSamePasswordFormsAsVerifier(0));
  ASSERT_EQ(1, GetPasswordCount(0));

  std::vector<sync_pb::SyncEntity> server_entities = GetFakeServer()->
    GetSyncEntitiesByModelType(syncer::ModelType::PASSWORDS);
  ASSERT_EQ(server_entities.size(), 1U);
  OperaDecryptAndVerify(server_entities[0], form, password_);
}
#endif  // OPERA_SYNC
