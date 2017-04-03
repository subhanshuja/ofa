// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_FRAUD_ADVISORY_H_
#define COMMON_FRAUD_PROTECTION_FRAUD_ADVISORY_H_

#include <list>
#include <string>

#include "base/compiler_specific.h"
#include "url/gurl.h"

namespace opera {

class FraudAdvisory {
 public:
  enum ServerSideType {
    ADVISORY_TYPE_UNKNOWN,
    ADVISORY_TYPE_PHISHING,
    ADVISORY_TYPE_MALWARE
  };

  FraudAdvisory();
  FraudAdvisory(const FraudAdvisory&);
  ~FraudAdvisory();

  ServerSideType type() const { return type_; }
  const std::string& text() const { return text_; }
  const std::string& url() const { return url_; }
  const std::string& homepage() const { return homepage_; }

  void set_type(ServerSideType type) { type_ = type; }
  void set_text(const std::string& text) { text_ = text; }
  void set_url(const std::string& url) { url_ = url; }
  void set_homepage(const std::string& homepage) { homepage_ = homepage; }

  void AddHostBasedFraud(const std::string& host_url);
  void AddRegExBasedFraud(const std::string& regexp);
  bool IsValid() const;
  bool IsFraudlentUrl(const GURL& url) const;

 private:
  class FraudDetector {
   public:
    explicit FraudDetector(const std::string& t) : template_(t) {}
    virtual ~FraudDetector() {}

    virtual bool IsFraudlentUrl(const GURL& url) const = 0;

   protected:
    std::string template_;
  };

  class HostFraudDetector : public FraudDetector {
   public:
    explicit HostFraudDetector(const std::string& t) : FraudDetector(t) {}
    ~HostFraudDetector() override {}

    bool IsFraudlentUrl(const GURL& url) const override;
  };

  class RegExDetector : public FraudDetector {
   public:
    explicit RegExDetector(const std::string& t) : FraudDetector(t) {}
    ~RegExDetector() override {}

    bool IsFraudlentUrl(const GURL& url) const override;
  };

  typedef std::list<FraudDetector*> FraudDetectors;

  ServerSideType type_;
  std::string url_;
  std::string homepage_;
  std::string text_;

  FraudDetectors detectors_;
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_FRAUD_ADVISORY_H_
