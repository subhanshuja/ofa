// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/certificates/certificate_utils.h"
%}

%typemap(jstype) jobjectArray GetCertificateChain(jobject) "byte[][]"
%typemap(jtype) jobjectArray GetCertificateChain(jobject) "byte[][]"
%rename("getCertificateChain") GetCertificateChain(jobject);
%rename("getConnectionStatusInfo") GetConnectionStatusInfo(jobject);

%include "certificate_utils.h"