/* -*- Mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Opera Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <pthread.h>

/* Need op_* functions in these files. */
#include "mini/c/shared/generic/vm_integ/op_wrappers.h"

/* Bream Graphics. */
#include "bream/vm/c/graphics/bream_graphics.c"
#include "bream/vm/c/graphics/deferredpainter.c"
#include "bream/vm/c/graphics/focus.c"
#include "bream/vm/c/graphics/gles1.c"
#include "bream/vm/c/graphics/image_info.c"
#include "bream/vm/c/graphics/imagedecoder.c"
#include "bream/vm/c/graphics/internal_io.c"
#include "bream/vm/c/graphics/obmlpainter.c"
#include "bream/vm/c/graphics/region.c"
#include "bream/vm/c/graphics/scale.c"
#include "bream/vm/c/graphics/swrender.c"
#include "bream/vm/c/graphics/textcache.c"
#include "bream/vm/c/graphics/thread.c"
#include "bream/vm/c/graphics/thread_pthread.c"
#include "bream/vm/c/graphics/tilescontainer.c"
#include "bream/vm/c/graphics/tilesworker.c"
#include "bream/vm/c/graphics/utils.c"
