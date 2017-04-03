// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "url/gurl.h"
%}

class GURL {
 public:
  GURL(const std::string& url_string);
  bool is_valid() const;
  bool is_empty() const;
  const std::string& spec() const;
  std::string host() const;
  bool HostIsIPAddress() const;
};
