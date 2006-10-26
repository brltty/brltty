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

cdef extern from "Programs/api.h":
	ctypedef struct brlapi_settings_t:
		char *authKey
		char *hostName

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

	void brlapi_closeConnection()
	int brlapi_initializeConnection(brlapi_settings_t*, brlapi_settings_t*)

	int brlapi_getDisplaySize(unsigned int*, unsigned int *y)
	int brlapi_getDriverId(char*, int)
	int brlapi_getDriverName(char*, int)

	int brlapi_enterTtyMode(int, char*)
	int brlapi_leaveTtyMode()
	int brlapi_setFocus(int)

	int brlapi_write(brlapi_writeStruct*)
	int brlapi_writeDots(unsigned char*)
	int brlapi_writeText(int, char*)

	int brlapi_ignoreKeyRange(unsigned long long, unsigned long long)
	int brlapi_unignoreKeyRange(unsigned long long, unsigned long long)
	int brlapi_readKey(int, unsigned long long*)
	int brlapi_expandKeyCode(unsigned long long, unsigned int *, unsigned int *, unsigned int *)

	int brlapi_enterRawMode(char*)
	int brlapi_leaveRawMode()
	int brlapi_recvRaw(void*, int)
	int brlapi_sendRaw(void*, int)

	brlapi_error_t* brlapi_error_location()
	char* brlapi_strerror(brlapi_error_t*)

cdef extern from "Python.h":
	# these are macros, we just need to make Pyrex aware of them
	int Py_BEGIN_ALLOW_THREADS
	int Py_END_ALLOW_THREADS
