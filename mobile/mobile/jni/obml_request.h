/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2015 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef JNI_OBML_REQUEST_H_
#define JNI_OBML_REQUEST_H_

#define EXPORT __attribute__((visibility("default")))

/* This function is called from libopera.so through dlsym(), meaning that if
 * the name of this function changes, so must the string in
 * chill/browser/cobranding/obml_request.cc too.
 */
EXPORT void MakeBlindOBMLRequest(const char* url);

#endif  // JNI_OBML_REQUEST_H_
