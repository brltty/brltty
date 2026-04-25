
.. _options-log-level:

==============================
BRLTTY Reference Manual
==============================

Access to the Console Screen for Blind Persons using Refreshable Braille Displays

:Authors: Nikhil Nair `<nn201@cus.cam.ac.uk> <mailto:nn201@cus.cam.ac.uk>`__; Nicolas Pitre `<nico@fluxnic.net> <mailto:nico@fluxnic.net>`__; Stéphane Doyon `<s.doyon@videotron.ca> <mailto:s.doyon@videotron.ca>`__; Dave Mielke `<dave@mielke.cc> <mailto:dave@mielke.cc>`__
:Date: Version 6.9.1, April 2026

Copyright (c) 1995-2026 by The BRLTTY Developers. BRLTTY is free software, and comes with ABSOLUTELY NO WARRANTY. It is placed under the terms of version 2 or later of **The GNU General Public License** as published by **The Free Software Foundation**.


Formalities
===========


License
-------

This program is free software.
You may redistribute it and/or modify it under the terms of
`The GNU Lesser General Public License <http://www.gnu.org/licenses/licenses.html#LGPL>`_
as published by `The Free Software Foundation <http://www.gnu.org/fsf/fsf.html>`_.
Version 2.1 (or any later version) of the license may be used.

You should have received a copy of the license along with this program.
It should be in the file ``LICENSE-LGPL`` in the top-level directory.
If not, write to the Free Software Foundation Inc.,
675 Mass Ave, Cambridge, MA 02139, USA.


Disclaimer
----------

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY - not even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the **GNU General Public License** for more details.


.. _contact:

.. _command-INFO:

Contact Information
-------------------

BRLTTY represents the work of a team.
For up-to-date information, see BRLTTY's web page at
[`http://brltty.app/ <http://brltty.app/>`_].
As of this writing, the team includes:

- Dave Mielke (maintainer, active)

  Web
    `http://mielke.cc/ <http://mielke.cc/>`_

  E-Mail
    `<dave@mielke.cc> <mailto:dave@mielke.cc>`_

- Samuel Thibault (active)

  Web
    `http://dept-info.labri.fr/~thibault/ <http://dept-info.labri.fr/~thibault/>`_

  E-Mail
    `<samuel.thibault@ens-lyon.org> <mailto:samuel.thibault@ens-lyon.org>`_

- Mario Lang (active)

  Web
    `http://delysid.org/ <http://delysid.org/>`_

  E-Mail
    `<mlang@tugraz.at> <mailto:mlang@tugraz.at>`_

- Nicolas Pitre

  Web
    `http://www.fluxnic.net/ <http://www.fluxnic.net/>`_

  E-Mail
    `<nico@fluxnic.net> <mailto:nico@fluxnic.net>`_

- Stéphane Doyon

  Web
    `http://pages.infinit.net/sdoyon/ <http://pages.infinit.net/sdoyon/>`_

  E-Mail
    `<s.doyon@videotron.ca> <mailto:s.doyon@videotron.ca>`_

- Nikhil Nair  (author)

  E-Mail
    `<nn201@cus.cam.ac.uk> <mailto:nn201@cus.cam.ac.uk>`_


Questions, comments, suggestions, criticisms, and contributions are all welcome.
Even though our e-mail addresses are listed above,
the best way to contact us is via BRLTTY's mailing list.
You can post to the list by sending e-mail to
`<BRLTTY@brltty.app> <mailto:BRLTTY@brltty.app>`_.
If you aren't subscribed to the list
then your posts will be held for moderator approval.
To subscribe, unsubscribe, change settings, view archives, etc,
go to the list's information page at
`http://brltty.app/mailman/listinfo/brltty <http://brltty.app/mailman/listinfo/brltty>`_.


Introduction
============

BRLTTY gives a braille user access to the text consoles of a Linux/Unix system.
It runs as a background process (daemon)
which operates a refreshable braille display,
and can be started very early in the system boot sequence.
It enables a braille user, therefore, to easily independently handle
aspects of system administration such as
single user mode entry,
file system recovery,
and boot problem analysis.
It even greatly eases such routine tasks as logging in.

BRLTTY reproduces a rectangular portion of the screen
(referred to within this document as 'the window')
as braille text on the display.
Controls on the display can be used to
move the window around on the screen,
to enable and disable various viewing options,
and to perform special functions.


Feature Summary
---------------

BRLTTY provides the following capabilities:

- Full implementation of the usual screen review facilities.
- Choice between ``block``, ``underline``, or ``no`` cursor.
- Optional ``underline`` to indicate specially highlighted text.
- Optional use of ``blinking`` (rates individually settable) for cursor, special highlighting underline, and/or capital letters.
- Screen freezing for leisurely review.
- Intelligent cursor routing, allowing easy fetching of cursor within text editors, web browsers, etc., without moving ones hands from the braille display.

.. _command-PASTE:

- A cut-and-paste function (linear or rectangular) which is particularly useful for copying long file names, copying text between virtual terminals, entering complicated commands, etc.
- Table driven in-line contracted braille (English and French provided).
- Support for multiple braille codes.
- Ability to identify an unknown character.
- Ability to inspect character highlighting.
- An on-line help facility for braille display commands.
- A preferences menu.
- Basic speech support.
- Modular design allowing relatively easy addition of drivers for other braille displays and speech synthesizers.
- An Application Programming Interface.



Starting BRLTTY
===============

Once BRLTTY is installed, run it as ``brltty``. It reads its configuration
from a file
(see :ref:`The Configuration File <configure>`)
which sets defaults such as the braille driver, the device to which the
display is connected, and the text table. Most of those defaults can be
overridden on the command line
(see :ref:`Command Line Options <options>`).

A few options are particularly handy right after install:

* :ref:`-h <options-help>` lists every option.
* :ref:`-V <options-version>` prints BRLTTY's version, the API version,
  and the selected drivers.
* :ref:`-v <options-verify>` shows the option values BRLTTY will actually
  run with after combining the configuration file, the environment, and
  the command line.

You almost certainly want BRLTTY started automatically at boot so that
your display is alive by the time the login prompt appears. On modern
Linux distributions BRLTTY ships as a packaged systemd service — install
your distribution's ``brltty`` package and enable the unit. See
``Documents/README.Systemd`` for the unit names, the difference between
the default instance and per-device instances, and how Udev hot-plug
support is wired up so that connecting a USB display starts BRLTTY
automatically.



.. _utility-brltty-config:

The brltty-config Utility
=========================

A shell script that exposes BRLTTY's installation-time directory layout to
other scripts. Source it (don't execute it) from a Bourne-compatible
shell::

  . brltty-config

The script sets environment variables reflecting the runtime layout chosen
at build time: ``BRLTTY_VERSION``, ``BRLTTY_EXECUTE_ROOT``,
``BRLTTY_PROGRAM_DIRECTORY``, ``BRLTTY_LIBRARY_DIRECTORY``,
``BRLTTY_WRITABLE_DIRECTORY``, ``BRLTTY_DATA_DIRECTORY``,
``BRLTTY_MANPAGE_DIRECTORY``, ``BRLAPI_VERSION``, and others. Run it once
and inspect the environment for the full list.


Using BRLTTY
============

Before starting BRLTTY, you need to set up your braille display.
In most cases this is done simply
by connecting it to an available serial port,
and then turning it on.
After your display has been set up,
run BRLTTY simply by typing the command ``brltty`` at a shell prompt
(this must be done as root).
Check the
:ref:`-d <options-braille-device>` command line option,
the :ref:`braille-device <configure-braille-device>` configuration file directive,
and the ``--with-braille-device`` build option
for alternatives regarding how to tell BRLTTY which device your display is connected to.
Check the
:ref:`-b <options-braille-driver>` command line option,
the :ref:`braille-driver <configure-braille-driver>` configuration file directive,
and the ``--with-braille-driver`` build option
for alternatives regarding how to tell BRLTTY which kind of braille display you have.
Check the
:ref:`-B <options-braille-parameters>` command line option,
and the :ref:`braille-parameters <configure-braille-parameters>` configuration file directive
for alternatives regarding how to pass parameters to the driver for your braille display.

A message giving the program name (BRLTTY) and its version number
will appear briefly
(see the :ref:`-M <options-message-timeout>` command line option)
on the braille display.
The display will then show a small area of the screen including the cursor.
By default, the cursor is represented as
dots 7 and 8 superimposed on the character it is on.

Any screen activity will be reflected on the braille display.
The display will also follow the progress of the cursor on the screen.
This feature is known as **cursor tracking**.

Just typing on the keyboard and reading the display, however, isn't enough.
Try entering a command which will cause an error, and pressing **enter**.
The error appears on the screen, but, unless you have a multi-line display,
the chances are that it isn't visible on the braille display.
All you see thereon is another shell prompt.
What's needed, then, is some way to move
the braille *window* around the screen.
The keys on the braille display itself can be used to send commands to BRLTTY
which, in addition to a lot of other things, can also do exactly that.


.. _commands:

Commands
--------

Unfortunately, the various braille displays don't offer a standard set of controls.
Some have the six standard dot keys, some have eight, and others have none.
Some have thumb keys, but there's no standard number of them.
Some have a button above each braille cell.
Some have rocker switches.
Some have an easy-to-reach bar which works much like a joystick.
Most have varying combinations of the above.
Because the nature and layout of each display is so different,
please refer to the documentation for your particular display
in order to find out exactly what its keys do.

BRLTTY commands are referred to by name within this manual.
If you forget which key(s) on your braille display to use
for a particular command, then refer to its driver's help page.
The main key you should immediately commit to memory, therefore, is the one
for the :ref:`HELP <command-HELP>` command.
Use the regular motion keys (as described below) to navigate the help page,
and press the ``help`` key again to quit.


.. _vertical-motion:

Vertical Motion
~~~~~~~~~~~~~~~

See also
the :ref:`PRINDENT/NXINDENT <command-PRINDENT-NXINDENT>`,
and the :ref:`PRDIFCHAR/NXDIFCHAR <command-PRDIFCHAR-NXDIFCHAR>`
routing key commands.

.. _command-LNUP-LNDN:

LNUP/LNDN
  Go up/down one line. If identical line skipping has been activated (see the :ref:`SKPIDLNS <command-SKPIDLNS>` command), then these commands, rather than moving exactly one line, are aliases for the :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>` commands.

.. _command-WINUP-WINDN:

WINUP/WINDN
  Go up/down one window. If the window is only 1 line high then move 5 lines.

.. _command-PRDIFLN-NXDIFLN:

PRDIFLN/NXDIFLN
  Go up/down to the nearest line with different content. If identical line skipping has been activated (see the :ref:`SKPIDLNS <command-SKPIDLNS>` command), then these commands, rather than skipping identical lines, are aliases for the :ref:`LNUP/LNDN <command-LNUP-LNDN>` commands.

.. _command-ATTRUP-ATTRDN:

ATTRUP/ATTRDN
  Go up/down to the nearest line with different attributes (character highlighting).

.. _command-TOP-BOT:

TOP/BOT
  Go to the top/bottom line.

.. _command-TOP_LEFT-BOT_LEFT:

TOP_LEFT/BOT_LEFT
  Go to the top-left/bottom-left corner.

.. _command-PRPGRPH-NXPGRPH:

PRPGRPH/NXPGRPH
  Go to the nearest line of the previous/next paragraph (the first non-blank line beyond the nearest blank line). The current line is included when searching for the inter-paragraph space.

.. _command-PRPROMPT-NXPROMPT:

PRPROMPT/NXPROMPT
  Go to the previous/next command prompt.

.. _command-PRSEARCH-NXSEARCH:

PRSEARCH/NXSEARCH
  Search backward/forward for the nearest occurrence of the character string within the cut buffer (see :ref:`Cut and Paste <cut>`) which isn't within the braille window. The search proceeds to the left/right, starting at the character immediately to the left/right of the window, and wrapping at the edge of the screen. The search isn't case sensitive.


.. _horizontal-motion:

Horizontal Motion
~~~~~~~~~~~~~~~~~

.. _command-SETLEFT:

See also the :ref:`SETLEFT <command-SETLEFT>` routing key command.

.. _command-CHRLT-CHRRT:

CHRLT/CHRRT
  Go left/right one character.

.. _command-HWINLT-HWINRT:

HWINLT/HWINRT
  Go left/right half a window.

.. _command-FWINLT-FWINRT:

FWINLT/FWINRT
  Go left/right one window. These commands are particularly useful because they automatically wrap when they reach the edge of the screen. Other features, like their ability to skip blank windows (see the :ref:`SKPBLNKWINS <command-SKPBLNKWINS>` command), further enhance their usefulness.

.. _command-FWINLTSKIP-FWINRTSKIP:

FWINLTSKIP/FWINRTSKIP
  Go left/right to the nearest non-blank window.

.. _command-LNBEG-LNEND:

LNBEG/LNEND
  Go to the beginning/end of the line.


.. _implicit-motion:

Implicit Motion
~~~~~~~~~~~~~~~

.. _command-GOTOMARK:

See also the :ref:`GOTOMARK <command-GOTOMARK>` routing key command.

.. _command-BACK:

.. _command-HOME:

HOME
  Go to where the cursor is.

BACK
  Go back to where the most recent motion command put the braille window. This is an easy way to get right back to where you were reading after an unexpected event (like cursor tracking) moves the braille window at an inopportune moment.

.. _command-RETURN:

RETURN

  - If the most recent motion of the braille window was automatic, e.g. as a result of cursor tracking, then go back to where the most recent motion command put it (see the :ref:`BACK <command-BACK>` command).
  - If the cursor isn't within the braille window then go to where the cursor is (see the :ref:`HOME <command-HOME>` command).


.. _feature-activation:

Feature Activation
~~~~~~~~~~~~~~~~~~

Each of these commands has three forms:
**activate** (turn the feature on),
**deactivate** (turn the feature off),
and **toggle** (if it's off then turn it on, and if it's on then turn it off).
Unless specifically noted, each of these features
is initially **off**, and, when **on**, affects BRLTTY's operation as a whole.
The initial setting of some of these features can be changed
via the :ref:`preferences menu <preferences-menu>`.

.. _command-FREEZE:

FREEZE
  Freeze the screen image. BRLTTY makes a copy of the screen (content and attributes) as of the moment when the screen image is frozen, and then ignores all updating of the screen until it's unfrozen. This feature makes it easy, for example, to sample the output of an application which writes too much too quickly.

.. _command-DISPMD:

DISPMD
  Show the highlighting (the attributes) of each character within the braille window, rather than the characters themselves (the content). This feature is useful, for example, when you need to locate a highlighted item. When showing screen content, the text table is used (see the :ref:`-t <options-text-table>` command line option, the :ref:`text-table <configure-text-table>` configuration file directive, and the ``--with-text-table`` build option). When showing screen attributes, the attributes table is used (see the :ref:`-a <options-attributes-table>` command line option, the :ref:`attributes-table <configure-attributes-table>` configuration file directive, and the ``--with-attributes-table`` build option). This feature only affects the current virtual terminal.

.. _command-SIXDOTS:

SIXDOTS
  Show characters using 6-dot, rather than 8-dot, braille. Dots 7 and 8 are still used by other features like cursor representation and highlighted character underlining. If a contraction table has been selected (see the :ref:`-c <options-contraction-table>` command line option and the :ref:`contraction-table <configure-contraction-table>` configuration file directive), then it is used. This setting can also be changed with the :ref:`Text Style <preference-text-style>` preference.

.. _command-SLIDEWIN:

SLIDEWIN
  If cursor tracking (see the :ref:`CSRTRK <command-CSRTRK>` command) is **on**, then, whenever the cursor moves too close to (or beyond) either end of the braille window, horizontally reposition the window such that the cursor, while remaining on that side, is nearer the centre. If this feature is **off**, then the braille window is always positioned such that its left end is a multiple of its width from the left edge of the screen. This setting can also be changed with the :ref:`Sliding Window <preference-sliding-window>` preference.

.. _command-SKPIDLNS:

SKPIDLNS
  Rather than explicitly moving exactly one line either up or down, skip past lines which have the same content as the current line. This feature affects the :ref:`LNUP/LNDN <command-LNUP-LNDN>` commands, as well as the line wrapping feature of the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` and :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>` commands. This setting can also be changed with the :ref:`Skip Identical Lines <preference-skip-identical-lines>` preference.

.. _command-SKPBLNKWINS:

SKPBLNKWINS
  Skip past blank windows when reading either forward or backward. This feature affects the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` commands. This setting can also be changed with the Skip Blank Windows preference.

.. _command-CSRVIS:

CSRVIS
  Show the cursor by superimposing a dot pattern (see the :ref:`CSRSIZE <command-CSRSIZE>` command) on top of the character where it is. This feature is initially **on**. This setting can also be changed with the :ref:`Show Cursor <preference-show-cursor>` preference.

.. _command-CSRHIDE:

CSRHIDE
  Hide the cursor (see the :ref:`CSRVIS <command-CSRVIS>` command) in order to accurately read the character beneath it. This feature only affects the current virtual terminal.

.. _command-CSRTRK:

CSRTRK
  Track (follow) the cursor. If the cursor moves to a location which isn't within the braille window, then automatically move the braille window to the cursor's new location. You'll usually want this feature turned on since it minimizes the effects of screen scrolling, and since, during input, the region wherein you're currently typing is always visible. If this feature causes the braille window to jump at an inopportune moment, then use the :ref:`BACK <command-BACK>` command to get back to where you were reading. You may need to turn this feature off when using an application which continually updates the screen while maintaining a fixed data layout. This feature is initially **on**. This feature only affects the current virtual terminal.

.. _command-CSRSIZE:

CSRSIZE
  Represent the cursor with all eight dots (a solid block), rather than with just dots 7 and 8 (an underline). This setting can also be changed with the :ref:`Cursor Style <preference-cursor-style>` preference.

.. _command-CSRBLINK:

CSRBLINK
  Blink (turn on and off according to a predefined interval) the symbol representing the cursor (see the :ref:`CSRVIS <command-CSRVIS>` command). This setting can also be changed with the :ref:`Blinking Cursor <preference-blinking-cursor>` preference.

.. _command-ATTRVIS:

ATTRVIS
  Underline (with combinations of dots 7 and 8) highlighted characters.

  no underline
    White on black (normal), gray on black, white on blue, black on cyan.

  dots 7 and 8
    Black on white (reverse video).

  dot 8
    Everything else.

  This setting can also be changed with the :ref:`Show Attributes <preference-show-attributes>` preference.

.. _command-ATTRBLINK:

ATTRBLINK
  Blink (turn on and off according to a predefined interval) the attribute underline (see the :ref:`ATTRVIS <command-ATTRVIS>` command). This feature is initially **on**. This setting can also be changed with the :ref:`Blinking Attributes <preference-blinking-attributes>` preference.

.. _command-CAPBLINK:

CAPBLINK
  Blink (turn on and off according to a predefined interval) capital (uppercase) letters. This setting can also be changed with the :ref:`Blinking Capitals <preference-blinking-capitals>` preference.

TUNES
  Play a short predefined tune (see :ref:`Alert Tunes <tunes>`) whenever a significant event occurs. This feature is initially **on**. This setting can also be changed with the :ref:`Alert Tunes <tunes>` preference.

.. _preference-autorepeat:

.. _command-AUTOREPEAT:

AUTOREPEAT
  Automatically repeat a command at a regular interval after an initial delay while its key (combination) remains pressed. Only some drivers support this functionality, the primary limitation being that many braille displays don't signal key presses and key releases as distinctly separate events. This feature is initially **on**. This setting can also be changed with the :ref:`Autorepeat <preference-autorepeat>` preference.

.. _command-AUTOSPEAK:

.. _preference-autospeak:

AUTOSPEAK
  Automatically speak:

  - the new line when the braille window is moved vertically.
  - characters which are entered or deleted.
  - the character to which the cursor is moved.

  This feature is initially **off**. This setting can also be changed with the :ref:`Autospeak <preference-autospeak>` preference.


.. _mode-selection:

Mode Selection
~~~~~~~~~~~~~~

.. _command-HELP:

HELP
  Switch to the braille display driver's help page. This is where you can find an on-line summary of things like what your braille display's keys do, and how to interpret its status cells. Use the regular :ref:`vertical <vertical-motion>` and :ref:`horizontal <horizontal-motion>` motion commands to navigate the help page. Invoke the ``help`` command again to return to the screen.

INFO
  Switch to the status display (see section :ref:`The Status Display <status>` for full details). It presents a summary including the position of the cursor, the position of the braille window, and the states of a number of BRLTTY's features. Invoke this command again to return to the screen.

LEARN
  Switch to command learn mode (see section :ref:`Command Learn Mode <learn>` for full details). This is how you can interactively learn what your braille display's keys do. Invoke this command again to return to the screen. This command isn't available if the --disable-learn-mode build option was specified.


.. _preference-maintenance:

Preference Maintenance
~~~~~~~~~~~~~~~~~~~~~~

.. _command-PREFMENU:

PREFMENU
  Switch to the preferences menu (see :ref:`The Preferences Menu <preferences-menu>` for full details). Invoke this command again to return to normal operation.

.. _command-PREFSAVE:

PREFSAVE
  Save the current preferences settings (see :ref:`Preferences <preferences>` for full details).

.. _command-PREFLOAD:

PREFLOAD
  Restore the most recently saved preferences settings (see :ref:`Preferences <preferences>` for full details).


.. _menu-navigation:

Menu Navigation
~~~~~~~~~~~~~~~

.. _command-MENU_FIRST_ITEM-MENU_LAST_ITEM:

MENU_FIRST_ITEM/MENU_LAST_ITEM
  Go to the first/last item in the menu.

.. _command-MENU_PREV_ITEM-MENU_NEXT_ITEM:

MENU_PREV_ITEMMENU_NEXT_ITEM/
  Go to the previous/next item in the menu.

.. _command-MENU_PREV_SETTING-MENU_NEXT_SETTING:

MENU_PREV_SETTING/MENU_NEXT_SETTING
  Decrement/increment the current menu item's setting.


.. _speech-controls:

Speech Controls
~~~~~~~~~~~~~~~

.. _command-SAY_LINE:

SAY_LINE
  Speak the current line. The :ref:`Say-Line Mode <preference-sayline-mode>` preference determines if pending speech is discarded first.

.. _command-SAY_ABOVE:

SAY_ABOVE
  Speak the top portion of the screen (ending with the current line).

.. _command-SAY_BELOW:

SAY_BELOW
  Speak the bottom portion of the screen (starting with the current line).

.. _command-MUTE:

MUTE
  Stop speaking immediately.

.. _command-SPKHOME:

SPKHOME
  Go to where the speech cursor is.

.. _command-SAY_SLOWER-SAY_FASTER:

SAY_SLOWER/SAY_FASTER
  Decrease/increase the speech rate (see also the :ref:`Speech Rate <preference-speech-rate>` preference). This command is only available if a driver which supports it is being used.

.. _command-SAY_SOFTER-SAY_LOUDER:

SAY_SOFTER/SAY_LOUDER
  Decrease/increase the speech volume (see also the :ref:`Speech Volume <preference-speech-volume>` preference). This command is only available if a driver which supports it is being used.


.. _speech-navigation:

Speech Navigation
~~~~~~~~~~~~~~~~~

SPEAK_CURR_CHAR
  a


Virtual Terminal Switching
~~~~~~~~~~~~~~~~~~~~~~~~~~

See also the :ref:`SWITCHVT <command-SWITCHVT>` routing key command.

.. _command-SWITCHVT_PREV-SWITCHVT_NEXT:

SWITCHVT_PREV/SWITCHVT_NEXT
  Switch to the previous/next virtual terminal.


Other Commands
~~~~~~~~~~~~~~

.. _command-CSRJMP_VERT:

CSRJMP_VERT
  Route (bring) the cursor to anywhere on the top line of the braille window (see :ref:`Cursor Routing <routing>` for full details). The cursor is moved by simulating vertical arrow-key presses. This command doesn't always work because some applications either move the cursor somewhat unpredictably or use the arrow keys for purposes other than cursor motion. It's somewhat safer than the other cursor routing commands, though, because it makes no attempt to simulate the left- and right-arrows.

PASTE
  Insert the characters within the cut buffer at the current cursor location (see :ref:`Cut and Paste <cut>` for full details).

.. _command-RESTARTBRL:

RESTARTBRL
  Stop, and then restart the braille display driver.

.. _command-RESTARTSPEECH:

RESTARTSPEECH
  Stop, and then restart the speech synthesizer driver.


.. _commands-characters:

Character Commands
~~~~~~~~~~~~~~~~~~

.. _command-ROUTE:

ROUTE
  Route (bring) the cursor to the character associated with the routing key (see :ref:`Cursor Routing <routing>` for full details). The cursor is moved by simulating arrow-key presses. This command doesn't always work because some applications either move the cursor somewhat unpredictably or use the arrow keys for purposes other than cursor motion.

.. _command-CUTBEGIN:

CUTBEGIN
  Anchor the start of the cut block at the character associated with the routing key (see :ref:`Cut and Paste <cut>` for full details). This command clears the cut buffer.

.. _command-CUTAPPEND:

CUTAPPEND
  Anchor the start of the cut block at the character associated with the routing key (see :ref:`Cut and Paste <cut>` for full details). This command doesn't clear the cut buffer.

.. _command-CUTRECT:

CUTRECT
  Anchor the end of the cut block at the character associated with the routing key, and append the rectangular region to the cut buffer (see :ref:`Cut and Paste <cut>` for full details).

.. _command-CUTLINE:

CUTLINE
  Anchor the end of the cut block at the character associated with the routing key, and append the linear region to the cut buffer (see :ref:`Cut and Paste <cut>` for full details).

.. _command-COPYCHARS:

COPYCHARS
  Copy the character block anchored by the two routing keys to the cut buffer (see :ref:`Cut and Paste <cut>` for full details).

.. _command-APNDCHARS:

APNDCHARS
  Append the character block anchored by the two routing keys to the cut buffer (see :ref:`Cut and Paste <cut>` for full details).

.. _command-PRINDENT-NXINDENT:

PRINDENT/NXINDENT
  Go up/down to the nearest line which isn't indented more than the column associated with the routing key.

.. _command-DESCCHAR:

DESCCHAR
  Momentarily (see the :ref:`-M <options-message-timeout>` command line option) display a message describing the character associated with the routing key. It reveals the decimal and hexadecimal values of the character, the foreground and background colours, and, when present, special attributes (``bright`` and ``blink``). The message looks like this:


  .. parsed-literal::

    char 65 (0x41): white on black bright blink

SETLEFT
  Horizontally reposition the braille window so that its left edge is at the column associated with the routing key. This feature makes it very easy to put the window exactly where it's needed, and, therefore, for displays which have routing keys, almost eliminates the need for a lot of elementary window motion (like the :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>` and :ref:`HWINLT/HWINRT <command-HWINLT-HWINRT>` commands).

.. _command-PRDIFCHAR-NXDIFCHAR:

PRDIFCHAR/NXDIFCHAR
  Go up/down to the nearest line which has a different character in the column associated with the routing key.


.. _commands-base:

Base Commands
~~~~~~~~~~~~~

.. _command-SWITCHVT:

SWITCHVT
  Switch to the virtual terminal whose number (counting from 1) matches that of the routing key. See also the :ref:`SWITCHVT_PREV/SWITCHVT_NEXT <command-SWITCHVT_PREV-SWITCHVT_NEXT>` virtual terminal switching commands.

.. _command-SETMARK:

SETMARK
  Mark (remember) the current position of the braille window in a register associated with the routing key. See the :ref:`GOTOMARK <command-GOTOMARK>` command. This feature only affects the current virtual terminal.

GOTOMARK
  Move the braille window to the position formerly marked (see the :ref:`SETMARK <command-SETMARK>` command) with the same routing key. This feature only affects the current virtual terminal.


.. _configure:

The Configuration File
----------------------

System defaults for many settings may be established within a configuration file.
The default name for this file is ``/etc/brltty.conf``,
although it may be overridden with the
:ref:`-f <options-configuration-file>` command line option.
It doesn't need to exist.
A template for it can be found within the ``Documents`` subdirectory.

Blank lines are ignored.
A comment begins with a number sign (``#``),
and continues to the end of the line.
The following directives are recognized:

.. _configure-api-parameters:

``api-parameters`` *name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the Application Programming Interface. If the same parameter is specified more than once, then its rightmost assignment is used. For a description of the parameters accepted by the interface, please see the **BrlAPI** reference manual. See the ``--with-api-parameters`` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-A <options-api-parameters>` command line option.

.. _configure-attributes-table:

``attributes-table`` *file*
  Specify the attributes table (see section :ref:`Attributes Tables <table-attributes>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.atb`` extension is optional. The default is to use the built-in table (see the ``--with-attributes-table`` build option). This directive can be overridden with the :ref:`-a <options-attributes-table>` command line option.

.. _configure-braille-device:

``braille-device`` *device*\ ``,``...
  Specify the device to which the braille display is connected (see section :ref:`Braille Device Specification <operand-braille-device>`). See the ``--with-braille-device`` build option for the default established during the build procedure. This directive can be overridden with the :ref:`-d <options-braille-device>` command line option.

.. _configure-braille-driver:

``braille-driver`` *driver*\ ``,``...|\ ``auto``
  Specify the braille display driver (see section :ref:`Driver Specification <operand-driver>`). The default is to perform autodetection. This directive can be overridden with the :ref:`-b <options-braille-driver>` command line option.

.. _configure-braille-parameters:

``braille-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the braille display drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the ``--with-braille-parameters`` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-B <options-braille-parameters>` command line option.

.. _configure-contraction-table:

``contraction-table`` *file*
  Specify the contraction table (see section :ref:`Contraction Tables <table-contraction>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ctb`` extension is optional. The contraction table is used when the 6-dot braille feature is activated (see the :ref:`SIXDOTS <command-SIXDOTS>` command and the :ref:`Text Style <preference-text-style>` preference). The default is to display uncontracted 6-dot braille. This directive can be overridden with the :ref:`-c <options-contraction-table>` command line option. It isn't available if the ``--disable-contracted-braille`` build option was specified.

.. _configure-keyboard-table:

``keyboard-table`` *file*\ \|\ ``auto``
  Specify the keyboard table (see section :ref:`Key Tables <table-key>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ktb`` extension is optional. The default is not to use a keyboard table. This directive can be overridden with the :ref:`-k <options-keyboard-table>` command line option.

.. _configure-keyboard-properties:

``keyboard-properties`` *name*\ ``=``\ *value*\ ``,``...
  Specify the properties of the keyboard(s) to be monitored. If the same property is specified more than once, then its rightmost assignment is used. See section :ref:`Keyboard Properties <keyboard-properties>` for a list of the properties which may be specified. The default is to monitor all keyboards. This directive can be overridden with the :ref:`-K <options-keyboard-properties>` command line option.

.. _configure-midi-device:

``midi-device`` *device*
  Specify the device to use for the Musical Instrument Digital Interface (see section :ref:`MIDI Device Specification <operand-midi-device>`). This directive can be overridden with the :ref:`-m <options-midi-device>` command line option. It isn't available if the ``--disable-midi-support`` build option was specified.

.. _configure-pcm-device:

``pcm-device`` *device*
  Specify the device to use for digital audio (see section :ref:`PCM Device Specification <operand-pcm-device>`). This directive can be overridden with the :ref:`-p <options-pcm-device>` command line option. It isn't available if the ``--disable-pcm-support`` build option was specified.

.. _configure-preferences-file:

``preferences-file`` *file*
  Specify the location of the file which is to be used for the saving and loading of user preferences. If a relative path is supplied, then it's anchored at ``/var/lib/brltty``. The default is to use ``brltty.prefs``. This directive can be overridden with the :ref:`-F <options-preferences-file>` command line option.

.. _configure-release-device:

``release-device`` *boolean*
  Whether or not to release the device to which the braille display is connected when the current screen or window can't be read.

  on
    Release the device.

  off
    Don't release the device.

  The default setting is ``on`` on Windows platforms and ``off`` on all other platforms. This directive can be overridden with the :ref:`-r <options-release-device>` command line option.

.. _configure-screen-driver:

``screen-driver`` *driver*
  Specify the screen driver (see section :ref:`Supported Screen Drivers <screen>`). See the ``--with-screen-driver`` build option for the default established during the build procedure. This directive can be overridden with the :ref:`-x <options-screen-driver>` command line option.

.. _configure-screen-parameters:

``screen-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the screen drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Supported Screen Drivers <screen>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the ``--with-screen-parameters`` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-X <options-screen-parameters>` command line option.

.. _configure-speech-driver:

``speech-driver`` *driver*\ ``,``...|\ ``auto``
  Specify the speech synthesizer driver (see section :ref:`Driver Specification <operand-driver>`). The default is to perform autodetection. This directive can be overridden with the :ref:`-s <options-speech-driver>` command line option. It isn't available if the ``--disable-speech-support`` build option was specified.

.. _configure-speech-input:

``speech-input`` *name*
  Specify the name of the file system object (FIFO, named pipe, named socket, etc) which can be used by other applications for text-to-speech conversion via BRLTTY's speech driver. This directive can be overridden with the :ref:`-i <options-speech-input>` command line option. It isn't available if the ``--disable-speech-support`` build option was specified.

.. _configure-speech-parameters:

``speech-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the speech synthesizer drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the ``--with-speech-parameters`` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-S <options-speech-parameters>` command line option.

.. _configure-text-table:

``text-table`` *file*\ \|\ ``auto``
  Specify the text table (see section :ref:`Text Tables <table-text>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ttb`` extension is optional. The default is to perform locale-based autoselection, with fallback to the built-in table (see the ``--with-text-table`` build option). This directive can be overridden with the :ref:`-t <options-text-table>` command line option.


.. _options:

Command Line Options
--------------------

Many settings can be explicitly specified when invoking BRLTTY.
The ``brltty`` command accepts the following options:

.. _options-attributes-table:

``-a``\ *file* ``--attributes-table=``\ *file*
  Specify the attributes table (see section :ref:`Attributes Tables <table-attributes>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.atb`` extension is optional. See the :ref:`attributes-table <configure-attributes-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Attributes Table <preference-attributes-table>` preference.

.. _options-braille-driver:

``-b``\ *driver*\ ``,``...|\ ``auto`` ``--braille-driver=``\ *driver*\ ``,``...|\ ``auto``
  Specify the braille display driver (see section :ref:`Driver Specification <operand-driver>`). See the :ref:`braille-driver <configure-braille-driver>` configuration file directive for the default run-time setting.

.. _options-contraction-table:

``-c``\ *file* ``--contraction-table=``\ *file*
  Specify the contraction table (see section :ref:`Contraction Tables <table-contraction>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ctb`` extension is optional. The contraction table is used when the 6-dot braille feature is activated (see the :ref:`SIXDOTS <command-SIXDOTS>` command and the :ref:`Text Style <preference-text-style>` preference). See the :ref:`contraction-table <configure-contraction-table>` configuration file directive for the default run-time setting. This setting can be changed with the Contraction Table preference. This option isn't available if the ``--disable-contracted-braille`` build option was specified.

.. _options-braille-device:

``-d``\ *device*\ ``,``... ``--braille-device=``\ *device*\ ``,``...
  Specify the device to which the braille display is connected (see section :ref:`Braille Device Specification <operand-braille-device>`). See the :ref:`braille-device <configure-braille-device>` configuration file directive for the default run-time setting.

.. _options-standard-error:

``-e`` ``--standard-error``
  Write diagnostic messages to standard error. The default is to record them via ``syslog``.

.. _options-configuration-file:

``-f``\ *file* ``--configuration-file=``\ *file*
  Specify the location of the :ref:`configuration file <configure>` which is to be used for the establishing of default run-time settings.

.. _options-help:

``-h`` ``--help``
  Display a summary of the command line options accepted by BRLTTY, and then exit.

.. _options-speech-input:

``-i``\ *name* ``--speech-input=``\ *name*
  Specify the name of the file system object (FIFO, named pipe, named socket, etc) which can be used by other applications for text-to-speech conversion via BRLTTY's speech driver. If not specified, the file system object is not created. See the :ref:`speech-input <configure-speech-input>` configuration file directive for the default run-time setting. This option isn't available if the ``--disable-speech-support`` build option was specified.

.. _options-keyboard-table:

``-k``\ *file* ``--keyboard-table=``\ *file*
  Specify the keyboard table (see section :ref:`Key Tables <table-key>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ktb`` extension is optional. See the :ref:`keyboard-table <configure-keyboard-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Keyboard Table <preference-keyboard-table>` preference.

``-l``\ *level* ``--log-level=``\ *level*
  Specify the severity threshold for diagnostic message generation. The following levels are recognized.

  0
    emergency

  1
    alert

  2
    critical

  3
    error

  4
    warning

  5
    notice

  6
    information

  7
    debug

  Either the number or the name may be supplied, and the name may be abbreviated. If not specified, then ``information`` is assumed (see the :ref:`-q <options-quiet>` option for more details).

.. _options-midi-device:

``-m``\ *device* ``--midi-device=``\ *device*
  Specify the device to use for the Musical Instrument Digital Interface (see section :ref:`MIDI Device Specification <operand-midi-device>`). See the :ref:`midi-device <configure-midi-device>` configuration file directive for the default run-time setting. This option isn't available if the ``--disable-midi-support`` build option was specified.

.. _options-no-daemon:

``-n`` ``--no-daemon``
  Specify that BRLTTY is to remain in the foreground. If not specified, then BRLTTY becomes a background process (daemon) after initializing itself but before starting any of the selected drivers.

.. _options-pcm-device:

``-p``\ *device* ``--pcm-device=``\ *device*
  Specify the device to use for digital audio (see section :ref:`PCM Device Specification <operand-pcm-device>`). See the :ref:`pcm-device <configure-pcm-device>` configuration file directive for the default run-time setting. This option isn't available if the ``--disable-pcm-support`` build option was specified.

.. _options-quiet:

``-q`` ``--quiet``
  Log less information. This option changes the default log level (see the :ref:`-l <options-log-level>` option) to ``notice`` if either the :ref:`-v <options-verify>` or the :ref:`-V <options-version>` option is specified, and to ``warning`` otherwise.

.. _options-release-device:

``-r`` ``--release-device``
  Release the device to which the braille display is connected when the current screen or window can't be read. See the :ref:`release-device <configure-release-device>` configuration file directive for the default run-time setting.

.. _options-speech-driver:

``-s``\ *driver*\ ``,``...|\ ``auto`` ``--speech-driver=``\ *driver*\ ``,``...|\ ``auto``
  Specify the speech synthesizer driver (see section :ref:`Driver Specification <operand-driver>`). See the :ref:`speech-driver <configure-speech-driver>` configuration file directive for the default run-time setting. This option isn't available if the ``--disable-speech-support`` build option was specified.

.. _options-text-table:

``-t``\ *file* ``--text-table=``\ *file*
  Specify the text table (see section :ref:`Text Tables <table-text>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the ``--with-data-directory`` and the ``--with-execute-root`` build options for more details). The ``.ttb`` extension is optional. See the :ref:`text-table <configure-text-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Text Table <preference-text-table>` preference.

.. _options-verify:

``-v`` ``--verify``
  Display the current versions of BRLTTY itself, of the server side of its application programming interface, and of the selected braille and speech drivers, and then exit. If the :ref:`-q <options-quiet>` option isn't specified, then also display the values of the options after all sources have been considered. If more than one braille driver (see the :ref:`-b <options-braille-driver>` command line option) and/or more than one braille device (see the :ref:`-d <options-braille-device>` command line option) has been specified then braille display autodetection is performed. If more than one speech driver (see the :ref:`-s <options-speech-driver>` command line option) has been specified then speech synthesizer autodetection is performed.

.. _options-screen-driver:

``-x``\ *driver* ``--screen-driver=``\ *driver*
  Specify the screen driver (see section :ref:`Supported Screen Drivers <screen>`). See the :ref:`screen-driver <configure-screen-driver>` configuration file directive for the default run-time setting.

.. _options-api-parameters:

``-A``\ *name*\ ``=``\ *value*\ ``,``... ``--api-parameters=``\ *name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the Application Programming Interface. If the same parameter is specified more than once, then its rightmost assignment is used. For a description of the parameters accepted by the interface, please see the **BrlAPI** reference manual. See the :ref:`api-parameters <configure-api-parameters>` configuration file directive for the default run-time settings.

.. _options-braille-parameters:

``-B``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``... ``--braille-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the braille display drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`braille-parameters <configure-braille-parameters>` configuration file directive for the default run-time settings.

.. _options-environment-variables:

``-E`` ``--environment-variables``
  Recognize environment variables when determining the default settings for unspecified command line options (see section :ref:`Command Line Options <options>`). If this option is specified, and if the environment variable associated with an unspecified option is defined, then the value of that environment variable is used. The names of these environment variables are based on the long names of the options they correspond to:

  - All letters are in upper case.
  - Underscores (``_``) are used instead of minus signs (``-``).
  - The prefix ``BRLTTY_`` is added.

  This option is particularly useful on the Linux operating system as it allows default settings to be passed to BRLTTY via boot parameters. The following environment variables are supported:

  ``BRLTTY_API_PARAMETERS``
    Parameters for the Application Programming Interface (see the :ref:`-A <options-api-parameters>` command line option).

  ``BRLTTY_ATTRIBUTES_TABLE``
    The attributes table (see the :ref:`-a <options-attributes-table>` command line option).

  ``BRLTTY_BRAILLE_DEVICE``
    The braille display device (see the :ref:`-d <options-braille-device>` command line option).

  ``BRLTTY_BRAILLE_DRIVER``
    The braille display driver (see the :ref:`-b <options-braille-driver>` command line option).

  ``BRLTTY_BRAILLE_PARAMETERS``
    Parameters for the braille display driver (see the :ref:`-B <options-braille-parameters>` command line option).

  ``BRLTTY_CONFIGURATION_FILE``
    The configuration file (see the :ref:`-f <options-configuration-file>` command line option).

  ``BRLTTY_CONTRACTION_TABLE``
    The contraction table (see the :ref:`-c <options-contraction-table>` command line option).

  ``BRLTTY_KEYBOARD_TABLE``
    The keyboard table (see the :ref:`-k <options-keyboard-table>` command line option).

  ``BRLTTY_KEYBOARD_PROPERTIES``
    The keyboard properties (see the :ref:`-K <options-keyboard-properties>` command line option).

  ``BRLTTY_MIDI_DEVICE``
    The Musical Instrument Digital Interface device (see the :ref:`-m <options-midi-device>` command line option).

  ``BRLTTY_PCM_DEVICE``
    The digital audio device (see the :ref:`-p <options-pcm-device>` command line option).

  ``BRLTTY_PREFERENCES_FILE``
    The location of the file which is to be used for the saving and loading of user preferences (see the :ref:`-F <options-preferences-file>` command line option).

  ``BRLTTY_RELEASE_DEVICE``
    Whether or not to release the device to which the braille display is connected when the current screen or window can't be read (see the :ref:`-r <options-release-device>` command line option).

  ``BRLTTY_SCREEN_PARAMETERS``
    Parameters for the screen driver (see the :ref:`-X <options-screen-parameters>` command line option).

  ``BRLTTY_SPEECH_DRIVER``
    The speech synthesizer driver (see the :ref:`-s <options-speech-driver>` command line option).

  ``BRLTTY_SPEECH_INPUT``
    The name of the file system object which can be used by other applications for text-to-speech conversion via BRLTTY's speech driver (see the :ref:`-i <options-speech-input>` command line option).

  ``BRLTTY_SPEECH_PARAMETERS``
    Parameters for the speech synthesizer driver (see the :ref:`-S <options-speech-parameters>` command line option).

  ``BRLTTY_TEXT_TABLE``
    The text table (see the :ref:`-t <options-text-table>` command line option).

.. _options-preferences-file:

``-F``\ *file* ``--preferences-file=``\ *file*
  Specify the location of the file which is to be used for the saving and loading of user preferences. If a relative path is supplied, then it's anchored at ``/var/lib/brltty``. See the :ref:`preferences-file <configure-preferences-file>` configuration file directive for the default run-time setting.

.. _options-install-service:

``-I`` ``--install-service``
  Install BRLTTY as the BrlAPI service. This means that:

  - BRLTTY will be automatically started when the system is booted.
  - Applications can know that a BrlAPI server is running.

  This option is only supported on the Windows platform.

.. _options-keyboard-properties:

``-K``\ *name*\ ``=``\ *value*\ ``,``... ``--keyboard-properties=``\ *name*\ ``=``\ *value*\ ``,``...
  Specify the properties of the keyboard(s) to be monitored. If the same property is specified more than once, then its rightmost assignment is used. See section :ref:`Keyboard Properties <keyboard-properties>` for a list of the properties which may be specified. See the :ref:`keyboard-properties <configure-keyboard-properties>` configuration file directive for the default run-time settings.

.. _options-message-timeout:

``-M``\ *csecs* ``--message-timeout=``\ *csecs*
  Specify the amount of time (in hundredths of a second) that BRLTTY keeps its own internally generated messages on the braille display. If not specified, then ``400`` (4 seconds) is assumed.

.. _options-no-api:

``-N`` ``--no-api``
  Disable the application programming interface.

.. _options-pid-file:

``-P``\ *file* ``--pid-file=``\ *file*
  Specify the file wherein BRLTTY is to write its process identifier (pid). If not specified, BRLTTY doesn't write its process identifier anywhere.

.. _options-remove-service:

``-R`` ``--remove-service``
  Remove the BrlAPI service. This means that:

  - BRLTTY will not be automatically started when the system is booted.
  - Applications can know that no BrlAPI server is running.

  This option is only supported on the Windows platform.

.. _options-speech-parameters:

``-S``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``... ``--speech-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the speech synthesizer drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`speech-parameters <configure-speech-parameters>` configuration file directive for the default run-time settings.

.. _options-version:

``-V`` ``--version``
  Display the current versions of BRLTTY itself, of the server side of its application programming interface, and of those drivers which have been linked into its binary, and then exit. If the :ref:`-q <options-quiet>` option isn't specified, then also display copyright information.

.. _options-screen-parameters:

``-X``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``... ``--screen-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the screen drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Supported Screen Drivers <screen>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`screen-parameters <configure-screen-parameters>` configuration file directive for the default run-time settings.


Feature Descriptions
====================


.. _routing:

Cursor Routing
--------------

When moving the braille window around the screen
while examining the text, say, in an editor,
you often need to bring the cursor
to a specific character within the braille window.
You'll probably find this to be a rather difficult task for a number of reasons.
One is that you may not know where the cursor is,
and that you may lose your place while trying to find it.
Another is that the cursor may move unpredictably as the arrow keys are pressed
(some editors, for example, don't allow the cursor to be
more to the right than the end of the line it's on).
Cursor routing provides just such a capability
by knowing where the cursor is,
by simulating the same arrow-key presses which you'd have to enter manually,
and by monitoring the progress of the cursor as it moves.

Some braille displays have a button, known as a routing key, above each cell.
These keys use the :ref:`ROUTE <command-ROUTE>` command
to route the cursor right to the desired location.

Cursor routing, while very convenient and effective,
is, strictly speaking, not completely reliable.
One reason for this is that its current implementation assumes
VT100 cursor key escape sequences.
Another is that some applications do non-standard things
in response to detecting that a cursor key has been pressed.
A minor problem found within some editors (like ``vi``),
as already mentioned above,
is that they throw in some unpredictable horizontal motion
when vertical motion is requested
because they don't allow the cursor to be to the right of the end of a line.
A major problem found within some web browsers (like ``lynx``)
is that the up- and down-arrow keys are used to move among the links
(which may skip lines and/or move the cursor horizontally,
but which rarely just moves the cursor one line in the desired direction),
and that the left- and right-arrow keys are used to select links
(which has absolutely nothing to do with any form of cursor motion whatsoever,
and which even totally changes the screen content).

Cursor routing may not work very well on a heavily loaded system,
and definitely doesn't work very well when working on a remote system over a slow link.
This is so because of all of the checks which must be made along the way
in order to deal with unpredictable cursor motion
and in order to ensure that any mistake has at least a fighting chance to be undone.
Even  though BRLTTY tries to be fairly clever,
it must still essentially wait to see what happens after each simulated arrow-key press.

Once a cursor routing request has been made,
BRLTTY keeps trying to route the cursor to the desired location until
a timeout expires before the cursor reaches that location,
the cursor seems to be moving in the wrong direction,
or you switch to a different virtual terminal.
An attempt is first made to use vertical motion
to bring the cursor to the right line,
and, only if that succeeds, an attempt is then made
to use horizontal motion to bring the cursor to the right column.
If another request is made while one is still in progress,
then the first one is aborted and the second one is initiated.

A safer but less powerful cursor routing command,
:ref:`CSRJMP_VERT <command-CSRJMP_VERT>`,
uses just vertical motion to bring the cursor
to anywhere on the top line of the braille window.
It's especially useful in conjunction with applications (like ``lynx``)
wherein horizontal cursor motion must never be attempted.


.. _cut:

Cut and Paste
-------------

This feature enables you to grab some text which is already on the screen
and re-enter it at the current cursor position.
Using it saves time and avoids errors
when a long and/or complicated piece of text needs to be copied,
and even when the same short and simple piece of text needs to be copied many times.
It's particularly useful for things like
long file names,
complicated command lines,
E-mail addresses,
and URLs. Cutting and pasting text involves three simple steps:
#. Mark either the top-left corner of the rectangular area or the beginning of the linear area on the screen which is to be grabbed (cut). If your display has routing keys, then move the braille window so that the first character to be cut appears anywhere within it, and then:

   - invoke the :ref:`CUTBEGIN <command-CUTBEGIN>` command to start a new cut buffer
   - invoke the :ref:`CUTAPPEND <command-CUTAPPEND>` command to append to the existing cut buffer

#. by pressing the key(s) associated with it and then pressing the routing key associated with the character.
#. Mark either the bottom-right corner of the rectangular area or the end of the linear area on the screen which is to be grabbed (cut). If your display has routing keys, then move the braille window so that the last character to be cut appears anywhere within it, and then

   - invoke the :ref:`CUTRECT <command-CUTRECT>` command to cut a rectangular area
   - invoke the :ref:`CUTLINE <command-CUTLINE>` command to cut a linear area

#. by pressing the key(s) associated with it and then pressing the routing key associated with the character. Marking the end of the cut area appends the selected screen content to the cut buffer. Excess white-space is removed from the end of each line in the cut buffer so that unwanted trailing spaces won't be pasted back in. Control characters are replaced with blanks.
#. Insert (paste) the text where it's needed. Place the cursor over the character where the text is to be pasted, and invoke the :ref:`PASTE <command-PASTE>` command. You can paste the same text any number of times without recutting it. This description assumes that you're already in some sort of input mode. If you paste when you're in some other kind of mode (like ``vi``'s command mode), then you'd better be aware of what the characters in the cut buffer will do.


The cut buffer is also used by
the :ref:`PRSEARCH/NXSEARCH <command-PRSEARCH-NXSEARCH>` commands.


.. _gpm:

Pointer (Mouse) Support via GPM
-------------------------------

If BRLTTY is configured with the ``--enable-gpm`` build option
on a system where the ``gpm`` application has been installed,
then it'll interact with the pointer (mouse).

Moving the pointer drags the braille window
(see the :ref:`Window Follows Pointer <preference-window-follows-pointer>` preference).
Whenever the pointer is moved beyond the edge of the braille window,
the braille window is dragged along (one character at a time).
This gives the braille user another two-dimensional way
to inspect the screen content
or to quickly move the braille window to a desired location.
It also gives a sighted observer an easy way to move the braille window
to something he'd like the braille user to read.

``gpm`` uses reverse video to show where the pointer is.
Underlining of highlighted characters
(see the :ref:`ATTRVIS <command-ATTRVIS>` command for details)
should be turned on, therefore, when the braille user wishes to use the pointer.

This feature also gives the braille user access to ``gpm``'s cut-and-paste capability.
Although you should read ``gpm``'s own documentation,
here are some notes on how it works.

- Copy the current character to the cut buffer by single-clicking the left button.
- Copy the current word (space-delimited) to the cut buffer by double-clicking the left button.
- Copy the current line to the cut buffer by triple-clicking the left button.
- Copy a linear region to the cut buffer as follows:

  #. Place the pointer on the first character of the region.
  #. Press (and hold) the left button.
  #. Move the pointer to the last character of the region (all currently selected characters are highlighted).
  #. Release the left button.

- Paste (input) the current contents of the cut buffer by clicking the middle button of a three-button mouse or by clicking the right button of a two-button mouse.
- Append to the cut buffer by using the right button of a three-button mouse.


.. _tunes:
.. _command-TUNES:

Alert Tunes
-----------

BRLTTY alerts you to the occurrence of significant events
by playing short predefined tunes.
This feature can be activated and deactivated with
either the :ref:`TUNES <command-TUNES>` command
or the :ref:`Alert Tunes <tunes>` preference.
The tunes are played via the internal beeper by default,
but other alternatives can be selected
with the :ref:`Tune Device <preference-tune-device>` preference.

Each significant event is associated, from highest to lowest priority,
with one or more of the following:

a tune
  If a tune has been associated with the event, if the :ref:`Alert Tunes <tunes>` preference (see also the :ref:`TUNES <command-TUNES>` command) is active, and if the selected tune device (see the :ref:`Tune Device <preference-tune-device>` preference) can be opened, then the tune is played.

a dot pattern
  If a dot pattern has been associated with the event, and if the :ref:`Alert Dots <preference-alert-dots>` preference is active, then the dot pattern is briefly displayed on every braille cell. Some braille displays don't respond quickly enough for this mechanism to work effectively.

a message
  If a message has been associated with the event, and if the :ref:`Alert Messages <preference-alert-messages>` preference is active, then it is displayed for a few seconds (see the :ref:`-M <options-message-timeout>` command line option).


These events include:

- When the braille display driver starts or stops.
- When a lengthy command completes.
- When a command cannot be executed.
- When a mark is set.
- When the start or end of the cut block is set.
- When a feature is activated or deactivated.
- When cursor tracking is turned on or off.
- When the screen image is frozen or unfrozen.
- When the braille window wraps either down to the beginning of the next line or up to the end of the previous line.
- When identical lines are skipped.
- When a requested motion cannot be performed.
- When cursor routing starts, finishes, or fails.


.. _preferences:

Preferences Settings
--------------------

When BRLTTY starts, it loads a file which contains your preferences settings.
The file doesn't need to exist,
and is created the first time the settings are saved
with the :ref:`PREFSAVE <command-PREFSAVE>` command.
The most recently saved settings can be restored at any time
with the :ref:`PREFLOAD <command-PREFLOAD>` command.

The name for this file is ``/etc/brltty-``\ *driver*\ ``.prefs``.
where *driver* is the two-letter :ref:`driver identification code <drivers>`.


.. _preferences-menu:

The Preferences Menu
~~~~~~~~~~~~~~~~~~~~

The preferences settings are saved as binary data
which, therefore, can't be edited by hand.
BRLTTY, however, has a simple menu from which you can easily change them.

The menu is activated by the :ref:`PREFMENU <command-PREFMENU>` command.
The braille display briefly
(see the :ref:`-M <options-message-timeout>` command line option)
shows the menu title,
and then presents the current item and its current setting.


Navigating the Menu
^^^^^^^^^^^^^^^^^^^

See :ref:`Menu Navigation Commands <menu-navigation>` for the full list of
commands which enable you to select items and change settings within the menu.
For backward compatibility with old drivers, the window motion commands,
which have modified meanings in this context, can also be used.

``TOP``/``BOT``, ``TOP_LEFT``/``BOT_LEFT``, ``PAGE_UP``/``PAGE_DOWN``
  Go to the first/last item in the menu (same as :ref:`MENU_FIRST_ITEM/MENU_LAST_ITEM <command-MENU_FIRST_ITEM-MENU_LAST_ITEM>`).

``LNUP``/``LNDN``, ``PRDIFLN``/``NXDIFLN``, ``CURSOR_UP``/``CURSOR_DOWN``
  Go to the previous/next item in the menu (same as :ref:`MENU_PREV_ITEM/MENU_NEXT_ITEM <command-MENU_PREV_ITEM-MENU_NEXT_ITEM>`).

``WINUP``/``WINDN``, ``CHRLT``/``CHRRT``, ``CURSOR_LEFT``/``CURSOR_RIGHT``, ``BACK``/``HOME``
  Decrement/increment the current menu item's setting (same as :ref:`MENU_PREV_SETTING/MENU_NEXT_SETTING <command-MENU_PREV_SETTING-MENU_NEXT_SETTING>`).


Notes:

- The routing keys can also be used to select a setting for the current item. If the item has numeric settings, then the entire row of routing keys acts as a scroll bar which covers the full range of valid values. If the item has named settings, then the routing keys correspond ordinally with the settings.
- Use the ``PREFLOAD`` command to undo all of the changes which were made since entering the menu.
- Use the ``PREFMENU`` command (again) to leave the new settings in effect, exit the menu, and resume normal operation. If the <sq/Save Settings on Exit/ item is set, then, in addition, the new settings are written to the preferences settings file. Any command not recognized by the menu system also does these same things.


The Menu Items
^^^^^^^^^^^^^^

.. _preference-save-on-exit:

Save on Exit
  When exiting the preferences menu:

  No
    Don't automatically save the preferences settings.

  Yes
    Automatically save the preferences settings.

  The initial setting is ``No``.

.. _preference-text-style:

Text Style
  When displaying screen content (see the :ref:`DISPMD <command-DISPMD>` command), show characters:

  8-dot
    With all eight dots.

  6-dot
    With only dots 1 through 6. If a contraction table has been selected (see the :ref:`-c <options-contraction-table>` command line option and the :ref:`contraction-table <configure-contraction-table>` configuration file directive), then it is used.

  This setting can also be changed with the :ref:`SIXDOTS <command-SIXDOTS>` command.

.. _preference-skip-identical-lines:

Skip Identical Lines
  When moving either up or down exactly one line with the :ref:`LNUP/LNDN <command-LNUP-LNDN>` commands, as well as the line wrapping feature of the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` and :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>` commands:

  No
    Don't skip past lines which have the same content as the current line.

  Yes
    Skip past lines which have the same content as the current line.

  This setting can also be changed with the :ref:`SKPIDLNS <command-SKPIDLNS>` command.

Skip Blank Windows
  When moving either left or right with the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` commands:

  No
    Don't skip past blank windows.

  Yes
    Skip past blank windows.

  This setting can also be changed with the :ref:`SKPBLNKWINS <command-SKPBLNKWINS>` command.

.. _preference-which-blank-windows:

Which Blank Windows
  If blank windows are to be skipped:

  All
    Skip all of them.

  End of Line
    Only skip those which are at the end (on the right side) of a line.

  Rest of Line
    Only skip those which are at the end (on the right side) of a line when reading forward, and at the beginning (on the left side) of a line when reading backward.

.. _preference-sliding-window:

Sliding Window
  If the cursor is being tracked (see the :ref:`CSRTRK <command-CSRTRK>` command), and the cursor moves too close to (or beyond) either end of the braille window:

  No
    Horizontally reposition the window such that its left end is a multiple of its width from the left edge of the screen.

  Yes
    Horizontally reposition the window such that the cursor, while remaining on that side of the window, is nearer the centre.

  This setting can also be changed with the :ref:`SLIDEWIN <command-SLIDEWIN>` command.

.. _preference-eager-sliding-window:

Eager Sliding Window
  If the braille window is to slide:

  No
    Reposition it whenever the cursor moves beyond either end.

  Yes
    Reposition it whenever the cursor moves too close to either end.

  The initial setting is ``No``.

.. _preference-window-overlap:

Window Overlap
  When moving either left or right with the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` commands, this setting specifies how many characters horizontally adjacent braille windows should overlap each other by. The initial setting is ``0``.

Autorepeat
  While the key (combination) for a command remains pressed:

  No
    Don't automatically repeat the command.

  Yes
    Automatically repeat the command at a regular interval after an initial delay.

  The following commands are eligible for autorepeating:

  - The :ref:`LNUP/LNDN <command-LNUP-LNDN>` commands.
  - The :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>` commands.
  - The :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>` commands.
  - Braille window panning operations (see the :ref:`Autorepeat Panning <preference-autorepeat-panning>` preference).
  - The Page-Up and Page-Down operations.
  - The Cursor-Up and Cursor-Down operations.
  - The Cursor-Left and Cursor-Right operations.
  - The Backspace and Delete operations.
  - Character entry.

  Only some drivers support this functionality, the primary limitation being that many braille displays don't signal both key presses and key releases as distinctly separate events. This setting can also be changed with the :ref:`AUTOREPEAT <command-AUTOREPEAT>` command. The initial setting is ``Yes``.

.. _preference-autorepeat-panning:

Autorepeat Panning
  When the :ref:`Autorepeat <preference-autorepeat>` preference is enabled:

  No
    Don't autorepeat braille window panning operations.

  Yes
    Autorepeat braille window panning operations.

  This preference affects the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` commands. The initial setting is ``No``.

.. _preference-autorepeat-delay:

Autorepeat Delay
  When a character is to be autorepeated, this setting specifies the amount of time (see the note on :ref:`time settings <time-settings>` below) which must pass before autorepeating begins. The initial setting is ``50``.

.. _preference-autorepeat-interval:

Autorepeat Interval
  When a character is being autorepeated, this setting specifies the amount of time (see the note on :ref:`time settings <time-settings>` below) between each reexecution. The initial setting is ``10``.

.. _preference-show-cursor:

Show Cursor
  When displaying screen content (see the :ref:`DISPMD <command-DISPMD>` command):

  No
    Don't show the cursor.

  Yes
    Show the cursor.

  This setting can also be changed with the :ref:`CSRVIS <command-CSRVIS>` command. The initial setting is ``Yes``.

.. _preference-cursor-style:

Cursor Style
  When showing the cursor, represent it:

  Underline
    With dots 7 and 8.

  Block
    With all eight dots.

  This setting can also be changed with the :ref:`CSRSIZE <command-CSRSIZE>` command.

.. _preference-blinking-cursor:

Blinking Cursor
  When the cursor is to be shown:

  No
    Leave it visible all the time.

  Yes
    Make it alternately visible and invisible according to a predefined interval.

  This setting can also be changed with the :ref:`CSRBLINK <command-CSRBLINK>` command.

.. _preference-cursor-visible-time:

Cursor Visible Time
  When the cursor is to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that it is to be visible. The initial setting is ``40``.

.. _preference-cursor-invisible-time:

Cursor Invisible Time
  When the cursor is to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that it is to be invisible. The initial setting is ``40``.

.. _preference-show-attributes:

Show Attributes
  When displaying screen content (see the :ref:`DISPMD <command-DISPMD>` command):

  No
    Don't underline highlighted characters.

  Yes
    Underline highlighted characters.

  This setting can also be changed with the :ref:`ATTRVIS <command-ATTRVIS>` command.

.. _preference-blinking-attributes:

Blinking Attributes
  When highlighted characters are to be underlined:

  No
    Leave the indicator visible all the time.

  Yes
    Make the indicator alternately visible and invisible according to a predefined interval.

  This setting can also be changed with the :ref:`ATTRBLINK <command-ATTRBLINK>` command.

.. _preference-attributes-visible-time:

Attributes Visible Time
  When the highlighted character underline is to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that it is to be visible. The initial setting is ``20``.

.. _preference-attributes-invisible-time:

Attributes Invisible Time
  When the highlighted character underline is to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that it is to be invisible. The initial setting is ``60``.

.. _preference-blinking-capitals:

Blinking Capitals
  When displaying screen content (see the :ref:`DISPMD <command-DISPMD>` command):

  No
    Leave capital letters visible all the time.

  Yes
    Make capital letters alternately visible and invisible according to a predefined interval.

  This setting can also be changed with the :ref:`CAPBLINK <command-CAPBLINK>` command.

.. _preference-capitals-visible-time:

Capitals Visible Time
  When capital letters are to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that they're to be visible. The initial setting is ``60``.

.. _preference-capitals-invisible-time:

Capitals Invisible Time
  When capital letters are to be blinked, this setting specifies the length of time (see the note on :ref:`time settings <time-settings>` below) during each cycle that they're to be invisible. The initial setting is ``20``.

.. _preference-braille-firmness:

Braille Firmness
  Adjust the firmness (or stiffness) of the braille dots. It can be set to:

  - Maximum
  - High
  - Medium
  - Low
  - Minimum

  This preference is only available if a driver which supports it is being used. The initial setting is ``Medium``.

.. _preference-braille-sensitivity:

Braille Sensitivity
  Adjust the sensitivity of the braille dots to touch. It can be set to:

  - Maximum
  - High
  - Medium
  - Low
  - Minimum

  This preference is only available if a driver which supports it is being used. The initial setting is ``Medium``.

.. _preference-window-follows-pointer:

Window Follows Pointer
  When moving the pointer device (mouse):

  No
    Don't drag the braille window.

  Yes
    Drag the braille window.

  This preference is only presented if the ``--enable-gpm`` build option was specified.

.. _preference-highlight-window:

Highlight Window
  When moving the braille window:

  No
    Don't highlight the new screen area.

  Yes
    Highlight the new screen area.

  This feature enables a sighted observer to see where the braille window is, and, therefore, to know what the braille user is reading. Any motion of the braille window (manual, cursor tracking, etc.), other than when it moves in response to pointer (mouse) motion (see the :ref:`Window Follows Pointer <preference-window-follows-pointer>` preference), causes the area of the screen corresponding to the new location of the braille window to be highlighted. If the :ref:`Show Attributes <preference-show-attributes>` preference is enabled then only the character corresponding to the upper-left corner of the braille window is highlighted.

Alert Tunes
  Whenever a significant event with an associated tune occurs (see :ref:`Alert Tunes <tunes>`):

  No
    Don't play the tune.

  Yes
    Play the tune.

  This setting can also be changed with the :ref:`TUNES <command-TUNES>` command. The initial setting is ``Yes``.

.. _preference-tune-device:

Tune Device
  Play alert tunes via:

  Beeper
    The internal beeper (console tone generator). This setting is supported on Linux, on OpenBSD, on FreeBSD, and on NetBSD. It's always safe to use, although it may be somewhat soft. This device isn't available if the ``--disable-beeper-support`` build option was specified.

  PCM
    The digital audio interface on the sound card. This setting is supported on Linux (via ``/dev/dsp``), on Solaris (via ``/dev/audio``), on OpenBSD (via ``/dev/audio0``), on FreeBSD (via ``/dev/dsp``), and on NetBSD (via ``/dev/audio0``). It doesn't work when this device is already being used by another application. This device isn't available if the ``--disable-pcm-support`` build option was specified.

  MIDI
    The Musical Instrument Digital Interface on the sound card This setting is supported on Linux (via ``/dev/sequencer``). It doesn't work when this device is already being used by another application. This device isn't available if the ``--disable-midi-support`` build option was specified.

  FM
    The FM synthesizer on an AdLib, OPL3, Sound Blaster, or equivalent sound card. This setting is supported on Linux. It works even if the FM synthesizer is already being used by another application. The results are unpredictable, and potentially not very good, if it's used with a sound card which doesn't support this feature. This device isn't available if the ``--disable-fm-support`` build option was specified.

  The initial setting is ``Beeper`` on those platforms which support it, and ``PCM`` on those platforms which don't.

.. _preference-pcm-volume:

PCM Volume
  If the digital audio interface of the sound card is being used to play the alert tunes, this setting specifies the loudness (as a percentage of the maximum) at which they are to be played. The initial setting is ``70``.

.. _preference-midi-volume:

MIDI Volume
  If the Musical Instrument Digital Interface (MIDI) of the sound card is being used to play the alert tunes, this setting specifies the loudness (as a percentage of the maximum) at which they are to be played. The initial setting is ``70``.

.. _preference-midi-instrument:

MIDI Instrument
  If the Musical Instrument Digital Interface (MIDI) of the sound card is being used to play the alert tunes, this setting specifies which instrument is to be used (see the :ref:`MIDI Instrument Table <midi>`). The initial setting is ``Acoustic Grand Piano``.

.. _preference-fm-volume:

FM Volume
  If the FM synthesizer of the sound card is being used to play the alert tunes, this setting specifies the loudness (as a percentage of the maximum) at which they are to be played. The initial setting is ``70``.

.. _preference-alert-dots:

Alert Dots
  Whenever a significant event with an associated dot pattern occurs (see :ref:`Alert Tunes <tunes>`):

  No
    Don't display the dot pattern.

  Yes
    Briefly display the dot pattern.

  If alert tunes are to be played (see the :ref:`TUNES <command-TUNES>` command and the :ref:`Alert Tunes <tunes>` preference), if a tune has been associated with the event, and if the selected tune device can be opened, then, regardless of the setting of this preference, the dot pattern isn't displayed.

.. _preference-alert-messages:

Alert Messages
  Whenever a significant event with an associated message occurs (see :ref:`Alert Tunes <tunes>`):

  No
    Don't display the message.

  Yes
    Display the message.

  If alert tunes are to be played (see the :ref:`TUNES <command-TUNES>` command and the :ref:`Alert Tunes <tunes>` preference), if a tune has been associated with the event, and if the selected tune device can be opened, or if alert dot patterns are to be displayed (see the :ref:`Alert Dots <preference-alert-dots>` preference) and if a dot pattern has been associated with the event, then, regardless of the setting of this preference, the message isn't displayed.

.. _preference-sayline-mode:

Say-Line Mode
  When using the :ref:`SAY_LINE <command-SAY_LINE>` command:

  Immediate
    Discard pending speech.

  Enqueue
    Don't discard pending speech.

  The initial setting is ``Immediate``.

Autospeak

  No
    Only speak when explicitly requested to do so.

  Yes
    Automatically speak:

    - the new line when the braille window is moved vertically.
    - characters which are entered or deleted.
    - the character to which the cursor is moved.


  This setting can also be changed with the :ref:`AUTOSPEAK <command-AUTOSPEAK>` command. The initial setting is ``No``.

.. _preference-speech-rate:

Speech Rate
  Adjust the speech rate (``0`` is the slowest, ``20`` is the fastest). This preference is only available if a driver which supports it is being used. This setting can also be changed with the :ref:`SAY_SLOWER/SAY_FASTER <command-SAY_SLOWER-SAY_FASTER>` commands. The initial setting is ``10``.

.. _preference-speech-volume:

Speech Volume
  Adjust the speech volume (``0`` is the softest, ``20`` is the loudest). This preference is only available if a driver which supports it is being used. This setting can also be changed with the :ref:`SAY_SOFTER/SAY_LOUDER <command-SAY_SOFTER-SAY_LOUDER>` commands. The initial setting is ``10``.

.. _preference-speech-pitch:

Speech Pitch
  Adjust the speech pitch (``0`` is the lowest, ``20`` is the highest). This preference is only available if a driver which supports it is being used. The initial setting is ``10``.

.. _preference-speech-punctuation:

Speech Punctuation
  Adjust the amount of punctuation which is spoken. It can be set to:

  - None
  - Some
  - All

  This preference is only available if a driver which supports it is being used. The initial setting is ``Some``.

.. _preference-status-style:

Status Style
  This setting specifies the way that the status cells are to be used. You shouldn't normally need to play with it. It enables BRLTTY's developers to test status cell configurations for braille displays which they don't actually have.

  None
    Don't use the status cells. This setting is always safe, but it's also quite useless.

  Alva
    The status cells contain:

    1
      The location of the cursor (see below).

    2
      The location of the top-left corner of the braille window (see below).

    3
      A letter indicating BRLTTY's state. In order of precedence:

      a
        Screen attributes are being shown (see the :ref:`DISPMD <command-DISPMD>` command).

      f
        The screen image is frozen (see the :ref:`FREEZE <command-FREEZE>` command).

      f
        The cursor is being tracked (see the :ref:`CSRTRK <command-CSRTRK>` command).

      *blank*
        Nothing special.


    The locations of the cursor and the braille window are presented in an interesting way. Dots 1 through 6 represent the line number with a letter from ``a`` (for 1) through ``y`` (for 25). Dots 7 and 8 (the extra two at the bottom) represent the horizontal braille window number as follows:

    No Dots
      The first (leftmost) window.

    Dot 7
      The second window.

    Dot 8
      The third window.

    Dots 7 and 8
      The fourth window.

    In both cases, the indicators wrap: line 26 is represented by the letter ``a``, and the fifth horizontal braille window is represented by no dots at the bottom.

  Tieman
    The status cells contain:

    1-2
      The columns (counting from 1) of the cursor (shown in the top half of the cells) and the top-left corner of the braille window (shown in the bottom half of the cells).

    3-4
      The rows (counting from 1) of the cursor (shown in the top half of the cells) and the top-left corner of the braille window (shown in the bottom half of the cells).

    5
      Each dot indicates if a feature is turned on as follows:

      Dot 1
        The screen image is frozen (see the :ref:`FREEZE <command-FREEZE>` command).

      Dot 2
        Screen attributes are being displayed (see the :ref:`DISPMD <command-DISPMD>` command).

      Dot 3
        Alert tunes are being played (see the :ref:`TUNES <command-TUNES>` command).

      Dot 4
        The cursor is being shown (see the :ref:`CSRVIS <command-CSRVIS>` command).

      Dot 5
        The cursor is a solid block (see the :ref:`CSRSIZE <command-CSRSIZE>` command).

      Dot 6
        The cursor is blinking (see the :ref:`CSRBLINK <command-CSRBLINK>` command).

      Dot 7
        The cursor is being tracked (see the :ref:`CSRTRK <command-CSRTRK>` command).

      Dot 8
        The braille window will slide (see the :ref:`SLIDEWIN <command-SLIDEWIN>` command).


  PowerBraille 80
    The status cells contain:

    1
      The row (counting from 1) corresponding to the top of the braille window. The tens digit is shown in the top half of the cell, and the units digit is shown in the bottom half of the cell.

  Generic
    This setting passes a lot of information to the braille driver, and the driver itself decides how to present it.

  MDV
    The status cells contain:

    1-2
      The location of the top-left corner of the braille window. The row (counting from 1) is shown in the top half of the cells, and the column (counting from 1) is shown in the bottom half of the cells.

  Voyager
    The status cells contain:

    1
      The row (counting from 1) corresponding to the top of the braille window (see below).

    2
      The row (counting from 1) whereon the cursor is (see below).

    3
      If the screen is frozen (see the :ref:`FREEZE <command-FREEZE>` command), then the letter ``F``. If it isn't, then the column (counting from 1) wherein the cursor is (see below).

    Row and column numbers are shown as two digits within a single cell. The tens digit is shown in the top half of the cell, and the units digit is shown in the bottom half of the cell.

  The initial setting is braille display driver dependent.

.. _preference-text-table:

Text Table
  Select the text table. See section :ref:`Text Tables <table-text>` for details. See the :ref:`-t <options-text-table>` command line option for the initial setting. This preference isn't saved.

.. _preference-attributes-table:

Attributes Table
  Select the attributes table. See section :ref:`Attributes Tables <table-attributes>` for details. See the :ref:`-a <options-attributes-table>` command line option for the initial setting. This preference isn't saved.

Contraction Table
  Select the contraction table. See section :ref:`Contraction Tables <table-contraction>` for details. See the :ref:`-c <options-contraction-table>` command line option for the initial setting. This preference isn't saved.

.. _preference-keyboard-table:

.. _time-settings:

Keyboard Table
  Select the keyboard table. See section :ref:`Key Tables <table-key>` for details. See the :ref:`-k <options-keyboard-table>` command line option for the initial setting. This preference isn't saved.


Notes:

- All time settings are in hundredths of a second. They are multiples of 4 within the range 1 through 100.


.. _status:

The Status Display
------------------

The status display is a summary of BRLTTY's current state
which fits completely within the braille window.
Some braille displays have a set of status cells
which are used to permanently display some of this information as well
(see the documentation for your display's driver).
The data presented by this display isn't static, and may change at any time
in response to screen updates and/or BRLTTY commands.

Use the :ref:`INFO <command-INFO>` command
to switch to the status display,
and use it again to return to the screen.
The layout of the information contained therein
is dependent on the size of the braille window.


Displays with 21 Cells or More
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Short pneumonics have been used, even though they're rather cryptic,
in order to show the precise column layout.

.. parsed-literal::

  *wx*:*wy* *cx*:*cy* *vt* *tcmfdu*

*wx*\ ``:``\ *wy*
  The column and row (counting from 1) on the screen corresponding to the top-left corner of the braille window.

*cx*\ ``:``\ *cy*
  The column and row (counting from 1) on the screen corresponding to the position of the cursor.

*vt*
  The number (counting from 1) of the current virtual terminal.

*t*
  The state of the cursor tracking feature (see the :ref:`CSRTRK <command-CSRTRK>` command).

  blank
    Cursor tracking is off.

  ``t``
    Cursor tracking is on.

*c*
  The state of the cursor visibility features (see the :ref:`CSRVIS <command-CSRVIS>` and :ref:`CSRBLINK <command-CSRBLINK>` commands).

  blank
    The cursor isn't visible, and won't blink when made visible.

  ``b``
    The cursor isn't visible, and will blink when made visible.

  ``v``
    The cursor is visible, and isn't blinking.

  ``B``
    The cursor is visible, and is blinking.

*m*
  The current display mode (see the :ref:`DISPMD <command-DISPMD>` command).

  ``t``
    Screen content (text) is being displayed.

  ``a``
    Screen highlighting (attributes) is being displayed.

*f*
  The state of the frozen screen feature (see the :ref:`FREEZE <command-FREEZE>` command).

  blank
    The screen isn't frozen.

  ``f``
    The screen is frozen.

*d*
  The number of braille dots being used to display each character (see the :ref:`SIXDOTS <command-SIXDOTS>` command).

  ``8``
    All eight dots are being used.

  ``6``
    Only dots 1 through 6 are being used.

*u*
  The state of the uppercase (capital letter) display features (see the :ref:`CAPBLINK <command-CAPBLINK>` command).

  blank
    Uppercase letters don't blink.

  ``B``
    Uppercase letters blink.


Displays with 20 Cells or Less
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Short pneumonics have been used, even though they're rather cryptic,
in order to show the precise column layout.

.. parsed-literal::

  *xx**yy**s* *vt* *tcmfdu*

*xx*
  The columns (counting from 1) on the screen corresponding to the position of the cursor (shown in the top half of the cells) and to the top-left corner of the braille window (shown in the bottom half of the cells).

*yy*
  The rows (counting from 1) on the screen corresponding to the position of the cursor (shown in the top half of the cells) and to the top-left corner of the braille window (shown in the bottom half of the cells).

*s*
  The settings of some of BRLTTY's features. A feature is turned on if its corresponding dot is raised.

  Dot 1
    Frozen screen image (see the :ref:`FREEZE <command-FREEZE>` command).

  Dot 2
    Display attributes (see the :ref:`DISPMD <command-DISPMD>` command).

  Dot 3
    Alert tunes (see the :ref:`TUNES <command-TUNES>` command).

  Dot 4
    Visible cursor (see the :ref:`CSRVIS <command-CSRVIS>` command).

  Dot 5
    Block cursor (see the :ref:`CSRSIZE <command-CSRSIZE>` command).

  Dot 6
    Blinking cursor (see the :ref:`CSRBLINK <command-CSRBLINK>` command).

  Dot 7
    Cursor tracking (see the :ref:`CSRTRK <command-CSRTRK>` command).

  Dot 8
    Sliding window (see the :ref:`SLIDEWIN <command-SLIDEWIN>` command).

*vt*
  The number (counting from 1) of the current virtual terminal.

*t*
  The state of the cursor tracking feature (see the :ref:`CSRTRK <command-CSRTRK>` command).

  blank
    Cursor tracking is off.

  ``t``
    Cursor tracking is on.

*c*
  The state of the cursor visibility features (see the :ref:`CSRVIS <command-CSRVIS>` and :ref:`CSRBLINK <command-CSRBLINK>` commands).

  blank
    The cursor isn't visible, and won't blink when made visible.

  ``b``
    The cursor isn't visible, and will blink when made visible.

  ``v``
    The cursor is visible, and isn't blinking.

  ``B``
    The cursor is visible, and is blinking.

*m*
  The current display mode (see the :ref:`DISPMD <command-DISPMD>` command).

  ``t``
    Screen content (text) is being displayed.

  ``a``
    Screen highlighting (attributes) is being displayed.

*f*
  The state of the frozen screen feature (see the :ref:`FREEZE <command-FREEZE>` command).

  blank
    The screen isn't frozen.

  ``f``
    The screen is frozen.

*d*
  The number of braille dots being used to display each character (see the :ref:`SIXDOTS <command-SIXDOTS>` command).

  ``8``
    All eight dots are being used.

  ``6``
    Only dots 1 through 6 are being used.

*u*
  The state of the uppercase (capital letter) display features (see the :ref:`CAPBLINK <command-CAPBLINK>` command).

  blank
    Uppercase letters don't blink.

  ``B``
    Uppercase letters blink.


.. _learn:
.. _command-LEARN:

Command Learn Mode
------------------

Command learn mode is an interactive way to learn
what the keys on the braille display do.
It can be accessed
either by the :ref:`LEARN <command-LEARN>` command
or via the ``brltest`` utility.
This feature isn't available if the
--disable-learn-mode build option was specified.

When this mode is entered,
the message ``command learn mode`` is written to the braille display.
Then, as each key (or key combination) on the display is pressed,
a short message describing its BRLTTY function is written.
This mode exits immediately if the key (or key combination)
for the :ref:`LEARN <command-LEARN>` command is pressed.
It exits automatically, and the message ``done`` is written,
if ten seconds elapse without any key on the display being pressed.
Note that some displays don't signal the driver
and/or some drivers don't signal BRLTTY
until all the keys are released.

If a message is longer than the braille display is wide,
then it's displayed in segments.
The length of each segment but the last is one less than the display's width,
with the rightmost character on the display being set to a minus sign.
Each such segment remains on the display
either for a few seconds
(see the :ref:`-M <options-message-timeout>` command line option)
or until any key on the display is pressed.


.. _tables:

Tables
======

BRLTTY's behaviour is customised through four kinds of plain-text
table files.
They all share the same basic syntax —
one directive per line,
``UTF-8`` encoding,
blank lines and ``#``-comment lines ignored —
and they can be split across subtables
(``*.tti``, ``*.ati``, ``*.cti``, ``*.kti``)
pulled in with an ``include`` directive.
This chapter explains what each kind of table is for
and how to select one at runtime.
For the exact directive syntax —
the reference material you need in order to write or edit a table —
see the topic-specific README file pointed to from each section.


.. _table-text:

Text Tables
-----------

Files named ``*.ttb`` are text tables.
They tell BRLTTY how to translate each character on the screen
into its corresponding 8-dot computer-braille dot pattern.
Because the mapping between characters and dots
varies from language to language
(and even between conventions within a language),
BRLTTY ships one text table per locale.

A text table is selected automatically at startup
based on your locale,
falling back to the
:ref:`North American Braille Computer Code <nabcc>`
(NABCC) table if no better match is found.
To pick one explicitly, use the
:ref:`-t <options-text-table>` command line option,
the :ref:`text-table <configure-text-table>` configuration file directive,
or the Text Table preference.

The following text tables are provided:

.. csv-table::
   :header-rows: 1
   :file: ../../text-table.csv

See ``Documents/README.TextTables`` for the text-table file format
and the directive reference.


.. _table-attributes:

Attributes Tables
-----------------

Files named ``*.atb`` are attributes tables.
Instead of showing the text on the screen,
they let you display its *visual* attributes —
foreground and background colour, intensity, blink —
as braille dot patterns.
Attributes mode is toggled on and off with the
:ref:`DISPMD <command-DISPMD>` command;
an attributes table controls how
the eight ``VGA`` attribute bits map to the eight dots of a braille cell.

The attributes tables shipped with BRLTTY are:

.. csv-table::
   :header-rows: 1
   :file: ../../attributes-table.csv

To select one, use the
:ref:`-a <options-attributes-table>` command line option,
the :ref:`attributes-table <configure-attributes-table>` configuration file directive,
or the Attributes Table preference.

See ``Documents/README.AttributesTables`` for the attributes-table
file format and the directive reference.


.. _table-contraction:

Contraction Tables
------------------

Files named ``*.ctb`` are contraction tables.
Where a text table maps one character to one braille cell,
a contraction table encodes the shorthand conventions
used by literary braille:
common letter sequences, whole words, and punctuation patterns
each map to shorter sequences of cells.
This is what lets far more reading fit on a small display.
The shorthand is historically called "grade 2" braille
(as opposed to uncontracted "grade 1");
rules differ per language,
so contraction tables, like text tables, come one per locale.

Contracted braille is displayed when both conditions hold:

- a contraction table has been selected —
  see the :ref:`-c <options-contraction-table>` command line option
  and the :ref:`contraction-table <configure-contraction-table>` configuration file directive,
  and
- 6-dot braille mode is active —
  toggle it with the :ref:`SIXDOTS <command-SIXDOTS>` command,
  or set the starting state via the
  :ref:`Text Style <preference-text-style>` preference.

Contraction support isn't compiled in
if the ``--disable-contracted-braille`` build option was used.

The following contraction tables are provided:

.. csv-table::
   :header-rows: 1
   :file: ../../contraction-table.csv

See ``Documents/README.ContractionTables`` for
the contraction-table file format,
the opcode reference,
and the character-class machinery.


.. _table-key:

Key Tables
----------

Files named ``*.ktb`` are key tables.
They bind physical keys —
on a braille display, on a standard keyboard, or on both —
to BRLTTY commands.
Most users never have to think about key tables:
each braille display driver ships with a sensible default binding
for every supported model,
so plugging in a display "just works".
A key table becomes interesting when you want to remap keys,
adapt a newer display to an older driver,
or use your computer keyboard as braille input.

There are two naming conventions:

- **Braille display key tables** have names of the form
  ``brl-``\ *xx*\ ``-``\ *model*\ ``.ktb``,
  where *xx* is the two-letter
  :ref:`driver identification code <drivers>`
  and *model* identifies the display.

- **Keyboard tables** have names of the form
  ``kbd-``\ *kind*\ ``.ktb``
  and drive the ordinary computer keyboard
  when BRLTTY is monitoring it.

The keyboard tables shipped with BRLTTY are:

.. csv-table::
   :header-rows: 1
   :file: ../../keyboard-table.csv

Specify a keyboard table with the
:ref:`-k <options-keyboard-table>` command line option,
the :ref:`keyboard-table <configure-keyboard-table>` configuration file directive,
or the :ref:`Keyboard Table <preference-keyboard-table>` preference.

See ``Documents/README.KeyTables`` for the key-table file format
and the full directive reference
(``bind``, ``context``, ``hotkey``, ``map``, ``hide``,
``superimpose``, ``assign``, ``ifkey``, ``ignore``,
``include``, ``note``, ``title``).


.. _keyboard-properties:

Keyboard Properties
~~~~~~~~~~~~~~~~~~~

By default BRLTTY monitors every keyboard it can find.
To narrow that down —
useful when several keyboards are attached
and only one is to be used for braille input —
specify one or more of the following properties
via the
:ref:`-K <options-keyboard-properties>` command line option
or the
:ref:`keyboard-properties <configure-keyboard-properties>` configuration file directive:

type
  The bus type:
  one of ``any``, ``ps2``, ``usb``, ``bluetooth``.

vendor
  The USB vendor identifier,
  a 16-bit unsigned integer.

product
  The USB product identifier,
  a 16-bit unsigned integer.

Vendor and product identifiers may be given in
decimal (no prefix),
octal (``0``-prefixed),
or hexadecimal (``0x``-prefixed).
A value of ``0`` means "match any",
equivalent to not specifying the property at all.


Supported Braille Displays
==========================

BRLTTY supports the following braille displays:

.. csv-table::
   :header-rows: 1
   :file: ../../braille-driver.csv


.. _synthesizers:

Supported Speech Synthesizers
=============================

BRLTTY supports the following speech synthesizers:

.. csv-table::
   :header-rows: 1
   :file: ../../speech-driver.csv


.. _drivers:

Driver Identification Codes
===========================


.. csv-table::
   :header-rows: 1
   :file: ../../driver-code.csv


.. _screen:

Supported Screen Drivers
========================

BRLTTY supports the following screen drivers:

as
  AT-SPI

hd
  This driver provides direct access to the Hurd console screen. It's only selectable, and is the default, on Hurd systems.

lx
  This driver provides direct access to the Linux console screen. It's only selectable, and is the default, on Linux systems.

sc
  This driver provides access to the ``screen`` program. It's selectable on all systems, and is the default if no native screen driver is available. The patch for ``screen`` which we provide (see the ``Patches`` subdirectory) must be applied. Use of this driver, due to the fact that ``screen`` must be concurrently running, makes BRLTTY effectively useful only after the user has logged in.

wn
  This driver provides direct access to the Windows console screen. It's only selectable, and is the default, on Windows/Cygwin systems.


Operand Syntax
==============


.. _operand-driver:

Driver Specification
--------------------

A braille display or speech synthesizer driver must be specified
via its two-letter :ref:`driver identification code <drivers>`.

A comma-delimited list of drivers may be specified.
If this is done then autodetection is performed using each listed driver in sequence.
You may need to experiment in order to determine the most reliable order
since some drivers autodetect better than others.

If the single word ``auto`` is specified then autodetection is performed
using only those drivers which are known to be reliable for this purpose.


.. _operand-braille-device:

Braille Device Specification
----------------------------

The general form of a braille device specification
(see the :ref:`-d <options-braille-device>` command line option,
the :ref:`braille-device <configure-braille-device>` configuration file directive,
and the ``--with-braille-device`` build option)
is ``qualifier:``\ *data*.
For backward compatibility with earlier releases,
if the qualifier is omitted then ``serial:`` is assumed.

The following device types are supported:

Bluetooth
  For a bluetooth device, specify ``bluetooth:``\ *address*. The address must be six two-digit hexadecimal numbers separated by colons, e.g. ``01:23:45:67:89:AB``.

Serial
  For a serial device, specify ``serial:``\ */path/to/device*. The ``serial:`` qualifier is optional (for backward compatibility). If a relative path is given then it's anchored at ``/dev`` (the usual location where devices are defined on a Unix-like system). The following device specifications all refer to the first serial device on Linux:

  - ``serial:/dev/ttyS0``
  - ``serial:ttyS0``
  - ``/dev/ttyS0``
  - ``ttyS0``

USB
  For a USB device, specify ``usb:``. BRLTTY will search for the first USB device which matches the braille display driver being used. If this is inadequate, e.g. if you have more than one USB braille display which requires the same driver, then you can refine the device specification by appending the serial number of the display to it, e.g. ``usb:12345``. N.B.: The "identification by serial number" feature doesn't work for some models because some manufacturers either don't set the USB serial number descriptor at all or do set it but not to a unique value.


A comma-delimited list of braille devices may be specified.
If this is done then autodetection is performed on each listed device in sequence.
This feature is particularly useful if you have
a braille display with more than one interface,
e.g. both a serial and a USB port.
In this case it's usually better to list the USB port first,
e.g. ``usb:,serial:/dev/ttyS0``,
since the former tends to autodetect more reliably than the latter.


.. _operand-pcm-device:

PCM Device Specification
------------------------

In most cases the PCM device is
the full path to an appropriate system device.
Exceptions are:

ALSA
  The name of and arguments for the physical or logical device, i.e. *name*\ [``:``\ *argument*\ ``,``\ ...].


The default PCM device is:

.. list-table::
   :header-rows: 1

   * - Platform
     - Device

   * - FreeBSD
     - /dev/dsp

   * - Linux/ALSA
     - hw:0,0

   * - Linux/OSS
     - /dev/dsp

   * - NetBSD
     - /dev/audio

   * - OpenBSD
     - /dev/audio

   * - Qnx
     - preferred PCM output device

   * - Solaris
     - /dev/audio


.. _operand-midi-device:

MIDI Device Specification
-------------------------

In most cases the MIDI device is
the full path to an appropriate system device.
Exceptions are:

ALSA
  The client and port separated by a colon, i.e. *client*\ ``:``\ *port*. Each may be specified either as a number or as a case-sensitive substring of its name.


The default MIDI device is:

.. list-table::
   :header-rows: 1

   * - Platform
     - Device

   * - Linux/ALSA
     - the first available MIDI output port

   * - Linux/OSS
     - /dev/sequencer


.. _dots:

Standard Braille Dot Numbering Convention
=========================================

A standard braille cell has six dots arranged in three rows and two
columns:

- Dot 1: top-left
- Dot 2: middle-left
- Dot 3: bottom-left
- Dot 4: top-right
- Dot 5: middle-right
- Dot 6: bottom-right

Computer braille adds a fourth row at the bottom:

- Dot 7: below-left
- Dot 8: below-right

See ``Documents/README.BrailleDots`` for a diagram and more discussion.


.. _nabcc:

North American Braille Computer Code
====================================

BRLTTY's default text table implements the North American Braille
Computer Code (NABCC), an 8-dot encoding that extends 6-dot ASCII
braille to cover the full Latin-1 range. The authoritative
character-to-dot mapping is ``Tables/Text/en-nabcc.tti`` in the BRLTTY
source tree.


.. _midi:

MIDI Instrument Table
=====================

Alert tunes played through a sound card's MIDI synthesizer use the
standard General MIDI instrument numbering, where program number 0 is
``Acoustic Grand Piano`` and program number 127 is ``Gunshot``. See
the General MIDI Level 1 specification for the full list.
