// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/common/localized_error.cc
// @final-synchronized 135d586ae43b9260ef2ac198d5eddc158a5743fc

#include "common/error_pages/localized_error.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "components/strings/grit/components_strings.h"
#include "content/child/file_info_util.h"

#include "opera_common/grit/opera_common_resources.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"

#if defined(ENABLE_SEARCH_IN_ERROR_PAGE)
#include "common/error_pages/error_page_search_data_provider.h"
#endif  // ENABLE_SEARCH_IN_ERROR_PAGE

using blink::WebURLError;

namespace {

enum NAV_SUGGESTIONS {
  SUGGEST_NONE                  = 0,
  SUGGEST_RELOAD                = 1 << 0,
  SUGGEST_CHECK_CONNECTION      = 1 << 1,
  SUGGEST_DNS_CONFIG            = 1 << 2,
  SUGGEST_FIREWALL_CONFIG       = 1 << 3,
  SUGGEST_PROXY_CONFIG          = 1 << 4,
  SUGGEST_DISABLE_EXTENSION     = 1 << 5,
  SUGGEST_LEARNMORE             = 1 << 6,
  SUGGEST_VIEW_POLICIES         = 1 << 7,
  SUGGEST_CONTACT_ADMINISTRATOR = 1 << 8,
  SUGGEST_UNSUPPORTED_CIPHER    = 1 << 9,
};

struct LocalizedErrorMap {
  int error_code;
  unsigned int heading_resource_id;
  // Detailed summary used when the error is in the main frame and shown on
  // mouse over when the error is in frame.
  unsigned int summary_resource_id;
  int suggestions;  // Bitmap of SUGGEST_* values.
  unsigned int error_explanation_id;
};

const LocalizedErrorMap net_error_options[] = {
  {net::ERR_TIMED_OUT,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_TIMED_OUT,
   SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_CONNECTION_TIMED_OUT,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_TIMED_OUT,
   SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_CONNECTION_CLOSED,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_CLOSED,
   SUGGEST_RELOAD,
  },
  {net::ERR_CONNECTION_RESET,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_RESET,
   SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_CONNECTION_REFUSED,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_REFUSED,
   SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_CONNECTION_FAILED,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_FAILED,
   SUGGEST_RELOAD,
  },
  {net::ERR_NAME_NOT_RESOLVED,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_NAME_NOT_RESOLVED,
   SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_DNS_CONFIG |
       SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_ADDRESS_UNREACHABLE,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_ADDRESS_UNREACHABLE,
   SUGGEST_RELOAD | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_NETWORK_ACCESS_DENIED,
   IDS_ERRORPAGES_HEADING_NETWORK_ACCESS_DENIED,
   IDS_ERRORPAGES_SUMMARY_NETWORK_ACCESS_DENIED,
   SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_PROXY_CONNECTION_FAILED,
   IDS_ERRORPAGES_HEADING_INTERNET_DISCONNECTED,
   IDS_ERRORPAGES_SUMMARY_PROXY_CONNECTION_FAILED,
   SUGGEST_NONE,
  },
  {net::ERR_INTERNET_DISCONNECTED,
   IDS_ERRORPAGES_HEADING_INTERNET_DISCONNECTED,
   IDS_ERRORPAGES_HEADING_INTERNET_DISCONNECTED,
   SUGGEST_CHECK_CONNECTION | SUGGEST_FIREWALL_CONFIG,
  },
  {net::ERR_FILE_NOT_FOUND,
   IDS_ERRORPAGES_HEADING_FILE_NOT_FOUND,
   IDS_ERRORPAGES_SUMMARY_FILE_NOT_FOUND,
   SUGGEST_RELOAD,
  },
  {net::ERR_CACHE_MISS,
   IDS_ERRORPAGES_HEADING_CACHE_READ_FAILURE,
   IDS_ERRORPAGES_SUMMARY_CACHE_READ_FAILURE,
   SUGGEST_RELOAD,
  },
  {net::ERR_CACHE_READ_FAILURE,
   IDS_ERRORPAGES_HEADING_CACHE_READ_FAILURE,
   IDS_ERRORPAGES_SUMMARY_CACHE_READ_FAILURE,
   SUGGEST_RELOAD,
  },
  {net::ERR_NETWORK_IO_SUSPENDED,
   IDS_ERRORPAGES_HEADING_CONNECTION_INTERRUPTED,
   IDS_ERRORPAGES_SUMMARY_NETWORK_IO_SUSPENDED,
   SUGGEST_RELOAD,
  },
  {net::ERR_TOO_MANY_REDIRECTS,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_TOO_MANY_REDIRECTS,
   SUGGEST_RELOAD,
  },
  {net::ERR_EMPTY_RESPONSE,
   IDS_ERRORPAGES_HEADING_EMPTY_RESPONSE,
   IDS_ERRORPAGES_SUMMARY_EMPTY_RESPONSE,
   SUGGEST_RELOAD,
  },
  {net::ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_LENGTH,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_INVALID_RESPONSE,
   SUGGEST_RELOAD,
  },
  {net::ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_INVALID_RESPONSE,
   SUGGEST_RELOAD,
  },
  {net::ERR_RESPONSE_HEADERS_MULTIPLE_LOCATION,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_INVALID_RESPONSE,
   SUGGEST_RELOAD,
  },
  {net::ERR_CONTENT_LENGTH_MISMATCH,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_CLOSED,
   SUGGEST_RELOAD,
  },
  {net::ERR_INCOMPLETE_CHUNKED_ENCODING,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_CONNECTION_CLOSED,
   SUGGEST_RELOAD,
  },
  {net::ERR_SSL_PROTOCOL_ERROR,
   IDS_ERRORPAGES_HEADING_INSECURE_CONNECTION,
   IDS_ERRORPAGES_SUMMARY_INVALID_RESPONSE,
   SUGGEST_NONE,
  },
  {net::ERR_BAD_SSL_CLIENT_AUTH_CERT,
   IDS_ERRORPAGES_HEADING_INSECURE_CONNECTION,
   IDS_ERRORPAGES_SUMMARY_BAD_SSL_CLIENT_AUTH_CERT,
   SUGGEST_NONE,
  },
  {net::ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY,
   IDS_ERRORPAGES_HEADING_INSECURE_CONNECTION,
   IDS_ERRORPAGES_SUMMARY_SSL_SECURITY_ERROR,
   SUGGEST_NONE,
  },
  {net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN,
   IDS_ERRORPAGES_HEADING_INSECURE_CONNECTION,
   IDS_ERRORPAGES_SUMMARY_PINNING_FAILURE,
   SUGGEST_NONE,
  },
  {net::ERR_TEMPORARILY_THROTTLED,
   IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
   IDS_ERRORPAGES_SUMMARY_NOT_AVAILABLE,
   SUGGEST_NONE,
  },
  {net::ERR_BLOCKED_BY_CLIENT,
   IDS_ERRORPAGES_HEADING_BLOCKED,
   IDS_ERRORPAGES_SUMMARY_BLOCKED_BY_EXTENSION,
   SUGGEST_RELOAD | SUGGEST_DISABLE_EXTENSION,
  },
};

const LocalizedErrorMap http_error_options[] = {
  {403,
   IDS_ERRORPAGES_HEADING_ACCESS_DENIED,
   IDS_ERRORPAGES_SUMMARY_FORBIDDEN,
   SUGGEST_NONE,
  },
  {404,
   IDS_ERRORPAGES_HEADING_NOT_FOUND,
   IDS_ERRORPAGES_SUMMARY_NOT_FOUND,
   SUGGEST_NONE,
  },
  {410,
   IDS_ERRORPAGES_HEADING_NOT_FOUND,
   IDS_ERRORPAGES_SUMMARY_GONE,
   SUGGEST_NONE,
  },
  {500,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_WEBSITE_CANNOT_HANDLE_REQUEST,
   SUGGEST_RELOAD,
  },
  {501,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_WEBSITE_CANNOT_HANDLE_REQUEST,
   SUGGEST_NONE,
  },
  {502,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_WEBSITE_CANNOT_HANDLE_REQUEST,
   SUGGEST_RELOAD,
  },
  {503,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_WEBSITE_CANNOT_HANDLE_REQUEST,
   SUGGEST_RELOAD,
  },
  {504,
   IDS_ERRORPAGES_HEADING_PAGE_NOT_WORKING,
   IDS_ERRORPAGES_SUMMARY_GATEWAY_TIMEOUT,
   SUGGEST_RELOAD,
  },
};

#if defined(ENABLE_SEARCH_IN_ERROR_PAGE)
bool ShouldShowSearchBox(const std::string& error_domain, int error_code) {
  if (error_domain == net::kErrorDomain) {
    if (error_code == net::ERR_NAME_NOT_RESOLVED)
      return true;
  } else if (error_domain == LocalizedError::kHttpErrorDomain) {
    if (error_code == 404 || error_code == 410)
      return true;
  }

  return false;
}
#endif  // ENABLE_SEARCH_IN_ERROR_PAGE

const LocalizedErrorMap* FindErrorMapInArray(const LocalizedErrorMap* maps,
                                                   size_t num_maps,
                                                   int error_code) {
  for (size_t i = 0; i < num_maps; ++i) {
    if (maps[i].error_code == error_code)
      return &maps[i];
  }
  return NULL;
}

const LocalizedErrorMap* LookupErrorMap(const std::string& error_domain,
                                        int error_code) {
  if (error_domain == net::kErrorDomain) {
    return FindErrorMapInArray(net_error_options,
                               arraysize(net_error_options),
                               error_code);
  } else if (error_domain == LocalizedError::kHttpErrorDomain) {
    return FindErrorMapInArray(http_error_options,
                               arraysize(http_error_options),
                               error_code);
  } else {
    NOTREACHED();
    return NULL;
  }

}

bool LocaleIsRTL() {
#if defined(TOOLKIT_GTK)
  // base::i18n::IsRTL() uses the GTK text direction, which doesn't work within
  // the renderer sandbox.
  return base::i18n::ICUIsRTL();
#else
  return base::i18n::IsRTL();
#endif
}

}  // namespace

const char LocalizedError::kHttpErrorDomain[] = "http";

void LocalizedError::GetStrings(const blink::WebURLError& error,
                                base::DictionaryValue* error_strings,
                                const std::string& locale) {
  bool rtl = LocaleIsRTL();
  error_strings->SetString("textdirection", rtl ? "rtl" : "ltr");

  // Grab the strings and settings that depend on the error type.  Init
  // options with default values.
  LocalizedErrorMap options = {
    0,
    IDS_ERRORPAGES_HEADING_NOT_AVAILABLE,
    IDS_ERRORPAGES_SUMMARY_NOT_AVAILABLE,
    SUGGEST_NONE,
  };

  const std::string error_domain = error.domain.utf8();
  int error_code = error.reason;
  const LocalizedErrorMap* error_map = LookupErrorMap(error_domain, error_code);
  if (error_map)
    options = *error_map;

  if (options.suggestions != SUGGEST_NONE) {
    error_strings->SetString(
        "suggestionsHeading",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_HEADING));
  }

  const GURL failed_url = error.unreachableURL;

  // If we got "access denied" but the url was a file URL, then we say it was a
  // file instead of just using the "not available" default message. Just adding
  // ERR_ACCESS_DENIED to the map isn't sufficient, since that message may be
  // generated by some OSs when the operation doesn't involve a file URL.
  if (error_domain == net::kErrorDomain &&
      error_code == net::ERR_ACCESS_DENIED &&
      failed_url.scheme() == "file") {

    options.heading_resource_id = IDS_ERRORPAGES_HEADING_FILE_ACCESS_DENIED;
    options.summary_resource_id = IDS_ERRORPAGES_SUMMARY_FILE_ACCESS_DENIED;
    options.suggestions = SUGGEST_NONE;
  }

  base::string16 failed_url_string(base::UTF8ToUTF16(failed_url.spec()));
  // URLs are always LTR.
  if (rtl)
    base::i18n::WrapStringWithLTRFormatting(&failed_url_string);

  base::string16 host_name(base::ASCIIToUTF16(failed_url.host()));

  if (failed_url.SchemeIsHTTPOrHTTPS())
    error_strings->SetString("title", host_name);
  else
    error_strings->SetString("title", failed_url_string);

  base::DictionaryValue* heading = new base::DictionaryValue;
  heading->SetString("msg",
                     l10n_util::GetStringUTF16(options.heading_resource_id));
  heading->SetString("hostName", host_name);
  error_strings->Set("heading", heading);

  base::DictionaryValue* summary = new base::DictionaryValue;
  summary->SetString("msg",
      l10n_util::GetStringUTF16(options.summary_resource_id));
  // TODO(tc): We want the unicode url and host here since they're being
  //           displayed.
  summary->SetString("failedUrl", failed_url_string);
  summary->SetString("hostName", host_name);
  summary->SetString("productName",
                     l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  error_strings->Set("summary", summary);

  if (options.suggestions & SUGGEST_RELOAD) {
    base::DictionaryValue* suggest_reload = new base::DictionaryValue;
    suggest_reload->SetString("msg",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_RELOAD));
    suggest_reload->SetString("reloadUrl", failed_url_string);
    error_strings->Set("suggestionsReload", suggest_reload);
  }

  if (options.suggestions & SUGGEST_CHECK_CONNECTION) {
    base::DictionaryValue* suggest_check_connection = new base::DictionaryValue;
    suggest_check_connection->SetString("msg",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_CHECK_CONNECTION));
    error_strings->Set("suggestionsCheckConnection", suggest_check_connection);
  }

  if (options.suggestions & SUGGEST_DNS_CONFIG) {
    base::DictionaryValue* suggest_dns_config = new base::DictionaryValue;
    suggest_dns_config->SetString("msg",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_DNS_CONFIG));
    error_strings->Set("suggestionsDNSConfig", suggest_dns_config);
  }

  if (options.suggestions & SUGGEST_FIREWALL_CONFIG) {
    base::DictionaryValue* suggest_firewall_config = new base::DictionaryValue;
    suggest_firewall_config->SetString("msg",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_FIREWALL_CONFIG));
    suggest_firewall_config->SetString("productName",
        l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
    error_strings->Set("suggestionsFirewallConfig", suggest_firewall_config);
  }

  if (options.suggestions & SUGGEST_DISABLE_EXTENSION) {
    base::DictionaryValue* suggestion = new base::DictionaryValue;
    suggestion->SetString("msg",
        l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUGGESTION_DISABLE_EXTENSION));
    suggestion->SetString("reloadUrl", failed_url_string);
    error_strings->Set("suggestionsDisableExtension", suggestion);
  }

#if defined(ENABLE_SEARCH_IN_ERROR_PAGE)
  // Get strings for the search box.
  if (ShouldShowSearchBox(error_domain, error_code))
    opera::ErrorPageSearchDataProvider::GetInstance()->
        GetStrings(error_strings);
#endif  // ENABLE_SEARCH_IN_ERROR_PAGE
}

base::string16 LocalizedError::GetErrorDetails(const blink::WebURLError& error) {
  const LocalizedErrorMap* error_map =
      LookupErrorMap(error.domain.utf8(), error.reason);
  if (error_map)
    return l10n_util::GetStringUTF16(error_map->summary_resource_id);
  else
    return l10n_util::GetStringUTF16(IDS_ERRORPAGES_SUMMARY_NOT_AVAILABLE);
}

bool LocalizedError::HasStrings(const std::string& error_domain,
                                int error_code) {
  return LookupErrorMap(error_domain, error_code) != NULL;
}
