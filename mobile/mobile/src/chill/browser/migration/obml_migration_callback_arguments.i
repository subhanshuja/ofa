// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/migration/obml_migration_callback_arguments.h"
#include "common/swig_utils/op_arguments.h"
%}

class OBMLMigrationCallbackArguments : public OpArguments {
 public:
  bool success;
};
