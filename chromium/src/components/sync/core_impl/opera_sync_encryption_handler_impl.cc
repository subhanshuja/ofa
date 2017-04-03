// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/core_impl/opera_sync_encryption_handler_impl.h"

#include <queue>
#include <string>

#include "base/base64.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/tracked_objects.h"
#include "components/sync/base/cryptographer.h"
#include "components/sync/base/experiments.h"
#include "components/sync/core/read_node.h"
#include "components/sync/core/read_transaction.h"
#include "components/sync/core/user_share.h"
#include "components/sync/core/write_node.h"
#include "components/sync/core/write_transaction.h"
#include "components/sync/protocol/encryption.pb.h"
#include "components/sync/protocol/nigori_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/nigori_util.h"
#include "components/sync/syncable/syncable_base_transaction.h"
#include "components/sync/util/opera_tag_node_mapping.h"

namespace syncer {

namespace {
// The maximum number of times we will automatically overwrite the nigori node
// because the encryption keys don't match (per chrome instantiation).
// We protect ourselves against nigori rollbacks, but it's possible two
// different clients might have contrasting view of what the nigori node state
// should be, in which case they might ping pong (see crbug.com/119207).
static const int kNigoriOverwriteLimit = 10;
}

OperaSyncEncryptionHandlerImpl::Vault::Vault(Encryptor* encryptor,
                                             ModelTypeSet encrypted_types,
                                             PassphraseType passphrase_type)
    : cryptographer(encryptor),
      encrypted_types(encrypted_types),
      passphrase_type(passphrase_type) {}

OperaSyncEncryptionHandlerImpl::Vault::~Vault() {}

OperaSyncEncryptionHandlerImpl::OperaSyncEncryptionHandlerImpl(
    UserShare* user_share,
    Encryptor* encryptor,
    const std::string& restored_key_for_bootstrapping,
    const std::string& restored_keystore_key_for_bootstrapping)
    : user_share_(user_share),
      vault_unsafe_(encryptor,
                    SensitiveTypes(),
                    PassphraseType::IMPLICIT_PASSPHRASE),
      encrypt_everything_(false),
      nigori_overwrite_count_(0),
      weak_ptr_factory_(this) {
  DCHECK(restored_keystore_key_for_bootstrapping.empty());
  // We only bootstrap the user provided passphrase. The keystore key is handled
  // at Init time once we're sure the nigori is downloaded.
  vault_unsafe_.cryptographer.Bootstrap(restored_key_for_bootstrapping);
}

OperaSyncEncryptionHandlerImpl::~OperaSyncEncryptionHandlerImpl() {}

void OperaSyncEncryptionHandlerImpl::AddObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!observers_.HasObserver(observer));
  observers_.AddObserver(observer);
}

void OperaSyncEncryptionHandlerImpl::RemoveObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(observers_.HasObserver(observer));
  observers_.RemoveObserver(observer);
}

void OperaSyncEncryptionHandlerImpl::Init() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  WriteNode node(&trans);

  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK)
    return;
  if (!ApplyNigoriUpdateImpl(node.GetNigoriSpecifics(),
                             trans.GetWrappedTrans())) {
    WriteEncryptionStateToNigori(&trans);
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Sync.PassphraseType",
      static_cast<unsigned>(GetPassphraseType(trans.GetWrappedTrans())),
      static_cast<unsigned>(PassphraseType::PASSPHRASE_TYPE_SIZE));

  // Always trigger an encrypted types and cryptographer state change event at
  // init time so observers get the initial values.
  FOR_EACH_OBSERVER(Observer, observers_,
                    OnEncryptedTypesChanged(
                        UnlockVault(trans.GetWrappedTrans()).encrypted_types,
                        encrypt_everything_));
  FOR_EACH_OBSERVER(
      SyncEncryptionHandler::Observer, observers_,
      OnCryptographerStateChanged(
          &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer));

  // If the cryptographer is not ready (either it has pending keys or we
  // failed to initialize it), we don't want to try and re-encrypt the data.
  // If we had encrypted types, the DataTypeManager will block, preventing
  // sync from happening until the the passphrase is provided.
  if (UnlockVault(trans.GetWrappedTrans()).cryptographer.is_ready())
    ReEncryptEverything(&trans);
}

void OperaSyncEncryptionHandlerImpl::SetEncryptionPassphrase(
    const std::string& passphrase,
    bool is_explicit) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We do not accept empty passphrases.
  if (passphrase.empty()) {
    NOTREACHED() << "Cannot encrypt with an empty passphrase.";
    return;
  }

  // All accesses to the cryptographer are protected by a transaction.
  WriteTransaction trans(FROM_HERE, user_share_);
  KeyParams key_params = {"localhost", "dummy", passphrase};
  WriteNode node(&trans);
  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK) {
    NOTREACHED();
    return;
  }

  bool nigori_has_explicit_passphrase =
      node.GetNigoriSpecifics().op_keybag_is_frozen();
  std::string bootstrap_token;
  sync_pb::EncryptedData pending_keys;
  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer;

  if (cryptographer->has_pending_keys())
    pending_keys = cryptographer->GetPendingKeys();
  bool success = false;

  // There are six cases to handle here:
  // 1. The user has no pending keys and is setting their current GAIA password
  //    as the encryption passphrase. This happens either during first time sync
  //    with a clean profile, or after re-authenticating on a profile that was
  //    already signed in with the cryptographer ready.
  // 2. The user has no pending keys, and is overwriting an (already provided)
  //    implicit passphrase with an explicit (custom) passphrase.
  // 3. The user has pending keys for an explicit passphrase that is somehow set
  //    to their current GAIA passphrase.
  // 4. The user has pending keys encrypted with their current GAIA passphrase
  //    and the caller passes in the current GAIA passphrase.
  // 5. The user has pending keys encrypted with an older GAIA passphrase
  //    and the caller passes in the current GAIA passphrase.
  // 6. The user has previously done encryption with an explicit passphrase.
  // Furthermore, we enforce the fact that the bootstrap encryption token will
  // always be derived from the newest GAIA password if the account is using
  // an implicit passphrase (even if the data is encrypted with an old GAIA
  // password). If the account is using an explicit (custom) passphrase, the
  // bootstrap token will be derived from the most recently provided explicit
  // passphrase (that was able to decrypt the data).
  if (!nigori_has_explicit_passphrase) {
    if (!cryptographer->has_pending_keys()) {
      if (cryptographer->AddKey(key_params)) {
        // Case 1 and 2. We set a new GAIA passphrase when there are no pending
        // keys (1), or overwriting an implicit passphrase with a new explicit
        // one (2) when there are no pending keys.
        DVLOG(1) << "Setting " << (is_explicit ? "explicit" : "implicit" )
                 << " passphrase for encryption.";
        cryptographer->GetBootstrapToken(&bootstrap_token);

        // With M26, sync accounts can be in only one of two encryption states:
        // 1) Encrypt only passwords with an implicit passphrase.
        // 2) Encrypt all sync datatypes with an explicit passphrase.
        // We deprecate the "EncryptAllData" and "CustomPassphrase" histograms,
        // and keep track of an account's encryption state via the
        // "CustomEncryption" histogram. See http://crbug.com/131478.
        UMA_HISTOGRAM_BOOLEAN("Sync.CustomEncryption", is_explicit);
        success = true;
      } else {
        NOTREACHED() << "Failed to add key to cryptographer.";
        success = false;
      }
    } else {  // cryptographer->has_pending_keys() == true
      if (is_explicit) {
        // This can only happen if the nigori node is updated with a new
        // implicit passphrase while a client is attempting to set a new custom
        // passphrase (race condition).
        DVLOG(1) << "Failing because an implicit passphrase is already set.";
        success = false;
      } else {  // is_explicit == false
        if (cryptographer->DecryptPendingKeys(key_params)) {
          // Case 4. We successfully decrypted with the implicit GAIA passphrase
          // passed in.
          DVLOG(1) << "Implicit internal passphrase accepted for decryption.";
          cryptographer->GetBootstrapToken(&bootstrap_token);
          success = true;
        } else {
          // Case 5. Encryption was done with an old GAIA password, but we were
          // provided with the current GAIA password. We need to generate a new
          // bootstrap token to preserve it. We build a temporary cryptographer
          // to allow us to extract these params without polluting our current
          // cryptographer.
          DVLOG(1) << "Implicit internal passphrase failed to decrypt, adding "
                   << "anyways as default passphrase and persisting via "
                   << "bootstrap token.";
          Cryptographer temp_cryptographer(cryptographer->encryptor());
          temp_cryptographer.AddKey(key_params);
          temp_cryptographer.GetBootstrapToken(&bootstrap_token);
          // We then set the new passphrase as the default passphrase of the
          // real cryptographer, even though we have pending keys. This is safe,
          // as although Cryptographer::is_initialized() will now be true,
          // is_ready() will remain false due to having pending keys.
          cryptographer->AddKey(key_params);
          success = false;
        }
      }  // is_explicit
    }  // cryptographer->has_pending_keys()
  } else {  // nigori_has_explicit_passphrase == true
    // Case 6. We do not want to override a previously set explicit passphrase,
    // so we return a failure.
    DVLOG(1) << "Failing because an explicit passphrase is already set.";
    success = false;
  }

  DVLOG_IF(1, !success)
      << "Failure in SetEncryptionPassphrase; notifying and returning.";
  DVLOG_IF(1, success)
      << "Successfully set encryption passphrase; updating nigori and "
         "reencrypting.";

  FinishSetPassphrase(success, bootstrap_token, is_explicit, &trans, &node);
}

void OperaSyncEncryptionHandlerImpl::SetDecryptionPassphrase(
    const std::string& passphrase) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We do not accept empty passphrases.
  if (passphrase.empty()) {
    NOTREACHED() << "Cannot decrypt with an empty passphrase.";
    return;
  }

  // All accesses to the cryptographer are protected by a transaction.
  WriteTransaction trans(FROM_HERE, user_share_);
  KeyParams key_params = {"localhost", "dummy", passphrase};
  WriteNode node(&trans);
  if (node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK) {
    NOTREACHED();
    return;
  }

  Cryptographer* cryptographer =
      &UnlockVaultMutable(trans.GetWrappedTrans())->cryptographer;
  if (!cryptographer->has_pending_keys()) {
    // Note that this *can* happen in a rare situation where data is
    // re-encrypted on another client while a SetDecryptionPassphrase() call is
    // in-flight on this client. It is rare enough that we choose to do nothing.
    NOTREACHED() << "Attempt to set decryption passphrase failed because there "
                 << "were no pending keys.";
    return;
  }

  bool nigori_has_explicit_passphrase =
      node.GetNigoriSpecifics().op_keybag_is_frozen();
  std::string bootstrap_token;
  sync_pb::EncryptedData pending_keys;
  pending_keys = cryptographer->GetPendingKeys();
  bool success = false;

  // There are three cases to handle here:
  // 7. We're using the current GAIA password to decrypt the pending keys. This
  //    happens when signing in to an account with a previously set implicit
  //    passphrase, where the data is already encrypted with the newest GAIA
  //    password.
  // 8. The user is providing an old GAIA password to decrypt the pending keys.
  //    In this case, the user is using an implicit passphrase, but has changed
  //    their password since they last encrypted their data, and therefore
  //    their current GAIA password was unable to decrypt the data. This will
  //    happen when the user is setting up a new profile with a previously
  //    encrypted account (after changing passwords).
  // 9. The user is providing a previously set explicit passphrase to decrypt
  //    the pending keys.
  if (!nigori_has_explicit_passphrase) {
    if (cryptographer->is_initialized()) {
      // We only want to change the default encryption key to the pending
      // one if the pending keybag already contains the current default.
      // This covers the case where a different client re-encrypted
      // everything with a newer gaia passphrase (and hence the keybag
      // contains keys from all previously used gaia passphrases).
      // Otherwise, we're in a situation where the pending keys are
      // encrypted with an old gaia passphrase, while the default is the
      // current gaia passphrase. In that case, we preserve the default.
      Cryptographer temp_cryptographer(cryptographer->encryptor());
      temp_cryptographer.SetPendingKeys(cryptographer->GetPendingKeys());
      if (temp_cryptographer.DecryptPendingKeys(key_params)) {
        // Check to see if the pending bag of keys contains the current
        // default key.
        sync_pb::EncryptedData encrypted;
        cryptographer->GetKeys(&encrypted);
        if (temp_cryptographer.CanDecrypt(encrypted)) {
          DVLOG(1) << "Implicit user provided passphrase accepted for "
                   << "decryption, overwriting default.";
          // Case 7. The pending keybag contains the current default. Go ahead
          // and update the cryptographer, letting the default change.
          cryptographer->DecryptPendingKeys(key_params);
          cryptographer->GetBootstrapToken(&bootstrap_token);
          success = true;
        } else {
          // Case 8. The pending keybag does not contain the current default
          // encryption key. We decrypt the pending keys here, and in
          // FinishSetPassphrase, re-encrypt everything with the current GAIA
          // passphrase instead of the passphrase just provided by the user.
          DVLOG(1) << "Implicit user provided passphrase accepted for "
                   << "decryption, restoring implicit internal passphrase "
                   << "as default.";
          std::string bootstrap_token_from_current_key;
          cryptographer->GetBootstrapToken(&bootstrap_token_from_current_key);
          cryptographer->DecryptPendingKeys(key_params);
          // Overwrite the default from the pending keys.
          cryptographer->AddKeyFromBootstrapToken(
              bootstrap_token_from_current_key);
          success = true;
        }
      } else {  // !temp_cryptographer.DecryptPendingKeys(..)
        DVLOG(1) << "Implicit user provided passphrase failed to decrypt.";
        success = false;
      }  // temp_cryptographer.DecryptPendingKeys(...)
    } else {  // cryptographer->is_initialized() == false
      if (cryptographer->DecryptPendingKeys(key_params)) {
        // This can happpen in two cases:
        // - First time sync on android, where we'll never have a
        //   !user_provided passphrase.
        // - This is a restart for a client that lost their bootstrap token.
        // In both cases, we should go ahead and initialize the cryptographer
        // and persist the new bootstrap token.
        //
        // Note: at this point, we cannot distinguish between cases 7 and 8
        // above. This user provided passphrase could be the current or the
        // old. But, as long as we persist the token, there's nothing more
        // we can do.
        cryptographer->GetBootstrapToken(&bootstrap_token);
        DVLOG(1) << "Implicit user provided passphrase accepted, initializing"
                 << " cryptographer.";
        success = true;
      } else {
        DVLOG(1) << "Implicit user provided passphrase failed to decrypt.";
        success = false;
      }
    }  // cryptographer->is_initialized()
  } else {  // nigori_has_explicit_passphrase == true
    // Case 9. Encryption was done with an explicit passphrase, and we decrypt
    // with the passphrase provided by the user.
    if (cryptographer->DecryptPendingKeys(key_params)) {
      DVLOG(1) << "Explicit passphrase accepted for decryption.";
      cryptographer->GetBootstrapToken(&bootstrap_token);
      success = true;
    } else {
      DVLOG(1) << "Explicit passphrase failed to decrypt.";
      success = false;
    }
  }  // nigori_has_explicit_passphrase

  DVLOG_IF(1, !success)
      << "Failure in SetDecryptionPassphrase; notifying and returning.";
  DVLOG_IF(1, success)
      << "Successfully set decryption passphrase; updating nigori and "
         "reencrypting.";

  FinishSetPassphrase(success, bootstrap_token, nigori_has_explicit_passphrase,
                      &trans, &node);
}

void OperaSyncEncryptionHandlerImpl::EnableEncryptEverything() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  ModelTypeSet* encrypted_types =
      &UnlockVaultMutable(trans.GetWrappedTrans())->encrypted_types;
  if (encrypt_everything_) {
    DCHECK(*encrypted_types == EncryptableUserTypes());
    return;
  }
  DVLOG(1) << "Enabling encrypt everything.";
  encrypt_everything_ = true;
  // Change |encrypted_types_| directly to avoid sending more than one
  // notification.
  *encrypted_types = EncryptableUserTypes();
  FOR_EACH_OBSERVER(
      Observer, observers_,
      OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_));
  WriteEncryptionStateToNigori(&trans);
  if (UnlockVault(trans.GetWrappedTrans()).cryptographer.is_ready())
    ReEncryptEverything(&trans);
}

bool OperaSyncEncryptionHandlerImpl::IsEncryptEverythingEnabled() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return encrypt_everything_;
}

// Note: this is called from within a syncable transaction, so we need to post
// tasks if we want to do any work that creates a new sync_api transaction.
void OperaSyncEncryptionHandlerImpl::ApplyNigoriUpdate(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(trans);
  if (!ApplyNigoriUpdateImpl(nigori, trans)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&OperaSyncEncryptionHandlerImpl::RewriteNigori,
                              weak_ptr_factory_.GetWeakPtr()));
  }

  FOR_EACH_OBSERVER(
      SyncEncryptionHandler::Observer, observers_,
      OnCryptographerStateChanged(&UnlockVaultMutable(trans)->cryptographer));
}

void OperaSyncEncryptionHandlerImpl::UpdateNigoriFromEncryptedTypes(
    sync_pb::NigoriSpecifics* nigori,
    syncable::BaseTransaction* const trans) const {
  syncable::UpdateNigoriFromEncryptedTypes(UnlockVault(trans).encrypted_types,
                                           encrypt_everything_, nigori);
}

bool OperaSyncEncryptionHandlerImpl::NeedKeystoreKey(
    syncable::BaseTransaction* const trans) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

ModelTypeSet OperaSyncEncryptionHandlerImpl::GetEncryptedTypes(
    syncable::BaseTransaction* const trans) const {
  return UnlockVault(trans).encrypted_types;
}

PassphraseType OperaSyncEncryptionHandlerImpl::GetPassphraseType(
    syncable::BaseTransaction* const trans) const {
  return UnlockVault(trans).passphrase_type;
}

bool OperaSyncEncryptionHandlerImpl::SetKeystoreKeys(
    const google::protobuf::RepeatedPtrField<google::protobuf::string>& keys,
    syncable::BaseTransaction* const trans) {
  LOG(WARNING) << "Server unexpectedly sent " << keys.size()
               << " encryption keys, ignoring.";
  return true;
 }

Cryptographer* OperaSyncEncryptionHandlerImpl::GetCryptographerUnsafe() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return &vault_unsafe_.cryptographer;
}

ModelTypeSet OperaSyncEncryptionHandlerImpl::GetEncryptedTypesUnsafe() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return vault_unsafe_.encrypted_types;
}

base::Time OperaSyncEncryptionHandlerImpl::migration_time() const {
  return migration_time_;
}

base::Time OperaSyncEncryptionHandlerImpl::custom_passphrase_time() const {
  return custom_passphrase_time_;
}

void OperaSyncEncryptionHandlerImpl::RestoreNigori(
    const SyncEncryptionHandler::NigoriState& nigori_state) {}

// This function iterates over all encrypted types.  There are many scenarios in
// which data for some or all types is not currently available.  In that case,
// the lookup of the root node will fail and we will skip encryption for that
// type.
void OperaSyncEncryptionHandlerImpl::ReEncryptEverything(
    WriteTransaction* trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(UnlockVault(trans->GetWrappedTrans()).cryptographer.is_ready());

  std::vector<std::string> bookmark_root_server_tags;
  // The original Chromium nodes.
  bookmark_root_server_tags.push_back("google_chrome_bookmarks");
  bookmark_root_server_tags.push_back("bookmark_bar");
  bookmark_root_server_tags.push_back("synced_bookmarks");
  bookmark_root_server_tags.push_back("other_bookmarks");

  opera_sync::TagNodeMappingVector v = opera_sync::tag_node_mapping();
  // The custom Opera nodes.
  for (auto it = v.begin(); it != v.end(); it++)
    bookmark_root_server_tags.push_back(it->tag_name);
  std::vector<std::string>::iterator bookmark_root_server_tags_it = bookmark_root_server_tags.begin();

  for (ModelTypeSet::Iterator iter =
           UnlockVault(trans->GetWrappedTrans()).encrypted_types.First();
       iter.Good(); iter.Inc()) {
    if (iter.Get() == PASSWORDS || IsControlType(iter.Get()))
      continue; // These types handle encryption differently.

    bool more_of_this_type = true;
    while (more_of_this_type) {
      ReadNode type_root(trans);
      if (iter.Get() == BOOKMARKS) {
        if (bookmark_root_server_tags_it == bookmark_root_server_tags.end()) {
          more_of_this_type = false;
          continue;
        } else {
          std::string current_tag = *bookmark_root_server_tags_it;
          bookmark_root_server_tags_it++;
          if (type_root.InitByTagLookupForBookmarks(current_tag) != BaseNode::INIT_OK)
            continue;
        }
      } else {
        more_of_this_type = false;
        // Don't try to reencrypt if the type's data is unavailable.
        if (type_root.InitTypeRoot(iter.Get()) != BaseNode::INIT_OK)
          continue;
      }
      // Iterate through all children of this datatype.
      std::queue<int64_t> to_visit;
      int64_t child_id = type_root.GetFirstChildId();
      to_visit.push(child_id);
      while (!to_visit.empty()) {
        child_id = to_visit.front();
        to_visit.pop();
        if (child_id == kInvalidId)
          continue;
        WriteNode child(trans);
        if (child.InitByIdLookup(child_id) != BaseNode::INIT_OK)
          continue;  // Possible for locally deleted items.
        if (child.GetIsFolder()) {
          to_visit.push(child.GetFirstChildId());
        }
        if (child.GetEntry()->GetUniqueServerTag().empty()) {
          // Rewrite the specifics of the node with encrypted data if necessary
          // (only rewrite the non-unique folders).
          child.ResetFromSpecifics();
        }
        to_visit.push(child.GetSuccessorId());
      }
    }
  }

  // Passwords are encrypted with their own legacy scheme.  Passwords are always
  // encrypted so we don't need to check GetEncryptedTypes() here.
  ReadNode passwords_root(trans);
  if (passwords_root.InitTypeRoot(PASSWORDS) == BaseNode::INIT_OK) {
    int64_t child_id = passwords_root.GetFirstChildId();
    while (child_id != kInvalidId) {
      WriteNode child(trans);
      if (child.InitByIdLookup(child_id) != BaseNode::INIT_OK)
        break;  // Possible if we failed to decrypt the data for some reason.
      child.SetPasswordSpecifics(child.GetPasswordSpecifics());
      child_id = child.GetSuccessorId();
    }
  }

  DVLOG(1) << "Re-encrypt everything complete.";

  // NOTE: We notify from within a transaction.
  FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                    OnEncryptionComplete());
}

bool OperaSyncEncryptionHandlerImpl::ApplyNigoriUpdateImpl(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Applying nigori node update.";
  bool nigori_types_need_update =
      !UpdateEncryptedTypesFromNigori(nigori, trans);

  if (nigori.op_keybag_is_frozen() &&
      passphrase_type_ != PassphraseType::CUSTOM_PASSPHRASE) {
      passphrase_type_ = PassphraseType::CUSTOM_PASSPHRASE;
      FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                        OnPassphraseTypeChanged(passphrase_type_, GetExplicitPassphraseTime()));
  }

  if (nigori.op_custom_passphrase_time() != 0) {
    custom_passphrase_time_ =
        ProtoTimeToTime(nigori.op_custom_passphrase_time());
  }

  Cryptographer* cryptographer = &UnlockVaultMutable(trans)->cryptographer;
  bool nigori_needs_new_keys = false;
  if (!nigori.op_encryption_keybag().blob().empty()) {
    if (cryptographer->CanDecrypt(nigori.op_encryption_keybag())) {
      cryptographer->InstallKeys(nigori.op_encryption_keybag());
      // We only update the default passphrase if this was a new explicit
      // passphrase. Else, since it was decryptable, it must not have been a new
      // key.
      if (nigori.op_keybag_is_frozen())
        cryptographer->SetDefaultKey(nigori.op_encryption_keybag().key_name());

      // Check if the cryptographer's keybag is newer than the nigori's
      // keybag. If so, we need to overwrite the nigori node.
      sync_pb::EncryptedData new_keys = nigori.op_encryption_keybag();
      if (!cryptographer->GetKeys(&new_keys))
        NOTREACHED();
      if (nigori.op_encryption_keybag().SerializeAsString() !=
          new_keys.SerializeAsString())
        nigori_needs_new_keys = true;
    } else {
      cryptographer->SetPendingKeys(nigori.op_encryption_keybag());
    }
  } else {
    nigori_needs_new_keys = true;
  }

  // If we've completed a sync cycle and the cryptographer isn't ready
  // yet or has pending keys, prompt the user for a passphrase.
  if (cryptographer->has_pending_keys()) {
    DVLOG(1) << "OnPassphraseRequired Sent";
    sync_pb::EncryptedData pending_keys = cryptographer->GetPendingKeys();
    FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                      OnPassphraseRequired(REASON_DECRYPTION,
                                           pending_keys));
  } else if (!cryptographer->is_ready()) {
    DVLOG(1) << "OnPassphraseRequired sent because cryptographer is not "
             << "ready";
    FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                      OnPassphraseRequired(REASON_ENCRYPTION,
                                           sync_pb::EncryptedData()));
  }

  // Check if the current local encryption state is stricter/newer than the
  // nigori state. If so, we need to overwrite the nigori node with the local
  // state.
  bool explicit_passphrase = passphrase_type_ == PassphraseType::CUSTOM_PASSPHRASE;
  if (nigori.op_keybag_is_frozen() != explicit_passphrase ||
      nigori.encrypt_everything() != encrypt_everything_ ||
      nigori_types_need_update ||
      nigori_needs_new_keys) {
    return false;
  }
  return true;
}

void OperaSyncEncryptionHandlerImpl::RewriteNigori() {
  DVLOG(1) << "Overwriting stale nigori node.";
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteTransaction trans(FROM_HERE, user_share_);
  WriteEncryptionStateToNigori(&trans);
}

void OperaSyncEncryptionHandlerImpl::WriteEncryptionStateToNigori(
    WriteTransaction* trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WriteNode nigori_node(trans);
  // This can happen in tests that don't have nigori nodes.
  if (nigori_node.InitTypeRoot(NIGORI) != BaseNode::INIT_OK)
    return;
  sync_pb::NigoriSpecifics nigori = nigori_node.GetNigoriSpecifics();
  const Cryptographer& cryptographer =
      UnlockVault(trans->GetWrappedTrans()).cryptographer;
  if (cryptographer.is_ready() &&
      nigori_overwrite_count_ < kNigoriOverwriteLimit) {
    // Does not modify the encrypted blob if the unencrypted data already
    // matches what is about to be written.
    sync_pb::EncryptedData original_keys = nigori.op_encryption_keybag();
    if (!cryptographer.GetKeys(nigori.mutable_op_encryption_keybag()))
      NOTREACHED();

    if (nigori.op_encryption_keybag().SerializeAsString() !=
        original_keys.SerializeAsString()) {
      // We've updated the nigori node's encryption keys. In order to prevent
      // a possible looping of two clients constantly overwriting each other,
      // we limit the absolute number of overwrites per client instantiation.
      nigori_overwrite_count_++;
      UMA_HISTOGRAM_COUNTS("Sync.AutoNigoriOverwrites",
                           nigori_overwrite_count_);
    }

  }

  syncable::UpdateNigoriFromEncryptedTypes(
    UnlockVault(trans->GetWrappedTrans()).encrypted_types,
    encrypt_everything_,
    &nigori);


  // Note: we don't try to set using_explicit_passphrase here since if that
  // is lost the user can always set it again. The main point is to preserve
  // the encryption keys so all data remains decryptable.
  if (!custom_passphrase_time_.is_null()) {
    nigori.set_op_custom_passphrase_time(
        TimeToProtoTime(custom_passphrase_time_));
  }

  nigori_node.SetNigoriSpecifics(nigori);
}

bool OperaSyncEncryptionHandlerImpl::UpdateEncryptedTypesFromNigori(
    const sync_pb::NigoriSpecifics& nigori,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ModelTypeSet* encrypted_types = &UnlockVaultMutable(trans)->encrypted_types;
  if (nigori.encrypt_everything()) {
    if (!encrypt_everything_) {
      encrypt_everything_ = true;
      *encrypted_types = EncryptableUserTypes();
      DVLOG(1) << "Enabling encrypt everything via nigori node update";
      FOR_EACH_OBSERVER(
          Observer, observers_,
          OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_));
    }

    DCHECK(*encrypted_types == EncryptableUserTypes());
    return true;
  }

  ModelTypeSet nigori_encrypted_types;
  nigori_encrypted_types = syncable::GetEncryptedTypesFromNigori(nigori);
  nigori_encrypted_types.PutAll(SensitiveTypes());

  // If anything more than the sensitive types were encrypted, and
  // encrypt_everything is not explicitly set to false, we assume it means
  // a client intended to enable encrypt everything.
  if (!nigori.has_encrypt_everything() &&
      !Difference(nigori_encrypted_types, SensitiveTypes()).Empty()) {
    if (!encrypt_everything_) {
      encrypt_everything_ = true;
      *encrypted_types = EncryptableUserTypes();
      FOR_EACH_OBSERVER(
          Observer, observers_,
          OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_));
    }
    DCHECK(*encrypted_types == EncryptableUserTypes());
    return false;
  }

  MergeEncryptedTypes(nigori_encrypted_types, trans);
  return *encrypted_types == nigori_encrypted_types;
}

void OperaSyncEncryptionHandlerImpl::FinishSetPassphrase(
    bool success,
    const std::string& bootstrap_token,
    bool is_explicit,
    WriteTransaction* trans,
    WriteNode* nigori_node) {
  DCHECK(thread_checker_.CalledOnValidThread());
  FOR_EACH_OBSERVER(
      SyncEncryptionHandler::Observer,
      observers_,
      OnCryptographerStateChanged(
          &UnlockVaultMutable(trans->GetWrappedTrans())->cryptographer));

  // It's possible we need to change the bootstrap token even if we failed to
  // set the passphrase (for example if we need to preserve the new GAIA
  // passphrase).
  if (!bootstrap_token.empty()) {
    DVLOG(1) << "Passphrase bootstrap token updated.";
    FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                      OnBootstrapTokenUpdated(bootstrap_token,
                                              PASSPHRASE_BOOTSTRAP_TOKEN));
  }

  const Cryptographer& cryptographer =
      UnlockVault(trans->GetWrappedTrans()).cryptographer;
  if (!success) {
    if (cryptographer.is_ready()) {
      LOG(ERROR) << "Attempt to change passphrase failed while cryptographer "
                 << "was ready.";
    } else if (cryptographer.has_pending_keys()) {
      FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                        OnPassphraseRequired(REASON_DECRYPTION,
                                             cryptographer.GetPendingKeys()));
    } else {
      FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                        OnPassphraseRequired(REASON_ENCRYPTION,
                                             sync_pb::EncryptedData()));
    }
    return;
  }
  DCHECK(cryptographer.is_ready());
  // TODO(zea): trigger migration if necessary.

  sync_pb::NigoriSpecifics specifics(nigori_node->GetNigoriSpecifics());
  // Does not modify specifics.encrypted() if the original decrypted data was
  // the same.
  if (!cryptographer.GetKeys(specifics.mutable_op_encryption_keybag()))
    NOTREACHED();
  if (is_explicit && passphrase_type_ != PassphraseType::CUSTOM_PASSPHRASE) {
    passphrase_type_ = PassphraseType::CUSTOM_PASSPHRASE;
    FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
      OnPassphraseTypeChanged(passphrase_type_, GetExplicitPassphraseTime()));
  }
  if (passphrase_type_ == PassphraseType::CUSTOM_PASSPHRASE &&
      custom_passphrase_time_.is_null()) {
    custom_passphrase_time_ = base::Time::Now();
    specifics.set_op_custom_passphrase_time(TimeToProtoTime(custom_passphrase_time_));
  }

  specifics.set_op_keybag_is_frozen(is_explicit);
  nigori_node->SetNigoriSpecifics(specifics);

  // Must do this after OnPassphraseStateChanged, in order to ensure the PSS
  // checks the passphrase state after it has been set.
  FOR_EACH_OBSERVER(SyncEncryptionHandler::Observer, observers_,
                    OnPassphraseAccepted());

  // Does nothing if everything is already encrypted.
  // TODO(zea): If we just migrated and enabled encryption, this will be
  // redundant. Figure out a way to not do this unnecessarily.
  ReEncryptEverything(trans);
}

void OperaSyncEncryptionHandlerImpl::MergeEncryptedTypes(
    ModelTypeSet new_encrypted_types,
    syncable::BaseTransaction* const trans) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Only EncryptableUserTypes may be encrypted.
  DCHECK(EncryptableUserTypes().HasAll(new_encrypted_types));

  ModelTypeSet* encrypted_types = &UnlockVaultMutable(trans)->encrypted_types;
  if (!encrypted_types->HasAll(new_encrypted_types)) {
    *encrypted_types = new_encrypted_types;
    FOR_EACH_OBSERVER(
        Observer, observers_,
        OnEncryptedTypesChanged(*encrypted_types, encrypt_everything_));
  }
}

OperaSyncEncryptionHandlerImpl::Vault* OperaSyncEncryptionHandlerImpl::UnlockVaultMutable(
    syncable::BaseTransaction* const trans) {
  DCHECK_EQ(user_share_->directory.get(), trans->directory());
  return &vault_unsafe_;
}

const OperaSyncEncryptionHandlerImpl::Vault&
    OperaSyncEncryptionHandlerImpl::UnlockVault(
        syncable::BaseTransaction* const trans) const {
  DCHECK_EQ(user_share_->directory.get(), trans->directory());
  return vault_unsafe_;
}

base::Time OperaSyncEncryptionHandlerImpl::GetExplicitPassphraseTime() const {
  DCHECK_NE(passphrase_type_, PassphraseType::FROZEN_IMPLICIT_PASSPHRASE);

  if (passphrase_type_ == PassphraseType::CUSTOM_PASSPHRASE)
    return custom_passphrase_time();
  return base::Time();
}

}  // namespace browser_sync
