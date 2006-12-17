"""
This module is a binding for BrlAPI, a BrlTTY bridge for applications.

The reference C API documentation is available online http://mielke.cc/brltty/doc/BrlAPIref-HTML, as well as in manual pages.

This documentation is only a python helper, you should also read C manual pages.

Example : 
import brlapi
b = brlapi.Connection()
b.enterTtyMode()
b.ignoreKeyRange([0, brlapi.KEY_MAX])
b.acceptKeyRange([brlapi.KEY_TYPE_SYM, brlapi.KEY_TYPE_SYM|brlapi.KEY_CODE_MASK])
b.acceptKeySet([brlapi.KEY_CMD_LNUP, brlapi.KEY_CMD_LNDN|brlapi.KEY_FLAGS_MASK])
b.writeText("Press any key to continue ... ¤")
key = b.readKey()
k = expandKeyCode(key)
b.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
b.writeText(None,1)
b.readKey()
w = brlapi.Write()
w.regionBegin = 1
w.regionSize = 40
w.text = u"Press any key to exit ¤                 "
underline = chr(brlapi.DOT7 + brlapi.DOT8)
w.attrOr = "".center(21,underline) + "".center(19,chr(0))
b.write(w)
b.readKey()
b.leaveTtyMode()
"""

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

cimport c_brlapi
include "constants.auto.pyx"

cdef returnerrno():
	"""This function returns str message error from errno"""
	cdef c_brlapi.brlapi_error_t *error
	error = c_brlapi.brlapi_error_location()
	return c_brlapi.brlapi_strerror(error)

class ConnectionError:
	"""Error while connecting to BrlTTY"""
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return self.value

class OperationError:
	"""Error while getting an attribute"""
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return self.value

cdef class Write:
	"""Structure containing arguments to be given to Bridge.write()
	See brlapi_writeStruct_t(3)."""
	cdef c_brlapi.brlapi_writeStruct_t props

	def __new__(self):
		self.props = <c_brlapi.brlapi_writeStruct_t> c_brlapi.BRLAPI_WRITESTRUCT_INITIALIZER

	property displayNumber:
		"""Display number -1 == unspecified"""
		def __get__(self):
			return self.props.displayNumber
		def __set__(self, val):
			self.props.displayNumber = val

	property regionBegin:
		"""Region of display to update, 1st character of display is 1"""
		def __get__(self):
			return self.props.regionBegin
		def __set__(self, val):
			self.props.regionBegin = val

	property regionSize:
		"""Number of characters held in text, attrAnd and attrOr. For multibytes text, this is the number of multibyte characters. Combining and double-width characters count for 1"""
		def __get__(self):
			return self.props.regionSize
		def __set__(self, val):
			self.props.regionSize = val

	property text:
		"""Text to display"""
		def __get__(self):
			if (not self.props.text):
				return None
			else:
				return self.props.text
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (type(val) == unicode):
				val = val.encode('UTF-8')
				self.charset = 'UTF-8'
			if (self.props.text):
				c_brlapi.free(self.props.text)
			if (val):
				size = len(val)
				c_val = val
				self.props.text = <char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.text,<void*>c_val,size)
				self.props.text[size] = 0
				self.props.textSize = size
			else:
				self.props.text = NULL

	property cursor:
		"""-1 == don't touch, 0 == turn off, 1 = 1st char of display, ..."""
		def __get__(self):
			return self.props.cursor
		def __set__(self, val):
			self.props.cursor = val

	property charset:
		"""Character set of the text"""
		def __get__(self):
			if (not self.props.charset):
				return None
			else:
				return self.props.charset
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.charset):
				c_brlapi.free(self.props.charset)
			if (val):
				size = len(val)
				c_val = val
				self.props.charset = <char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.charset,<void*>c_val,size)
				self.props.charset[size] = 0
			else:
				self.props.charset = NULL

	property attrAnd:
		"""And attributes; applied first"""
		def __get__(self):
			if (not self.props.attrAnd):
				return None
			else:
				return <char*>self.props.attrAnd
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.attrAnd):
				c_brlapi.free(self.props.attrAnd)
			if (val):
				size = len(val)
				c_val = val
				self.props.attrAnd = <unsigned char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.attrAnd,<void*>c_val,size)
				self.props.attrAnd[size] = 0
			else:
				self.props.attrAnd = NULL

	property attrOr:
		"""Or attributes; applied after ANDing"""
		def __get__(self):
			if (not self.props.attrOr):
				return None
			else:
				return <char*>self.props.attrOr
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.attrOr):
				c_brlapi.free(self.props.attrOr)
			if (val):
				size = len(val)
				c_val = val
				self.props.attrOr = <unsigned char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.attrOr,<void*>c_val,size)
				self.props.attrOr[size] = 0
			else:
				self.props.attrOr = NULL

cdef class Connection:
	"""Class which manages the bridge between BrlTTY and your program"""
	cdef c_brlapi.brlapi_handle_t *h
	cdef c_brlapi.brlapi_connectionSettings_t settings
	cdef int fd
	def __init__(self, host = None, auth = None):
		"""Connect your program to BrlTTY using settings

		See brlapi_openConnection(3)
		
		Setting host to None defaults it to localhost, using the local installation's default TCP port, or to the content of the BRLAPI_HOST environment variable, if it exists.
		Note: Please check that resolving this name works before complaining.

		Setting auth to None defaults it to local installation setup or to the content of the BRLAPI_AUTH environment variable, if it exists."""
		cdef c_brlapi.brlapi_connectionSettings_t client

		if auth:
			client.auth = auth
		else:
			client.auth = NULL

		if host:
			client.host = host
		else:
			client.host = NULL

		self.h = <c_brlapi.brlapi_handle_t*> c_brlapi.malloc(c_brlapi.brlapi_getHandleSize())

		c_brlapi.Py_BEGIN_ALLOW_THREADS
		self.fd = c_brlapi.brlapi__openConnection(self.h, &client, &self.settings)
		c_brlapi.Py_END_ALLOW_THREADS
		if self.fd == -1:
			c_brlapi.free(self.h)
			raise ConnectionError("couldn't connect to %s with key %s: %s" % (self.settings.host,self.settings.auth,returnerrno()))

	def __del__(self):
		"""Close the BrlAPI conection"""
		c_brlapi.brlapi__closeConnection(self.h)
		c_brlapi.free(self.h)

	property host:
		"""To get authorized to connect, libbrlapi has to tell the BrlAPI server a secret key, for security reasons. This is the path to the file which holds it; it will hence have to be readable by the application."""
		def __get__(self):
			return self.settings.host

	property auth:
		"""This tells where the BrlAPI server resides : it might be listening on another computer, on any TCP port. It should look like "foo:1", which means TCP port number BRLAPI_SOCKETPORTNUM+1 on computer called "foo"."""
		def __get__(self):
			return self.settings.auth

	property fileDescriptor:
		"""Returns the Unix file descriptor that the connection uses"""
		def __get__(self):
			return self.fd

	property displaySize:
		"""Get the size of the braille display
		See brlapi_getDisplaySize(3)."""
		def __get__(self):
			cdef unsigned int x
			cdef unsigned int y
			cdef int retval
			c_brlapi.Py_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi__getDisplaySize(self.h, &x, &y)
			c_brlapi.Py_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return (x, y)
	
	property driverId:
		"""Identify the driver used by BrlTTY
		See brlapi_getDriverId(3)."""
		def __get__(self):
			cdef char id[3]
			cdef int retval
			c_brlapi.Py_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi__getDriverId(self.h, id, sizeof(id))
			c_brlapi.Py_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return id

	property driverName:
		"""Get the complete name of the driver used by BrlTTY
		See brlapi_getDriverName(3)."""
		def __get__(self):
			cdef char name[21]
			cdef int retval
			c_brlapi.Py_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi__getDriverName(self.h, name, sizeof(name))
			c_brlapi.Py_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return name

	def enterTtyMode(self, tty = TTY_DEFAULT, driverName = None):
		"""Ask for some tty, with some key mechanism

		See brlapi_enterTtyMode(3).

		* tty : If tty >= 0, application takes control of the specified tty
			If tty == -1, the library first tries to get the tty number from the WINDOWID environment variable (form xterm case), then the CONTROLVT variable, and at last reads /proc/self/stat (on linux)
		* driverName : Tells how the application wants readKey() to return key presses. None or "" means BrlTTY commands are required, whereas a driver name means that raw key codes returned by this driver are expected."""
		cdef int retval
		cdef int c_tty
		cdef char *c_driverName
		c_tty = tty
		if not driverName:
			c_driverName = NULL
		else:
			c_driverName = driverName
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterTtyMode(self.h, c_tty, c_driverName)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def enterTtyModeWithPath(self, path = [], how = None):
		"""Ask for some tty, with some key mechanism

		See brlapi_enterTtyModeWithPath(3).

		* tty is an array of ttys representing the tty path to be got. Can be None.
		* driverName : has the same meaning as in enterTtyMode.
		
		Providing an empty array or None means to get the root."""
		cdef int retval
		cdef int *c_ttys
		cdef int c_nttys
		cdef char *c_how
		if not path:
			c_ttys = NULL
			c_nttys = 0
		else:
			c_nttys = len(path)
			c_ttys = <int*>c_brlapi.malloc(c_nttys * sizeof(int))
			for i from 0 <= i < c_nttys:
				c_ttys[i] = path[i]
		if not how:
			c_how = NULL
		else:
			c_how = how
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterTtyModeWithPath(self.h, c_ttys, c_nttys, c_how)
		c_brlapi.Py_END_ALLOW_THREADS
		if (c_ttys):
			c_brlapi.free(c_ttys)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def leaveTtyMode(self):
		"""Stop controlling the tty
		See brlapi_leaveTtyMode(3)."""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__leaveTtyMode(self.h)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def setFocus(self, tty):
		"""Tell the current tty to brltty.
		See brlapi_setFocus(3).
		This is intended for focus tellers, such as brltty, xbrlapi, screen, ... enterTtyMode() must have been called before hand to tell where this focus applies in the tty tree."""
		cdef int retval
		cdef int c_tty
		c_tty = tty
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__setFocus(self.h, c_tty)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def write(self, Write writeStruct):
		"""Update a specific region of the braille display and apply and/or masks.
		See brlapi_write(3).
		* s : gives information necessary for the update"""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__write(self.h, &writeStruct.props)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def writeDots(self, dots):
		"""Write the given dots array to the display.
		See brlapi_writeDots(3).
		* dots : points on an array of dot information, one per character. Its size must hence be the same as what displaysize provides."""
		cdef int retval
		cdef char *c_dots
		cdef unsigned char *c_udots
		c_dots = dots
		c_udots = <unsigned char *>c_dots
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__writeDots(self.h, c_udots)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def writeText(self, str, cursor = CURSOR_OFF):
		"""Write the given \0-terminated string to the braille display.
		See brlapi_writeText(3).
		If the string is too long, it is cut. If it's too short, spaces are appended. The current LC_CTYPE locale is considered, unless it is left as default "C", in which case the charset is assumed to be 8bits, and the same as the server's.

		* cursor : gives the cursor position; if equal to 0, no cursor is shown at all; if cursor == -1, the cursor is left where it is
		* str : points to the string to be displayed"""
		w = Write()
		w.cursor = cursor
		if (str):
			(x, y) = self.displaySize
			dispSize = x * y
			if (len(str) < dispSize):
				str = str + "".center(dispSize - len(str), ' ')
			w.regionBegin = 1
			w.regionSize = dispSize
			w.text = str[0 : dispSize]
		return self.write(w)

	def readKey(self, block = True):
		"""Read a key from the braille keyboard.
		See brlapi_readKey(3).

		This function returns one key press's code.

		If None or "" was given to enterTtyPath(), a brltty command is returned. It is hence pretty driver-independent, and should be used by default when no other option is possible.

		By default, all commands but those which restart drivers and switch virtual are returned to the application and not to brltty. If the application doesn't want to see some command events, it should call either ignoreKeySet() or ignoreKeyRange().

		If some driver name was given to enterTtyMode(), a raw keycode is returned, as specified by the terminal driver. It generally corresponds to the very code that the terminal tells to the driver. This should only be used by applications which are dedicated to a particular braille terminal. Hence, checking the terminal type thanks to a call to driverid or even drivername before getting tty control is a pretty good idea.

		By default, all the keypresses will be passed to the client, none will go through brltty, so the application will have to handle console switching itself for instance."""
		cdef unsigned long long code
		cdef int retval
		cdef int c_block
		c_block = block
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__readKey(self.h, c_block, <unsigned long long*>&code)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		elif retval <= 0 and block == False:
			return None
		else:
			return code

	def expandKeyCode(self, code):
		"""Expand a keycode into its individual components.
		See brlapi_expandKeyCode(3)."""
		cdef c_brlapi.brlapi_expandedKeyCode_t ekc
		cdef int retval
		retval = c_brlapi.brlapi_expandKeyCode(code, &ekc)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return { "type":ekc.type, "command":ekc.command, "argument":ekc.argument, "flags":ekc.flags }
	
	def ignoreKeyRange(self, range):
		"""Ignore some key presses from the braille keyboard.
		See brlapi_ignoreKeyRange(3).

		This function asks the server to give keys between x and y (inclusive, and x can be equal to y) to brltty, rather than returning them to the application via readKey().

		Note: The given codes are either raw keycodes if some driver name was given to enterTtyMode(), or brltty commands if None or "" was given."""
		cdef int retval
		cdef unsigned long long x,y
		x = range[0]
		y = range[1]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__ignoreKeyRange(self.h, x, y)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def acceptKeyRange(self, range):
		"""Accept some key presses from the braille keyboard.
		See brlapi_acceptKeyRange(3).

		This function asks the server to return keys between x and y (inclusive, and x can be equal to y) to the application, and not give them to brltty.

		Note: You shouldn't ask the server to give you key presses which are usually used to switch between TTYs, unless you really know what you are doing ! The given codes are either raw keycodes if some driver name was given to enterTtyMode(), or brltty commands if None or "" was given."""
		cdef int retval
		cdef unsigned long long x,y
		x = range[0]
		y = range[1]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__acceptKeyRange(self.h, x, y)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def ignoreKeySet(self, set):
		"""Ignore some key presses from the braille keyboard.
		See brlapi_ignoreKeySet(3).

		This function asks the server to give the keys in the set to brltty, rather than returning them to the application via readKey().

		Note: The given codes are either raw keycodes if some driver name was given to enterTtyMode(), or brltty commands if None or "" was given."""
		cdef int retval
		cdef unsigned long long x,y
		cdef unsigned long long *c_set
		cdef unsigned int c_n
		c_n = len(set)
		c_set = <unsigned long long*>c_brlapi.malloc(c_n * sizeof(unsigned long long))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__ignoreKeySet(self.h, c_set, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def acceptKeySet(self, set):
		"""Accept some key presses from the braille keyboard.
		See brlapi_acceptKeySet(3).

		This function asks the server to return keys in the set to the application, and not give them to brltty.

		Note: You shouldn't ask the server to give you key presses which are usually used to switch between TTYs, unless you really know what you are doing ! The given codes are either raw keycodes if some driver name was given to enterTtyMode(), or brltty commands if None or "" was given."""
		cdef int retval
		cdef unsigned long long x,y
		cdef unsigned long long *c_set
		cdef unsigned int c_n
		c_n = len(set)
		c_set = <unsigned long long*>c_brlapi.malloc(c_n * sizeof(unsigned long long))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__acceptKeySet(self.h, c_set, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def enterRawMode(self, drivername):
		"""Switch to Raw mode
		See brlapi_enterRawMode(3).
		
		* driver : Specifies the name of the driver for which the raw communication will be established"""
		cdef int retval
		cdef char *c_drivername
		c_drivername = drivername
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterRawMode(self.h, c_drivername)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def leaveRawMode(self):
		"""leave Raw mode
		See brlapi_leaveRawMode(3)."""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__leaveRawMode(self.h)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

