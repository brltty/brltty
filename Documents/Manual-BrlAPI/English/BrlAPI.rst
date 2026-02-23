========================
BrlAPI Reference manual
========================

:Author: Sébastien Hinderer `<Sebastien.Hinderer@ens-lyon.org> <mailto:Sebastien.Hinderer@ens-lyon.org>`_
         and Samuel Thibault `<Samuel.Thibault@ens-lyon.org> <mailto:Samuel.Thibault@ens-lyon.org>`_
:Date: V1.5, November 2019

This document describes ``BrlAPI``.

.. contents::
   :depth: 3

.. _sec-intro:

Introduction
============

*BrlAPI* is a service provided by the *brltty* daemon.

Its purpose is to allow programmers to write applications that take advantage
of a braille terminal in order to deliver a blind user suitable information
for his/her specific needs.

While an application communicates with the braille terminal, everything
*brltty* sends to the braille terminal in the application's console is
ignored, whereas each piece of data coming from the braille terminal is sent to
the application, rather than to *brltty*.

Concepts
--------

All throughout this manual, a few terms will be used which are either
specific to braille terminals, or introduced because of *BrlAPI*. They are defined
below. Taking a few minutes to go through this glossary will save a lot
of time and questions later.

Authorization key
   A file containing arbitrary data, that has to be sent to the server by the
   client, to prove it is allowed to establish a connection and then control
   the braille terminal.

Braille display
   The small screen on the braille terminal that is able to display braille text.

Braille keyboard
   The keyboard of the braille terminal.

Braille terminal
   A computer designed to display text in braille. In this case, the text is
   supposed to come from another computer running Linux or any other Unix system.

Brltty
   The background process that gives a blind person access to the console screen
   thanks to a braille terminal or speech synthetizer.

Client
   An application designed to handle a braille terminal thanks to *BrlAPI*.

Command
   A code returned by the driver, indicating an action to do, for instance
   "go to previous line", "go to next line", etc.

Driver
   A library that has functions to communicate with a braille terminal.
   Basically, a driver has functions to open communication with the
   braille terminal, close the communication, write on the braille
   display, and read keypresses from the braille keyboard, plus some special
   functions that will be described in detail in this manual.

Key
   A code that is returned by the driver when a key is pressed. This is
   different from a command, because the command concept is driver-independent
   (all drivers use the same command codes - those defined by *brltty*), whereas
   codes used for returning keypresses may vary between drivers.

BrlAPI's Library
   This library helps clients to connect and use *BrlAPI*'s
   server thanks to a series of ``brlapi_``-prefixed functions.

Packet
   A sequence of bytes making up the atomic unit in communications, either between
   braille drivers and braille terminals or between the server and clients.

Raw mode
   Mode in which the client application exchanges packets with the driver.
   Normal operations like sending text for display or reading keypresses are
   not available in this mode. It lets applications take advantage of advanced
   functionalities of the driver's communication protocol.

Server
   The part of *brltty* that controls incoming connections and communication
   between clients and braille drivers.

Suspend mode
   Mode in which the server keeps the device driver closed, so that the client
   can connect directly to the device.

Tty
   Synonym for console, terminal, ...  Linux' console consist of several Virtual
   Ttys (VTs).  The screen program's windows also are Ttys.  X-window system's
   xterms emulate Ttys as well.

How to read this manual
-----------------------

This manual is split in five parts.

:ref:`General description <sec-general>`
   Describes more precisely what *BrlAPI*
   is and how it works in collaboration with *brltty*'s core, the braille driver
   and clients. In this part, a "connection-use-disconnection" scenario
   will be described step by step, explaining for each step what *BrlAPI* does in
   reaction to client instructions. These explanations will take place at a
   user level.

:ref:`Concurrency management <sec-concurrency>`
   This part explains how concurrency between *BrlAPI* clients is handled
   thanks to focus tellers.

:ref:`Installation and configuration <sec-install>`
   This part explains in detail how to install and configure the API. For
   instructions on how to install and configure *brltty*, please report to
   the *brltty* documentation.

:ref:`Library description <sec-library>`
   This part describes how client applications
   can communicate with the server using the *BrlAPI* library that
   comes with *brltty*. Each function will be briefly described,
   classified by categories. More exhaustive descriptions of every
   function are available in the corresponding online manual pages.

:ref:`Writing braille drivers <sec-drivers>`
   This part describes how the braille drivers included in *brltty* should be
   written in order to take advantage of *BrlAPI*'s services.

:ref:`Protocol reference <sec-protocol>`
   This part describes in detail the communication
   protocol that is used to communicate between server and clients.

What should be read probably depends on what should be done by applications with
*BrlAPI*.

Reading chapters :ref:`General description <sec-general>`, :ref:`Concurrency
management <sec-concurrency>` and :ref:`Installation and configuration
<sec-install>` is recommended, since they provide
useful information and (hopefully) lead to a good understanding of *BrlAPI*,
for an efficient use.

Chapter :ref:`Library description <sec-library>` concerns writing
applications that take advantage of braille terminals so as to bring specific
(and more useful) information to blind people.

Chapter :ref:`Drivers <sec-drivers>` is for braille driver implementation: either adding a braille driver
to *brltty* or modifying an existing one so that it can benefit from
*BrlAPI*'s features, this chapter will be of interest, since it describes
exactly what is needed to write a driver for *brltty*: the core
of drivers interface for instance.

Finally, chapter :ref:`Protocol reference <sec-protocol>` is for *not using* the library, but using the *BrlAPI*
server directly, when the library might not be sufficient: it describes the
underlying protocol that will have to be used to do so.

.. _sec-general:

General description of *BrlAPI*
===============================

Here is explained what *BrlAPI* is, and what it precisely does.
These explanations should be simple enough to be accessible to every user.
For a more technical review of *BrlAPI*'s functionalities, please see chapter
:ref:`Library description <sec-library>`.

Historical notes.
-----------------

Originally, *brltty* was designed to give access to the Linux
console to visually impaired people, through a braille terminal
or a speech synthetizer. At that time, applications running in the
console were not taking care of the presence of a braille
terminal (most applications didn't even know what a braille
terminal was).

This situation where applications are not aware of the presence of a
special device is elegant of course, since it lets use an
unlimited number of applications which don't need to be specially
designed for visually impaired people.

However, it appeared that applications specially designed to take
advantage of a braille terminal could be wanted, to
provide the suitable information to blind users, for instance.
The idea of *BrlAPI* is to propose an efficient communication
mechanism, to control the braille display, read keys from the
braille keyboard, or to exchange data with the braille terminal at
a lower level (e.g. to write file transfer protocols between
braille terminals and Linux systems).

Why *BrlAPI* is part of *brltty*.
---------------------------------

Instead of rewriting a whole communication program from
scratch, we chose to add communication
mechanisms to *brltty*. This choice has two main justifications.

On the one hand, integration to *brltty* allows us to use the
increasing number of drivers written for *brltty*, thus handling
a large number of braille terminals without having to rewrite any
piece of existing code.

On the other hand, if an application chooses to send its own
information to the braille display, and to process braille keys,
*brltty* has to be warned, so that it won't try to communicate
with the braille terminal while the application already does.
To make this synchronzation between *brltty*
and client applications possible, it seemed easier to add the
communication mechanisms to *brltty*'s core, instead of writing an
external program providing them.

How it works.
-------------

We are now going to describe the steps an application should
go through to get control of the braille terminal, and what
happens on *brltty*'s side at each step. This step-by-step
description will let us introduce more precisely some
concepts that are useful for every *BrlAPI* user.

Connection.
~~~~~~~~~~~

The first thing any client application has to do is to
connect (in the Unix sense of the word) to *BrlAPI* which is
an mere application server. If this is not
clear, the only thing to be remembered is that this
step allows the client application to let the server know about its
presence. At this stage, nothing special is done on *brltty*'s
side.

Authorization.
~~~~~~~~~~~~~~

Since Unix is designed to allow many users to work on the
same machine, it's quite possible that there are more than one
user accounts on the system. Most probably, one doesn't want
any user with an account on the machine to be able to communicate
with the braille terminal (just imagine what would happen if,
while somebody was working with the braille terminal, another user
connected to the system began to communicate with it,
preventing the first one from doing his job...). That's why *BrlAPI* has to
provide a way to determine whether a user who established a
connection is really allowed to communicate with the braille
terminal. To achieve this, *BrlAPI* requires that each
application that wants to control a braille terminal sends an
authorization key before doing anything else. The control of
the braille terminal will only be possible for the client once
it has sent the proper authorization key. What is called
authorization key is in fact a Unix file containing data (it
must be non-empty) on your system. All the things you have to do is to give
read permissions on this file to users that are allowed to
communicate with the braille terminal, and only to them. This
way, only authorized users will have access to the
authorization key and then be able to send it to *BrlAPI*.
To see how to do that, please see chapter :ref:`Installation and configuration <sec-install>`.

At the end of this step, the user is authorized to take
control of the braille terminal. On *brltty*'s side, some data
structures are allocated to store information on the client,
but this has no user-level side-effect.

Real use of the braille terminal.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the client is properly connected and authorized,
there are two possible types of communication with the braille
terminal. The chosen type of communication depends on what the
client plans to do. If its purpose is to display information on
the braille display or to process braille keys, it will have to
take control of the Linux tty on which it is running. If its
purpose is to exchange data with the braille terminal (e.g. for
file transfer), it will enter what is called "raw mode".

Braille display and braille key presses processing.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the client wants to display something on the braille
display or to process braille keys itself, rather than letting
*brltty* process them, it has to take control of the Linux
terminal it is running on.

Once a client has obtained the control of his tty, *BrlAPI*
will completely discard *brltty*'s display on this tty (and only
this one), leaving the braille display free for the client.

At the same time, if a key is pressed on the braille
keyboard, *BrlAPI* checks whether the client application is
interested in this key or not. If it is, the key is passed to
it, either as a key code or as a *brltty* command. If it is not, the
key code is converted into a *brltty* command and returned to
*brltty*.

Once the client is not interested in displaying text
or reading braille keys any more, it has to leave the tty, so
that either *brltty* can continue its job, or another client can
take control of it.

Parameter handling.
^^^^^^^^^^^^^^^^^^^

The server exposes some parameters to the client. Some parameters are global to
all clients (e.g. the braille display size), while others are local per client
(e.g. retaindots, i.e. whether to send Perkins presses as dot patterns or as
letters). Some parameters are read-only (e.g. the braille display size), while
others are read-write (e.g. retaindots). Some parameters may change during
execution, while others change only when a client set it.

Clients can either request the current value of a parameter, or set its value
(if it is read-write), or request the server to notify on value change.

Raw mode.
^^^^^^^^^

Only one client can be in raw mode at the same time. In
this mode, data coming from the braille terminal are checked
by the driver (to ensure they are valid), but instead of being processed,
they are delivered "as-is" to the client that is in raw mode.

In the other direction, packets sent to *BrlAPI* by the
client that is in raw mode are passed to the driver which is
expected to deliver them to the braille terminal without any
modification.

Suspend Mode.
^^^^^^^^^^^^^

Only one client can be in suspend mode at the same time. This mode is also
exclusive with raw mode. In this mode, the server keeps the device driver
closed, and thus the client can open the device directly by itself.

**This mode is not recommended**, since the client will then have to
reimplement device access. Raw mode should really be preferred, since it lets
the client take advantage of server's ability to talk with the device
(USB/bluetooth support for instance).

Remarks.
^^^^^^^^

- The operations described in the three previous
  subsections are not completely mutually exclusive. An
  application that controls its current tty can enter raw or suspend
  mode, provided that no other application already is in this
  mode.

- Not every braille driver supports raw mode. It has
  to be specially (re)written to support it, since it has
  to provide special functions to process incoming and outgoing
  packets. The same restriction is true (but less strong)
  concerning the ability to deliver/convert keycodes into
  commands: not every driver has this ability, it has to be
  modified to get it.

- Operations previously described can be repeated.
  You can, for instance, use raw mode to transfer data onto
  your braille terminal, display text in braille, return to raw
  mode..., all that without having to reconnect to *BrlAPI* before
  each operation.

Disconnection.
~~~~~~~~~~~~~~

Once the client has finished using the braille terminal, it
has to disconnect from the API, so that the memory structures
allocated for the connection can be freed and eventually used by
another client. This step is transparent for the user, in the
sense that it involves no change on the braille display.


.. _sec-concurrency:

Concurrency management between *BrlAPI* clients
================================================

An essential purpose of *BrlAPI* is to manage concurrent access to the
braille display between the *brltty* daemon and applications. This
concurrency is managed "per Tty". We first describe this with a flat view, and
then consider Tty hierarchy.

VT switching
------------

Let's first describe how things work with the simple case of a single series of
Virtual Ttys (VTs), the linux console for instance.

As described in :ref:`General Description <sec-general>`, before being able
to write output, a *BrlAPI* client has to "get" a tty, i.e. it sends to the
*BrlAPI* server the number of the linux' Virtual Tty on which it is running.
The *BrlAPI* server uses this information so as to know which client's output
should be shown on the braille display, according to the focus teller's
information.

Let's say some client *A* is running on VT 2.  It "got" VT 2 and wrote some
output on its *BrlAPI* connection.  The focus teller is *brltty* here: it
always tells to the *BrlAPI* server which VT is currently shown on the screen
and gets usual keyboard presses (it is "active").

Let's say VT 1 is active, then the *BrlAPI* server shows *brltty*'s output
on the braille display.  I.e. the usual *brltty* screen reading appears.
Moreover, when braille keys are pressed, they are passed to *brltty*, so that
usual screen reading can be performed.  When the user switches to VT 2,
*brltty* (as focus teller) tells it to the *BrlAPI* server, which then
remembers that client *A* has got it and has produced some output.  The
server then displays this output on the braille display.  Note that *A*
doesn't need to re-submit its output: the server had recorded it so as to be
able to show it as soon as the focus switches to VT 2.  Whenever some key of the
braille device is pressed, *BrlAPI* looks whether it is in the list of keys
that client *A* said to be of his interest.  If it is, it is passed to *A*
(and not to *brltty*). If it isn't, it is passed to *brltty* (and not to
*A*).

As a consequence, whenever clients get and release Ttys and the user switches
between Ttys, either the *brltty* screen reading or the client's output is
automatically shown according to rather natural rules.

A pile of "paper sheets"
------------------------

Let's look at VT 2 by itself. What is shown on the braille display can be seen
as the result of a pile of two paper sheets.  *brltty* is represented by the
bottom sheet on which its screen reading is written, and client *A* by the
top sheet on which its output is written. *A*'s sheet hence "covers"
*brltty*'s sheet: *A*'s output "mask" *brltty*'s screen reading.

*A* may yet want to temporarily let *brltty*'s screen reading appear on VT
2, while still receiving some key presses, for instance.  For this, it sends a
"void" write.  The server then clears the recorded output for this connection
(in the sheet representation, the sheet becomes "transparent").  As a
consequence, *brltty*'s output is automatically shown (by transparency in the
sheet representation), just like if *A* had released the Tty.

Keypresses are handled in a similar way: *A*'s desire to get key presses is
satisfied first before *brltty*.

Let's say some other client *B* (probably launched by *A*) also gets VT 2
and outputs some text on its *BrlAPI* connection.  This adds a third sheet,
on top of the two previous ones.  It means that the *BrlAPI* server will show
*B*'s output on the braille device.  If *A* then outputs some text, the
server will record it (on *A*'s sheet which hence becomes opaque again), but
it won't be displayed on the braille device, since *B*'s sheet is still at
the top and opaque (i.e. with some text on it).  But if *B* issues a void
write, the server clears its output buffer (i.e. *B*'s sheet becomes
transparent), and as a result *A*'s output appear on the braille display (by
transparency through *B*'s sheet).

The sheet order is by default determined by the Tty "get"ting order. Clients
can however change their priority (which by default is 50) to a higher value in
order to show up higher in the pile, or to a lower value in order to hide lower
in the pile.

Hierarchy
---------

Now, what happens when running some *screen* program on, say, VT 3?  It
emulates a series of Ttys, whose output actually appear on the same VT 3.
That's where a hierarchy level appears: the focus information is not only the VT
number but also, in the case of VT 3, which *screen* window is active.  This
hence forms a *tree* of Ttys: the "root" being the vga driver's output, whose
sons are VTs, and VT 3 has the *screen* windows as sons.  *Brltty* is a
focus teller for the root, *screen* will have to be a focus teller for VT 3.
*Screen* should then get VT 3, not display anything (so that the usual
*brltty* screen reading will be shown by transparency), and tell the
*BrlAPI* server which *screen* window is active (at startup and at each
window switch).  This is not implemented directly in *screen* yet, but this
may be achieved via a second *brltty* daemon running the Screen driver (but
it isn't yet able to get the current window number though) and the *BrlAPI*
driver.

A *BrlAPI* client *C* running in some *screen* window number 1 would
then have to get the Tty path "VT 3 then window 1", which is merely expressed
as "3 1".  The window number is available in the ``WINDOW`` environment
variable, set by *screen*. The VT number, which actually represents the "path
to screen's output" should be available in the ``WINDOWPATH`` environment
variable, also set by *screen*.  The client can thus merely concatenate the
content of ``WINDOWPATH`` (which could hold many levels of window numbers) and
of ``WINDOW`` and give the result as tty path to the *BrlAPI* server, which
then knows precisely where the client's usual output resides.  In practice,
applications just need to call ``brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT)``, and
the the *BrlAPI* client library will automatically perform all that.

Whenever the user switches to VT 3, the *BrlAPI* server remembers the window
that *screen* told to be active.  If it was window 1, it then displays
*C*'s output (if any).  Else *brltty*'s usual screen reading is shown.
Of course, several clients may be run in window 1 as well, and the "sheet pile"
mechanism applies: *brltty*'s sheet first (at the root of the Ttys tree), then
*screen*'s sheet (which is transparent, on VT 3), then *C*'s sheet (on
window 1 of VT 3), then other clients' sheets (on the same window).

Ttys are hence organized in a tree, each client adding its sheet at some tty in
the tree.

The X-window case
------------------

Let's say some X server is running on VT 7 of a Linux system. Xorg's *xinit*
and *xdm* commands automatically set the X session's ``WINDOWPATH``
environment variable to "7", so that X11 *BrlAPI* clients started from
the session just need to call *brlapi_enterTtyMode(xid)* where *xid*
is the X-window ID of the window of the client. The *BrlAPI* library
will automatically prepend the content of ``WINDOWPATH`` to it.

For text-based *BrlAPI* clients running in an xterm (which should just call
``brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT)`` as explained in the previous
section), *BrlAPI* detects the window id thanks to the ``WINDOWID`` variable
set by xterm.

Screen readers are not bound to a particular window, so they should call
*brlapi_enterTtyModeWithPath(NULL, 0)* to let the *BrlAPI* library only
send the content of ``WINDOWPATH``, expressing that screen readers take the
whole tty.  The user should notably launch *xbrlapi*, which is a focus
teller for X-window as well as a keyboard simulator (*brltty* can't reliably
simulate them at the kernel level in such situation).  For accessing AT-SPI
contents (like gnome or kde applications), Orca should also be launched.  For
accessing AT-SPI terminals (like gnome-terminal) in the same way as in the
console, a second *brltty* daemon running the at-spi screen driver and the
*BrlAPI* driver can also be launched.  All three would get the VT of the
X session, in that order (for now): *xbrlapi* first, then *orca* and
*brltty* at last.  When the X focus is on an AT-SPI terminal, *brltty*
will hence be able to grab the braille display and key presses.  Else *orca*
would get them.  And *xbrlapi* would finally get remaining key presses and
simulate them.

Note: old versions of ``xinit``, ``xdm``, ``kdm`` or ``gdm`` do not
automatically set the ``WINDOWPATH`` variable. The user can set it by hand in
his ``~/.xsession``, ``~/.xinitrc``, ``~/.gdmrc``... to "7"

Note: some Operating Systems like Solaris do not have VTs. In that case
``WINDOWPATH`` is empty or not even set.  Everything explained above still
work fine.

Detaching
---------

Several programs allow detaching: *screen* and *VNC* for instance. In such
situation, an intermediate *BrlAPI* server should be run for each such
session. Clients would connect to it, and it would prepend the "current tty"
path on the fly while forwarding things to the root *BrlAPI* server. This
intermediate server is yet to be written (but it is actually relatively close to
be).


.. _sec-install:

Installation and configuration of *BrlAPI*
==========================================

``make install`` will install libbrlapi.so in /lib, and include files in
/usr/include/brltty. An authorization key will also typically be set in
/etc/brlapi.key (if it is not, just create it and put arbitrary data in it), but
it won't be readable by anybody else than root. It is up
to you to define a group of users who will have the right to read it and hence
be able to connect to the server. For instance, you may want to do::

   # addgroup brlapi
   # chgrp brlapi /etc/brlapi.key
   # chmod g+r /etc/brlapi.key
   # addgroup user1 brlapi
   # addgroup user2 brlapi
   ...


.. _sec-library:

Library description
===================

Let's now see how one can write dedicated applications. Basic notions will be
seen, along with a very simple client. Greater details are given as online
manual pages.

The historical test program for *BrlAPI* was something like:

- connect to *BrlAPI*
- get driver id
- get driver name
- get display size
- try entering raw mode, immediately leave raw mode.
- get tty control
- write something on the display
- wait for a key press
- leave tty control
- disconnect from *BrlAPI*

It is here rewritten, its working briefly explained.

Connecting to *BrlAPI*
----------------------

Connection to *BrlAPI* is needed first, thanks to the
``brlapi_openConnection`` call. For this, a
``brlapi_connectionSettings_t`` variable must be filled which will hold the
settings the library needs to connect to the server. Just giving ``NULL``
will work for local use. The other parameter lets you get back the parameters
which were actually used to initialize connection. ``NULL`` will also be nice
for now.

::

     if (brlapi_openConnection(NULL, NULL)==BRLAPI_INVALID_FILE_DESCRIPTOR) {
       brlapi_perror("brlapi_openConnection");
       exit(1);
     }

The connection might fail, so testing is needed.

Getting driver name
-------------------

Knowing the type of the braille device might be useful:

::

     char name[BRLAPI_MAXNAMELENGTH+1];
     if (brlapi_getDriverName(name, sizeof(name)) < 0)
       brlapi_perror("brlapi_getDriverName");
     else
       fprintf(stderr, "Driver name: %s\n", name);

This is particularly useful before entering raw mode to achieve file
transfers for instance, just to check that the device is really the one
expected.

Getting display size
--------------------

Before writing on the braille display, the size should be always first
checked to be sure everything will hold on it:

::

     if (brlapi_getDisplaySize(&x, &y) < 0)
       brlapi_perror("brlapi_getDisplaySize");
     else
       fprintf(stderr, "Braille display has %d line%s of %d column%s\n",
         y, y>1?"s":"", x, x>1?"s":"");

Entering raw mode, immediately leaving raw mode.
-------------------------------------------------

Entering raw mode is very simple:

::

     fprintf(stderr, "Trying to enter in raw mode... ");
     if (brlapi_enterRawMode(name) < 0)
       brlapi_perror("brlapi_enterRawMode");
     else {
       fprintf(stderr, "Ok, leaving raw mode immediately\n");
       brlapi_leaveRawMode();
     }

Not every driver supports raw mode, so testing is needed.

While in raw mode, ``brlapi_sendRaw`` and ``brlapi_recvRaw``
can be used to send and get data directly to and from the device.
It should be used with care, improper use might completely thrash the device!

Getting tty control
-------------------

Let's now display something on the device. control of the tty must be get
first:

::

     fprintf(stderr, "Taking control of the tty... ");
     if (brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT, NULL) >= 0)
     {
       fprintf(stderr, "Ok\n");

The first parameter tells the server the number of the tty to take
control of. Setting BRLAPI_TTY_DEFAULT lets the library determine it for us.

The server is asked to send *brltty* commands, which are device-independent.

Getting control might fail if, for instance, another application already took
control of this tty, so testing is needed.

From now on, the braille display is detached from the screen.

Writing something on the display
---------------------------------

The application can now write things on the braille display without
altering the screen display:

::

       fprintf(stderr, "Writing to braille display... ");
       if (brlapi_writeText(0, "Press a braille key to continue...") >= 0)
       {
         fprintf(stderr, "Ok\n");

The cursor is also asked *not* to be shown: its position is set to 0.

"Writing to braille display... Ok" is now displayed on the screen, and
"Press a braille key to continue..." on the braille display.

Waiting for a key press
-----------------------

To have a break for the user to be able to read these messages,
a key press (a command here, which is driver-independent) may be waited for:

::

         fprintf(stderr, "Waiting until a braille key is pressed to continue... ");
         if (brlapi_readKey(1, &key) > 0)
           fprintf(stderr, "got it! (code=%"BRLAPI_PRIxKEYCODE")\n", key);

The command is returned, as described in ``<brlapi_constants.h>``
and ``<brlapi_keycodes.h>``.
It is not transmitted to *brltty*: it is up to the application to define
the behavior, here cleanly exiting, as described below.

The first parameter tells the lib to block until a key press is indeed read.

Understanding commands
----------------------

There are two kinds of commands: braille commands (line up/down, top/bottom,
etc.) and X Keysyms (i.e. regular keyboard keys). One way to discover which key
was pressed is to just use a switch statement:

::

           switch(key) {
             case BRLAPI_KEY_TYPE_CMD|BRLAPI_KEY_CMD_LNUP:
               fprintf(stderr, "line up\n");
               break;
             case BRLAPI_KEY_TYPE_CMD|BRLAPI_KEY_CMD_LNDN:
               fprintf(stderr, "line down\n");
               break;
             case BRLAPI_KEY_TYPE_SYM|XK_Tab:
               fprintf(stderr, "tab\n");
               break;
             default:
               fprintf(stderr, "unknown key\n");
               break;
           }

Another way is to ask BrlAPI to expand the keycode into separate information
parts:

::

           brlapi_expandedKeyCode_t ekey;
           brlapi_expandKeyCode(key, &ekey);
           fprintf(stderr, "type %u, command %u, argument %u, flags %u\n",
             ekey.type, ekey.command, ekey.argument, ekey.flags);

Eventually, named equivalents are provided:

::

           brlapi_describedKeyCode_t dkey;
           int i;

           brlapi_describeKeyCode(key, &dkey);
           fprintf(stderr, "type %s, command %s, argument %u, flags",
             dkey.type, dkey.command, dkey.argument);
           for (i = 0; i < dkey.flags; i++)
             fprintf(stderr, " %s", dkey.flag[i]);
           fprintf(stderr, "\n");


Leaving tty control
-------------------

Let's now leave the tty:

::

       fprintf(stderr, "Leaving tty... ");
       if (brlapi_leaveTtyMode() >= 0)
         fprintf(stderr, "Ok\n");

But control of another tty can still be get for instance, by calling
``brlapi_enterTtyMode()`` again...

Disconnecting from *BrlAPI*
---------------------------

Let's disconnect from *BrlAPI*:

::

     brlapi_closeConnection();

The application can as well still need to connect to another server on another
computer for instance, by calling ``brlapi_openConnection()``
again...

Putting everything together...
------------------------------

::

   #include <stdio.h>
   #include <stdlib.h>
   #include <brlapi.h>

   int main()
   {
     brlapi_keyCode_t key;
     char name[BRLAPI_MAXNAMELENGTH+1];
     unsigned int x, y;

   /* Connect to BrlAPI */
     if (brlapi_openConnection(NULL, NULL)==BRLAPI_INVALID_FILE_DESCRIPTOR)
     {
       brlapi_perror("brlapi_openConnection");
       exit(1);
     }

   /* Get driver name */
     if (brlapi_getDriverName(name, sizeof(name)) < 0)
       brlapi_perror("brlapi_getDriverName");
     else
       fprintf(stderr, "Driver name: %s\n", name);

   /* Get display size */
     if (brlapi_getDisplaySize(&x, &y) < 0)
       brlapi_perror("brlapi_getDisplaySize");
     else
       fprintf(stderr, "Braille display has %d line%s of %d column%s\n",
         y, y>1?"s":"", x, x>1?"s":"");

   /* Try entering raw mode, immediately go out from raw mode */
     fprintf(stderr, "Trying to enter in raw mode... ");
     if (brlapi_enterRawMode(name) < 0)
       brlapi_perror("brlapi_enterRawMode");
     else {
       fprintf(stderr, "Ok, leaving raw mode immediately\n");
       brlapi_leaveRawMode();
     }

   /* Get tty control */
     fprintf(stderr, "Taking control of the tty... ");
     if (brlapi_enterTtyMode(BRLAPI_TTY_DEFAULT, NULL) >= 0)
     {
       fprintf(stderr, "Ok\n");

   /* Write something on the display */
       fprintf(stderr, "Writing to braille display... ");
       if (brlapi_writeText(0, "Press a braille key to continue...") >= 0)
       {
         fprintf(stderr, "Ok\n");

   /* Wait for a key press */
         fprintf(stderr, "Waiting until a braille key is pressed to continue... ");
         if (brlapi_readKey(1, &key) > 0) {
           brlapi_expandedKeyCode_t ekey;
           brlapi_describedKeyCode_t dkey;
           int i;

           fprintf(stderr, "got it! (code=%"BRLAPI_PRIxKEYCODE")\n", key);

           brlapi_expandKeyCode(key, &ekey);
           fprintf(stderr, "type %u, command %u, argument %u, flags %u\n",
             ekey.type, ekey.command, ekey.argument, ekey.flags);

           brlapi_describeKeyCode(key, &dkey);
           fprintf(stderr, "type %s, command %s, argument %u, flags",
             dkey.type, dkey.command, dkey.argument);
           for (i = 0; i < dkey.flags; i++)
             fprintf(stderr, " %s", dkey.flag[i]);
           fprintf(stderr, "\n");
         } else brlapi_perror("brlapi_readKey");

       } else brlapi_perror("brlapi_writeText");

   /* Leave tty control */
       fprintf(stderr, "Leaving tty... ");
       if (brlapi_leaveTtyMode() >= 0)
         fprintf(stderr, "Ok\n");
       else brlapi_perror("brlapi_leaveTtyMode");

     } else brlapi_perror("brlapi_enterTtyMode");

   /* Disconnect from BrlAPI */
     brlapi_closeConnection();
     return 0;
   }

This should compile well thanks to
``gcc apiclient.c -o apiclient -lbrlapi``

.. _sec-drivers:

Writing (*BrlAPI*-compliant) drivers for *brltty*
=================================================

In this chapter, we will describe in details how to write a
driver for *brltty*. We begin with a general description of the
structure the driver should have, before explaining more precisely
what each function is supposed to do.

Overview of the driver's structure
----------------------------------

A braille driver is in fact a library that is either
dynamically loaded by *brltty* at startup, or statically linked to
it during the compilation, depending on the options given to the
``./configure`` script.

This library has to provide every function needed by the core,
plus some additional functions, that are not mandatory, but which
improve communication with *BrlAPI* and the service level provided
to client applications.

Basically, a driver library needs to provide a function to open
the communication with the braille terminal, one to close this
communication, one to read key codes from the braille keyboard, and
one to write text on the braille display. As we will see in a
moment, other functions are required.

Moreover, a driver can provide additional functionalities, by
defining some macros asserting that it has these functionalities,
and by defining associated functions.

Basic driver structure
----------------------

*Every* *brltty* driver *must* consist in at least
a file called braille.c, located in an appropriate subdirectory of
the BrailleDrivers subdirectory. This braille.c file must have the
following layout

::

       #include "prologue.h"
       /* Include standard C headers */
       #include "Programs/brl.h"
       #include "Programs/misc.h"
       #include "Programs/scr.h"
       #include "Programs/message.h"
       /* Include other files */

       static void brl_identify() { }

       static int brl_open(BrailleDisplay *brl, char **parameters, const char *tty) { ... }

       static void brl_close(BrailleDisplay *brl) { ... }

       static void brl_writeWindow(BrailleDisplay *brl) { ... }

       static void brl_writeStatus(BrailleDisplay *brl) { ... }

       static int brl_readCommand(BrailleDisplay *brl, DriverCommandContext context) { ... }

Before giving a detailed description of what each function is
supposed to do, we define the ``BrailleDisplay`` structure,
since each function has an argument of type ``BrailleDisplay
*``. The ``BrailleDisplay`` structure is defined like this:

::

       typedef struct {

         int x, y; /* The dimensions of the display */

         int helpPage; /* The page number within the help file */

         unsigned char *buffer; /* The contents of the display */

         unsigned isCoreBuffer:1; /* The core allocated the buffer */

         unsigned resizeRequired:1; /* The display size has changed */

         unsigned int writeDelay;

         void (*bufferResized)(int rows, int columns);

       } BrailleDisplay;

We now describe each function's semantics and calling
convention.

The ``brl_identify()`` function takes no argument and returns
nothing. It is called as soon as the driver is loaded, and its
purpose is to print some information about the driver in the system
log. To achieve this, the only thing this function has to do is to
call LOG_PRINT with appropriate arguments (log level and string to
put in the syslog).

The ``brl_open()`` function takes 3 arguments and returns an int. Its
purpose is to initialize the communication with the braille
terminal. Generally, this function has to open the file referred to by
the ``tty`` argument, and to configure the associated communication
port. The ``parameters`` argument contains parameters passed to the
driver with the -B command-line option. It's up to the driver's
author to decide whether or not he/she wants to use this argument,
and what for. The function can perform some additional tasks such
as trying to identify precisely which braille terminal model is
connected to the computer, by sending it a request and analyzing its
answer. The value that is finally returned depends on the success of
the initialization process. If it fails, th function has to return
-1. The function returns 0 on success.

The ``brl_close()`` function takes just one argument, and returns
nothing. The name of this function should be self-explanatory; it's
goal is to close (finish) the communication between the computer and
the braille terminal. In general, the only thing this function has
to do is to close the file descriptor associated to the braille
terminal's communication port.

The ``brl_writeWindow()`` function takes just one argument of type
BrailleDisplay, and returns nothing. This function displays the
specified text on the braille window. This routine is the right
place to check if the text that has to be displayed is not already
on the braille display, to send it only if necessary. More
generally, if the braille terminal supports partial refresh of the
display, the calculus of what exactly has to be sent to the braille
display to have a proper display, according to what was previously
displayed should be done in this function.

The ``brl_writeStatus()`` function is very similar to ``brl_writeWindow()``.
The only difference is that whereas ``brl_writeWindow()`` writes on the
main braille display, ``brl_writeStatus()`` writes on an auxiliary braille
display, which occasionally appears on some braille terminals. The
remarks that have been done concerning optimizations for refreshing
the display still apply here.

The ``brl_readCommand()`` function takes two arguments, and returns an
integer. Its purpose is to read commands from the braille keyboard
and to pass them to *brltty*'s core, which in turn will process them.
The first argument, of type ``BrailleDisplay``, is for future use, and
can safely be ignored for the moment. The second argument indicates
in which context (state) *brltty* is. For instance, it specifies if
*brltty* is in a menu, displays a help screen, etc. This information
can indeed be of some interest when translating a key into a
command, especially if the keys can have different meanings,
depending on the context. So, this function has to read keypresses
from the braille keyboard, and to convert them into commands,
according to the given context, these commands then being returned
to *brltty*. For a complete list of available command codes, please
have a look at ``brl.h`` in the Programs subdirectory. Two codes have special
meanings:

eof
   specifies that no command is available now, and that
   no key is waiting to be converted into command in a near future.

CMD_NOOP
   specifies that no command is available, but
   that one will be, soon. As a consequence, brl_readCommand will be
   called again immediately. Returning CMD_NOOP is appropriate for
   instance when a key is composed of two consecutive data packets.
   When the first of them is received, one can expect that the second
   will arrive quickly, so that trying to read it as soon as possible
   is a good idea.

Enhancements for *BrlAPI*
--------------------------

To improve the level of service provided to client
applications communicating with braille drivers through *BrlAPI*, the
drivers should declare some additional functions that will then be
called by the API when needed.

For each additional feature that has to be implemented in a
driver, a specific macro must be defined, in addition to the
functions implementing that feature. For the moment, two features
are supported by *BrlAPI*:

- reading braille terminal specific key codes,

- exchanging raw data packets between the braille
  terminal and a client application running on the PC.

For each feature presented below, only a short description of each
concerned macro and function will be given. For a more complete description
of concepts used here, please refer to chapters :ref:`Introduction <sec-intro>` and :ref:`General description <sec-general>`.

Exchanging raw data packets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Under some circumstances, an application running on the PC
can be interested in a raw level communication with the braille
terminal. For instance, to implement a file transfer protocol,
commands to display braille or to read keys are not enough. In
such a case, one must have a way to send raw data to the
terminal, and to receive them from it.

A driver that wants to provide such a mechanism has to define
three functions: one to send packets, another one to receive them,
and the last one to reset the communication when problems occur.

The macro that declares that a driver is able to transmit packets
is:

::

   #define BRL_HAVE_PACKET_IO

The prototypes of the functions the driver should define are:

::

   static int brl_writePacket(BrailleDisplay *brl, const unsigned char *packet, int size);
   static int brl_readPacket(BrailleDisplay *brl, unsigned char *p, int size);
   static void brl_rescue(BrailleDisplay *brl)

``brl_writePacket()`` sends a packet of ``size`` bytes, stored
at ``packet``, to the braille terminal. If the communication protocol
allows to determined if a packet has been send properly (e.g. the
terminal sends back an acknowledgement for each packet he
receives), then this function should wait the acknowledgement,
and, if it is not received, retransmission of the packet should take
place.

``brl_readPacket()`` reads a packet of at most ``size`` bytes, and
stores it at the specified address. The read must not block. I.e.,
if no packet is available, the function should return immediately,
returning 0.

``brl_rescue()`` is called by *BrlAPI* when a client
application terminates without properly leaving the raw mode. This
function should restore the terminal's state, so that it is
able to display text in braille again.

Remarks.
^^^^^^^^

- If the driver provides such functions, every other
  functions should use them, instead of trying to communicate
  directly with the braille terminal. For instance, ``readCommand()``
  should call ``readPacket()``, and then extract a key from the packet,
  rather than reading directly from the communication port's file
  descriptor. The same applies for ``brl_writeWindow()``, which should
  use ``brl_writePacket()``, rather than writing on the communication
  port's file descriptor.

- For the moment, the argument of type BrailleDisplay
  can safely be ignored by the functions described here.

.. _sec-protocol:

Protocol reference
==================

Under some circumstances, it may be preferable to communicate directly with
*BrlAPI*'s server rather than using *BrlAPI*'s
library. Here are the needed details to be able
to do this. This chapter is also of interest if a precise understanding of
how the communication stuff works is desired, to be sure to understand how
to write multithreaded clients, for instance.

In all the following, *integer* will mean an unsigned 32 bits integer in
network byte order (ie most significant bytes first).

Reliable packet transmission channel
-------------------------------------

The protocol between *BrlAPI*'s server and clients is based on exchanges
of packets. So as to avoid locks due to packet loss, these exchanges are
supposed reliable, and ordering must be preserved, thus *BrlAPI* needs
a reliable packet transmission channel.

To achieve this, *BrlAPI* uses a TCP-based connection, on which packets
are transmitted this way:

- the size in bytes of the packet is transmitted first as an integer,
- then the type of the packet, as an integer,
- and finally the packet data.

The size does not include the { size, type } header, so that packets which
don't need any data have a size of 0 byte. The type of the packet can be
either of ``BRLAPI_PACKET_*`` constants defined in ``api_protocol.h``. Each type of
packet will be further discussed below.

*BrlAPI*'s library ships two functions to achieve packets sending and receiving
using this protocol: ``brlapi_writePacket`` and ``brlapi_readPacket``. It
is a good idea to use these functions rather than rewriting them, since this protocol
might change one day in favor of a real reliable packet transmission protocol
such as the experimental RDP.

Responses from the server
-------------------------

As described below, many packets are 'acknowledged'. It means that upon
reception, the server sends either:

- a ``BRLAPI_PACKET_ACK`` packet, with no data, which means the operation
  corresponding to the received packet was successful,
- or a ``BRLAPI_PACKET_ERROR`` packet, the data being an integer
  which should be one of ``BRLAPI_ERROR_*`` constants. This
  means the operation corresponding to the received packet failed.

Some other packets need some information as a response.
Upon reception, the server will send either:

- a packet of the same type, its data being the response,
- or a ``BRLAPI_PACKET_ERROR`` packet.

If at some point an ill-formed or non-sense packet is received by the server,
and ``BRLAPI_PACKET_EXCEPTION`` is returned, holding the guilty packet for
further analysis.

Operating modes
---------------

The connection between the client and the server can be in either of the
four following modes:

- authorization mode: this is the initial mode, when the client hasn't
  got the authorization to use the server yet. The server first sends a
  ``BRLAPI_PACKET_VERSION`` packet that announces the server version. The client
  must send back a ``BRLAPI_PACKET_VERSION`` for announcing its own version too.
  The server then sends a ``BRLAPI_PACKET_AUTH`` packet that announces
  which authorization methods are allowed. The client can then send
  ``BRLAPI_PACKET_AUTH`` packets, which makes the connection enter normal mode.
  If no authorization is needed, the server can announce the ``NONE`` method, the
  client then doesn't need to send a ``BRLAPI_PACKET_AUTH`` packet.

- normal mode: the client is authorized to use the server, but didn't ask for a tty
  or raw mode. The client can send either of these types of packet:

  - ``BRLAPI_PACKET_GETDRIVERNAME``
    or ``BRLAPI_PACKET_GETDISPLAYSIZE`` to get pieces of information from the server,
  - ``BRLAPI_PACKET_ENTERTTYMODE`` to enter tty handling mode,
  - ``BRLAPI_PACKET_ENTERRAWMODE`` to enter raw mode,

- tty handling mode: the client holds the control of a tty: *brltty* has
  no power on it any more, masked keys excepted. It's up to the client to manage
  display and keypresses. For this, it can send either of these types of packet:

  - ``BRLAPI_PACKET_LEAVETTYMODE`` to leave tty handling mode and go back to
    normal mode,
  - ``BRLAPI_PACKET_IGNOREKEYRANGE`` and ``BRLAPI_PACKET_ACCEPTKEYRANGE`` to mask and unmask keys,
  - ``BRLAPI_PACKET_WRITE`` to display text on this tty,
  - ``BRLAPI_PACKET_ENTERRAWMODE`` to enter raw mode,
  - ``BRLAPI_PACKET_GETDRIVERNAME``
    or ``BRLAPI_PACKET_GETDISPLAYSIZE`` to get pieces of information from the server,

  And the server might send ``BRLAPI_PACKET_KEY`` packets to signal key presses.

- raw mode: the client wants to exchange packets directly with the braille
  terminal. Only these types of packet will be accepted.

  - ``BRLAPI_PACKET_LEAVERAWMODE`` to get back to previous mode, either normal or
    tty handling mode.
  - ``BRLAPI_PACKET_PACKET`` to send a packet to the braille terminal.

  And the server might send ``BRLAPI_PACKET_PACKET`` packets to give received packets
  from the terminal to the client.

- suspend mode: the client wants to completely drive the braille terminal.
  The device driver is hence kept closed. No type of packet is allowed except
  ``BRLAPI_PACKET_RESUME``

Termination of the connection is initiated by the client in normal mode by
simply closing its side of the socket. The server will then close the
connection.


Details for each type of packet
-------------------------------

Here is described the semantics of each type of packet. Most of them are
directly linked to some of *BrlAPI*'s library's functions. Reading their
online manual page as well will hence be of good help for understanding.

``BRLAPI_PACKET_VERSION``
~~~~~~~~~~~~~~~~~~~~~~~~~~

This must be the first packet ever transmitted from the server to the client and
from the client to the server. The server sends one first for letting the client
know its protocol version. Data is an integer indicating the protocol version.

Then client must then respond the same way for giving its
version.  If the protocol version can't be handled by the server, a
``BRLAPI_ERROR_PROTOCOL_VERSION`` error packet is returned and the connection
is closed.

``BRLAPI_PACKET_AUTH``
~~~~~~~~~~~~~~~~~~~~~~~

This must be the second packet ever transmitted from the server to the client
and from the client to the server. The server sends one first for letting the
client know which authorization methods are available.  Data is the allowed
authorization types, as integers.

If the ``NONE`` method is not announced by the server, the client can then try
to get authorized by sending packets whose data is the type of authorization
that is tried (as an integer), and eventually some data (if the authorization
type needs it).

If the authorization is successful, the server acknowledges the packet, and
other types of packets might be used, other ``BRLAPI_PACKET_AUTH`` shouldn't be
sent by the client.

If the authorization is not successful, the server sends a
``BRLAPI_ERROR_AUTHENTICATION`` error, and the client can try another
authorization method.

Authorization methods are as follow:

- ``NONE``: the client doesn't need to send an authorization packet.
- ``KEY``: data holds a secret key, the authorization is successful only
  if the key matches the server secret key.
- ``CREDENTIALS``: Operating-System-specific credentials are explicitly
  sent over the socket, the authorization is successful if the server considers
  the credentials sufficient.

Note: when the Operating system permits it, the server may use implicit
credential check, and then advertise the ``none`` method.

``BRLAPI_PACKET_GETDRIVERNAME`` (see *brlapi_getDriverName()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This should be sent by the client when it needs the full name of
the current ``brltty`` driver. The returned string is \\0 terminated.

``BRLAPI_PACKET_GETMODELID`` (see *brlapi_getModelIdentifier()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This should be sent by the client when it needs to identify
which model of braille display is currently used by ``brltty``.
The returned string is \\0 terminated.

``BRLAPI_PACKET_GETDISPLAYSIZE`` (see *brlapi_getDisplaySize()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This should be sent by the client when it needs to know the braille display
size. The returned data are two integers: width and then height.

``BRLAPI_PACKET_ENTERTTYMODE`` (see *brlapi_enterTtyMode()* and *brlapi_enterTtyModeWithPath()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This should be sent by the client to get control of a tty. Sent data are
first a series of integers: the first one gives the number of following
integers, which are the numbers of ttys that leads to the tty that
the application wants to take control of (it can be empty if the tty is
one of the machine's VT). The last integer of this series tells the number of
the tty to get control of. Finally, how key presses should be reported is sent:
either a driver name or "", preceded by the number of characters in the driver
name (0 in the case of ""), as an unsigned byte. This packet is then
acknowledged by the server.

``BRLAPI_PACKET_KEY`` (see *brlapi_readKey()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As soon as the client gets a tty, it must be prepared to handle
``BRLAPI_PACKET_KEY`` incoming packets
at any time (as soon as the key
was pressed on the braille terminal, hopefully).
The data holds a key code as 2 integers, or
the key flags then the command code
as 2 integers, depending on what has been request in the
``BRLAPI_PACKET_ENTERTTYMODE`` packet.

``BRLAPI_PACKET_SETFOCUS`` (see *brlapi_setFocus()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For the server to know which tty is active, one particular client is responsible
for sending ``BRLAPI_PACKET_SETFOCUS`` packets. They hold a single integer telling
the new current tty. For instance, when running an X server on VT 7, the
``xbrlapi`` client would have sent a ``BRLAPI_PACKET_ENTERTTYMODE(7)`` and will send
window IDs whenever X focus changes, allowing display and keypresses switching
between xterms.

``BRLAPI_PACKET_LEAVETTYMODE`` (see *brlapi_leaveTtyMode()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This should be sent to free the tty and masked keys lists.
This is acknowledged by the server.

``BRLAPI_PACKET_IGNOREKEYRANGE`` and ``BRLAPI_PACKET_ACCEPTKEYRANGE`` (see *brlapi_ignoreKeyRange()* and *brlapi_acceptKeyRange()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the client doesn't want every key press to be signaled to it, but some of
them to be given to ``brltty`` for normal processing, it can send
``BRLAPI_PACKET_IGNOREKEYRANGE`` packets to
tell ranges of key codes which shouldn't be
sent to it, but given to ``brltty``, and ``BRLAPI_PACKET_ACCEPTKEYRANGE``
packets to tell ranges
of key codes which should be sent to it, and not given to
``brltty``. The server keeps a dynamic list of ranges, so that arbitrary
sequences of such packets can be sent.
A range is composed of 2 keycodes: the "first" and the "last" boundaries.
Each keycode is composed of 2 integers: the key flags then the command code.
The range expressed by these two keycodes is the set of keycodes whose command
codes are between the command code of the "first" keycode and the "last" keycode
(inclusive), and whose flags contain at least the flags of the "first" keycode
and at most the flags of the "last" keycode. Setting the "first" and "last"
keycode to the same value express only one keycode, for instance. Setting the
first and last keycode to the same command code but setting no flags in the
"first" keycode and setting one flag in the "last" keycode expresses only two
keycode, with the same command code and no flags set except possibly the flag
that is set in the "last" keycode. Setting one flag *i* in the "first"
keycode and setting the same flag plus another flag *j* in the "last" keycode
expresses that the keycodes in the range have flag *i* set and possibly flag
*j* set, but no other flag. Several such ranges can be provided one after the
other.

``BRLAPI_PACKET_WRITE`` (see *brlapi_write()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To display text on the braille terminal and set the position of the cursor,
the client can send a ``BRLAPI_PACKET_WRITE`` packet. The packet begins
with an integer holding flags (see ``BRLAPI_WF_*``). These flags indicate
which data will then be available, in the following order (corresponding to
flag weight):

- A display number can be given as a integer, in case the braille
  display has several. If not given, usual display is used.
- A region must be given as two integers indicating the beginning and the
  number of characters of the part of the braille display which is to be updated,
  the first cell of the display being numbered 1. For braille displays that have
  several lines, the first cell of the second line of the display is numbered the
  length of lines plus one, etc., in other words the display is handled as the
  concatenation of the lines of the display.
  If the number is negative, its absolute value is taken into account, and the
  update is padded or truncated to fill the rest of the display.
- The text to display can then be given, preceded by its size in bytes
  expressed as an integer. It will erase the corresponding region in the AND and
  OR fields. If the region size is positive, the text's length in characters must exactly match the region
  size. For multibyte text, this is the number of wide characters. Notably,
  combining and double-width characters count for 1.
- Then an AND field can be given, one byte per character: the 8-dot
  representation of the above text will be AND-ed with this field, hence allowing
  to erase some unwanted parts of characters. Dots are coded as described in
  ISO/TR 11548-1: dot 1 is set iff bit 0 is set, dot 2 is set iff bit 1 is set,
  ...  dot *i+1* is set if bit *i* is set. This also corresponds to the
  low-order byte of the coding of unicode's braille row ``U+2800``.
- As well, an OR field may be given, one byte per character: the 8-dot
  result of the AND operation above (or the 8-dot representation of the text if
  no AND operation was performed) is OR-ed with this field, hence allowing
  to set some dots, to underline characters for instance.
- A cursor position can be specified. 1 representing
  the first character of the display, 0 turning the cursor off. If not given,
  the cursor (if any) is left unmodified.
- Last but not least, the charset of the text can be specified: the length
  of the name first in one byte, then the name itself in ASCII characters. If the
  charset is not specified, an 8-bit charset is assumed, and it is assumed to be
  the same as the server's. Multibyte charsets may be used, AND and OR fields'
  bytes will correspond to each text's wide *character*, be it a combining or a
  double-width character.

A ``BRLAPI_PACKET_WRITE`` packet without any flag (and hence no data) means a
"void" WRITE: the server clears the output buffer for this connection.

``BRLAPI_PACKET_ENTERRAWMODE`` (see *brlapi_enterRawMode()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enter raw mode, the client must send a ``BRLAPI_PACKET_ENTERRAWMODE`` packet,
which is acknowledged. Once in raw mode, no other packet than
``BRLAPI_PACKET_LEAVERAWMODE`` or ``BRLAPI_PACKET_PACKET`` will be accepted.
The data must hold the special value ``BRLAPI_DEVICE_MAGIC``: ``0xdeadbeef``, then
the name of the driver (one byte for the length, then the name) to avoid
erroneous raw mode activating.

``BRLAPI_PACKET_LEAVERAWMODE`` (see *brlapi_leaveRawMode()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To leave raw mode, the client must send a ``BRLAPI_PACKET_LEAVERAWMODE`` packet, which
is acknowledged.

``BRLAPI_PACKET_PACKET`` (see *brlapi_sendRaw()* and *brlapi_recvRaw()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While in raw mode, only ``BRLAPI_PACKET_PACKET`` packets can be exchanged between
the client and the server: to send a packet to the braille terminal, the
client merely sends a ``BRLAPI_PACKET_PACKET`` packet, its data being the packet to
send to the terminal. Whenever its receives a packet from the terminal, the
server does exactly the same, so that packet exchanges between the terminal and
the server are exactly reproduced between the server and the client.

``BRLAPI_PACKET_SUSPENDDRIVER`` (see *brlapi_suspendDriver()*)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enter suspend mode, the client must send a ``BRLAPI_PACKET_SUSPEND`` packet,
which is acknowledge. Once in suspend mode, no other packet than
``BRLAPI_PACKET_RESUME`` will be accepted.
The data must hold the special value ``BRLAPI_DEVICE_MAGIC``: ``0xdeadbeef``,
then the name of the driver (one byte for the length, then the name) to avoid
erroneous raw mode activating.

``BRLAPI_PACKET_PARAM_REQUEST``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This packet is sent by the client to request values of parameters. The packet
begins with an integer which holds flags (see ``BRLAPI_PARAMF_*``) which
describe which, how, and when the value should be returned by the server:

- When the ``BRLAPI_PARAMF_GLOBAL`` flag is set, the server will
  return/subscribe the global value instead of the local value.
- When the ``BRLAPI_PARAMF_GET`` flag is set, the server acknowledges the
  request by returning the latest value with a ``BRLAPI_PACKET_PARAM_VALUE``
  packet. Otherwise the server acknowledges the request with a
  ``BRLAPI_PACKET_ACK`` packet, without providing the value.
- When the ``BRLAPI_PARAMF_SUBSCRIBE`` flag is set, the server will keep
  sending asynchronously the value of the parameter whenever it changes, with
  ``BRLAPI_PACKET_PARAM_UPDATE`` packets, until
  another request packet has the ``BRLAPI_PARAMF_UNSUBSCRIBE`` flag set for this
  parameter.
- When the ``BRLAPI_PARAMF_SELF`` flag is set along
  ``BRLAPI_PARAMF_SUBSCRIBE``, the server will send the value of the parameter
  when it is changed even by the client itself.
- When the ``BRLAPI_PARAMF_UNSUBSCRIBE`` flag is set, the server
  will stop sending asynchronously the value of the parameter with
  ``BRLAPI_PACKET_PARAM_UPDATE`` packets.

It does not make sense to set both the ``BRLAPI_PARAMF_SUBSCRIBE`` and
``BRLAPI_PARAMF_UNSUBSCRIBE`` flags.

Then an integer representing the parameter to be requested.
Then two integers that form (in big-endian order) a 64bit value used to
subspecify the precise parameter to be requested (e.g. a keycode number).

If several ``BRLAPI_PARAMF_SUBSCRIBE`` packets are sent by the client, as
many ``BRLAPI_PARAMF_UNSUBSCRIBE`` packets have to be sent by the client before
the server stops sending ``BRLAPI_PACKET_PARAM_UPDATE`` packets.

``BRLAPI_PACKET_PARAM_VALUE``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This packet is sent by the client or the server to provide the value of a
parameter. The packet begins with an integer which holds flags (see
``BRLAPI_PVF_*``) which describe which value is being transmitted:

- When the ``BRLAPI_PVF_GLOBAL`` flag is set, the value is the global value
  instead of the local value.

Then an integer representing the parameter being transmitted. Then two integers
that form (in big-endian order) a 64bit value used to subspecify the precise
parameter being transmitted (e.g. a keycode number).  Eventually, the packet
contains the value.

When the packet is sent by the client, it defines the new value of the
parameter, and if it is a global value, the server broadcasts the new value to
all clients which have subscribed to updates. The packet is then acknowledged by
the server on success. If the value can not be changed, the server returns an
error (e.g. ``BRLAPI_ERROR_READONLY_PARAMETER``).

``BRLAPI_PACKET_PARAM_UPDATE``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This packet is sent asynchronously by the server to provide an update of a
value of a parameter. This is sent only if the client has previously sent a
``BRLAPI_PACKET_PARAM_REQUEST`` packet with the ``BRLAPI_PARAMF_SUBSCRIBE``
for the corresponding parameter.

It is structured exactly like a ``BRLAPI_PACKET_PARAM_VALUE`` packet.

``BRLAPI_PACKET_SYNCHRONIZE``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This packet is sent by the client and just acknowledged by the server. This
allows the client to perform a round-try with the server, thus collecting any
pending exception notification.
