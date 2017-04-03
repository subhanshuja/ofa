// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/java_bridge/java_bridge.h"
%}

namespace opera {

class NativeInterfaceBase {
};

class JavaBridge {
 public:
  static void SetNativeInterface(opera::NativeInterfaceBase*);
  static void RemoveNativeInterface();
 private:
  JavaBridge();
};

}  // namespace opera
