// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webdata_services/web_data_service_wrapper.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "components/autofill/core/browser/webdata/autocomplete_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_profile_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/autofill/core/browser/webdata/autofill_wallet_metadata_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_wallet_syncable_service.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/password_manager/core/browser/webdata/logins_table.h"
#include "components/search_engines/keyword_table.h"
#include "components/search_engines/keyword_web_data_service.h"
#include "components/signin/core/browser/webdata/token_service_table.h"
#include "components/signin/core/browser/webdata/token_web_data.h"
#include "components/webdata/common/web_database_service.h"
#include "components/webdata/common/webdata_constants.h"
#include "content/public/browser/browser_thread.h"

#if defined(OS_WIN)
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"
#endif

#if defined(OPERA_SYNC)
#include "common/sync/sync_config.h"
#include "common/oauth2/token_cache/token_cache_webdata.h"
#include "common/oauth2/token_cache/token_cache_table.h"
#endif  // OPERA_SYNC

namespace {

const base::FilePath::CharType kCorruptedWebDataFilename[] =
    FILE_PATH_LITERAL("Web Data.corrupted");
const base::FilePath::CharType kTooNewWebDataFilename[] =
    FILE_PATH_LITERAL("Web Data.too_new");

base::File::Error RenameFile(const base::FilePath& current_filename,
                             const base::FilePath& desired_filename) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));
  base::DeleteFile(desired_filename, false);
  bool success = base::ReplaceFile(current_filename, desired_filename, NULL);
  return success ? base::File::FILE_OK : base::File::FILE_ERROR_NOT_FOUND;
}

void ReturnRenameResultToWrapper(base::WeakPtr<WebDataServiceWrapper> wrapper,
                                 const base::File::Error& result) {
  DCHECK(wrapper != NULL);
  if (result == base::File::FILE_OK) {
    wrapper->set_loading_status(WebDataServiceWrapper::LOADING_RECREATED);
    //chrome::AttemptRestart();
  } else {
    wrapper->set_loading_status(WebDataServiceWrapper::LOADING_FINAL_FAILURE);
    wrapper->NotifyAboutFatalError();
  }
}

}  // namespace

WebDataServiceWrapper::WebDataServiceWrapper()
    : loading_status_(LOADING_INIT),
      error_type_(ERROR_LOADING_PASSWORD),
      sql_init_status_(sql::INIT_OK),
      weak_ptr_factory_(this) {
}

WebDataServiceWrapper::WebDataServiceWrapper(
    const base::FilePath& context_path,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_thread,
    const scoped_refptr<base::SingleThreadTaskRunner>& db_thread,
    const ShowErrorCallback& show_error_callback)
    : loading_status_(LOADING_INIT),
      error_type_(ERROR_LOADING_PASSWORD),
      sql_init_status_(sql::INIT_OK),
      show_error_callback_(show_error_callback),
      weak_ptr_factory_(this) {
  web_data_path_ = context_path.Append(kWebDataFilename);
  web_database_ = new WebDatabaseService(web_data_path_, ui_thread, db_thread);
#if 0
  // All tables objects that participate in managing the database must
  // be added here.
  web_database_->AddTable(base::WrapUnique(new autofill::AutofillTable));
  web_database_->AddTable(base::WrapUnique(new OperaKeywordTable));
  // TODO(mdm): We only really need the LoginsTable on Windows for IE7 password
  // access, but for now, we still create it on all platforms since it deletes
  // the old logins table. We can remove this after a while, e.g. in M22 or so.
  web_database_->AddTable(base::WrapUnique(new LoginsTable));
#endif
#if defined(OPERA_SYNC)
  if (opera::SyncConfig::UsesOAuth2()) {
    web_database_->AddTable(
        base::WrapUnique(new opera::oauth2::TokenCacheTable));
  }
#else
  web_database_->AddTable(base::WrapUnique(new TokenServiceTable));
#endif  // OPERA_SYNC

  // Register a callback to be notified that the database has loaded.
  web_database_->RegisterDBLoadedCallback(
      base::Bind(&WebDataServiceWrapper::ProfileDBLoadedCallback,
                 weak_ptr_factory_.GetWeakPtr()));
  // Register a callback to be notified that the database has failed to load.
  web_database_->RegisterDBErrorCallback(
      base::Bind(&WebDataServiceWrapper::ProfileErrorCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 ERROR_LOADING_PASSWORD));

  web_database_->LoadDatabase();
#if 0
  autofill_web_data_ = new autofill::AutofillWebDataService(
      web_database_, ui_thread, db_thread,
      base::Bind(&WebDataServiceWrapper::ProfileErrorCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 ERROR_LOADING_AUTOFILL));
  autofill_web_data_->Init();

  keyword_web_data_ = new OperaKeywordWebDataService(
      web_database_, ui_thread,
      base::Bind(&WebDataServiceWrapper::ProfileErrorCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 ERROR_LOADING_KEYWORD));
  keyword_web_data_->Init();
#endif
#if defined(OPERA_SYNC)
  if (opera::SyncConfig::UsesOAuth2()) {
    opera_token_cache_web_data_ = new opera::oauth2::TokenCacheWebData(
        web_database_, ui_thread, db_thread,
        base::Bind(show_error_callback, ERROR_LOADING_TOKEN));
    opera_token_cache_web_data_->Init();
  }
#else
  token_web_data_ = new TokenWebData(
      web_database_, ui_thread, db_thread,
      base::Bind(show_error_callback, ERROR_LOADING_TOKEN));
  token_web_data_->Init();
#endif  // OPERA_SYNC

#if defined(OS_WIN)
  password_web_data_ = new PasswordWebDataService(
      web_database_, ui_thread,
      base::Bind(&WebDataServiceWrapper::ProfileErrorCallback,
                 weak_ptr_factory_.GetWeakPtr(),
                 ERROR_LOADING_PASSWORD));
  password_web_data_->Init();
#endif
}

WebDataServiceWrapper::~WebDataServiceWrapper() {
}

void WebDataServiceWrapper::NotifyAboutFatalError() {
  show_error_callback_(error_type_, sql_init_status_, "");
}

void WebDataServiceWrapper::ProfileDBLoadedCallback() {
  loading_status_ = LOADING_SUCCESS;
}

void WebDataServiceWrapper::ProfileErrorCallback(
    ErrorType type,
    sql::InitStatus status,
    const std::string& diagnostics) {
  error_type_ = type;
  sql_init_status_ = status;

  switch (loading_status_) {
    case LOADING_INIT:
      loading_status_ = LOADING_RECREATING;
      show_error_callback_(type, status, diagnostics);
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::Bind(&WebDataServiceWrapper::RecreatingWebDatabase,
                                weak_ptr_factory_.GetWeakPtr()));
      return;

    case LOADING_RECREATING:
      return;

    case LOADING_RECREATED:
      loading_status_ = LOADING_FINAL_FAILURE;
      break;

    case LOADING_FINAL_FAILURE:
    case LOADING_SUCCESS:
      return;
  }
}

void WebDataServiceWrapper::RecreatingWebDatabase() {
  Shutdown();

  base::FilePath failed_web_data_path(web_data_path_.DirName().Append(
      (sql_init_status_ == sql::INIT_FAILURE) ? kCorruptedWebDataFilename
                                              : kTooNewWebDataFilename));

  content::BrowserThread::PostTaskAndReplyWithResult(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&RenameFile, web_data_path_, failed_web_data_path),
      base::Bind(&ReturnRenameResultToWrapper, weak_ptr_factory_.GetWeakPtr()));
}

void WebDataServiceWrapper::Shutdown() {
  autofill_web_data_->ShutdownOnUIThread();
  keyword_web_data_->ShutdownOnUIThread();
#if OPERA_SYNC
  if (opera::SyncConfig::UsesOAuth2()) {
    opera_token_cache_web_data_->ShutdownOnUIThread();
  }
#else
  token_web_data_->ShutdownOnUIThread();
#endif

#if defined(OS_WIN)
  password_web_data_->ShutdownOnUIThread();
#endif

  web_database_->ShutdownDatabase();
}

scoped_refptr<autofill::AutofillWebDataService>
WebDataServiceWrapper::GetAutofillWebData() {
  return autofill_web_data_.get();
}

scoped_refptr<KeywordWebDataService>
WebDataServiceWrapper::GetKeywordWebData() {
  return keyword_web_data_.get();
}

#if defined(OPERA_SYNC)
scoped_refptr<opera::oauth2::TokenCacheWebData>
WebDataServiceWrapper::GetOperaTokenCacheWebData() {
  DCHECK(opera::SyncConfig::UsesOAuth2());
  return opera_token_cache_web_data_.get();
}
#endif  // OPERA_SYNC

scoped_refptr<TokenWebData> WebDataServiceWrapper::GetTokenWebData() {
#if 0
  return token_web_data_.get();
#else
  return nullptr;
#endif
}

#if defined(OS_WIN)
scoped_refptr<PasswordWebDataService>
WebDataServiceWrapper::GetPasswordWebData() {
  return password_web_data_.get();
}
#endif
