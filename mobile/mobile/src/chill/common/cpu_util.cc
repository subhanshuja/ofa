// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/common/cpu_util.h"

#include <cpu-features.h>

#include "base/logging.h"

namespace opera {

// A mapping for AndroidCpuFamily values
const char* const kCpuFamily[ANDROID_CPU_FAMILY_MAX] = {
        "unknown", "arm", "x86", "mips", "arm64", "x86_64", "mips64"};

const char* GetCpuFamilyName() {
  AndroidCpuFamily cpu_family = android_getCpuFamily();
  CHECK_LT(cpu_family, ANDROID_CPU_FAMILY_MAX);
  return kCpuFamily[cpu_family];
}

bool IsNeonSupported() {
  return android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM &&
         android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON;
}

}  // namespace opera
