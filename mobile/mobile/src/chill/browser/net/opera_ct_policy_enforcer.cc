// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software

#include "mobile/mobile/src/chill/browser/net/opera_ct_policy_enforcer.h"

#include "net/cert/ct_policy_status.h"

namespace opera {

net::ct::CertPolicyCompliance OperaCTPolicyEnforcer::DoesConformToCertPolicy(
    net::X509Certificate* cert,
    const net::SCTList& verified_scts,
    const net::NetLogWithSource& net_log) {
  return net::ct::CertPolicyCompliance::CERT_POLICY_COMPLIES_VIA_SCTS;
}

net::ct::EVPolicyCompliance OperaCTPolicyEnforcer::DoesConformToCTEVPolicy(
    net::X509Certificate* cert,
    const net::ct::EVCertsWhitelist* ev_whitelist,
    const net::SCTList& verified_scts,
    const net::NetLogWithSource& net_log) {
  return net::ct::EVPolicyCompliance::EV_POLICY_COMPLIES_VIA_SCTS;
}

}  // namespace opera
