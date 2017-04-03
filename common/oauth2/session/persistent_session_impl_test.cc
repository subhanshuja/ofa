// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <array>
#include <memory>

#include "components/os_crypt/os_crypt.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/testing_pref_store.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/syncable_prefs/pref_service_mock_factory.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "components/sync/api/time.h"

#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/pref_names.h"
#include "common/oauth2/session/persistent_session_impl.h"
#include "common/oauth2/session/session_constants.h"

#if defined(OPERA_DESKTOP)
#include "desktop/common/opera_pref_names.h"
#endif  // OPERA_DESKTOP

namespace opera {

namespace oauth2 {

namespace {

const char kMockRefreshToken[] = "mock-refresh-token-string";
const char kMockSessionId[] = "mock-session-id-string";
const char kMockUsername[] = "mock-username-string";
const char kMockUserId[] = "12349";
const base::Time kMockStartTime = base::Time::FromJsTime(1462864523000);

struct SessionStateInfo {
  SessionState state;
  std::string username;
  std::string refresh_token;
  std::string session_id;
  base::Time start_time;
  std::string user_id;
  SessionStartMethod start_method;
};

}  // namespace

class PersistentSessionImplTest
    : public ::testing::TestWithParam<SessionState> {
 public:
  PersistentSessionImplTest() {
#if defined(OS_MACOSX)
    OSCrypt::UseMockKeychain(true);
#endif  // defined(OS_MACOSX)
    PrepareTest();
  }
  ~PersistentSessionImplTest() override {}

  void PrepareTest() {
    session_.reset();
    reset_callback_called();

    PersistentPrefStore* testing_user_prefs = new TestingPrefStore();
    OverlayUserPrefStore* overlay_user_prefs =
        new OverlayUserPrefStore(testing_user_prefs);

    syncable_prefs::PrefServiceMockFactory mock_pref_service_factory;
    mock_pref_service_factory.set_user_prefs(
        make_scoped_refptr(overlay_user_prefs));
    scoped_refptr<user_prefs::PrefRegistrySyncable> registry(
        new user_prefs::PrefRegistrySyncable);
    syncable_prefs::PrefServiceSyncable* otr_prefs =
        mock_pref_service_factory.CreateSyncable(registry.get()).release();

    PersistentSessionImpl::RegisterProfilePrefs(registry.get());

#if defined(OPERA_DESKTOP)
    registry->RegisterBooleanPref(prefs::kFullStatisticsEnabled, true);
#endif  // OPERA_DESKTOP

    pref_service_ = otr_prefs;

    SetMetricsEnabled(true);

    session_.reset(new PersistentSessionImpl(nullptr, pref_service_));
    session_->Initialize(
        base::Bind(&PersistentSessionImplTest::SessionStateChangedCallback,
                   base::Unretained(this)));
  }

  void PresetStoredSession(SessionStateInfo given) {
    DictionaryPrefUpdate updater(pref_service_, kOperaOAuth2SessionPref);
    updater.Get()->SetStringWithoutPathExpansion(
        session::kRefreshToken, crypt::OSEncrypt(given.refresh_token));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kSessionId, crypt::OSEncrypt(given.session_id));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kUserId, crypt::OSEncrypt(given.user_id));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kUserName, crypt::OSEncrypt(given.username));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kSessionState,
        crypt::OSEncryptInt64(SessionStateToInt(given.state)));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kStartMethod,
        crypt::OSEncryptInt64(SessionStartMethodToInt(given.start_method)));
    updater.Get()->SetStringWithoutPathExpansion(
        session::kStartTime,
        crypt::OSEncryptInt64(given.start_time.ToInternalValue()));

    pref_service_->CommitPendingWrite();
  }

  void CheckStoredSession(SessionStateInfo expected) {
    const base::DictionaryValue* dict =
        pref_service_->GetDictionary(kOperaOAuth2SessionPref);
    if (dict && !dict->empty()) {
      ASSERT_EQ(expected.refresh_token,
                LoadStringPref(dict, session::kRefreshToken));
      ASSERT_EQ(expected.username, LoadStringPref(dict, session::kUserName));
      ASSERT_EQ(expected.session_id, LoadStringPref(dict, session::kSessionId));
      ASSERT_EQ(expected.user_id, LoadStringPref(dict, session::kUserId));
      SessionState read_state = OA2SS_UNSET;
      int64_t session_state_int = LoadInt64Pref(dict, session::kSessionState);
      if (session_state_int != -1) {
        read_state = SessionStateFromInt(session_state_int);
      }
      ASSERT_EQ(expected.state, read_state);

      base::Time read_start_time = base::Time();
      int64_t session_start_time = LoadInt64Pref(dict, session::kStartTime);
      if (session_start_time != -1) {
        read_start_time = base::Time::FromInternalValue(session_start_time);
      }
      ASSERT_EQ(expected.start_time, read_start_time);

      SessionStartMethod read_start_method = SSM_UNSET;
      int64_t session_start_method_int =
          LoadInt64Pref(dict, session::kStartMethod);
      if (session_start_method_int != -1) {
        read_start_method = SessionStartMethodFromInt(session_start_method_int);
      }
      ASSERT_EQ(expected.start_method, read_start_method);
    }
  }

  void SetMetricsEnabled(bool enabled) {
#if defined(OPERA_DESKTOP)
    pref_service_->SetBoolean(prefs::kFullStatisticsEnabled, enabled);
#endif  // OPERA_DESKTOP
  }

  void SessionStateChangedCallback() { callback_called_ = true; }

 protected:
  std::string DumpState(SessionStateInfo given,
                        SessionStateInfo expected) const {
    std::string ret =
        "given:\n\tstate=" + std::string(SessionStateToString(given.state)) +
        "\n\tusername=" + given.username + "\n\trefresh_token=" +
        given.refresh_token + "\n\tsession_id=" + given.session_id +
        "\n\tstart_time=" + syncer::GetTimeDebugString(given.start_time) +
        "\n\tuser_id=" + given.user_id + "\n\tstart_method=" +
        SessionStartMethodToString(given.start_method);
    ret += "\nsession:\n\tstate=" +
           std::string(SessionStateToString(session_->GetState())) +
           "\n\tusername=" + session_->GetUsername() + "\n\trefresh_token=" +
           session_->GetRefreshToken() + "\n\tsession_id=" +
           session_->GetSessionId() + "\n\tstart_time=" +
           syncer::GetTimeDebugString(session_->GetStartTime()) +
           "\n\tuser_id=" + session_->GetUserId() + "\n\tstart_method=" +
           SessionStartMethodToString(session_->GetStartMethod());
    ret += "\nexpected:\n\tstate=" +
           std::string(SessionStateToString(expected.state)) + "\n\tusername=" +
           expected.username + "\n\trefresh_token=" + expected.refresh_token +
           "\n\tsession_id=" + expected.session_id + "\n\tstart_time=" +
           syncer::GetTimeDebugString(expected.start_time) + "\n\tuser_id=" +
           expected.user_id + "\n\tstart_method=" +
           SessionStartMethodToString(expected.start_method);
    return ret;
  }

  bool callback_called() const { return callback_called_; }
  void reset_callback_called() { callback_called_ = false; }

  PrefService* pref_service_;

  bool callback_called_;

  std::unique_ptr<PersistentSessionImpl> session_;
};

TEST_F(PersistentSessionImplTest, DiagnosticSnapshotContents) {
  session_->LoadSession();
  std::unique_ptr<base::DictionaryValue> session_info(
      new base::DictionaryValue);

  session_info->SetString("id", "");
  session_info->SetBoolean("refresh_token_present", false);
  session_info->SetString("state", "INACTIVE");
  session_info->SetString("username", "");
  session_info->SetDouble("start_time", 0.0);
  session_info->SetString("user_id", "");
  session_info->SetString("start_type", "UNSET");

  EXPECT_EQ("session", session_->GetDiagnosticName());
  const auto& actual = session_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(session_info->Equals(actual.get()))
      << "Actual: " << *actual << "\nExpected: " << *session_info;
}

TEST_F(PersistentSessionImplTest, RestoreDefaultSession) {
  session_->LoadSession();

  EXPECT_EQ(OA2SS_INACTIVE, session_->GetState());
  EXPECT_EQ("", session_->GetRefreshToken());
  EXPECT_EQ("", session_->GetSessionId());
  EXPECT_EQ("", session_->GetUsername());
  EXPECT_EQ("", session_->GetUserId());
}

TEST_F(PersistentSessionImplTest, ClearSession) {
  SessionStateInfo given;
  given.refresh_token = kMockRefreshToken;
  given.session_id = kMockSessionId;
  given.start_method = SSM_AUTH_TOKEN;
  given.start_time = kMockStartTime;
  given.state = OA2SS_IN_PROGRESS;
  given.username = kMockUsername;
  given.user_id = kMockUserId;

  PresetStoredSession(given);
  session_->LoadSession();

  EXPECT_EQ(OA2SS_IN_PROGRESS, session_->GetState());
  EXPECT_EQ(kMockRefreshToken, session_->GetRefreshToken());
  EXPECT_EQ(kMockSessionId, session_->GetSessionId());
  EXPECT_EQ(kMockUsername, session_->GetUsername());
  EXPECT_EQ(kMockStartTime, session_->GetStartTime());
  EXPECT_EQ(kMockUserId, session_->GetUserId());

  session_->ClearSession();

  EXPECT_EQ(OA2SS_INACTIVE, session_->GetState());
  EXPECT_EQ("", session_->GetRefreshToken());
  EXPECT_EQ("", session_->GetSessionId());
  EXPECT_EQ("", session_->GetUsername());
  EXPECT_EQ(base::Time(), session_->GetStartTime());
  EXPECT_EQ("", session_->GetUserId());
}

TEST_F(PersistentSessionImplTest, StateChangeToAuthError) {
  SessionStateInfo given;
  given.refresh_token = kMockRefreshToken;
  given.session_id = kMockSessionId;
  given.start_method = SSM_AUTH_TOKEN;
  given.start_time = kMockStartTime;
  given.state = OA2SS_IN_PROGRESS;
  given.username = kMockUsername;
  given.user_id = kMockUserId;

  PresetStoredSession(given);

  ASSERT_FALSE(callback_called());
  session_->LoadSession();
  EXPECT_TRUE(callback_called());

  reset_callback_called();
  EXPECT_EQ(OA2SS_IN_PROGRESS, session_->GetState());
  session_->SetState(OA2SS_IN_PROGRESS);
  EXPECT_FALSE(callback_called());
  reset_callback_called();

  session_->SetState(OA2SS_AUTH_ERROR);
  EXPECT_TRUE(callback_called());
  reset_callback_called();
}

TEST_P(PersistentSessionImplTest, RestoreSession) {
  std::array<std::string, 2> kUsernames{{std::string(), kMockUsername}};
  std::array<std::string, 2> kRefreshTokens{{std::string(), kMockRefreshToken}};
  std::array<std::string, 2> kSessionIds{{std::string(), kMockSessionId}};
  std::array<base::Time, 2> kStartTimes{{base::Time(), kMockStartTime}};
  std::array<SessionStartMethod, SSM_LAST_ENUM_VALUE> kStartMethods;
  for (int i = 0; i < SSM_LAST_ENUM_VALUE; i++) {
    kStartMethods[i] = static_cast<SessionStartMethod>(i);
  }
  std::array<std::string, 2> kUserIds{{std::string(), kMockUserId}};

  SessionState state = GetParam();

  for (const auto& username : kUsernames) {
    for (const auto& refresh_token : kRefreshTokens) {
      for (const auto& session_id : kSessionIds) {
        for (const auto& start_time : kStartTimes) {
          for (const auto& user_id : kUserIds) {
            for (const auto& start_method : kStartMethods) {
              SessionStateInfo given;
              given.refresh_token = refresh_token;
              given.session_id = session_id;
              given.start_method = start_method;
              given.start_time = start_time;
              given.state = state;
              given.username = username;
              given.user_id = user_id;

              PrepareTest();
              PresetStoredSession(given);
              session_->LoadSession();

              SessionStateInfo expected;
              expected.refresh_token = "";
              expected.session_id = "";
              expected.start_method = SSM_UNSET;
              expected.start_time = base::Time();
              expected.state = OA2SS_INACTIVE;
              expected.username = "";
              expected.user_id = "";

              switch (state) {
                case OA2SS_AUTH_ERROR:
                  if (!username.empty() && refresh_token.empty() &&
                      !session_id.empty() && (start_time != base::Time()) &&
                      !user_id.empty() && (start_method != SSM_UNSET)) {
                    expected.refresh_token = refresh_token;
                    expected.session_id = session_id;
                    expected.start_method = start_method;
                    expected.start_time = start_time;
                    expected.state = state;
                    expected.username = username;
                    expected.user_id = user_id;
                  }
                  break;
                case OA2SS_IN_PROGRESS:
                  if (!username.empty() && !refresh_token.empty() &&
                      !session_id.empty() && (start_time != base::Time()) &&
                      !user_id.empty() && (start_method != SSM_UNSET)) {
                    expected.refresh_token = refresh_token;
                    expected.session_id = session_id;
                    expected.start_method = start_method;
                    expected.start_time = start_time;
                    expected.state = state;
                    expected.username = username;
                    expected.user_id = user_id;
                  }
                  break;
                default:
                  break;
              }

              SCOPED_TRACE(DumpState(given, expected));

              ASSERT_EQ(expected.state, session_->GetState());
              ASSERT_EQ(expected.session_id, session_->GetSessionId());
              ASSERT_EQ(expected.refresh_token, session_->GetRefreshToken());
              ASSERT_EQ(expected.username, session_->GetUsername());
              ASSERT_EQ(expected.start_time, session_->GetStartTime());
              ASSERT_EQ(expected.user_id, session_->GetUserId());
              ASSERT_EQ(expected.start_method, session_->GetStartMethod());
            }
          }
        }
      }
    }
  }
}

TEST_F(PersistentSessionImplTest, StoreUnset) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->StoreSession();
  SessionStateInfo expected;
  expected.refresh_token = "";
  expected.session_id = "";
  expected.start_method = SSM_UNSET;
  expected.start_time = base::Time();
  expected.state = OA2SS_UNSET;
  expected.username = "";
  expected.user_id = "";
  CheckStoredSession(expected);
}

TEST_F(PersistentSessionImplTest, StoreInactive) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);
  session_->StoreSession();

  SessionStateInfo expected;
  expected.refresh_token = "";
  expected.session_id = "";
  expected.start_method = SSM_UNSET;
  expected.start_time = base::Time();
  expected.state = OA2SS_INACTIVE;
  expected.username = "";
  expected.user_id = "";
  CheckStoredSession(expected);
}

TEST_F(PersistentSessionImplTest, StoreStarting) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);
  session_->SetUsername(kMockUsername);
  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetState(OA2SS_STARTING);
  session_->StoreSession();

  SessionStateInfo expected;
  expected.refresh_token = "";
  expected.session_id = "";
  expected.start_method = SSM_UNSET;
  expected.start_time = base::Time();
  expected.state = OA2SS_UNSET;
  expected.username = "";
  expected.user_id = "";
  CheckStoredSession(expected);
}

TEST_F(PersistentSessionImplTest, StoreInProgress) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);
  session_->SetUsername(kMockUsername);
  session_->SetStartMethod(SSM_OAUTH1);
  session_->SetState(OA2SS_STARTING);
  session_->SetRefreshToken(kMockRefreshToken);
  session_->SetUserId(kMockUserId);
  session_->SetState(OA2SS_IN_PROGRESS);
  const auto expected_session_id = session_->GetSessionId();
  const auto expected_start_time = session_->GetStartTime();
  session_->StoreSession();

  SessionStateInfo expected;
  expected.refresh_token = kMockRefreshToken;
  expected.session_id = expected_session_id;
  expected.start_method = SSM_OAUTH1;
  expected.start_time = expected_start_time;
  expected.state = OA2SS_IN_PROGRESS;
  expected.username = kMockUsername;
  expected.user_id = kMockUserId;
  CheckStoredSession(expected);
}

TEST_F(PersistentSessionImplTest, StateUnsetHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  EXPECT_FALSE(session_->IsLoggedIn());
  EXPECT_FALSE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, StateInactiveHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());

  session_->SetState(OA2SS_INACTIVE);

  EXPECT_FALSE(session_->IsLoggedIn());
  EXPECT_FALSE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, StateStartingHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());

  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  EXPECT_TRUE(session_->IsLoggedIn());
  EXPECT_FALSE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, StateInProgressHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  session_->SetUserId(kMockUserId);
  session_->SetRefreshToken(kMockRefreshToken);
  session_->SetState(OA2SS_IN_PROGRESS);

  EXPECT_TRUE(session_->IsLoggedIn());
  EXPECT_FALSE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, StateFinishingHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());

  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  session_->SetState(OA2SS_FINISHING);
  EXPECT_TRUE(session_->IsLoggedIn());
  EXPECT_FALSE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, StateAuthErrorHelpersSmoke) {
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());

  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  session_->SetUserId(kMockUserId);
  session_->SetRefreshToken(kMockRefreshToken);
  session_->SetState(OA2SS_AUTH_ERROR);

  EXPECT_TRUE(session_->IsLoggedIn());
  EXPECT_TRUE(session_->HasAuthError());
}

TEST_F(PersistentSessionImplTest, ReturnsSessionIdWithMetricsEnabled) {
  SetMetricsEnabled(true);
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  session_->SetUserId(kMockUserId);
  session_->SetRefreshToken(kMockRefreshToken);
  session_->SetState(OA2SS_IN_PROGRESS);

#if defined(OPERA_DESKTOP)
  EXPECT_FALSE(session_->GetSessionId().empty());
  EXPECT_EQ(session_->GetSessionId(), session_->GetSessionIdForDiagnostics());
#endif
}

TEST_F(PersistentSessionImplTest, DoesNotReturnSessionIdWithMetricsDisabled) {
  SetMetricsEnabled(false);
  ASSERT_EQ(OA2SS_UNSET, session_->GetState());
  session_->SetState(OA2SS_INACTIVE);

  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(kMockUsername);
  session_->SetState(OA2SS_STARTING);

  session_->SetUserId(kMockUserId);
  session_->SetRefreshToken(kMockRefreshToken);
  session_->SetState(OA2SS_IN_PROGRESS);

  EXPECT_FALSE(session_->GetSessionId().empty());
  EXPECT_EQ(std::string(), session_->GetSessionIdForDiagnostics());
}

SessionState kTestedStates[] = {OA2SS_UNSET,     OA2SS_INACTIVE,
                                OA2SS_STARTING,  OA2SS_IN_PROGRESS,
                                OA2SS_FINISHING, OA2SS_AUTH_ERROR};

OperaAuthError kTestedAuthErrors[] = {OAE_UNSET,           OAE_OK,
                                      OAE_INVALID_CLIENT,  OAE_INVALID_GRANT,
                                      OAE_INVALID_REQUEST, OAE_INVALID_SCOPE};

INSTANTIATE_TEST_CASE_P(ParametrizedSessionState,
                        PersistentSessionImplTest,
                        testing::ValuesIn(kTestedStates));

}  // namespace oauth2

}  // namespace opera
