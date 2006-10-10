# Copyright (C) 2005 Alexis ROBERT

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

	int brlapi_getTty(int, char*)
	int brlapi_leaveTty()
	int brlapi_setFocus(int)

	int brlapi_write(brlapi_writeStruct*)
	int brlapi_writeDots(unsigned char*)
	int brlapi_writeText(int, char*)

	int brlapi_ignoreKeyRange(unsigned int, unsigned int)
	int brlapi_unignoreKeyRange(unsigned int, unsigned int)
	int brlapi_readKey(int, unsigned long long*)

	int brlapi_getRaw(char*)
	int brlapi_leaveRaw()
	int brlapi_recvRaw(void*, int)
	int brlapi_sendRaw(void*, int)

	brlapi_error_t* brlapi_error_location()
	char* brlapi_strerror(brlapi_error_t*)
