###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2021 by
#   Alexis Robert <alexissoft@free.fr>
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

# File binding C functions

from libc.stdint cimport uint8_t, uint16_t, uint32_t, uint64_t

from cpython.ref cimport PyObject

cdef extern from "sys/types.h":
	ctypedef Py_ssize_t size_t
	ctypedef Py_ssize_t ssize_t

cdef extern from "Programs/brlapi_protocol.h":
	int BRLAPI_MAXPACKETSIZE

cdef extern from "Programs/brlapi.h":
	ctypedef struct brlapi_connectionSettings_t:
		char *auth
		char *host

	ctypedef struct brlapi_writeArguments_t:
		int displayNumber
		unsigned int regionBegin
		unsigned int regionSize
		char *text
		int textSize
		unsigned char *andMask
		unsigned char *orMask
		int cursor
		char *charset

	ctypedef struct brlapi_expandedKeyCode_t:
		unsigned int type
		unsigned int command
		unsigned int argument
		unsigned int flags

	ctypedef struct brlapi_describedKeyCode_t:
		char *type
		char *command
		unsigned int argument
		unsigned int flags
		char *flag[32]
		brlapi_expandedKeyCode_t values

	ctypedef struct brlapi_error_t:
		int brlerrno
		int libcerrno
		int gaierrno
		char *errfun

	brlapi_error_t brlapi_error

	ctypedef struct brlapi_handle_t

	ctypedef unsigned long long brlapi_keyCode_t

	void brlapi_getLibraryVersion(int *major, int *minor, int *revision) nogil
	size_t brlapi_getHandleSize()
	void brlapi__closeConnection(brlapi_handle_t *)
	int brlapi__openConnection(brlapi_handle_t *, brlapi_connectionSettings_t*, brlapi_connectionSettings_t*) nogil

	int brlapi__getDisplaySize(brlapi_handle_t *, unsigned int*x, unsigned int *y) nogil
	int brlapi__getDriverName(brlapi_handle_t *, char*, int) nogil
	int brlapi__getModelIdentifier(brlapi_handle_t *, char*, int) nogil

	int brlapi__enterTtyMode(brlapi_handle_t *, int, char*) nogil
	int brlapi__enterTtyModeWithPath(brlapi_handle_t *, int *, int, char*) nogil
	int brlapi__leaveTtyMode(brlapi_handle_t *) nogil
	int brlapi__setFocus(brlapi_handle_t *, int) nogil

	int brlapi__write(brlapi_handle_t *, brlapi_writeArguments_t*) nogil
	int brlapi__writeDots(brlapi_handle_t *, unsigned char*) nogil
	int brlapi__writeText(brlapi_handle_t *, int, char*) nogil

	ctypedef enum brlapi_rangeType_t:
		brlapi_rangeType_all
	
	ctypedef struct brlapi_range_t:
		brlapi_keyCode_t first
		brlapi_keyCode_t last

	int brlapi__ignoreKeys(brlapi_handle_t *, brlapi_rangeType_t, brlapi_keyCode_t *, unsigned int) nogil
	int brlapi__acceptKeys(brlapi_handle_t *, brlapi_rangeType_t, brlapi_keyCode_t *, unsigned int) nogil
	int brlapi__ignoreAllKeys(brlapi_handle_t *) nogil
	int brlapi__acceptAllKeys(brlapi_handle_t *) nogil
	int brlapi__ignoreKeyRanges(brlapi_handle_t *, brlapi_range_t *, unsigned int) nogil
	int brlapi__acceptKeyRanges(brlapi_handle_t *, brlapi_range_t *, unsigned int) nogil
	int brlapi__readKey(brlapi_handle_t *, int, brlapi_keyCode_t*) nogil
	int brlapi__readKeyWithTimeout(brlapi_handle_t *, int, brlapi_keyCode_t*) nogil
	int brlapi_expandKeyCode(brlapi_keyCode_t, brlapi_expandedKeyCode_t *)
	int brlapi_describeKeyCode(brlapi_keyCode_t, brlapi_describedKeyCode_t *)

	int brlapi__enterRawMode(brlapi_handle_t *, char*) nogil
	int brlapi__leaveRawMode(brlapi_handle_t *) nogil
	int brlapi__recvRaw(brlapi_handle_t *, void*, int)
	int brlapi__sendRaw(brlapi_handle_t *, void*, int)

	ctypedef int brlapi_param_t
	ctypedef uint64_t brlapi_param_subparam_t
	ctypedef uint32_t brlapi_param_flags_t
	ctypedef void *brlapi_paramCallbackDescriptor_t

	ctypedef int brlapi_param_type_t
	ctypedef struct brlapi_param_properties_t:
		brlapi_param_type_t type
		uint16_t count
		uint8_t isArray
		uint8_t hasSubparam

	void *brlapi__getParameterAlloc(brlapi_handle_t *, brlapi_param_t, unsigned long long, brlapi_param_flags_t, size_t *) nogil
	int brlapi__setParameter(brlapi_handle_t *, brlapi_param_t, unsigned long long, brlapi_param_flags_t, void*, size_t) nogil
	const brlapi_param_properties_t *brlapi_getParameterProperties(brlapi_param_t parameter) nogil
	brlapi_paramCallbackDescriptor_t brlapi__watchParameter(brlapi_handle_t *, brlapi_param_t, uint64_t, brlapi_param_flags_t, brlapi_paramCallback_t, void *, void*, size_t);
	int brlapi__unwatchParameter(brlapi_handle_t *,
	brlapi_paramCallbackDescriptor_t)

	brlapi_error_t* brlapi_error_location()
	char* brlapi_strerror(brlapi_error_t*)
	brlapi_keyCode_t BRLAPI_KEY_MAX
	brlapi_keyCode_t BRLAPI_KEY_FLAGS_MASK
	brlapi_keyCode_t BRLAPI_KEY_TYPE_MASK
	brlapi_keyCode_t BRLAPI_KEY_CODE_MASK
	brlapi_keyCode_t BRLAPI_KEY_CMD_BLK_MASK
	brlapi_keyCode_t BRLAPI_KEY_CMD_ARG_MASK

cdef extern from "bindings.h":
	brlapi_writeArguments_t brlapi_writeArguments_initialized
	char *brlapi_protocolException()
	void brlapi_protocolExceptionInit(brlapi_handle_t *)

	ctypedef struct brlapi_python_paramCallbackDescriptor_t:
		brlapi_paramCallbackDescriptor_t brlapi_descr
		PyObject *callback

	ctypedef struct brlapi_python_callbackData_t:
		brlapi_param_t parameter
		brlapi_param_subparam_t subparam
		brlapi_param_flags_t flags
		const void *data
		size_t len

	brlapi_python_paramCallbackDescriptor_t *brlapi_python_watchParameter(brlapi_handle_t *, brlapi_param_t, uint64_t, int, object) except NULL
	int brlapi_python_unwatchParameter(brlapi_handle_t *, brlapi_python_paramCallbackDescriptor_t *)

cdef extern from "stdlib.h":
	void *malloc(size_t)
	void free(void*)
	char *strdup(char *)

cdef extern from "string.h":
	void *memcpy(void *, void *, size_t)
