// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_
#define COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/api/syncable_service.h"
#include "sql/init_status.h"

#if defined(OPERA_SYNC)
namespace opera {
namespace oauth2 {
class TokenCacheWebData;
}  // namespace oauth2
}  // namespace opera

#endif  // OPERA_SYNC

class KeywordWebDataService;
class TokenWebData;
class WebDatabaseService;

#if defined(OS_WIN)
class PasswordWebDataService;
#endif

namespace autofill {
class AutofillWebDataBackend;
class AutofillWebDataService;
}  // namespace autofill

namespace base {
class FilePath;
class SingleThreadTaskRunner;
}  // namespace base

// WebDataServiceWrapper is a KeyedService that owns multiple WebDataServices
// so that they can be associated with a context.
class WebDataServiceWrapper : public KeyedService {
 public:
  // Status of loading process in case of corrupted or too new web database:
  enum LoadingStatus {
    LOADING_INIT,
    LOADING_RECREATING,
    LOADING_RECREATED,
    LOADING_FINAL_FAILURE,
    LOADING_SUCCESS
  };

  // ErrorType indicates which service encountered an error loading its data.
  enum ErrorType {
    ERROR_LOADING_AUTOFILL,
    ERROR_LOADING_KEYWORD,
    ERROR_LOADING_TOKEN,
    ERROR_LOADING_PASSWORD,
  };

  // Shows an error message if a loading error occurs.
  using ShowErrorCallback = void (*)(ErrorType,
                                     sql::InitStatus,
                                     const std::string& string);

  // Constructor for WebDataServiceWrapper that initializes the different
  // WebDataServices and starts the synchronization services using |flare|.
  // Since |flare| will be copied and called multiple times, it cannot bind
  // values using base::Owned nor base::Passed; it should only bind simple or
  // refcounted types.
  WebDataServiceWrapper(
      const base::FilePath& context_path,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_thread,
      const scoped_refptr<base::SingleThreadTaskRunner>& db_thread,
      const ShowErrorCallback& show_error_callback);
  ~WebDataServiceWrapper() override;

  // KeyedService:
  void Shutdown() override;

  // Create the various types of service instances.  These methods are virtual
  // for testing purpose.
  virtual scoped_refptr<autofill::AutofillWebDataService> GetAutofillWebData();
  virtual scoped_refptr<KeywordWebDataService> GetKeywordWebData();

#if defined(OPERA_SYNC)
  virtual scoped_refptr<opera::oauth2::TokenCacheWebData>
  GetOperaTokenCacheWebData();
#endif  // OPERA_SYNC
  virtual scoped_refptr<TokenWebData> GetTokenWebData();
#if defined(OS_WIN)
  virtual scoped_refptr<PasswordWebDataService> GetPasswordWebData();
#endif

  void set_loading_status(LoadingStatus loading_status) {
    loading_status_ = loading_status;
  }

  // Notify about fatal error in case when renaming of Web database was
  // impossible.
  void NotifyAboutFatalError();

 protected:
  // For testing.
  WebDataServiceWrapper();

 private:
  // Callback to notify that web database is already loaded.
  void ProfileDBLoadedCallback();

  // Callback to notify that loading of database has failed because it's
  // corrupted or too new. In first calling of method the web database file is
  // renamed (adding ".corrupted" or ".toonew" suffix) and Opera is restarted to
  // repeat the loading process of web database.
  void ProfileErrorCallback(ErrorType type,
                            sql::InitStatus status,
                            const std::string& diagnostics);

  void RecreatingWebDatabase();

  scoped_refptr<WebDatabaseService> web_database_;

  scoped_refptr<autofill::AutofillWebDataService> autofill_web_data_;
  scoped_refptr<KeywordWebDataService> keyword_web_data_;
#if defined(OPERA_SYNC)
  scoped_refptr<opera::oauth2::TokenCacheWebData>
      opera_token_cache_web_data_;
#else
  scoped_refptr<TokenWebData> token_web_data_;
#endif

#if defined(OS_WIN)
  scoped_refptr<PasswordWebDataService> password_web_data_;
#endif

  LoadingStatus loading_status_;
  ErrorType error_type_;
  sql::InitStatus sql_init_status_;
  base::FilePath web_data_path_;
  ShowErrorCallback show_error_callback_;

  base::WeakPtrFactory<WebDataServiceWrapper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebDataServiceWrapper);
};

#endif  // COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_
