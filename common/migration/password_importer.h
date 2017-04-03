// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_PASSWORD_IMPORTER_H_
#define COMMON_MIGRATION_PASSWORD_IMPORTER_H_

#include <stdint.h>
#include <vector>

#include "common/migration/importer.h"
#include "common/migration/wand/wand_manager.h"
#include "common/migration/wand/ssl_password_reader.h"
#include "common/ini_parser/ini_parser.h"
#include "base/memory/weak_ptr.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/strings/string16.h"

namespace opera {
namespace common {
namespace migration {

class PasswordImporterListener
    : public base::RefCountedThreadSafe<PasswordImporterListener> {
 public:
  /** This continuation callback should be invoked in response to
   * OnMasterPasswordNeeded. It must be invoked in the same thread that calls
   * OnMasterPasswordNeeded, so if you're delegating the password prompt to
   * the UI thread, use MessageLoop::PostTaskAndReply or similar.
   * The first argument is the password received from user input (if any),
   * the second is true if the user has refused to give his master password.
   * If the second argument is true, importing passwords is aborted.
   */
  typedef base::Callback<void(base::string16 password, bool cancelled)>
  MasterPasswordContinuation;

  /** Called if passwrods are encrypted with a master password which needs
   * to be given by the user.
   * The implementer should ask the user (ex. through a dialog box) for the
   * master password. This may be an asynchronous operation.
   * The caller will not block on this call, instead a continuation callback is
   * supplied. It must be called when the password is received or when the
   * user decides not to supply it.
   * @param failed_attempts_count Starts from 0. If the password supplied in
   * the continuation_callback is incorrect, this method will be called again
   * with an incremented @a failed_attempts_count.
   * @param continuation_callback Callback to invoke upon receiving the
   * password.
   */
  virtual void OnMasterPasswordNeeded(
      uint32_t failed_attempts_count,
      MasterPasswordContinuation continuation_callback) = 0;

  /** Called after successful parsing.
   * @param wand_manager Contains a hierarchy of Wand items that hold
   * sites, forms, logins and decrypted passwords.
   * @param result_listener Call OnImportFinished() with an appropriate status
   * after applying the parsed data (true if applied successfully, false
   * otherwise).
   */
  virtual void OnWandParsed(
      const WandManager& wand_manager,
      scoped_refptr<MigrationResultListener> result_listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PasswordImporterListener>;
  virtual ~PasswordImporterListener() {}
};

class PasswordImporter
    : public Importer,
      public base::SupportsWeakPtr<PasswordImporter> {
 public:
  PasswordImporter(
      scoped_refptr<PasswordImporterListener> password_listener,
      scoped_refptr<SslPasswordReader> ssl_password_reader);

  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> result_listener) override;

 protected:
  ~PasswordImporter() override;

  void ImportImplementation();

  void AskForMasterPassword();

  void ContinueWithMasterPassword(base::string16 master_password,
                                  bool cancelled);

  void ContinueWithObfuscationKey(const std::vector<uint8_t>& key);

  scoped_refptr<PasswordImporterListener> password_listener_;
  scoped_refptr<SslPasswordReader> ssl_password_reader_;

  /** A data-holder for per-Import objects.
   */
  struct ImportContext;
  std::unique_ptr<ImportContext> context_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_PASSWORD_IMPORTER_H_
