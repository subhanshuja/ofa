// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/java_bridge/java_bridge.h"

#include "base/logging.h"

#include "common/java_bridge/native_interface_base.h"

namespace opera {

// static
NativeInterfaceBase* JavaBridge::native_interface_ = NULL;

// static
base::ObserverList<JavaBridge::Observer> JavaBridge::observers_;

void JavaBridge::SetNativeInterface(NativeInterfaceBase* native_interface) {
  DCHECK(!native_interface_);
  DCHECK(native_interface);

  native_interface_ = native_interface;

  FOR_EACH_OBSERVER(Observer, observers_, OnInterfaceSet());
}

void JavaBridge::RemoveNativeInterface() {
  delete native_interface_;
}

NativeInterfaceBase* JavaBridge::GetInterface() {
  return native_interface_;
}

void JavaBridge::AddObserver(JavaBridge::Observer* observer, bool call_if_set) {
  observers_.AddObserver(observer);

  if (call_if_set && native_interface_)
    observer->OnInterfaceSet();
}

void JavaBridge::RemoveObserver(JavaBridge::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace opera
