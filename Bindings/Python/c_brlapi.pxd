###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2007 by Alexis Robert <alexissoft@free.fr>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License,
# or (at your option) any later version.
# Please see the file COPYING-API for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

# File binding C functions

cdef extern from "sys/types.h":
	ctypedef int size_t

cdef extern from "Programs/brlapi.h":
	ctypedef struct brlapi_connectionSettings_t:
		char *auth
		char *host

	ctypedef struct brlapi_writeStruct_t:
		int displayNumber
		unsigned int regionBegin
		unsigned int regionSize
		char *text
		int textSize
		unsigned char *attrAnd
		unsigned char *attrOr
		int cursor
		char *charset

	ctypedef struct brlapi_expandedKeyCode_t:
		unsigned int type
		unsigned int command
		unsigned int argument
		unsigned int flags

	ctypedef struct brlapi_error_t:
		int brlerror
		int libcerror
		int gaierror
		char *errfun

	ctypedef struct brlapi_handle_t

	ctypedef unsigned long long brlapi_keyCode_t

	size_t brlapi_getHandleSize()
	void brlapi__closeConnection(brlapi_handle_t *)
	int brlapi__openConnection(brlapi_handle_t *, brlapi_connectionSettings_t*, brlapi_connectionSettings_t*)

	int brlapi__getDisplaySize(brlapi_handle_t *, unsigned int*, unsigned int *y)
	int brlapi__getDriverName(brlapi_handle_t *, char*, int)

	int brlapi__enterTtyMode(brlapi_handle_t *, int, char*)
	int brlapi__enterTtyModeWithPath(brlapi_handle_t *, int *, int, char*)
	int brlapi__leaveTtyMode(brlapi_handle_t *)
	int brlapi__setFocus(brlapi_handle_t *, int)

	int brlapi__write(brlapi_handle_t *, brlapi_writeStruct_t*)
	int brlapi__writeDots(brlapi_handle_t *, unsigned char*)
	int brlapi__writeText(brlapi_handle_t *, int, char*)

	ctypedef enum brlapi_rangeType_t:
		brlapi_rangeType_all
	
	ctypedef struct brlapi_range_t:
		brlapi_keyCode_t first
		brlapi_keyCode_t last

	int brlapi__ignoreKeys(brlapi_handle_t *, brlapi_rangeType_t, brlapi_keyCode_t *, unsigned int)
	int brlapi__acceptKeys(brlapi_handle_t *, brlapi_rangeType_t, brlapi_keyCode_t *, unsigned int)
	int brlapi__ignoreAllKeys(brlapi_handle_t *)
	int brlapi__acceptAllKeys(brlapi_handle_t *)
	int brlapi__ignoreKeyRanges(brlapi_handle_t *, brlapi_range_t *, unsigned int)
	int brlapi__acceptKeyRanges(brlapi_handle_t *, brlapi_range_t *, unsigned int)
	int brlapi__readKey(brlapi_handle_t *, int, brlapi_keyCode_t*)
	int brlapi_expandKeyCode(brlapi_keyCode_t, brlapi_expandedKeyCode_t *)

	int brlapi__enterRawMode(brlapi_handle_t *, char*)
	int brlapi__leaveRawMode(brlapi_handle_t *)
	int brlapi__recvRaw(brlapi_handle_t *, void*, int)
	int brlapi__sendRaw(brlapi_handle_t *, void*, int)

	brlapi_error_t* brlapi_error_location()
	char* brlapi_strerror(brlapi_error_t*)
	brlapi_keyCode_t BRLAPI_KEY_MAX
	brlapi_keyCode_t BRLAPI_KEY_FLAGS_MASK
	brlapi_keyCode_t BRLAPI_KEY_TYPE_MASK
	brlapi_keyCode_t BRLAPI_KEY_CODE_MASK
	brlapi_keyCode_t BRLAPI_KEY_CMD_BLK_MASK
	brlapi_keyCode_t BRLAPI_KEY_CMD_ARG_MASK

cdef extern from "bindings.h":
	brlapi_writeStruct_t brlapi_writeStruct_initialized

cdef extern from "stdlib.h":
	void *malloc(size_t)
	void free(void*)

cdef extern from "string.h":
	void *memcpy(void *, void *, size_t)

cdef extern from "Python.h":
	# these are macros, we just need to make Pyrex aware of them
	int Py_BEGIN_ALLOW_THREADS
	int Py_END_ALLOW_THREADS
