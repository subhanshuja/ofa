// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_RESOURCE_CONTEXT_H_
#define CHILL_BROWSER_OPERA_RESOURCE_CONTEXT_H_

#include "content/public/browser/resource_context.h"

namespace opera {

class OperaURLRequestContextGetter;

class OperaResourceContext : public content::ResourceContext {
 public:
  explicit OperaResourceContext(bool off_the_record);
  virtual ~OperaResourceContext() {}

  net::HostResolver* GetHostResolver() override;
  net::URLRequestContext* GetRequestContext() override;

  void set_url_request_context_getter(OperaURLRequestContextGetter* getter);

  bool IsOffTheRecord() const;

 private:
  OperaURLRequestContextGetter* getter_;
  bool off_the_record_;

  DISALLOW_COPY_AND_ASSIGN(OperaResourceContext);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_RESOURCE_CONTEXT_H_
