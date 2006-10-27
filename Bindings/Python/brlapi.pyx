"""
This module is a binding for BrlAPI, a BrlTTY bridge for applications.

C API documentation : http://mielke.cc/brltty/doc/BrlAPIref-HTML

Example : 
import brlapi
b = brlapi.Connection()
b.enterTtyMode()
b.writeText("Press any key to continue ...")
key = b.readKey()
(command, argument, flags) = b.expandKeyCode(key)
b.writeText("Key %ld (%x %x %x) ! Press any key to exit ..." % (key, command, argument, flags))
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
	"""Structure containing arguments to be given to Bridge.write()"""
	cdef c_brlapi.brlapi_writeStruct props

	def __new__(self):
		self.props.regionBegin = 0
		self.props.regionSize = 0
		self.props.text = ""
		# I must add attrAnd & attrOr
		self.props.cursor = 0
		self.props.charset = ""

	property regionBegin:
		"""Display number -1 == unspecified"""
		def __get__(self):
			return self.props.regionBegin
		def __set__(self, val):
			self.props.regionBegin = val

	property regionSize:
		"""Region of display to update, 1st character of display is 1"""
		def __get__(self):
			return self.props.regionSize
		def __set__(self, val):
			self.props.regionSize = val

	property text:
		"""Number of characters held in text, attrAnd and attrOr. For multibytes text, this is the number of multibyte characters. Combining and double-width characters count for 1"""
		def __get__(self):
			return self.props.text
		def __set__(self, val):
			self.props.text = val

	property cursor:
		"""Or attributes; applied after ANDing"""
		def __get__(self):
			return self.props.cursor
		def __set__(self, val):
			self.props.cursor = val

	property charset:
		"""-1 == don't touch, 0 == turn off, 1 = 1st char of display, ..."""
		def __get__(self):
			return self.props.charset
		def __set__(self, val):
			self.props.charset = val

	property attrAnd:
		"""Text to display"""
		def __get__(self):
			return <char*>self.props.attrAnd
		def __set__(self, val):
			self.props.attrAnd = <unsigned char*>val

	property attrOr:
		"""And attributes; applies first"""
		def __get__(self):
			return <char*>self.props.attrOr
		def __set__(self, val):
			self.props.attrOr = <unsigned char*>val

cdef class Settings:
	"""Settings structure for BrlAPI connection"""
	cdef c_brlapi.brlapi_settings_t props
	def __init__(self, authkey = None, hostname = None):
		if authkey:
			self.props.authKey = authkey
		else:
			self.props.authKey = ""

		if hostname:
			self.props.hostName = hostname
		else:
			self.props.hostName = ""

	property authkey:
		"""To get authorized to connect, libbrlapi has to tell the BrlAPI server a secret key, for security reasons. This is the path to the file which holds it; it will hence have to be readable by the application.
		
		Setting None defaults it to local installation setup or to the content of the BRLAPI_AUTHPATH environment variable, if it exists."""
		def __get__(self):
			return self.props.authKey
		def __set__(self, val):
			self.props.authKey = val

	property hostname:
		"""This tells where the BrlAPI server resides : it might be listening on another computer, on any TCP port. It should look like "foo:1", which means TCP port number BRLAPI_SOCKETPORTNUM+1 on computer called "foo".
		
		Note: Please check that resolving this name works before complaining.
		
		Settings None defaults it to localhost, using the local installation's default TCP port, or to the content of the BRLAPI_HOSTNAME environment variable, if it exists."""
		def __get__(self):
			return self.props.hostName
		def __set__(self, val):
			self.props.hostName = val

cdef class Connection:
	"""Class which manages the bridge between BrlTTY and your program"""
	cdef c_brlapi.brlapi_handle_t *h
	def __init__(self, Settings clientSettings = None, Settings usedSettings = None):
		"""Connect your program to BrlTTY using settings"""
		cdef c_brlapi.brlapi_settings_t *client
		cdef c_brlapi.brlapi_settings_t *used
		cdef int retval

		self.h = <c_brlapi.brlapi_handle_t*> c_brlapi.malloc(c_brlapi.brlapi_getHandleSize())

		# Fix segmentation fault
		if clientSettings == None:
			client = NULL
		else:
			client = &clientSettings.props

		if usedSettings == None:
			used = NULL
		else:
			used = &usedSettings.props

		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__initializeConnection(self.h, client, used)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			if clientSettings == None:
				hostname = "localhost"
			else:
				hostname = clientSettings.hostname

			raise ConnectionError("couldn't connect to %s: %s" % (hostname,returnerrno()))

	def __del__(self):
		"""Close the BrlAPI conection"""
		c_brlapi.brlapi__closeConnection(self.h)
		c_brlapi.free(self.h)

	# TODO: loadAuthKey

	property displaysize:
		"""Get the size of the braille display"""
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
	
	property driverid:
		"""Identify the driver used by BrlTTY"""
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

	# TODO: getDriverInfo
	
	property drivername:
		"""Get the complete name of the driver used by BrlTTY"""
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

	def enterTtyMode(self, tty = -1, how = None):
		"""Ask for some tty, with some key mechanism
		* tty : If tty >= 0, application takes control of the specified tty
			If tty == -1, the library first tries to get the tty number from the WINDOWID environment variable (form xterm case), then the CONTROLVT variable, and at last reads /proc/self/stat (on linux)
		* how : Tells how the application wants readKey() to return key presses. None or "" means BrlTTY commands are required, whereas a driver name means that raw key codes returned by this driver are expected."""
		cdef int retval
		cdef int c_tty
		cdef char *c_how
		c_tty = tty
		if not how:
			c_how = NULL
		else:
			c_how = how
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterTtyMode(self.h, c_tty, c_how)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def leaveTtyMode(self):
		"""Stop controlling the tty"""
		cdef int retval
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__leaveTtyMode(self.h)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval
			

	# TODO : getTtyPath
	
	def setFocus(self, tty):
		"""Tell the current tty to brltty.
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
		* dots : points on an array of dot information, one per character. Its size must hence be the same as what displaysize provides."""
		cdef int retval
		cdef unsigned char *c_dots
		c_dots = <unsigned char *> dots
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__writeDots(self.h, c_dots)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def writeText(self, str, cursor = 0):
		"""Write the given \0-terminated string to the braille display.
		If the string is too long, it is cut. If it's too short, spaces are appended. The current LC_CTYPE locale is considered, unless it is left as default "C", in which case the charset is assumed to be 8bits, and the same as the server's.

		* cursor : gives the cursor position; if equal to 0, no cursor is shown at all; if cursor == -1, the cursor is left where it is
		* str : points to the string to be displayed"""
		cdef int retval
		cdef int c_cursor
		cdef char *c_str
		c_cursor = cursor
		c_str = str
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__writeText(self.h, c_cursor, c_str)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def readKey(self, block = True):
		"""Read a key from the braille keyboard.

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
		"""Expand a keycode into command, argument and flags parts."""
		cdef unsigned int command
		cdef unsigned int argument
		cdef unsigned int flags
		cdef int retval
		retval = c_brlapi.brlapi_expandKeyCode(code, <unsigned int*>&command, <unsigned int*>&argument, <unsigned int*>&flags)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return (command, argument, flags)
	
	def ignoreKeyRange(self, range):
		"""Ignore some key presses from the braille keyboard.

		This function asks the server to give keys between x and y to brltty, rather than returning them to the application via readKey().

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

	# TODO: ignoreKeySet & unignoreKeySet

	def unignoreKeyRange(self, range):
		"""Unignore some key presses from the braille keyboard.

		This function asks the server to return keys between x and y to the application, and not give them to brltty.

		Note: You shouldn't ask the server to give you key presses which are usually used to switch between TTYs, unless you really know what you are doing ! The given codes are either raw keycodes if some driver name was given to enterTtyMode(), or brltty commands if None or "" was given."""
		cdef int retval
		cdef unsigned long long x,y
		x = range[0]
		y = range[1]
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__unignoreKeyRange(self.h, x, y)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerror())
		else:
			return retval

	def enterRawMode(self, drivername):
		"""Switch to Raw mode
		
		* driver : Specifies the name of the driver for which the raw communication will be established"""
		cdef int retval
		cdef char *c_drivername
		c_drivername = drivername
		c_brlapi.Py_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi__enterRawMode(self.h, c_drivername)
		c_brlapi.Py_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerror())
		else:
			return retval

include "cmddefs.auto.pyx"
