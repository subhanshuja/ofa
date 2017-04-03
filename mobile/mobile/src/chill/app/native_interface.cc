// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/app/native_interface.h"

#include "common/java_bridge/java_bridge.h"

namespace opera {

// static
NativeInterface* NativeInterface::Get() {
  return static_cast<NativeInterface*>(JavaBridge::GetInterface());
}

}  // namespace opera
