// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "base/android/jni_android.h"
%}

%define VECTOR(NAME, TYPE)
%typemap(javaimports) std::vector<TYPE> %{
import java.util.AbstractCollection;
import java.util.AbstractList;
import java.util.Collection;
import java.util.Iterator;
%}
%typemap(javainterfaces) std::vector<TYPE> "Iterable<$typemap(jstype, TYPE)>"
%typemap(javacode) std::vector<TYPE> %{

  public Iterator<$typemap(jstype, TYPE)> iterator() {
    return getCollection().iterator();
  }
  public Collection<$typemap(jstype, TYPE)> getCollection() {
    return new AbstractCollection<$typemap(jstype, TYPE)>() {
      public $typemap(jstype, TYPE) get(int location) {
        return NAME.this.get(location);
      }

      public int size() {
        return (int)NAME.this.size();
      }
      public Iterator<$typemap(jstype, TYPE)> iterator() {
        return new AbstractList<$typemap(jstype, TYPE)>() {
          public $typemap(jstype, TYPE) get(int location) {
            return NAME.this.get(location);
          }

          public int size() {
            return (int)NAME.this.size();
          }
        }.iterator();
      }
    };
  }
%}
%template(NAME) std::vector<TYPE>;
%enddef

%typemap(javacode) SWIGTYPE %{
  public boolean equals(Object obj) {
    boolean equal = false;
    if (obj instanceof $javaclassname)
      equal = ((($javaclassname)obj).swigCPtr == this.swigCPtr);
    return equal;
  }

  public int hashCode() {
    return (int)swigCPtr;
  }
%}

%define _SWIG_SELFREF_CONSTRUCTOR(TYPENAME, NS...)
SWIG_JAVABODY_PROXY(private, public, TYPENAME)
%typemap(javaconstruct, directorconnect="$imclassname.$javaclazznamedirector_connect(this, swigCPtr, false, true)") TYPENAME {
    this($imcall, false);
    $moduleJNI ## .Init ## TYPENAME ## (swigCPtr, this, this);
    $directorconnect;
}
%inline {
void Init ## TYPENAME(NS ## TYPENAME* cPtr, jobject object) {
  cPtr->Reset(base::android::AttachCurrentThread(), object);
}
}
%enddef

%define SWIG_SELFREF_CONSTRUCTOR(TYPENAME)
_SWIG_SELFREF_CONSTRUCTOR(TYPENAME)
%enddef

%define SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(NS, TYPENAME)
_SWIG_SELFREF_CONSTRUCTOR(TYPENAME, NS ## ::)
%enddef

%define SWIG_ADD_DISOWN_METHOD(TYPENAME)
%typemap(javacode) TYPENAME %{
  public long getCPtrAndReleaseOwnership() {
    swigCMemOwn = false;
    return TYPENAME.getCPtr(this);
  }
%}
%enddef

%typemap(javain) SWIGTYPE *JAVADISOWN "$javainput.getCPtrAndReleaseOwnership()"

// Let the Java proxy object take ownership of the C++ instance it points to.
%define JAVA_PROXY_TAKES_OWNERSHIP(TYPENAME)
%typemap(javadirectorin) TYPENAME* "($jniinput == 0) ? null : new $javaclassname($jniinput, true)"
%enddef
