"""
This module implements a set of bindings for BrlAPI, a braille bridge for applications.

The reference C API documentation is available online http://mielke.cc/brltty/doc/BrlAPIref-HTML, as well as in manual pages.

This documentation is only a python helper, you should also read C manual pages.

Example : 
import brlapi
b = brlapi.Connection()
b.enterTtyMode()
b.ignoreKeys(brlapi.rangeType_all,[0])
b.acceptKeys(brlapi.rangeType_command,[brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_HOME, brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_WINUP, brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_WINDN])
b.writeText("Press home or winup/dn to continue ... ¤")
key = b.readKey()
k = b.expandKeyCode(key)
b.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
b.writeText(None,1)
b.readKey()
underline = chr(brlapi.DOT7 + brlapi.DOT8)
# Note: center() can take two arguments only starting from python 2.4
b.write(
    regionBegin = 1,
    regionSize = 40,
    text = u"Press any key to exit ¤                 ",
    orMask = "".center(21,underline) + "".center(19,chr(0)))
b.acceptKeys(brlapi.rangeType_all,[0])
b.readKey()
b.leaveTtyMode()
"""

###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2007 by
#   Alexis Robert <alexissoft@free.fr>
#   Samuel Thibault <Samuel.Thibault@ens-lyon.org>
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

cdef class WriteStruct:
	"""Structure containing arguments to be given to Connection.write()
	See brlapi_writeArguments_t(3).
	
	This is DEPRECATED. Use the named parameters of write() instead."""
	cdef c_brlapi.brlapi_writeArguments_t props

	def __new__(self):
		self.props = c_brlapi.brlapi_writeArguments_initialized

	property displayNumber:
		"""Display number DISPLAY_DEFAULT == unspecified"""
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
		"""CURSOR_LEAVE == don't touch, CURSOR_OFF == turn off, 1 = 1st char of display, ..."""
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
			if (not self.props.andMask):
				return None
			else:
				return <char*>self.props.andMask
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.andMask):
				c_brlapi.free(self.props.andMask)
			if (val):
				size = len(val)
				c_val = val
				self.props.andMask = <unsigned char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.andMask,<void*>c_val,size)
				self.props.andMask[size] = 0
			else:
				self.props.andMask = NULL

	property attrOr:
		"""Or attributes; applied after ANDing"""
		def __get__(self):
			if (not self.props.orMask):
				return None
			else:
				return <char*>self.props.orMask
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.orMask):
				c_brlapi.free(self.props.orMask)
			if (val):
				size = len(val)
				c_val = val
				self.props.orMask = <unsigned char*>c_brlapi.malloc(size+1)
				c_brlapi.memcpy(<void*>self.props.orMask,<void*>c_val,size)
				self.props.orMask[size] = 0
			else:
				self.props.orMask = NULL

cdef class Connection:
	"""Class which manages the bridge between your program and BrlAPI"""
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

	def enterTtyMode(self, tty = TTY_DEFAULT, driver = None):
		"""Ask for some tty, with some key mechanism

		See brlapi_enterTtyMode(3).

		* tty : If tty >= 0, application takes control of the specified tty
			If tty == TTY_DEFAULT, the library first tries to get the tty number from the WINDOWID environment variable (form xterm case), then the CONTROLVT variable, and at last reads /proc/self/stat (on linux)
		* driver : Tells how the application wants readKey() to return key presses. None or "" means BrlTTY commands are required, whereas a driver name means that raw key codes returned by this driver are expected."""
		cdef int retval
		cdef int c_tty
		cdef char *c_driver
		c_tty = tty
		if not driver:
			c_driver = NULL
		else:
			c_driver = driver
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterTtyMode(self.h, c_tty, c_driver)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def enterTtyModeWithPath(self, path = [], driver = None):
		"""Ask for some tty, with some key mechanism

		See brlapi_enterTtyModeWithPath(3).

		* tty is an array of ttys representing the tty path to be got. Can be None.
		* driver : has the same meaning as in enterTtyMode.
		
		Providing an empty array or None means to get the root."""
		cdef int retval
		cdef int *c_ttys
		cdef int c_nttys
		cdef char *c_driver
		if not path:
			c_ttys = NULL
			c_nttys = 0
		else:
			c_nttys = len(path)
			c_ttys = <int*>c_brlapi.malloc(c_nttys * sizeof(int))
			for i from 0 <= i < c_nttys:
				c_ttys[i] = path[i]
		if not driver:
			c_driver = NULL
		else:
			c_driver = driver
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterTtyModeWithPath(self.h, c_ttys, c_nttys, c_driver)
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

	# The writeArguments parameter must remain first (after self) in order
	# to maintain backward compatibility with old code which passes it
	# by position. New code should not use it since the plan is to remove
	# it once it's no longer being used. New code should supply attributes
	# by specifying the remaining parameters, as needed, by name.
	def write(self, WriteStruct writeArguments = None,
			displayNumber = c_brlapi.BRLAPI_DISPLAY_DEFAULT,
			regionBegin = 0,
			regionSize = 0,
			text = None,
			andMask = None,
			orMask = None,
			cursor = c_brlapi.BRLAPI_CURSOR_LEAVE,
			charset = None):
		"""Update a specific region of the braille display and apply and/or masks.
		See brlapi_write(3).
		* s : gives information necessary for the update"""
		cdef int retval
		if not writeArguments:
			writeArguments = WriteStruct()
		if displayNumber != c_brlapi.BRLAPI_DISPLAY_DEFAULT:
			writeArguments.displayNumber = displayNumber
		if regionBegin != 0:
			writeArguments.regionBegin = regionBegin
		if regionSize != 0:
			writeArguments.regionSize = regionSize
		if text:
			writeArguments.text = text
		if andMask:
			writeArguments.attrAnd = andMask
		if orMask:
			writeArguments.attrOr = orMask
		if cursor != c_brlapi.BRLAPI_CURSOR_LEAVE:
			writeArguments.cursor = cursor
		if charset:
			writeArguments.charset = charset
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__write(self.h, &writeArguments.props)
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

	def writeText(self, text, cursor = CURSOR_OFF):
		"""Write the given \0-terminated string to the braille display.
		See brlapi_writeText(3).
		If the string is too long, it is cut. If it's too short, spaces are appended. The current LC_CTYPE locale is considered, unless it is left as default "C", in which case the charset is assumed to be 8bits, and the same as the server's.

		* cursor : gives the cursor position; if equal to CURSOR_OFF, no cursor is shown at all; if cursor == CURSOR_LEAVE, the cursor is left where it is
		* text : points to the string to be displayed"""
		w = WriteStruct()
		w.cursor = cursor
		if (text):
			(x, y) = self.displaySize
			dispSize = x * y
			if (len(text) < dispSize):
				text = text + "".center(dispSize - len(text))
			w.regionBegin = 1
			w.regionSize = dispSize
			w.text = text[0 : dispSize]
		return self.write(w)

	def readKey(self, wait = True):
		"""Read a key from the braille keyboard.
		See brlapi_readKey(3).

		This function returns one key press's code.

		If None or "" was given to enterTtyPath(), a brltty command is returned. It is hence pretty driver-independent, and should be used by default when no other option is possible.

		By default, all commands but those which restart drivers and switch virtual are returned to the application and not to brltty. If the application doesn't want to see some command events, it should call either ignoreKeys() or ignoreKeyRanges().

		If some driver name was given to enterTtyMode(), a raw keycode is returned, as specified by the terminal driver. It generally corresponds to the very code that the terminal tells to the driver. This should only be used by applications which are dedicated to a particular braille terminal. Hence, checking the terminal type thanks to a call to drivername before getting tty control is a pretty good idea.

		By default, all the keypresses will be passed to the client, none will go through brltty, so the application will have to handle console switching itself for instance."""
		cdef c_brlapi.brlapi_keyCode_t code
		cdef int retval
		cdef int c_wait
		c_wait = wait
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__readKey(self.h, c_wait, <c_brlapi.brlapi_keyCode_t*>&code)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		elif retval <= 0 and wait == False:
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
	
	def ignoreKeys(self, type, set):
		"""Ignore some key presses from the braille keyboard.
		See brlapi_ignoreKeys(3).
		
		This function asks the server to give the provided keys to brltty, rather than returning them to the application via brlapi_readKey().
		
		The given codes should be brltty commands (nul or "" was given to brlapi_enterTtyMode())"""
		cdef int retval
		cdef c_brlapi.brlapi_rangeType_t c_type
		cdef c_brlapi.brlapi_keyCode_t *c_set
		cdef unsigned int c_n
		c_type = type
		c_n = len(set)
		c_set = <c_brlapi.brlapi_keyCode_t*>c_brlapi.malloc(c_n * sizeof(c_set[0]))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__ignoreKeys(self.h, c_type, c_set, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def acceptKeys(self, type, set):
		"""Accept some key presses from the braille keyboard.
		See brlapi_ignoreKeys(3).
		
		This function asks the server to give the provided keys to the application, and not give them to brltty.

		The given codes should be brltty commands (nul or "" was given to brlapi_enterTtyMode())"""
		cdef int retval
		cdef c_brlapi.brlapi_rangeType_t c_type
		cdef c_brlapi.brlapi_keyCode_t *c_set
		cdef unsigned int c_n
		c_type = type
		c_n = len(set)
		c_set = <c_brlapi.brlapi_keyCode_t*>c_brlapi.malloc(c_n * sizeof(c_set[0]))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__acceptKeys(self.h, c_type, c_set, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval
	
	def ignoreAllKeys(self):
		"""Ignore all key presses from the braille keyboard.
		See brlapi_ignoreAllKeys(3).
		
		This function asks the server to give all keys to brltty, rather than returning them to the application via brlapi_readKey()."""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__ignoreAllKeys(self.h)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval
	
	def acceptAllKeys(self):
		"""Accept all key presses from the braille keyboard.
		See brlapi_acceptAllKeys(3).
		
		This function asks the server to give all keys to the application, and not give them to brltty.

		Warning: after calling this function, make sure to call brlapi_ignoreKeys() for ignoring important keys like BRL_CMD_SWITCHVT_PREV/NEXT and such."""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__acceptAllKeys(self.h)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def ignoreKeyRanges(self, keys):
		"""Ignore some key presses from the braille keyboard.
		See brlapi_ignoreKeyRanges(3).
		
		This function asks the server to give the provided key ranges to brltty, rather than returning them to the application via brlapi_readKey().
		
		The given codes should be raw keycodes (i.e. some driver name was given to brlapi_enterTtyMode()) """
		cdef int retval
		cdef c_brlapi.brlapi_range_t *c_keys
		cdef unsigned int c_n
		c_n = len(keys)
		c_keys = <c_brlapi.brlapi_range_t*>c_brlapi.malloc(c_n * sizeof(c_keys[0]))
		for i from 0 <= i < c_n:
			c_keys[i].first = keys[i][0]
			c_keys[i].last = keys[i][1]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__ignoreKeyRanges(self.h, c_keys, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_keys)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def acceptKeyRanges(self, keys):
		"""Accept some key presses from the braille keyboard.
		See brlapi_acceptKeyRanges(3).
		
		This function asks the server to return the provided key ranges (inclusive) to the application, and not give them to brltty.
		
		The given codes should be raw keycodes (i.e. some driver name was given to brlapi_enterTtyMode()) """
		cdef int retval
		cdef c_brlapi.brlapi_range_t *c_keys
		cdef unsigned int c_n
		c_n = len(keys)
		c_keys = <c_brlapi.brlapi_range_t*>c_brlapi.malloc(c_n * sizeof(c_keys[0]))
		for i from 0 <= i < c_n:
			c_keys[i].first = keys[i][0]
			c_keys[i].last = keys[i][1]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__acceptKeyRanges(self.h, c_keys, c_n)
		c_brlapi.Py_END_ALLOW_THREADS
		c_brlapi.free(c_keys)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def enterRawMode(self, driver):
		"""Switch to Raw mode
		See brlapi_enterRawMode(3).
		
		* driver : Specifies the name of the driver for which the raw communication will be established"""
		cdef int retval
		cdef char *c_driver
		c_driver = driver
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterRawMode(self.h, c_driver)
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

