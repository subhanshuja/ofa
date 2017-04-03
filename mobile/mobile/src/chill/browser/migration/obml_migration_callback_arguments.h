// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_MIGRATION_OBML_MIGRATION_CALLBACK_ARGUMENTS_H_
#define CHILL_BROWSER_MIGRATION_OBML_MIGRATION_CALLBACK_ARGUMENTS_H_

#include "common/swig_utils/op_arguments.h"

class OBMLMigrationCallbackArguments : public OpArguments {
 public:
  bool success;
 private:
  SWIG_CLASS_NAME
};

#endif  // CHILL_BROWSER_MIGRATION_OBML_MIGRATION_CALLBACK_ARGUMENTS_H_
