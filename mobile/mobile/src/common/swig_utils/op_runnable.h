// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SWIG_UTILS_OP_RUNNABLE_H_
#define COMMON_SWIG_UTILS_OP_RUNNABLE_H_

#include "base/callback.h"

#include "common/swig_utils/op_arguments.h"

typedef base::Callback<void(const OpArguments&)> OpRunnable;

#endif  // COMMON_SWIG_UTILS_OP_RUNNABLE_H_
