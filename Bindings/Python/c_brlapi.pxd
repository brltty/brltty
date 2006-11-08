###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2006 by Alexis Robert <alexissoft@free.fr>
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

cdef extern from "Programs/api.h":
	ctypedef struct brlapi_settings_t:
		char *authKey
		char *host

	ctypedef struct brlapi_writeStruct:
		unsigned int regionBegin
		unsigned int regionSize
		char *text
		unsigned char *attrAnd
		unsigned char *attrOr
		int cursor
		char *charset

	ctypedef struct brlapi_error_t:
		int brlerror
		int libcerror
		int gaierror
		char *errfun
	ctypedef struct brlapi_handle_t

	size_t brlapi_getHandleSize()
	void brlapi__closeConnection(brlapi_handle_t *)
	int brlapi__openConnection(brlapi_handle_t *, brlapi_settings_t*, brlapi_settings_t*)

	int brlapi__getDisplaySize(brlapi_handle_t *, unsigned int*, unsigned int *y)
	int brlapi__getDriverId(brlapi_handle_t *, char*, int)
	int brlapi__getDriverName(brlapi_handle_t *, char*, int)

	int brlapi__enterTtyMode(brlapi_handle_t *, int, char*)
	int brlapi__leaveTtyMode(brlapi_handle_t *)
	int brlapi__setFocus(brlapi_handle_t *, int)

	int brlapi__write(brlapi_handle_t *, brlapi_writeStruct*)
	int brlapi__writeDots(brlapi_handle_t *, unsigned char*)
	int brlapi__writeText(brlapi_handle_t *, int, char*)

	int brlapi__ignoreKeyRange(brlapi_handle_t *, unsigned long long, unsigned long long)
	int brlapi__acceptKeyRange(brlapi_handle_t *, unsigned long long, unsigned long long)
	int brlapi__readKey(brlapi_handle_t *, int, unsigned long long*)
	int brlapi_expandKeyCode(unsigned long long, unsigned int *, unsigned int *, unsigned int *)

	int brlapi__enterRawMode(brlapi_handle_t *, char*)
	int brlapi__leaveRawMode(brlapi_handle_t *)
	int brlapi__recvRaw(brlapi_handle_t *, void*, int)
	int brlapi__sendRaw(brlapi_handle_t *, void*, int)

	brlapi_error_t* brlapi_error_location()
	char* brlapi_strerror(brlapi_error_t*)

cdef extern from "stdlib.h":
	void *malloc(size_t)
	void free(void*)

cdef extern from "Python.h":
	# these are macros, we just need to make Pyrex aware of them
	int Py_BEGIN_ALLOW_THREADS
	int Py_END_ALLOW_THREADS
