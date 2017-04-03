// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_JAVA_BRIDGE_JAVA_BRIDGE_H_
#define COMMON_JAVA_BRIDGE_JAVA_BRIDGE_H_

#include "base/macros.h"
#include "base/observer_list.h"

namespace opera {

class NativeInterfaceBase;

class JavaBridge {
 public:
  class Observer {
   public:
    // Called when the native interface is set
    virtual void OnInterfaceSet() = 0;
  };

  static void SetNativeInterface(NativeInterfaceBase* interface);
  static void RemoveNativeInterface();
  static NativeInterfaceBase* GetInterface();

  static void AddObserver(Observer* observer, bool call_if_set);
  static void RemoveObserver(Observer* observer);

 private:
  static NativeInterfaceBase* native_interface_;
  static base::ObserverList<Observer> observers_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(JavaBridge);
};

}  // namespace opera

#endif  // COMMON_JAVA_BRIDGE_JAVA_BRIDGE_H_
