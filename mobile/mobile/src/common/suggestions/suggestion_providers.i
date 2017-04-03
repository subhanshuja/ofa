// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/suggestion/suggestion_provider.h"
%}

namespace opera {

%nodefaultctor SuggestionProvider;
%nodefaultdtor SuggestionProvider;

SWIG_ADD_DISOWN_METHOD(SuggestionProvider)
class SuggestionProvider {
};

}  // opera
