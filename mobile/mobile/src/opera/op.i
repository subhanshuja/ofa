// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%module(directors="1") Op

SWIG_JAVABODY_PROXY(public, public, SWIGTYPE)
SWIG_JAVABODY_TYPEWRAPPER(public, public, public, SWIGTYPE)

%abstractdirector;

%include "enums.swg"
%javaconst(1);

%include "various.i"

%include "std_string.i"
%include "std_vector.i"

%include "../common/swig_utils/swig_utils.i"

%include "../common/bitmap/bitmap.i"
%include "../common/breakpad/breakpad.i"
%include "../common/color_util/op_url_color_table.i"
%include "../common/breakpad/dump_without_crashing.i"
%include "../common/java_bridge/java_bridge.i"
%include "../common/settings/settings.i"
%include "../common/suggestions/suggestions.i"
%include "../common/sync/sync.i"

%include "../chill/chill.i"

%include "../common/url_fixer_upper/url_fixer_upper.i"
%include "../common/utils/statfs_utils.i"
