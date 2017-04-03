// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/password_importer.h"

#include <string>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "common/migration/wand/wand_reader.h"
#include "common/migration/wand/wand_manager.h"
#include "common/presto/password_crypto/migration_master_password.h"
#include "common/presto/password_crypto/migration_password_encryption.h"

// For password encryption
using opera::presto_port::CryptoMasterPasswordEncryption;
using opera::presto_port::CryptoMasterPasswordHandler;

namespace opera {
namespace common {
namespace migration {

struct PasswordImporter::ImportContext {
  ImportContext(std::unique_ptr<std::istream> input,
                scoped_refptr<MigrationResultListener> result_listener) :
      input_(std::move(input)),
      result_listener_(result_listener),
      failed_master_password_attempts_count_(0),
      wand_version_(0) {
  }

  ~ImportContext() {
  }

  std::unique_ptr<std::istream> input_;
  scoped_refptr<MigrationResultListener> result_listener_;
  uint32_t failed_master_password_attempts_count_;
  int32_t wand_version_;
  CryptoMasterPasswordHandler password_handler_;
};

PasswordImporter::PasswordImporter(
    scoped_refptr<PasswordImporterListener> password_listener,
    scoped_refptr<SslPasswordReader> ssl_password_reader) :
    password_listener_(password_listener),
    ssl_password_reader_(ssl_password_reader) {
}

PasswordImporter::~PasswordImporter() {
}

void PasswordImporter::Import(std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> result_listener) {
  context_ = base::WrapUnique(
      new ImportContext(std::move(input), result_listener));
  ImportImplementation();
}

void PasswordImporter::ImportImplementation() {
  BinaryReader reader(context_->input_.get());
  context_->wand_version_ = reader.Read<int32_t>();
  int32_t encrypted_by_master_password = reader.Read<int32_t>();

  if (context_->input_->fail()) {
    LOG(ERROR) << "Could not read wand version and encryption status";
    context_->result_listener_->OnImportFinished(opera::PASSWORDS, false);
    return;
  }

  if (context_->wand_version_ < 2 || context_->wand_version_  > 6) {
    LOG(ERROR) << "Cannot import profiles using wand version "
               << context_->wand_version_;
    context_->result_listener_->OnImportFinished(opera::PASSWORDS, false);
    return;
  }

  if (!encrypted_by_master_password) {
    // Use built-in obfuscation key
    ContinueWithObfuscationKey(
          context_->password_handler_.GetObfuscationPasswordEncryptionKey());
  } else {
    // We cannot decrypt without user's master password
    AskForMasterPassword();
  }
}

void PasswordImporter::AskForMasterPassword() {
  // First, we need to read the system part password and salt from opcert6.dat
  if (!ssl_password_reader_ || !ssl_password_reader_->Read()) {
    LOG(ERROR) << "Could not read parts of the password/salt from opcert6.dat";
    context_->result_listener_->OnImportFinished(opera::PASSWORDS, false);
    return;
  }

  /* Now we need to set the system part password and salt in the password
   * handler.
   */
  context_->password_handler_.SetMasterPasswordCheckCode(
        ssl_password_reader_->GetSystemPartPasswordSalt(),
        ssl_password_reader_->GetSystemPartPassword());

  /* Ready to decode, just need the master password now. Ask the listener. */
  PasswordImporterListener::MasterPasswordContinuation continuation =
      base::Bind(&PasswordImporter::ContinueWithMasterPassword,
                 AsWeakPtr());
  password_listener_->OnMasterPasswordNeeded(
        context_->failed_master_password_attempts_count_,
        continuation);
}

void PasswordImporter::ContinueWithMasterPassword(
    base::string16 master_password,
    bool cancelled) {
  if (cancelled) {
    LOG(INFO) << "User aborted, did not supply master password";
    context_->result_listener_->OnImportFinished(opera::PASSWORDS, false);
    return;
  }
  std::string tmp = base::UTF16ToUTF8(master_password);
  std::vector<uint8_t> master_password8(tmp.begin(), tmp.end());
  if (!context_->password_handler_.CheckUserProvidedMasterPassword(
        master_password8)) {
    // The password is incorrect, ask again
    context_->failed_master_password_attempts_count_++;
    AskForMasterPassword();
    return;
  }
  ContinueWithObfuscationKey(
        context_->password_handler_.GetUserProvidedPasswordEncryptionKey(
          master_password8));
}

void PasswordImporter::ContinueWithObfuscationKey(
    const std::vector<uint8_t>& key) {
  WandManager manager;
  WandReader reader(key, context_->input_.get());
  bool success = manager.Parse(&reader, context_->wand_version_);
  if (success) {
    password_listener_->OnWandParsed(manager, context_->result_listener_);
  } else {
    context_->result_listener_->OnImportFinished(opera::PASSWORDS, false);
  }
}

}  // namespace migration
}  // namespace common
}  // namespace opera
