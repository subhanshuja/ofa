// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_INTERNAL_API_OPERA_SYNC_ENCRYPTION_HANDLER_IMPL_H_
#define SYNC_INTERNAL_API_OPERA_SYNC_ENCRYPTION_HANDLER_IMPL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/sync/base/cryptographer.h"
#include "components/sync/core/sync_encryption_handler.h"
#include "components/sync/syncable/nigori_handler.h"

namespace syncer {

class Encryptor;
struct UserShare;
class WriteNode;
class WriteTransaction;

// Sync encryption handler implementation.
//
// This class acts as the respository of all sync encryption state, and handles
// encryption related changes/queries coming from both the chrome side and
// the sync side (via NigoriHandler). It is capable of modifying all sync data
// (re-encryption), updating the encrypted types, changing the encryption keys,
// and creating/receiving nigori node updates.
//
// The class should live as long as the directory itself in order to ensure
// any data read/written is properly decrypted/encrypted.
//
// Note: See sync_encryption_handler.h for a description of the chrome visible
// methods and what they do, and nigori_handler.h for a description of the
// sync methods.
// All methods are non-thread-safe and should only be called from the sync
// thread unless explicitly noted otherwise.
class OperaSyncEncryptionHandlerImpl : public SyncEncryptionHandler,
                                       public syncable::NigoriHandler {
 public:
   OperaSyncEncryptionHandlerImpl(
      UserShare* user_share,
      Encryptor* encryptor,
      const std::string& restored_key_for_bootstrapping,
      const std::string& restored_keystore_key_for_bootstrapping);
   ~OperaSyncEncryptionHandlerImpl() override;

  // SyncEncryptionHandler implementation.
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  void Init() override;
  void SetEncryptionPassphrase(const std::string& passphrase,
                               bool is_explicit) override;
  void SetDecryptionPassphrase(const std::string& passphrase) override;
  void EnableEncryptEverything() override;
  bool IsEncryptEverythingEnabled() const override;

  // NigoriHandler implementation.
  // Note: all methods are invoked while the caller holds a transaction.
  void ApplyNigoriUpdate(const sync_pb::NigoriSpecifics& nigori,
                         syncable::BaseTransaction* const trans) override;
  void UpdateNigoriFromEncryptedTypes(
      sync_pb::NigoriSpecifics* nigori,
      syncable::BaseTransaction* const trans) const override;
  bool NeedKeystoreKey(syncable::BaseTransaction* const trans) const override;
  // Can be called from any thread.
  ModelTypeSet GetEncryptedTypes(
      syncable::BaseTransaction* const trans) const override;
  PassphraseType GetPassphraseType(
      syncable::BaseTransaction* const trans) const override;
  bool SetKeystoreKeys(
      const google::protobuf::RepeatedPtrField<google::protobuf::string>& keys,
      syncable::BaseTransaction* const trans) override;

  // Unsafe getters. Use only if sync is not up and running and there is no risk
  // of other threads calling this.
  Cryptographer* GetCryptographerUnsafe();
  ModelTypeSet GetEncryptedTypesUnsafe();

  base::Time migration_time() const;
  base::Time custom_passphrase_time() const;

  // Restore a saved nigori obtained from OnLocalSetPassphraseEncryption.
  //
  // Writes the nigori to the Directory and updates the Cryptographer.
  void RestoreNigori(const SyncEncryptionHandler::NigoriState& nigori_state);

 private:
  FRIEND_TEST_ALL_PREFIXES(SyncEncryptionHandlerImplTest,
                           NigoriEncryptionTypes);
  FRIEND_TEST_ALL_PREFIXES(SyncEncryptionHandlerImplTest,
                           EncryptEverythingExplicit);
  FRIEND_TEST_ALL_PREFIXES(SyncEncryptionHandlerImplTest,
                           EncryptEverythingImplicit);
  FRIEND_TEST_ALL_PREFIXES(SyncEncryptionHandlerImplTest,
                           UnknownSensitiveTypes);

  // Container for members that require thread safety protection.  All members
  // that can be accessed from more than one thread should be held here and
  // accessed via UnlockVault(..) and UnlockVaultMutable(..), which enforce
  // that a transaction is held.
  struct Vault {
    Vault(Encryptor* encryptor,
          ModelTypeSet encrypted_types,
          PassphraseType passphrase_type);
    ~Vault();

    // Sync's cryptographer. Used for encrypting and decrypting sync data.
    Cryptographer cryptographer;
    // The set of types that require encryption.
    ModelTypeSet encrypted_types;
    // The current state of the passphrase required to decrypt the encryption
    // keys stored in the nigori node.
    PassphraseType passphrase_type;

   private:
    DISALLOW_COPY_AND_ASSIGN(Vault);
  };

  // Iterate over all encrypted types ensuring each entry is properly encrypted.
  void ReEncryptEverything(WriteTransaction* trans);

  // Apply a nigori update. Updates internal and cryptographer state.
  // Returns true on success, false if |nigori| was incompatible, and the
  // nigori node must be corrected.
  // Note: must be called from within a transaction.
  bool ApplyNigoriUpdateImpl(const sync_pb::NigoriSpecifics& nigori,
                             syncable::BaseTransaction* const trans);

  // Wrapper around WriteEncryptionStateToNigori that creates a new write
  // transaction.
  void RewriteNigori();

  // Write the current encryption state into the nigori node. This includes
  // the encrypted types/encrypt everything state, as well as the keybag/
  // explicit passphrase state (if the cryptographer is ready).
  void WriteEncryptionStateToNigori(WriteTransaction* trans);

  // Updates local encrypted types from |nigori|.
  // Returns true if the local set of encrypted types either matched or was
  // a subset of that in |nigori|. Returns false if the local state already
  // had stricter encryption than |nigori|, and the nigori node needs to be
  // updated with the newer encryption state.
  // Note: must be called from within a transaction.
  bool UpdateEncryptedTypesFromNigori(
      const sync_pb::NigoriSpecifics& nigori,
      syncable::BaseTransaction* const trans);

  // The final step of SetEncryptionPassphrase and SetDecryptionPassphrase that
  // notifies observers of the result of the set passphrase operation, updates
  // the nigori node, and does re-encryption.
  // |success|: true if the operation was successful and false otherwise. If
  //            success == false, we send an OnPassphraseRequired notification.
  // |bootstrap_token|: used to inform observers if the cryptographer's
  //                    bootstrap token was updated.
  // |is_explicit|: used to differentiate between a custom passphrase (true) and
  //                a GAIA passphrase that is implicitly used for encryption
  //                (false).
  // |trans| and |nigori_node|: used to access data in the cryptographer.
  void FinishSetPassphrase(bool success,
                           const std::string& bootstrap_token,
                           bool is_explicit,
                           WriteTransaction* trans,
                           WriteNode* nigori_node);

  // Merges the given set of encrypted types with the existing set and emits a
  // notification if necessary.
  // Note: must be called from within a transaction.
  void MergeEncryptedTypes(ModelTypeSet new_encrypted_types,
                           syncable::BaseTransaction* const trans);

  // Helper methods for ensuring transactions are held when accessing
  // |vault_unsafe_|.
  Vault* UnlockVaultMutable(syncable::BaseTransaction* const trans);
  const Vault& UnlockVault(syncable::BaseTransaction* const trans) const;

  // If an explicit passphrase is in use, returns the time at which it was set
  // (if known). Else return base::Time().
  base::Time GetExplicitPassphraseTime() const;

  base::ThreadChecker thread_checker_;

  base::ObserverList<SyncEncryptionHandler::Observer> observers_;

  // The current user share (for creating transactions).
  UserShare* user_share_;

  // Container for all data that can be accessed from multiple threads. Do not
  // access this object directly. Instead access it via UnlockVault(..) and
  // UnlockVaultMutable(..).
  Vault vault_unsafe_;

  // Sync encryption state that is only modified and accessed from the sync
  // thread.
  // Whether all current and future types should be encrypted.
  bool encrypt_everything_;
  // The current state of the passphrase required to decrypt the encryption
  // keys stored in the nigori node.
  PassphraseType passphrase_type_;

  // The number of times we've automatically (i.e. not via SetPassphrase or
  // conflict resolver) updated the nigori's encryption keys in this chrome
  // instantiation.
  int nigori_overwrite_count_;

  // The time the custom passphrase was set for this account. Not valid
  // if there is no custom passphrase or the custom passphrase was set
  // before support for this field was added.
  base::Time custom_passphrase_time_;

  base::Time migration_time_;

  base::WeakPtrFactory<OperaSyncEncryptionHandlerImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OperaSyncEncryptionHandlerImpl);
};

}  // namespace syncer

#endif  // SYNC_INTERNAL_API_OPERA_SYNC_ENCRYPTION_HANDLER_IMPL_H_
