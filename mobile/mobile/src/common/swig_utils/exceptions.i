// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
const char* GetStoredException();
void HandleDirectorException(JNIEnv* env, jobject swigjobj);
%}

%allowexception;

%exception %{
  /* Intentionally not handling exceptions from: */
  $action
%}

%feature("director:except") %{
  if (jenv->ExceptionCheck()) {
    HandleDirectorException(jenv, swigjobj);
    return $null;
  }
%}

const char* GetStoredException();
