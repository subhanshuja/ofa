// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/net/opera_url_request_context_getter.h"
#include "chill/browser/opera_resource_context.h"

namespace opera {

OperaResourceContext::OperaResourceContext(bool off_the_record)
  : getter_(NULL),
    off_the_record_(off_the_record) {
}

net::HostResolver* OperaResourceContext::GetHostResolver() {
  CHECK(getter_);
  return getter_->host_resolver();
}

net::URLRequestContext* OperaResourceContext::GetRequestContext() {
  CHECK(getter_);
  return getter_->GetURLRequestContext();
}

void OperaResourceContext::set_url_request_context_getter(
    OperaURLRequestContextGetter* getter) {
  getter_ = getter;
}

bool OperaResourceContext::IsOffTheRecord() const {
  return off_the_record_;
}

}  // namespace opera
