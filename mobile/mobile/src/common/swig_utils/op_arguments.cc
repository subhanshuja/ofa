// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/swig_utils/op_arguments.h"

#include "base/logging.h"

OpArguments::~OpArguments() {
}

jobject OpArguments::GetObject(
    JNIEnv* jenv, const std::string& package_name) const {
  if (!object_.is_null()) {
    return object_.obj();
  }

  std::string clazz_name = GetClassName();
  if (clazz_name.empty()) {
    return NULL;
  }

  std::string full_name;
  full_name.append(package_name);
  full_name.append("/");
  full_name.append(clazz_name);

  jclass clazz = jenv->FindClass(full_name.c_str());
  DCHECK(clazz);
  jmethodID init_id = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
  DCHECK(init_id);
  object_.Reset(jenv, jenv->NewObject(
      clazz, init_id, reinterpret_cast<jlong>(this), false));

  return object_.obj();
}

OpArguments* OpArguments::SetObject(JNIEnv* jenv, jobject object) {
  object_.Reset(jenv, object);
  return this;
}

std::string OpArguments::GetClassName() const {
  std::string full_name = pretty_function();

  size_t pos0 = full_name.find_last_not_of(":", full_name.find_last_of(":"));

  if (pos0 == std::string::npos) {
    return std::string();
  }

  size_t pos1 = full_name.find_last_of(" \t:", pos0 - 1);

  if (pos1 == std::string::npos) {
    return std::string();
  }

  return full_name.substr(pos1 + 1, pos0 - pos1);
}
