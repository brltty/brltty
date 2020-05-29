"""
This module implements a set of bindings for BrlAPI, a braille bridge for applications.

The reference C API documentation is available online http://brltty.app/doc/BrlAPIref, as well as in manual pages.

This documentation is only a python helper, you should also read C manual pages.

Example : 
import brlapi
import errno
import Xlib.keysymdef.miscellany
try:
  b = brlapi.Connection()
  print("Server version " + str(b.getParameter(brlapi.PARAM_SERVER_VERSION, 0, brlapi.PARAMF_GLOBAL)))
  print("Display size " + str(b.getParameter(brlapi.PARAM_DISPLAY_SIZE, 0, brlapi.PARAMF_GLOBAL)))
  print("Driver " + b.getParameter(brlapi.PARAM_DRIVER_NAME, 0, brlapi.PARAMF_GLOBAL))
  print("Model " + b.getParameter(brlapi.PARAM_DEVICE_MODEL, 0, brlapi.PARAMF_GLOBAL))

  for cmd in b.getParameter(brlapi.PARAM_BOUND_COMMAND_CODES, 0, brlapi.PARAMF_GLOBAL):
    print("Command %x short name: %s" % (cmd, b.getParameter(brlapi.PARAM_COMMAND_SHORT_NAME, cmd, brlapi.PARAMF_GLOBAL)))

  for key in b.getParameter(brlapi.PARAM_DEVICE_KEY_CODES, 0, brlapi.PARAMF_GLOBAL):
    print("Key %x short name: %s" % (key, b.getParameter(brlapi.PARAM_KEY_SHORT_NAME, key, brlapi.PARAMF_GLOBAL)))

  # Make our output more prioritized
  b.setParameter(brlapi.PARAM_CLIENT_PRIORITY, 0, False, 70)

  def update_callback(param, subparam, flags, value):
    s = ""
    for i in value:
      s += unichr(0x2800 + ord(i))
    print("Got output update %s" % s)

  p = b.watchParameter(brlapi.PARAM_RENDERED_CELLS, 0, False, update_callback)

  b.enterTtyMode()
  b.ignoreKeys(brlapi.rangeType_all,[0])

  # Accept the home, window up and window down braille commands
  b.acceptKeys(brlapi.rangeType_command,[brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_HOME, brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_WINUP, brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_WINDN])

  # Accept the tab key
  b.acceptKeys(brlapi.rangeType_key,[brlapi.KEY_TYPE_SYM|Xlib.keysymdef.miscellany.XK_Tab])

  b.writeText("Trying to get a key within one second")
  key = b.readKeyWithTimeout(1000)
  print("got " + str(key))

  b.writeText("Press home, winup/dn or tab to continue ... ¤")
  key = b.readKey()

  k = brlapi.expandKeyCode(key)
  b.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
  b.writeText(None,1)
  b.acceptAllKeys()
  b.readKey()

  underline = chr(brlapi.DOT7 + brlapi.DOT8)
  # Note: center() can take two arguments only starting from python 2.4
  b.write(
      regionBegin = 1,
      regionSize = 40,
      text = "Press any key to continue               ",
      orMask = 25*underline + 15*chr(0))
  b.readKey()

  b.acceptAllKeys()
  b.writeText("Press any key")
  k = b.readKey()
  k = brlapi.expandKeyCode(key)
  b.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
  b.readKey()

  b.ignoreAllKeys()
  b.acceptKeyRanges([(brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_PASSDOTS, brlapi.KEY_TYPE_CMD|brlapi.KEY_CMD_PASSDOTS|brlapi.KEY_CMD_ARG_MASK)])
  b.writeText("Press a dot key")
  key = b.readKey()
  k = brlapi.expandKeyCode(key)
  b.writeText("Key %ld (%x %x %x %x) !" % (key, k["type"], k["command"], k["argument"], k["flags"]))
  b.acceptAllKeys()
  b.readKey()

  b.unwatchParameter(p)
  b.leaveTtyMode()
  b.closeConnection()

except brlapi.ConnectionError as e:
  if e.brlerrno == brlapi.ERROR_CONNREFUSED:
    print("Connection to %s refused. BRLTTY is too busy..." % e.host)
  elif e.brlerrno == brlapi.ERROR_AUTHENTICATION:
    print("Authentication with %s failed. Please check the permissions of %s" % (e.host,e.auth))
  elif e.brlerrno == brlapi.ERROR_LIBCERR and (e.libcerrno == errno.ECONNREFUSED or e.libcerrno == errno.ENOENT):
    print("Connection to %s failed. Is BRLTTY really running?" % (e.host))
  else:
    print("Connection to BRLTTY at %s failed: " % (e.host))
  print(e)
  print(e.brlerrno)
  print(e.libcerrno)
"""

###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2020 by
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

cimport c_brlapi
from libc.stdint cimport uint8_t, uint16_t, uint32_t, uint64_t, uintptr_t
import errno

include "constants.auto.pyx"

def getLibraryVersion():
	"""Get the BrlAPI version as a three-element list (major, minor, revision).
	See brlapi_getLibraryVersion(3)."""
	cdef int major
	cdef int minor
	cdef int revision
	with nogil:
		c_brlapi.brlapi_getLibraryVersion(&major, &minor, &revision)
	return (major, minor, revision)

def expandKeyCode(code):
	"""Expand a keycode into its individual components.
	See brlapi_expandKeyCode(3)."""
	cdef c_brlapi.brlapi_expandedKeyCode_t ekc
	cdef int retval
	retval = c_brlapi.brlapi_expandKeyCode(code, &ekc)
	if retval == -1:
		raise OperationError()
	else:
		return {
			"type": ekc.type,
			"command": ekc.command,
			"argument": ekc.argument,
			"flags": ekc.flags
		}

def describeKeyCode(code):
	"""Describe the individual components of a keycode symbolically.
	See brlapi_describeKeyCode(3)."""
	cdef c_brlapi.brlapi_describedKeyCode_t dkc
	cdef int retval
	retval = c_brlapi.brlapi_describeKeyCode(code, &dkc)
	if retval == -1:
		raise OperationError()
	else:
		flags = ()
		for flag in range(dkc.flags):
			flags += (dkc.flag[flag], )
		return {
			"type": dkc.type,
			"command": dkc.command,
			"argument": dkc.argument,
			"flags": flags
		}

cdef object _parameterToPython(c_brlapi.brlapi_param_t c_param, const void *c_value, size_t size):
	cdef const c_brlapi.brlapi_param_properties_t *props

	with nogil:
		props = c_brlapi.brlapi_getParameterProperties(c_param)

	if props == NULL:
		raise OperationError()

	if props.type == PARAM_TYPE_STRING:
		string = <char *>c_value
		s = string[:size]
		ret = s.decode("UTF-8")
	elif props.type == PARAM_TYPE_BOOLEAN:
		values8 = <uint8_t *>c_value
		ret = values8[0] != 0
	elif props.type == PARAM_TYPE_UINT8:
		if props.isArray:
			values8 = <uint8_t *>c_value
			ret = values8[:size]
		else:
			values8 = <uint8_t *>c_value
			ret = values8[0]
	elif props.type == PARAM_TYPE_UINT16:
		if props.isArray:
			values16 = <uint16_t *>c_value
			ret = [values16[i] for i in range(size//2)]
		else:
			values16 = <uint16_t *>c_value
			ret = values16[0]
	elif props.type == PARAM_TYPE_UINT32:
		if props.isArray:
			values32 = <uint32_t *>c_value
			ret = [values32[i] for i in range(size//4)]
		else:
			values32 = <uint32_t *>c_value
			ret = values32[0]
	elif props.type == PARAM_TYPE_UINT64:
		if props.isArray:
			values64 = <uint64_t *>c_value
			ret = [values64[i] for i in range(size//8)]
		else:
			values64 = <uint64_t *>c_value
			ret = values64[0]
	else:
		raise ValueError("Unsupported parameter type")

	return ret

class OperationError(Exception):
	"""Error while performing some operation"""
	def __init__(self):
		cdef char *exception
		exception = c_brlapi.brlapi_protocolException()
		if (exception):
			self.exception = exception
			c_brlapi.free(exception)
		else:
			self.exception = None
			self.brlerrno = c_brlapi.brlapi_error.brlerrno
			self.libcerrno = c_brlapi.brlapi_error.libcerrno
			self.gaierrno = c_brlapi.brlapi_error.gaierrno
			if (c_brlapi.brlapi_error.errfun):
				self.errfun = c_brlapi.brlapi_error.errfun
			else:
				self.errfun = b""

	def __str__(self):
		cdef c_brlapi.brlapi_error_t error
		if self.exception:
			return self.exception
		error.brlerrno = self.brlerrno
		error.libcerrno = self.libcerrno
		error.gaierrno = self.gaierrno
		str = self.errfun
		error.errfun = str
		return c_brlapi.brlapi_strerror(&error)

class ConnectionError(OperationError):
	"""Error while connecting to BrlTTY"""

	def __init__(self, host, auth):
		OperationError.__init__(self)
		self.host = host
		self.auth = auth

	def __str__(self):
		msg = "couldn't connect to %s with key %s: %s" % (self.host,self.auth,OperationError.__str__(self))
		msg = msg + "\n(brlerrno %d, libcerrno %d, gaierrno %d)" % (self.brlerrno, self.libcerrno, self.gaierrno)
		if self.brlerrno == ERROR_CONNREFUSED:
			msg = msg + "\nBRLTTY is too busy..."
		elif self.brlerrno == ERROR_AUTHENTICATION:
			msg = msg + "\nAuthentication failed. Please check you can read %s and it is not empty." % self.auth
		elif self.brlerrno == ERROR_LIBCERR and (self.libcerrno == errno.ECONNREFUSED or self.libcerrno == errno.ENOENT):
			msg = msg + "\nIs BRLTTY really running?"
		return msg

	def host(self):
		"""Host of BRLTTY server"""
		return self.settings.host

	def auth(self):
		"""Authentication method used"""
		return self.settings.auth

cdef class WriteStruct:
	"""Structure containing arguments to be given to Connection.write()
	See brlapi_writeArguments_t(3).
	
	This is DEPRECATED. Use the named parameters of write() instead."""
	cdef c_brlapi.brlapi_writeArguments_t props

	def __init__(self):
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
				self.charset = 'UTF-8'.encode("ASCII")
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
				if (type(val) == unicode):
					val = val.encode('ASCII')
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
			cdef char *c_val
			if (not self.props.andMask):
				return None
			else:
				c_val = <char*>self.props.andMask
				return c_val[:self.regionSize]
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.andMask):
				c_brlapi.free(self.props.andMask)
			if (val):
				if (type(val) == unicode):
					val = val.encode('latin1')
				size = len(val)
				c_val = val
				self.props.andMask = <unsigned char*>c_brlapi.malloc(size)
				c_brlapi.memcpy(<void*>self.props.andMask,<void*>c_val,size)
			else:
				self.props.andMask = NULL

	property attrOr:
		"""Or attributes; applied after ANDing"""
		def __get__(self):
			cdef char *c_val
			if (not self.props.orMask):
				return None
			else:
				c_val = <char*>self.props.orMask
				return c_val[:self.regionSize]
		def __set__(self, val):
			cdef c_brlapi.size_t size
			cdef char *c_val
			if (self.props.orMask):
				c_brlapi.free(self.props.orMask)
			if (val):
				if (type(val) == unicode):
					val = val.encode('latin1')
				size = len(val)
				c_val = val
				self.props.orMask = <unsigned char*>c_brlapi.malloc(size)
				c_brlapi.memcpy(<void*>self.props.orMask,<void*>c_val,size)
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

		with nogil:
			self.fd = <int>c_brlapi.brlapi__openConnection(self.h, &client, &self.settings)
		c_brlapi.brlapi_protocolExceptionInit(self.h)
		if self.fd == -1:
			c_brlapi.free(self.h)
			raise ConnectionError(self.settings.host, self.settings.auth)

	def closeConnection(self):
		"""Close the BrlAPI connection"""
		if self.fd != -1:
			c_brlapi.brlapi__closeConnection(self.h)
			self.fd = -1

	def __del__(self):
		"""Release resources used by the connection"""
		if self.fd != -1:
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
			with nogil:
				retval = c_brlapi.brlapi__getDisplaySize(self.h, &x, &y)
			if retval == -1:
				raise OperationError()
			else:
				return (x, y)
	
	property driverName:
		"""Get the complete name of the driver used by BrlTTY
		See brlapi_getDriverName(3)."""
		def __get__(self):
			cdef char name[21]
			cdef int retval
			with nogil:
				retval = c_brlapi.brlapi__getDriverName(self.h, name, sizeof(name))
			if retval == -1:
				raise OperationError()
			else:
				return name

	property modelIdentifier:
		"""Get the identifier for the model of the braille display
		See brlapi_getModelIdentifier(3)."""
		def __get__(self):
			cdef char identifier[21]
			cdef int retval
			with nogil:
				retval = c_brlapi.brlapi__getModelIdentifier(self.h, identifier, sizeof(identifier))
			if retval == -1:
				raise OperationError()
			else:
				return identifier

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
			if (type(driver) == unicode):
				driver = driver.encode('ASCII')
			c_driver = driver
		with nogil:
			retval = c_brlapi.brlapi__enterTtyMode(self.h, c_tty, c_driver)
		if retval == -1:
			raise OperationError()
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
			if (type(driver) == unicode):
				driver = driver.encode('ASCII')
			c_driver = driver
		with nogil:
			retval = c_brlapi.brlapi__enterTtyModeWithPath(self.h, c_ttys, c_nttys, c_driver)
		if (c_ttys):
			c_brlapi.free(c_ttys)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def leaveTtyMode(self):
		"""Stop controlling the tty
		See brlapi_leaveTtyMode(3)."""
		cdef int retval
		with nogil:
			retval = c_brlapi.brlapi__leaveTtyMode(self.h)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def setFocus(self, tty):
		"""Tell the current tty to brltty.
		See brlapi_setFocus(3).
		This is intended for focus tellers, such as brltty, xbrlapi, screen, ... enterTtyMode() must have been called before hand to tell where this focus applies in the tty tree."""
		cdef int retval
		cdef int c_tty
		c_tty = tty
		with nogil:
			retval = c_brlapi.brlapi__setFocus(self.h, c_tty)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	# The writeArguments parameter must remain first (after self) in order
	# to maintain backward compatibility with old code which passes it
	# by position. New code should not use it since the plan is to remove
	# it once it's no longer being used. New code should supply attributes
	# by specifying the remaining parameters, as needed, by name.
	def write(self, WriteStruct writeArguments = None,
			displayNumber = None,
			regionBegin = None,
			regionSize = None,
			text = None,
			andMask = None,
			orMask = None,
			cursor = None,
			charset = None):
		"""Update a specific region of the braille display and apply and/or masks.
		See brlapi_write(3).
		* s : gives information necessary for the update"""
		cdef int retval
		if not writeArguments:
			writeArguments = WriteStruct()
		if displayNumber != None:
			writeArguments.displayNumber = displayNumber
		if regionBegin != None:
			writeArguments.regionBegin = regionBegin
		if regionSize != None:
			writeArguments.regionSize = regionSize
		if text:
			writeArguments.text = text
		if andMask:
			writeArguments.attrAnd = andMask
		if orMask:
			writeArguments.attrOr = orMask
		if cursor != None:
			writeArguments.cursor = cursor
		if charset:
			writeArguments.charset = charset
		with nogil:
			retval = c_brlapi.brlapi__write(self.h, &writeArguments.props)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def writeDots(self, dots):
		"""Write the given dots array to the display.
		See brlapi_writeDots(3).
		* dots : points on an array of dot information, one per character. Its size must hence be the same as what displaysize provides."""
		cdef int retval
		cdef char *c_dots
		cdef unsigned char *c_udots
		(x, y) = self.displaySize
		dispSize = x * y
		if (type(dots) == unicode):
			dots = dots.encode('latin1')
		if (len(dots) < dispSize):
			dots = dots + b"".center(dispSize - len(dots), b'\0')
		c_dots = dots
		c_udots = <unsigned char *>c_dots
		with nogil:
			retval = c_brlapi.brlapi__writeDots(self.h, c_udots)
		if retval == -1:
			raise OperationError()
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

		while True:
			with nogil:
				retval = c_brlapi.brlapi__readKey(self.h, c_wait, <c_brlapi.brlapi_keyCode_t*>&code)
			if retval == -1 and not (c_brlapi.brlapi_error.brlerrno == ERROR_LIBCERR and c_brlapi.brlapi_error.libcerrno == errno.EINTR):
				raise OperationError()
			elif retval <= 0:
				if not wait:
					return None
			else:
				return code

	def readKeyWithTimeout(self, timeout_ms = -1):
		"""Read a key from the braille keyboard.
		See brlapi_readKeyWithtimeout(3).

                This function works like brlapi_readKey, except that parameter wait is replaced by a timeout_ms parameter."""
		cdef c_brlapi.brlapi_keyCode_t code
		cdef int retval
		cdef int c_timeout_ms
		c_timeout_ms = timeout_ms

		while True:
			with nogil:
				retval = c_brlapi.brlapi__readKeyWithTimeout(self.h, c_timeout_ms, <c_brlapi.brlapi_keyCode_t*>&code)
			if retval == -1 and not (c_brlapi.brlapi_error.brlerrno == ERROR_LIBCERR and c_brlapi.brlapi_error.libcerrno == errno.EINTR):
				raise OperationError()
			elif retval <= 0:
				if timeout_ms >= 0:
					return None
			else:
				return code

	def expandKeyCode(self, code):
		"""Expand a keycode into its individual components.
		This is a stub to maintain backward compatibility.
		Call brlapi.expandKeyCode(code) instead."""
		return expandKeyCode(code)

	def ignoreKeys(self, key_type, set):
		"""Ignore some key presses from the braille keyboard.
		See brlapi_ignoreKeys(3).
		
		This function asks the server to give the provided keys to brltty, rather than returning them to the application via brlapi_readKey().
		
		The given codes should be brltty commands (nul or "" was given to brlapi_enterTtyMode())"""
		cdef int retval
		cdef c_brlapi.brlapi_rangeType_t c_type
		cdef c_brlapi.brlapi_keyCode_t *c_set
		cdef unsigned int c_n
		c_type = key_type
		c_n = len(set)
		c_set = <c_brlapi.brlapi_keyCode_t*>c_brlapi.malloc(c_n * sizeof(c_brlapi.brlapi_keyCode_t))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		with nogil:
			retval = c_brlapi.brlapi__ignoreKeys(self.h, c_type, c_set, c_n)
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def acceptKeys(self, key_type, set):
		"""Accept some key presses from the braille keyboard.
		See brlapi_ignoreKeys(3).
		
		This function asks the server to give the provided keys to the application, and not give them to brltty.

		The given codes should be brltty commands (nul or "" was given to brlapi_enterTtyMode())"""
		cdef int retval
		cdef c_brlapi.brlapi_rangeType_t c_type
		cdef c_brlapi.brlapi_keyCode_t *c_set
		cdef unsigned int c_n
		c_type = key_type
		c_n = len(set)
		c_set = <c_brlapi.brlapi_keyCode_t*>c_brlapi.malloc(c_n * sizeof(c_brlapi.brlapi_keyCode_t))
		for i from 0 <= i < c_n:
			c_set[i] = set[i]
		with nogil:
			retval = c_brlapi.brlapi__acceptKeys(self.h, c_type, c_set, c_n)
		c_brlapi.free(c_set)
		if retval == -1:
			raise OperationError()
		else:
			return retval
	
	def ignoreAllKeys(self):
		"""Ignore all key presses from the braille keyboard.
		See brlapi_ignoreAllKeys(3).
		
		This function asks the server to give all keys to brltty, rather than returning them to the application via brlapi_readKey()."""
		cdef int retval
		with nogil:
			retval = c_brlapi.brlapi__ignoreAllKeys(self.h)
		if retval == -1:
			raise OperationError()
		else:
			return retval
	
	def acceptAllKeys(self):
		"""Accept all key presses from the braille keyboard.
		See brlapi_acceptAllKeys(3).
		
		This function asks the server to give all keys to the application, and not give them to brltty.

		Warning: after calling this function, make sure to call brlapi_ignoreKeys() for ignoring important keys like BRL_CMD_SWITCHVT_PREV/NEXT and such."""
		cdef int retval
		with nogil:
			retval = c_brlapi.brlapi__acceptAllKeys(self.h)
		if retval == -1:
			raise OperationError()
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
		c_keys = <c_brlapi.brlapi_range_t*>c_brlapi.malloc(c_n * sizeof(c_brlapi.brlapi_range_t))
		for i from 0 <= i < c_n:
			c_keys[i].first = keys[i][0]
			c_keys[i].last = keys[i][1]
		with nogil:
			retval = c_brlapi.brlapi__ignoreKeyRanges(self.h, c_keys, c_n)
		c_brlapi.free(c_keys)
		if retval == -1:
			raise OperationError()
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
		c_keys = <c_brlapi.brlapi_range_t*>c_brlapi.malloc(c_n * sizeof(c_brlapi.brlapi_range_t))
		for i from 0 <= i < c_n:
			c_keys[i].first = keys[i][0]
			c_keys[i].last = keys[i][1]
		with nogil:
			retval = c_brlapi.brlapi__acceptKeyRanges(self.h, c_keys, c_n)
		c_brlapi.free(c_keys)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def enterRawMode(self, driver):
		"""Switch to Raw mode
		See brlapi_enterRawMode(3).
		
		* driver : Specifies the name of the driver for which the raw communication will be established"""
		cdef int retval
		cdef char *c_driver
		if (type(driver) == unicode):
			driver = driver.encode('ASCII')
		c_driver = driver
		with nogil:
			retval = c_brlapi.brlapi__enterRawMode(self.h, c_driver)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def leaveRawMode(self):
		"""leave Raw mode
		See brlapi_leaveRawMode(3)."""
		cdef int retval
		with nogil:
			retval = c_brlapi.brlapi__leaveRawMode(self.h)
		if retval == -1:
			raise OperationError()
		else:
			return retval

	def getParameter(self, param, subparam = 0, flags = 0):
		"""Get the value of a parameter.
		See brlapi_getParameter(3).

		This gets the current content of a parameter"""
		cdef c_brlapi.brlapi_param_t c_param
		cdef c_brlapi.brlapi_param_subparam_t c_subparam
		cdef c_brlapi.brlapi_param_flags_t c_flags
		cdef void *c_value
		cdef size_t size
		cdef ssize_t retval
		cdef uint64_t *values64
		cdef uint32_t *values32
		cdef uint16_t *values16
		cdef uint8_t *values8
		cdef char *string

		c_param = param
		c_subparam = subparam
		c_flags = flags

		with nogil:
			c_value = c_brlapi.brlapi__getParameterAlloc(self.h, c_param, c_subparam, c_flags, &size)
		if c_value == NULL:
			raise OperationError()

		try:
			ret = _parameterToPython(c_param, c_value, size)
		except Exception as e:
			c_brlapi.free(c_value)
			raise(e)

		c_brlapi.free(c_value)
		return ret


	def setParameter(self, param, subparam, flags, value):
		"""Set the value of a parameter.
		See brlapi_setParameter(3).

		This sets the content of a parameter"""
		cdef c_brlapi.brlapi_param_t c_param
		cdef c_brlapi.brlapi_param_subparam_t c_subparam
		cdef c_brlapi.brlapi_param_flags_t c_flags
		cdef void *c_value
		cdef uint64_t *values64
		cdef uint32_t *values32
		cdef uint16_t *values16
		cdef uint8_t *values8
		cdef char *string
		cdef const c_brlapi.brlapi_param_properties_t *props

		c_param = param
		c_subparam = subparam
		c_flags = flags

		with nogil:
			props = c_brlapi.brlapi_getParameterProperties(c_param)

		if props == NULL:
			raise OperationError()

		if props.type == PARAM_TYPE_STRING:
			if type(value) != unicode and type(value) != str:
				raise ValueError("String value expected")

		if props.type == PARAM_TYPE_BOOLEAN:
			if props.isArray:
				if type(value[0]) != bool:
					raise ValueError("Boolean values expected")
			else:
				if type(value) != bool:
					raise ValueError("Boolean value expected")

		if props.type == PARAM_TYPE_UINT8 or \
		   props.type == PARAM_TYPE_UINT16 or \
		   props.type == PARAM_TYPE_UINT32:
			if props.isArray:
				if type(value[0]) != int:
					raise ValueError("Integer values expected")
			else:
				if type(value) != int:
					raise ValueError("Integer value expected")

		if props.type == PARAM_TYPE_STRING:
			if type(value) == unicode:
				value = value.encode('UTF-8')
			size = len(value)
			c_value = <void*>c_brlapi.malloc(size)
			values8 = <uint8_t *>c_value
			string = value
			c_brlapi.memcpy(<void*>values8,<void*>string,size)
		elif props.type == PARAM_TYPE_BOOLEAN or props.type == PARAM_TYPE_UINT8:
			if props.isArray:
				size = 1 * len(value)
				c_value = <void*>c_brlapi.malloc(size)
				values8 = <uint8_t *>c_value
				for i in len(value):
					values8[i] = value[i]
			else:
				size = 1
				c_value = <void*>c_brlapi.malloc(size)
				values8 = <uint8_t *>c_value
				values8[0] = value
		elif props.type == PARAM_TYPE_UINT16:
			if props.isArray:
				size = 2 * len(value)
				c_value = <void*>c_brlapi.malloc(size)
				values16 = <uint16_t *>c_value
				for i in len(value):
					values16[i] = value[i]
			else:
				size = 2
				c_value = <void*>c_brlapi.malloc(size)
				values16 = <uint16_t *>c_value
				values16[0] = value
		elif props.type == PARAM_TYPE_UINT32:
			if props.isArray:
				size = 4 * len(value)
				c_value = <void*>c_brlapi.malloc(size)
				values32 = <uint32_t *>c_value
				for i in len(value):
					values32[i] = value[i]
			else:
				size = 4
				c_value = <void*>c_brlapi.malloc(size)
				values32 = <uint32_t *>c_value
				values32[0] = value
		elif props.type == PARAM_TYPE_UINT64:
			if props.isArray:
				size = 4 * len(value)
				c_value = <void*>c_brlapi.malloc(size)
				values64 = <uint64_t *>c_value
				for i in len(value):
					values64[i] = value[i]
			else:
				size = 8
				c_value = <void*>c_brlapi.malloc(size)
				values64 = <uint64_t *>c_value
				values64[0] = value
		else:
			raise ValueError("Unsupported parameter type")

		with nogil:
			retval = c_brlapi.brlapi__setParameter(self.h, c_param, c_subparam, c_flags, c_value, size)
		if retval == -1:
			c_brlapi.free(c_value)
			raise OperationError()
		c_brlapi.free(c_value)

	def watchParameter(self, param, subparam, flags, func):
		"""Set a parameter change callback.
		See brlapi_watchParameter(3).

		This registers a parameter change callback: whenever the given
		parameter changes, the given function is called.

		This returns an entry object, to be passed to unwatchParameter."""
		cdef c_brlapi.brlapi_param_t c_param
		cdef c_brlapi.brlapi_param_subparam_t c_subparam
		cdef c_brlapi.brlapi_param_flags_t c_flags
		cdef c_brlapi.brlapi_python_paramCallbackDescriptor_t *descr

		c_param = param
		c_subparam = subparam
		c_flags = flags

		def cfunc(param):
			cdef c_brlapi.brlapi_python_callbackData_t *callbackData
			callbackData = <c_brlapi.brlapi_python_callbackData_t*> <uintptr_t> param

			parameter = callbackData.parameter
			subparam = callbackData.subparam
			flags = callbackData.flags
			data = _parameterToPython(callbackData.parameter, callbackData.data, callbackData.len)

			func(parameter, subparam, flags, data)

		descr = c_brlapi.brlapi_python_watchParameter(self.h, c_param, c_subparam, c_flags, cfunc)
		return <uintptr_t>descr

	def unwatchParameter(self, entry):
		"""Clear a parameter change callback.
		See brlapi_unwatchParameter(3).

		This unregisters a parameter change callback: the callback
		function previously registered with brlapi_watchParameter
		will not be called any more."""
		cdef uintptr_t descr

		descr = entry
		c_brlapi.brlapi_python_unwatchParameter(self.h, <c_brlapi.brlapi_python_paramCallbackDescriptor_t *>descr)
