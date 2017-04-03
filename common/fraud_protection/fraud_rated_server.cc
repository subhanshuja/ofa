// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection/fraud_rated_server.h"

#include <algorithm>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/escape.h"
#include "net/url_request/url_fetcher.h"
#include "net/http/http_status_code.h"
#include "third_party/libxml/chromium/libxml_utils.h"

#include "common/constants/switches.h"
#include "common/fraud_protection/fraud_advisory.h"
#include "common/fraud_protection/fraud_url_rating.h"
#include "common/net/sensitive_url_request_user_data.h"

#if defined(OPERA_DESKTOP)
#include "desktop/common/brand/brand_info.h"
#endif

namespace opera {

namespace {
const char* kSiteCheckHDNSuffix = "-Oscar0308";
const char* kSiteCheckUrlTemplate = "%p%h/?host=%n&hdn=%d";
const char* kAdvisoryImageUrlTemplate = "http://%h/img/logo-%i.jpg";

// The fraud check XML response consists of:
// <ce> tag carying the expire time of this response. After that time
// cached response expires and new request needs to be done. This becomes the
// propery of FraudRatedServer. Each FraudRatedServer keeps a list of
// advisories that are defined inside <sourcemap> tag.
//
// <sourcemap> tag is carying list of advisories. <sourcemap> contains
// one or more <source> tags. Each source (advisory) is defined by its
// unique ID, type (either fraud or malware) and other data such as
// source homepage, etc. Each advisory contains list of urls or regular
// expression (called a detectors) that are used to check if given Url
// is fraudlent or not. The detectors are defined by <us> and <rs> tags.
//
// <us> and <rs> tags carry a list of detectors for each advisory defined
// in <sourcemap> section. Each <u> or <r> tag (where <u> defines an Url
// detector and <r> defines a RegEx detector) has a sources id attribute
// that links each detector to a previously defined source.
//
// Finally, after putting everything together, we have a list of advisories
// where each advisory keeps a list of detectors.
//
// For example, following response XML:
//
// <?xml version="1.0" encoding="utf-8"?>
// <operatrust version="1.1">
//  <action type="searchresponse">
//    <host>hostname</host>
//    <ce>expire_time</ce>
//    <sourcemap>
//      <source id='1' type='1'>advisory_1</source>
//      <source id='2' type='1'>asvisory_2</source>
//    </sourcemap>
//    <rs>
//      <r src='1'>regex_1</r>
//      <r src='2'>regex_2</r>
//    </rs>
//    <us>
//      <u src='2'>url_1</u>
//    </us>
//   </action>
// </operatrust>
//
// will produce following structure:
//
// FraudRatedServer[hostname, expires after expire_time]
//  +- FraudAdvisory[advisory_1 of type 1]
//  |   +- RegExDetector[regex_1]
//  |
//  +- FraudAdvisory[advisory_2 of type 1]
//      +- RegExDetector[regex_2]
//      +- HostFraudDetector[url_1]

// Tags
const char* kTagExpireTime = "ce";              // Expire time
const char* kTagSource = "source";              // Advisory source
const char* kTagUrl = "u";                      // Fraudlent URL
const char* kTagRegEx = "r";                    // Fraud matching RegEx

// Attributes for Source Tag
const char* kAttrSourceId = "id";                // Source ID
const char* kAttrSourceType = "type";            // Source type
const char* kAttrSourceAdvisory = "advisory";    // Advisory URL
const char* kAttrSourceHomepage = "homepage";    // Advisory homepage

// Atributes for both: Url and RegEx tag
const char* kAttrAdvisorySourceId = "src";       // Source ID

std::string GetSitecheckHost() {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(opera::switches::kFraudCheckServer)) {
    return cmd_line->GetSwitchValueASCII(opera::switches::kFraudCheckServer);
  } else {
#if defined(OPERA_DESKTOP)
    auto sitecheck_host = brand_info::SitecheckHost();
    DCHECK(!sitecheck_host.empty());
    return sitecheck_host;
#else
    const char* const kSitecheckHost = "sitecheck2.opera.com";
    return kSitecheckHost;
#endif
  }
}

void ReplaceStringTemplateParam(std::string* template_str,
                                const std::string& param,
                                const std::string& value) {
  size_t pos = template_str->find(param);
  DCHECK(pos != std::string::npos);
  template_str->replace(pos, param.size(), value);
}

void GenerateSiteCheckRequestUrl(const std::string& hostname,
                                 const std::string& hdn,
                                 bool use_https,
                                 std::string* url) {
  *url = kSiteCheckUrlTemplate;

  ReplaceStringTemplateParam(url, "%p", use_https ? "https://" : "http://");

  ReplaceStringTemplateParam(url, "%h", GetSitecheckHost());
  ReplaceStringTemplateParam(url, "%n", hostname);
  ReplaceStringTemplateParam(url, "%d", hdn);
}

void GenerateAdvisoryImageUrl(int advisory_id, std::string* url) {
  *url = kAdvisoryImageUrlTemplate;

  ReplaceStringTemplateParam(url, "%h", GetSitecheckHost());
  ReplaceStringTemplateParam(url, "%i", base::IntToString(advisory_id));
}

}  // namespace

FraudRatedServer::FraudRatedServer(
    const std::string& hostname,
    bool is_secure,
    const RatingCompleteCallback& callback,
    net::URLRequestContextGetter* request_context_getter)
    : state_(SERVER_STATE_UNRATED),
      hostname_(hostname),
      is_secure_(is_secure),
      rating_bypassed_by_user_(false),
      rating_complete_callback_(callback),
      request_context_getter_(request_context_getter) {
}

FraudRatedServer::~FraudRatedServer() {
}

bool FraudRatedServer::IsExpired() const {
  // User bypassed servers never expire
  return !rating_bypassed_by_user_ && !expire_time_.is_null() &&
         expire_time_ < base::Time::NowFromSystemTime();
}

bool FraudRatedServer::IsRated(bool never_expire) const {
  return (never_expire || !IsExpired()) && state_ == SERVER_STATE_RATED;
}

void FraudRatedServer::GetRatingForUrl(const GURL& url,
                                       FraudUrlRating* rating) const {
  if (state_ == SERVER_STATE_RATED) {
    rating->rating_ = FraudUrlRating::RATING_URL_FRAUD_NOT_DETECTED;
    for (Advisories::const_iterator iter = advisories_.begin();
        iter != advisories_.end();
        ++iter) {
      if (iter->second.IsFraudlentUrl(url)) {
        rating->server_bypassed_ = IsBypassed();
        rating->advisory_text_ = iter->second.text();
        rating->advisory_url_ = iter->second.url();
        rating->advisory_homepage_ = iter->second.homepage();
        GenerateAdvisoryImageUrl(iter->first, &rating->advisory_logo_url_);

        switch (iter->second.type()) {
          case FraudAdvisory::ADVISORY_TYPE_PHISHING:
            rating->rating_ = FraudUrlRating::RATING_URL_PHISHING;
            break;
          case FraudAdvisory::ADVISORY_TYPE_MALWARE:
            rating->rating_ = FraudUrlRating::RATING_URL_MALWARE;
            break;
          default:
            NOTREACHED();
            break;
        }
        break;
      }
    }
  } else {
    rating->rating_ = FraudUrlRating::RATING_URL_NOT_RATED;
  }
}

void FraudRatedServer::StartRating() {
  DCHECK(hostname_.length());

  if (state_ == SERVER_STATE_RATING_IN_PROGRESS)
    return;

  advisories_.clear();
  state_ = SERVER_STATE_RATING_IN_PROGRESS;

  std::string hdn_value = hostname_;
  hdn_value.append(kSiteCheckHDNSuffix);

  base::MD5Digest md5;
  base::MD5Sum(hdn_value.c_str(), hdn_value.length(), &md5);

  std::string encoded_hash;
  base::Base64Encode(base::StringPiece(
      reinterpret_cast<const char*>(&md5), 16), &encoded_hash);

  encoded_hash = net::EscapeQueryParamValue(encoded_hash, false);

  std::string request;
  GenerateSiteCheckRequestUrl(hostname_, encoded_hash, is_secure_, &request);

  DCHECK(!url_fetcher_);

  url_fetcher_ =
      net::URLFetcher::Create(GURL(request), net::URLFetcher::GET, this);
  url_fetcher_->SetRequestContext(request_context_getter_);
  url_fetcher_->SetURLRequestUserData(
      opera::SensitiveURLRequestUserData::kUserDataKey,
      base::Bind(&SensitiveURLRequestUserData::Create));
  url_fetcher_->Start();
}

void FraudRatedServer::SetRatingBypassed() {
  rating_bypassed_by_user_ = true;
}

bool FraudRatedServer::IsBypassed() const {
  return rating_bypassed_by_user_;
}

void FraudRatedServer::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(source == url_fetcher_.get());
  if (url_fetcher_->GetResponseCode() != net::HTTP_OK) {
    OnServerRated(false);
    return;
  }

  std::string trust_info_xml;
  url_fetcher_->GetResponseAsString(&trust_info_xml);

  XmlReader xml_parser;
  if (!xml_parser.Load(trust_info_xml)) {
    OnServerRated(false);
    return;
  }

  while (xml_parser.Read()) {
    std::string node_name = xml_parser.NodeName();
    if (base::LowerCaseEqualsASCII(node_name, kTagExpireTime)) {
      XMLHandleExpireTime(xml_parser);
    } else if (base::LowerCaseEqualsASCII(node_name, kTagSource)) {
      XMLHandleSource(xml_parser);
    } else if (base::LowerCaseEqualsASCII(node_name, kTagUrl) ||
               base::LowerCaseEqualsASCII(node_name, kTagRegEx)) {
      XMLHandleFraudInfo(xml_parser);
    }
  }

  PruneInvalidAdvisories();
  OnServerRated(true);
}

void FraudRatedServer::OnServerRated(bool result) {
  url_fetcher_.reset(NULL);
  state_ = result ? SERVER_STATE_RATED : SERVER_STATE_UNRATED;
  rating_complete_callback_.Run(this);
}

void FraudRatedServer::XMLHandleExpireTime(XmlReader& xml_parser) {
  std::string expire_time;
  int expire_time_int;

  if (!xml_parser.ReadElementContent(&expire_time))
    return;

  if (base::StringToInt(expire_time, &expire_time_int) && expire_time_int)
    expire_time_ = base::Time::NowFromSystemTime() +
        base::TimeDelta::FromSeconds(expire_time_int);
}

void FraudRatedServer::XMLHandleSource(XmlReader& xml_parser) {
  std::string id_str, type_str;
  int advisory_id, type;

  if (!xml_parser.NodeAttribute(kAttrSourceId, &id_str) ||
      !xml_parser.NodeAttribute(kAttrSourceType, &type_str) ||
      !base::StringToInt(id_str, &advisory_id) ||
      !base::StringToInt(type_str, &type))
    return;

  FraudAdvisory& advisory = advisories_[advisory_id];
  switch (type) {
    case 1:
      advisory.set_type(FraudAdvisory::ADVISORY_TYPE_PHISHING);
      break;

    case 2:
      advisory.set_type(FraudAdvisory::ADVISORY_TYPE_MALWARE);
      break;
  }

  std::string attribute_val;
  if (xml_parser.NodeAttribute(kAttrSourceAdvisory, &attribute_val))
    advisory.set_url(attribute_val);

  attribute_val.clear();
  if (xml_parser.NodeAttribute(kAttrSourceHomepage, &attribute_val))
    advisory.set_homepage(attribute_val);

  attribute_val.clear();
  if (xml_parser.ReadElementContent(&attribute_val))
    advisory.set_text(attribute_val);
}

void FraudRatedServer::XMLHandleFraudInfo(XmlReader& xml_parser) {
  std::string id_str;
  int advisory_id;

  bool is_url_tag = base::LowerCaseEqualsASCII(xml_parser.NodeName(), kTagUrl);
  if (!xml_parser.NodeAttribute(kAttrAdvisorySourceId, &id_str) ||
      !base::StringToInt(id_str, &advisory_id))
    return;

  FraudAdvisory& advisory = advisories_[advisory_id];
  std::string val;
  if (xml_parser.ReadElementContent(&val)) {
    if (is_url_tag)
      advisory.AddHostBasedFraud(val);
    else
      advisory.AddRegExBasedFraud(val);
  }
}

void FraudRatedServer::PruneInvalidAdvisories() {
  Advisories::iterator iter = advisories_.begin();
  while (iter != advisories_.end()) {
    if (!iter->second.IsValid()) {
      advisories_.erase(iter++);
    } else {
      ++iter;
    }
  }
}

}  // namecpace opera
