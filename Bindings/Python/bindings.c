/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2005-2007 by
 *   Alexis Robert <alexissoft@free.fr>
 *   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * Please see the file COPYING-API for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* bindings.c provides initialized variables to the Python bindings */

#include "Programs/brlapi.h"

const brlapi_writeArguments_t brlapi_writeArguments_initialized = BRLAPI_WRITEARGUMENTS_INITIALIZER;
