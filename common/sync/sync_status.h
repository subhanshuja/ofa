// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_STATUS_H_
#define COMMON_SYNC_SYNC_STATUS_H_

#include <string>

#include "google_apis/gaia/google_service_auth_error.h"
#include "components/sync/base/model_type.h"

#include "common/sync/sync_server_info.h"

namespace opera {

/**
 * The status of the Sync mechanism as seen by the client.
 *
 * Most of it is actually what we know about the Sync server, and the data is
 * filled based on responses received from the server, if any.
 *
 * @see SyncServerInfo
 * @see SyncServerConnection
 */
class SyncStatus {
 public:
  enum TokenState {
    TS_UNSET,
    // Token is valid.
    TS_OK,
    // Token is being recovered or a retry attempt is made for renewal.
    TS_WARNING,
    // Token is lost.
    TS_ERROR,
    // User is logged out.
    TS_LOGGED_OUT
  };

  enum ButtonState { BS_INACTIVE, BS_OK, BS_WARNING, BS_INFO, BS_ERROR };

  /**
   * Constructs a SyncStatus that reflects the state of the Sync server.
   *
   * @param network_error represents the state of the network connection to
   *    the Sync server
   * @param server_info describes the last seen state of the Sync server
   * @param busy whether sync is in the middle of logging
   * @param logged_in whether sync is logged in succesfully
   * @param internal_error whether internal error happened
   * @param internal_error_message message explaining origin of internal error
   *    if such occurred
   * @param passphrase_required flag indicating whether the sync encryption
   *    handler requires entering the current encryption passphrase. Is
   *    updated with each Nigori node update, meaning that sync has to be
   *    logged in at least once to download the Nigori node with a non-
   *    decryptable keybag. Note that this might still be valid while
   *    sync is in an error state as the encryption handler operates
   *    separately, the UI has to choose what to do in such situation.
   * @param setup_in_progress flag indicating whether the ProfileSyncService
   *    setup is still in progress, i.e. user did not make the final choice
   *    regarding the synchronized datatype set. Note that this will be true
   *    during the initial sync setup, just after sync has been logged in.
   *    How this is handled depends on the UI we choose.
   */
  SyncStatus(int network_error,
             const SyncServerInfo& server_info,
             bool busy,
             bool logged_in,
             bool internal_error,
             const std::string& internal_error_message,
             bool disable_sync,
             bool passphrase_required,
             bool setup_in_progress,
             syncer::ModelTypeSet unconfigured_types);
  SyncStatus(const SyncStatus&);
  ~SyncStatus();

  bool operator==(const SyncStatus& other) const;
  bool operator!=(const SyncStatus& other) const;

  /**
   * @return @c true if the Sync mechanism is enabled.  It is implied that
   *    there are no server or internal errors.
   */
  bool enabled() const;

  /**
   * @return @c true if the Sync mechanism is busy processing a request
   *    (sending/receiving data, processing data, etc.)
   */
  bool busy() const;

  /**
   * @return @c true if the Sync mechanism encountered a network error.
   *    enabled() may still return @c true if we're trying to recover from the
   *    error with the intention to continue synchronizing ASAP.
   */
  bool network_error() const;

  /**
   * If network_error() returns @c true, use this to get the specific network
   * error.
   *
   * @return the specific network error
   * @see server_error_code()
   */
  int network_error_code() const { return network_error_; }

  /**
   * @return @c true if the Sync mechanism is not enabled because of an error
   *    indicated by the Sync server.  In this case, we are not trying to
   *    recover from the error and the Sync mechanism has to be started again.
   */
  bool server_error() const;

  /**
   * If server_error() returns @c true, use this to get the specific error
   * indicated by the Sync server.
   *
   * @return the specific server error
   * @see network_error_code()
   */
  int server_error_code() const;

  /**
   * If server_error() returns @c true, use this to get the specific server
   * error message (if any).
   */
  const std::string& server_message() const;

  /**
   * Returns true if internal error occurred - such error can happen only
   * on save/save validation and it causes immediate logout.
   */
  bool internal_error() const;

  bool token_error() const;

  bool token_warning() const;

  /**
   * If internal error occurred it will return message passed with a callback.
   */
  const std::string& internal_error_message() const;

  /**
   * Returns true if sync should be disabled on the client.
   * Only makes sense in case of errors.
   */
  bool disable_sync() const;

  /**
   * Whether Sync is successfully logged in or logged out/errored.
   */
  bool logged_in() const { return logged_in_; }

  /**
   * Whether Sync requires the user to supply a password for decryption.
   */
  bool passphrase_required() const { return passphrase_required_; }

  /**
   * Whether sync is being setup currently.
   */
  bool setup_in_progress() const { return setup_in_progress_; }

  bool first_start() const;

  void set_first_start(bool is_any_type_configured);

  bool has_unconfigured_types() const;

  bool is_configuring() const;
  void set_is_configuring(bool is_configuring);

  syncer::ModelTypeSet unconfigured_types() const;

  ButtonState button_state() const;

  bool request_login_again() const;
  void set_request_login_again(bool login_again);

  /**
  * Whether according to the current status it is possible to enter
  * the advanced sync config dialog. This controls the advanced config
  * button in opera://settings and also blocks opening the dialog directly
  * via opera://settings/syncSetup
  */
  bool can_show_advanced_config() const;

  void set_token_state(TokenState token_state);
  TokenState token_state() const;

  void set_auth_error(GoogleServiceAuthError::State auth_error);
  GoogleServiceAuthError::State auth_error() const;
  bool has_auth_error() const;

  std::string ToString() const;

 private:
  int network_error_;
  SyncServerInfo server_info_;
  bool busy_;
  bool logged_in_;
  bool internal_error_;
  std::string internal_error_message_;
  bool disable_sync_;
  bool passphrase_required_;
  bool setup_in_progress_;
  syncer::ModelTypeSet unconfigured_types_;
  bool is_configuring_;
  bool is_any_type_configured_;
  bool request_login_again_;
  TokenState token_state_;
  GoogleServiceAuthError::State auth_error_;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_STATUS_H_
