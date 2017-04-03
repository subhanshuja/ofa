// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_IMPL_H_
#define COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_IMPL_H_

#include <string>

#include "base/bind.h"
#include "components/pref_registry/pref_registry_syncable.h"

#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/session/persistent_session.h"

class PrefService;

namespace opera {
namespace oauth2 {

class PersistentSessionImpl : public PersistentSession,
                              public DiagnosticSupplier {
 public:
  PersistentSessionImpl(DiagnosticService* diagnostic_service,
                        PrefService* pref_service);
  ~PersistentSessionImpl() override;

  void Initialize(base::Closure session_state_changed_callback) override;

  // DiagnosticSupplier implementation.
  std::string GetDiagnosticName() const override;
  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void ClearSession() override;
  void LoadSession() override;
  void StoreSession() override;

  SessionStartMethod GetStartMethod() const override;
  void SetStartMethod(SessionStartMethod method) override;

  SessionState GetState() const override;
  void SetState(const SessionState& state) override;

  std::string GetRefreshToken() const override;
  void SetRefreshToken(const std::string& refresh_token) override;

  std::string GetUsername() const override;
  void SetUsername(const std::string& username) override;

  std::string GetUserId() const override;
  void SetUserId(const std::string& user_id) override;

  std::string GetSessionId() const override;

  base::Time GetStartTime() const override;

  bool HasAuthError() const override;
  bool IsLoggedIn() const override;
  bool IsInProgress() const override;

  std::string GetSessionIdForDiagnostics() const override;

 private:
  void GenerateSessionId();
  void UpdateStartTime();

  bool LoadedStateValid() const;
  bool StateIsStorable() const;

  void RequestTakeSnapshot();

  void ReportContentsHistogram() const;

  DiagnosticService* diagnostic_service_;
  PrefService* pref_service_;

  SessionStartMethod start_method_;
  SessionState state_;
  std::string refresh_token_;
  std::string username_;
  std::string session_id_;
  base::Time start_time_;
  std::string user_id_;

  base::Closure session_state_changed_callback_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_IMPL_H_
