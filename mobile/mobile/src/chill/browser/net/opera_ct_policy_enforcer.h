// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software

#ifndef MOBILE_MOBILE_SRC_CHILL_BROWSER_NET_OPERA_CT_POLICY_ENFORCER_H_
#define MOBILE_MOBILE_SRC_CHILL_BROWSER_NET_OPERA_CT_POLICY_ENFORCER_H_

#include "net/cert/ct_policy_enforcer.h"

namespace opera {

class OperaCTPolicyEnforcer : public net::CTPolicyEnforcer {
 public:
  virtual ~OperaCTPolicyEnforcer() {}

  net::ct::CertPolicyCompliance DoesConformToCertPolicy(
      net::X509Certificate* cert,
      const net::SCTList& verified_scts,
      const net::NetLogWithSource& net_log) override;

  net::ct::EVPolicyCompliance DoesConformToCTEVPolicy(
      net::X509Certificate* cert,
      const net::ct::EVCertsWhitelist* ev_whitelist,
      const net::SCTList& verified_scts,
      const net::NetLogWithSource& net_log) override;
};

}  // namespace opera

#endif  // MOBILE_MOBILE_SRC_CHILL_BROWSER_NET_OPERA_CT_POLICY_ENFORCER_H_
