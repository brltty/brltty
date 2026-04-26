==============================
BRLTTY Reference Manual
==============================

Access to the Console Screen for Blind Persons using Refreshable Braille Displays

:Authors: Nikhil Nair `<nn201@cus.cam.ac.uk> <mailto:nn201@cus.cam.ac.uk>`__; Nicolas Pitre `<nico@fluxnic.net> <mailto:nico@fluxnic.net>`__; Stéphane Doyon `<s.doyon@videotron.ca> <mailto:s.doyon@videotron.ca>`__; Dave Mielke `<dave@mielke.cc> <mailto:dave@mielke.cc>`__
:Date: Version 6.9.1, April 2026

Copyright (c) 1995-2026 by The BRLTTY Developers. BRLTTY is free software, and comes with ABSOLUTELY NO WARRANTY. It is placed under the terms of version 2.1 or later of **The GNU Lesser General Public License** as published by **The Free Software Foundation**.


Introduction
============

BRLTTY gives a braille user access to text consoles on Linux and
similar systems. It runs as a background daemon, drives a
refreshable braille display, and reflects what would normally
appear on the screen as braille so the user can read it. It can
be started very early in the boot sequence, which keeps the system
usable in single-user mode and during boot-time recovery as well
as for ordinary day-to-day work.

BRLTTY shows a rectangular portion of the screen — referred to in
this manual as *the window* — as braille text on the display.
Buttons on the display move the window around the screen, toggle
viewing options on and off, and trigger BRLTTY's various commands.

Headline features include cursor tracking and routing, contracted
braille (English and French ship in the box; many more languages
are covered by add-on tables), screen freezing for leisurely
review, attribute review, cut-and-paste, configurable cursor and
blink styles, an interactive preferences menu, an on-line learn
mode for discovering commands, optional speech output, and a
programmable API for client applications. See :ref:`Getting
Started <getting-started>` for installation and first steps, and
:ref:`Getting Help <getting-help>` for the mailing list and issue
tracker.

Linux is the primary platform. Other platforms are supported with
varying degrees of completeness — see ``Documents/README.Android``,
``Documents/README.Windows``, and the platform-specific READMEs in
``Documents/`` for details.


.. _getting-started:

Getting Started
===============

Installing
----------

Most Linux distributions ship BRLTTY as a package. Install it the
usual way for your system — for example::

  apt install brltty                # Debian, Ubuntu, derivatives
  dnf install brltty                # Fedora, RHEL, derivatives
  zypper install brltty             # openSUSE
  pacman -S brltty                  # Arch

If you'd rather build from source, clone or unpack the tarball, run
``./configure`` (``./configure --help`` lists the build-time options),
then ``make`` and ``make install``. The installed-files layout is
described by the ``brltty-config`` shell script (see
:ref:`The brltty-config Utility <utility-brltty-config>`).

Connect your braille display before starting BRLTTY for the first
time. Serial-port displays should be plugged in and powered on first;
USB and Bluetooth displays can be hot-plugged once Udev integration
is in place (see "Starting it at boot" below).

Starting it manually
--------------------

Once installed, run BRLTTY as root::

  brltty

It reads its configuration from a file
(see :ref:`The Configuration File <configure>`)
which sets defaults such as the braille driver, the device to which
the display is connected, and the text table. Most of those defaults
can be overridden on the command line
(see :ref:`Command Line Options <options>`).

A few options come up a lot right after install:

* :ref:`-h <options-help>` lists every option.
* :ref:`-V <options-version>` prints BRLTTY's version, its API version,
  and the linked-in drivers.
* :ref:`-v <options-verify>` reports the option values BRLTTY would
  actually run with after merging the configuration file, the
  environment, and the command line — useful for confirming a setting
  before committing to it.
* :ref:`-n <options>` keeps BRLTTY in the foreground (no daemonising)
  and pairs well with :ref:`-e <options>` (log to standard error) and
  :ref:`-l debug <options>` for diagnosing startup problems.

What you should see
-------------------

A startup banner with the program name and its version appears briefly
on the display
(see the :ref:`-M <options-message-timeout>` command line option for
the timeout). The display then shows a small rectangular region of
the screen including the cursor; by default the cursor itself is
shown as dots 7 and 8 superimposed on the character it's on.

As you type, the display follows the cursor — that's **cursor
tracking**. Anything that changes on the screen is reflected on the
display in real time.

The slice of screen the display is currently showing is called the
braille **window**. The window can be moved independently of the
cursor when you want to read elsewhere on the screen (a long error
message, the line above the prompt, another part of a man page) —
that's what most BRLTTY commands do.

Finding your way around
-----------------------

Two commands are worth memorising before any of the others:

* :ref:`HELP <command-HELP>` — show one-line help for whichever key
  you press next, then return to normal operation.
* :ref:`LEARN <command-LEARN>` — read out the name of every command
  as you press its key; useful for exploring an unfamiliar key map.
  Press Enter on the keyboard to leave learn mode (see
  :ref:`Command Learn Mode <learn>` for details).

Both are toggles: invoke them again to come back. The full
:ref:`Commands <commands>` chapter groups every command by purpose;
``Documents/README.CommandReference`` is the alphabetical reference.

Starting it at boot
-------------------

You almost certainly want BRLTTY started automatically at boot so
that your display is alive by the time the login prompt appears. On
modern Linux distributions BRLTTY ships as a packaged systemd
service — install your distribution's ``brltty`` package and enable
the unit. See ``Documents/README.Systemd`` for the unit names, the
difference between the default instance and per-device instances,
and how Udev hot-plug support is wired up so that connecting a USB
display starts BRLTTY automatically.



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

This chapter describes how BRLTTY's commands, configuration, and
options work once it's running. If you haven't yet got it running,
:ref:`Getting Started <getting-started>` walks through that.

The settings most often adjusted at the command line are the braille
driver
(:ref:`-b <options-braille-driver>` /
:ref:`braille-driver <configure-braille-driver>`),
the device the display is connected to
(:ref:`-d <options-braille-device>` /
:ref:`braille-device <configure-braille-device>`),
and any driver-specific parameters
(:ref:`-B <options-braille-parameters>` /
:ref:`braille-parameters <configure-braille-parameters>`).
Most other settings have sensible defaults.

When BRLTTY is running, the slice of screen currently shown on the
display is called the **window**. Anything you type or that the
screen redraws is reflected in real time, and the window follows the
cursor (cursor tracking). To read elsewhere on the screen — a long
error message above the prompt, the line you just left in an editor,
the bottom of a man page — you move the window independently of the
cursor; that's what most BRLTTY commands do.


.. _commands:

Commands
--------

BRLTTY commands are referred to by name throughout this manual. The
key (or key combination) that triggers each one depends on your braille
display — drivers ship default bindings, which can be customised via
key tables (see the :ref:`Tables <tables>` chapter).

The two commands worth memorising first are
:ref:`HELP <command-HELP>`, which shows your driver's key map, and
:ref:`LEARN <command-LEARN>`, which lets you press keys to discover
what they do. Both toggle: invoke them again to return to normal
operation.

This chapter groups the most-used commands by purpose. For the full
alphabetical list — including modifier syntax for character-anchored
commands — see ``Documents/README.CommandReference``.


.. _vertical-motion:

Vertical Motion
~~~~~~~~~~~~~~~

.. _command-LNUP-LNDN:

LNUP/LNDN
  Up/down one line. With identical-line skipping
  (:ref:`SKPIDLNS <command-SKPIDLNS>`) on, behaves like
  :ref:`PRDIFLN/NXDIFLN <command-PRDIFLN-NXDIFLN>`.

.. _command-WINUP-WINDN:

WINUP/WINDN
  Up/down one window (five lines if the window is one line tall).

.. _command-PRDIFLN-NXDIFLN:

PRDIFLN/NXDIFLN
  Up/down to the nearest differing line.

.. _command-ATTRUP-ATTRDN:

ATTRUP/ATTRDN
  Up/down to the nearest line whose character highlighting differs.

.. _command-TOP-BOT:

TOP/BOT
  Top/bottom line.

.. _command-TOP_LEFT-BOT_LEFT:

TOP_LEFT/BOT_LEFT
  Top-left/bottom-left corner.

.. _command-PRPGRPH-NXPGRPH:

PRPGRPH/NXPGRPH
  Previous/next paragraph (the first non-blank line beyond a blank
  line).

.. _command-PRPROMPT-NXPROMPT:

PRPROMPT/NXPROMPT
  Previous/next command prompt.

.. _command-PRSEARCH-NXSEARCH:

PRSEARCH/NXSEARCH
  Search backward/forward for the clipboard contents
  (see :ref:`Cut and Paste <cut>`). Wraps at the screen edge; not
  case-sensitive.

The :ref:`PRINDENT/NXINDENT <command-PRINDENT-NXINDENT>` and
:ref:`PRDIFCHAR/NXDIFCHAR <command-PRDIFCHAR-NXDIFCHAR>` commands also
move vertically but take a column number (typically a routing key)
specifying the column to inspect.


.. _horizontal-motion:

Horizontal Motion
~~~~~~~~~~~~~~~~~

.. _command-CHRLT-CHRRT:

CHRLT/CHRRT
  Left/right one character.

.. _command-HWINLT-HWINRT:

HWINLT/HWINRT
  Left/right half a window.

.. _command-FWINLT-FWINRT:

FWINLT/FWINRT
  Left/right one window. Wraps at the screen edge, and can skip blank
  windows (see :ref:`SKPBLNKWINS <command-SKPBLNKWINS>`).

.. _command-FWINLTSKIP-FWINRTSKIP:

FWINLTSKIP/FWINRTSKIP
  Left/right to the nearest non-blank window.

.. _command-LNBEG-LNEND:

LNBEG/LNEND
  Beginning/end of the line.

The :ref:`SETLEFT <command-SETLEFT>` routing-key command also moves
horizontally, to a specific column.


.. _implicit-motion:

Implicit Motion
~~~~~~~~~~~~~~~

.. _command-HOME:

HOME
  Go to where the cursor is.

.. _command-BACK:

BACK
  Go back to where the most recent motion command put the braille
  window. Handy after cursor tracking (or another implicit move) has
  taken you away from where you were reading.

.. _command-RETURN:

RETURN
  - If the most recent motion of the braille window was automatic
    (e.g. cursor tracking), behave like
    :ref:`BACK <command-BACK>`.
  - Otherwise, if the cursor is outside the braille window, behave
    like :ref:`HOME <command-HOME>`.

The :ref:`GOTOMARK <command-GOTOMARK>` routing-key command jumps to a
position previously remembered by ``SETMARK``.


.. _feature-activation:

Feature Activation
~~~~~~~~~~~~~~~~~~

Each command in this section has three forms — **on**, **off**, and
**toggle** — bound to separate keys (or key combinations) by your key
table. Unless noted, each feature is initially off and applies
globally; the corresponding initial value can also be set in the
:ref:`preferences menu <preferences-menu>`.

.. _command-FREEZE:

FREEZE
  Freeze the screen image, so you can read at leisure even while the
  application is still writing.

.. _command-DISPMD:

DISPMD
  Show character highlighting (attributes — colour, reverse video,
  blink) instead of the characters themselves. Useful for locating a
  highlighted item. Per virtual terminal.

.. _command-SIXDOTS:

SIXDOTS
  Show characters using 6-dot rather than 8-dot braille; dots 7 and 8
  remain available for the cursor representation and attribute
  underline. If a contraction table is in effect (see the
  :ref:`-c <options-contraction-table>` option), it is used. Also
  changeable via the
  :ref:`Text Style <preference-text-style>` preference.

.. _command-SLIDEWIN:

SLIDEWIN
  When cursor tracking is on, slide the window horizontally so the
  cursor stays near the centre, instead of jumping in window-sized
  steps. Also changeable via the
  :ref:`Sliding Window <preference-sliding-window>` preference.

.. _command-SKPIDLNS:

SKPIDLNS
  Skip past lines whose content matches the current line. Affects
  :ref:`LNUP/LNDN <command-LNUP-LNDN>` and the wrap behaviour of
  :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` and
  :ref:`FWINLTSKIP/FWINRTSKIP <command-FWINLTSKIP-FWINRTSKIP>`. Also
  changeable via the
  :ref:`Skip Identical Lines <preference-skip-identical-lines>`
  preference.

.. _command-SKPBLNKWINS:

SKPBLNKWINS
  Skip past blank windows when reading. Affects
  :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>`.

.. _command-CSRVIS:

CSRVIS
  Show the cursor by superimposing a dot pattern on its character.
  Initially **on**. Also changeable via the
  :ref:`Show Cursor <preference-show-cursor>` preference.

.. _command-CSRHIDE:

CSRHIDE
  Hide the cursor (overrides :ref:`CSRVIS <command-CSRVIS>` for the
  current virtual terminal) so the underlying character can be read.

.. _command-CSRTRK:

CSRTRK
  Track the cursor: move the braille window automatically when the
  cursor leaves it. Initially **on**, per virtual terminal. If the
  cursor jumps at an inopportune moment, use
  :ref:`BACK <command-BACK>` to return.

.. _command-CSRSIZE:

CSRSIZE
  Represent the cursor as a solid block (all eight dots) rather than
  as an underline (dots 7 and 8). Also changeable via the
  :ref:`Cursor Style <preference-cursor-style>` preference.

.. _command-CSRBLINK:

CSRBLINK
  Blink the cursor representation. Also changeable via the
  :ref:`Blinking Cursor <preference-blinking-cursor>` preference.

.. _command-ATTRVIS:

ATTRVIS
  Underline highlighted characters with combinations of dots 7 and 8:

  no underline
    White on black (normal), gray on black, white on blue, black on
    cyan.

  dots 7 and 8
    Black on white (reverse video).

  dot 8
    Everything else.

  Also changeable via the
  :ref:`Show Attributes <preference-show-attributes>` preference.

.. _command-ATTRBLINK:

ATTRBLINK
  Blink the attribute underline. Initially **on**. Also changeable
  via the
  :ref:`Blinking Attributes <preference-blinking-attributes>`
  preference.

.. _command-CAPBLINK:

CAPBLINK
  Blink uppercase letters. Also changeable via the
  :ref:`Blinking Capitals <preference-blinking-capitals>` preference.

.. _command-TUNES:

TUNES
  Play a short tune (see :ref:`Alert Tunes <tunes>`) on significant
  events. Initially **on**.

.. _command-AUTOREPEAT:

AUTOREPEAT
  Automatically repeat a command at a regular interval while its key
  remains pressed. Only some drivers support this; many displays
  don't signal key release as a distinct event. Initially **on**.

.. _command-AUTOSPEAK:

AUTOSPEAK
  Automatically speak the new line on vertical motion, characters as
  they're typed or deleted, and the character to which the cursor
  moves.

.. _command-ASPK_EMP_LINE:

ASPK_EMP_LINE
  Toggle whether AUTOSPEAK announces empty lines (rather than skipping
  over them silently).


.. _mode-selection:

Mode Selection
~~~~~~~~~~~~~~

.. _command-HELP:

HELP
  Switch to the braille driver's help page — your key reference. Use
  the regular :ref:`vertical <vertical-motion>` and
  :ref:`horizontal <horizontal-motion>` motion commands to read it,
  and invoke ``HELP`` again to leave.

.. _command-INFO:

INFO
  Switch to the status display
  (see :ref:`The Status Display <status>`): cursor and window
  positions, feature states. Invoke again to leave.

.. _command-LEARN:

LEARN
  Switch to command learn mode
  (see :ref:`Command Learn Mode <learn>`). Press any key to find out
  what command it sends. Invoke ``LEARN`` again to leave. Not
  available if BRLTTY was built with ``--disable-learn-mode``.


.. _preference-maintenance:

Preference Maintenance
~~~~~~~~~~~~~~~~~~~~~~

.. _command-PREFMENU:

PREFMENU
  Switch to the preferences menu
  (see :ref:`The Preferences Menu <preferences-menu>`). Invoke again
  to leave.

.. _command-PREFSAVE:

PREFSAVE
  Save the current preferences (see :ref:`Preferences <preferences>`).

.. _command-PREFLOAD:

PREFLOAD
  Restore the most recently saved preferences.


.. _menu-navigation:

Menu Navigation
~~~~~~~~~~~~~~~

.. _command-MENU_FIRST_ITEM-MENU_LAST_ITEM:

MENU_FIRST_ITEM/MENU_LAST_ITEM
  First/last item in the menu.

.. _command-MENU_PREV_ITEM-MENU_NEXT_ITEM:

MENU_PREV_ITEM/MENU_NEXT_ITEM
  Previous/next item.

.. _command-MENU_PREV_SETTING-MENU_NEXT_SETTING:

MENU_PREV_SETTING/MENU_NEXT_SETTING
  Decrement/increment the current item's setting.


.. _speech-controls:

Speech Controls
~~~~~~~~~~~~~~~

.. _command-SAY_LINE:

SAY_LINE
  Speak the current line. The
  :ref:`Say-Line Mode <preference-sayline-mode>` preference controls
  whether pending speech is interrupted first.

.. _command-SAY_ABOVE:

SAY_ABOVE
  Speak from the top of the screen down to the current line.

.. _command-SAY_BELOW:

SAY_BELOW
  Speak from the current line down to the bottom of the screen.

.. _command-MUTE:

MUTE
  Stop speaking immediately.

.. _command-SPKHOME:

SPKHOME
  Go to where the speech cursor is.

.. _command-SAY_SLOWER-SAY_FASTER:

SAY_SLOWER/SAY_FASTER
  Decrease/increase the speech rate
  (see :ref:`Speech Rate <preference-speech-rate>`). Driver-dependent.

.. _command-SAY_SOFTER-SAY_LOUDER:

SAY_SOFTER/SAY_LOUDER
  Decrease/increase the speech volume
  (see :ref:`Speech Volume <preference-speech-volume>`).
  Driver-dependent.

.. _command-SPK_PUNCT_LEVEL:

SPK_PUNCT_LEVEL
  Cycle the punctuation level — *none*, *some*, *most*, *all* —
  controlling how many punctuation characters are spoken aloud
  (see :ref:`Punctuation Level <preference-punctuation-level>`).


.. _speech-navigation:

Speech Navigation
~~~~~~~~~~~~~~~~~

These commands move the speech cursor and speak the unit it lands on.

SPEAK_CURR_CHAR / SPEAK_PREV_CHAR / SPEAK_NEXT_CHAR
  Speak the current, previous, or next character.

SPEAK_FRST_CHAR / SPEAK_LAST_CHAR
  Speak the first or last character of the current line.

SPEAK_CURR_WORD / SPEAK_PREV_WORD / SPEAK_NEXT_WORD
  Speak the current, previous, or next word.

SPEAK_CURR_LINE / SPEAK_PREV_LINE / SPEAK_NEXT_LINE
  Speak the current, previous, or next line.

SPEAK_FRST_LINE / SPEAK_LAST_LINE
  Speak the first or last line of the screen.


Virtual Terminal Switching
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _command-SWITCHVT_PREV-SWITCHVT_NEXT:

SWITCHVT_PREV/SWITCHVT_NEXT
  Switch to the previous/next virtual terminal.

The :ref:`SWITCHVT <command-SWITCHVT>` routing-key command jumps
directly to a numbered terminal.


Other Commands
~~~~~~~~~~~~~~

.. _command-CSRJMP_VERT:

CSRJMP_VERT
  Route the cursor to the top line of the braille window
  (see :ref:`Cursor Routing <routing>`) by simulating up/down arrow
  keys. Safer than the routing-key variant in that it doesn't
  simulate horizontal arrows.

.. _command-PASTE:

PASTE
  Insert the current clipboard contents at the cursor
  (see :ref:`Cut and Paste <cut>`).

.. _command-CLIP_SHOW:

CLIP_SHOW
  Display the current clipboard contents as a message. Bound by
  default to a long press of ``CLIP_SAVE``.

.. _command-CLIP_CLEAR:

CLIP_CLEAR
  Empty the clipboard. Bound by default to a long press of
  ``CLIP_RESTORE``.

.. _command-RESTARTBRL:

RESTARTBRL
  Stop and restart the braille display driver.

.. _command-RESTARTSPEECH:

RESTARTSPEECH
  Stop and restart the speech synthesizer driver.


.. _commands-characters:

Character Commands
~~~~~~~~~~~~~~~~~~

These commands take a column number, typically supplied by pressing
one of your display's routing keys.

.. _command-ROUTE:

ROUTE
  Route the cursor to the character at the routing key
  (see :ref:`Cursor Routing <routing>`), by simulating arrow-key
  presses.

.. _command-CUTBEGIN:

CUTBEGIN
  Anchor the start of a cut block at the routing key, replacing the
  clipboard (see :ref:`Cut and Paste <cut>`).

.. _command-CUTAPPEND:

CUTAPPEND
  Like :ref:`CUTBEGIN <command-CUTBEGIN>`, but keep the existing
  clipboard contents.

.. _command-CUTRECT:

CUTRECT
  Anchor the end of the cut block at the routing key and append the
  rectangular region to the clipboard.

.. _command-CUTLINE:

CUTLINE
  As :ref:`CUTRECT <command-CUTRECT>`, but a linear (line-wrapping)
  region.

.. _command-COPYCHARS:

COPYCHARS
  Copy the character block anchored by the two routing keys to the
  clipboard.

.. _command-APNDCHARS:

APNDCHARS
  Like :ref:`COPYCHARS <command-COPYCHARS>`, but append.

.. _command-COPY_SMART_NEW:

COPY_SMART_NEW
  Detect a URL, email address, or hostname at the routing-key column
  and copy it to the clipboard, replacing previous contents. Saves
  pinpointing both ends of a long selection by hand.

.. _command-COPY_SMART_ADD:

COPY_SMART_ADD
  Like :ref:`COPY_SMART_NEW <command-COPY_SMART_NEW>`, but append.

.. _command-PRINDENT-NXINDENT:

PRINDENT/NXINDENT
  Up/down to the nearest line whose indent is at most the routing-key
  column.

.. _command-DESCCHAR:

DESCCHAR
  Briefly display a description of the character at the routing key —
  decimal and hexadecimal value, foreground/background colours, and
  attributes (``bright``, ``blink``). Example::

    char 65 (0x41): white on black bright blink

.. _command-SETLEFT:

SETLEFT
  Reposition the braille window so its left edge is at the routing-key
  column. On displays with routing keys this largely replaces piecemeal
  motion via :ref:`CHRLT/CHRRT <command-CHRLT-CHRRT>` and
  :ref:`HWINLT/HWINRT <command-HWINLT-HWINRT>`.

.. _command-PRDIFCHAR-NXDIFCHAR:

PRDIFCHAR/NXDIFCHAR
  Up/down to the nearest line whose character at the routing-key
  column differs.


.. _commands-base:

Base Commands
~~~~~~~~~~~~~

.. _command-SWITCHVT:

SWITCHVT
  Switch to the virtual terminal whose number (counting from 1)
  matches the routing key. See also
  :ref:`SWITCHVT_PREV/SWITCHVT_NEXT <command-SWITCHVT_PREV-SWITCHVT_NEXT>`.

.. _command-SETMARK:

SETMARK
  Mark the current braille-window position in a register associated
  with the routing key. Per virtual terminal.

.. _command-GOTOMARK:

GOTOMARK
  Move the braille window to the position previously
  :ref:`marked <command-SETMARK>` with the same routing key. Per
  virtual terminal.


.. _configure:

The Configuration File
----------------------

A configuration file lets you set defaults so the same options don't
have to be passed on every invocation. BRLTTY reads ``/etc/brltty.conf``
by default, or whatever file the
``-f`` option points at; the file is
optional, and BRLTTY runs fine without one as long as the necessary
settings are supplied another way.

The authoritative reference for the file's contents is
``Documents/brltty.conf`` in the source tree (also installed as
``/etc/brltty.conf`` by most distributions). Every directive is
documented there in an explanatory comment block, with examples — that
is the file most users will edit, and reading its comments is the
fastest way to learn the format.

Each directive is a keyword followed by an operand on a single line.
Blank lines are ignored, as is anything from a ``#`` to the end of a
line. Keywords are case-insensitive. The directives that come up most
often in user configurations:

.. _configure-braille-driver:

``braille-driver`` *driver*\ ``,``...|\ ``auto``
  The braille display driver (see :ref:`Driver Specification
  <operand-driver>`). ``auto`` autodetects, which is also the default.
  Overridable with :ref:`-b <options-braille-driver>`.

.. _configure-braille-device:

``braille-device`` *device*\ ``,``...
  Where the display is connected (see :ref:`Braille Device
  Specification <operand-braille-device>`).
  Overridable with :ref:`-d <options-braille-device>`.

.. _configure-braille-parameters:

``braille-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Driver-specific parameters. A name may be qualified by a driver code
  (see :ref:`Driver Identification Codes <drivers>`) so that a given
  setting only applies to that driver; otherwise it applies to all of
  them. The available parameters are listed in each driver's own
  README. Overridable with :ref:`-B <options-braille-parameters>`.

.. _configure-text-table:

``text-table`` *file*\ \|\ ``auto``
  The :ref:`text table <table-text>` (character-to-braille mapping).
  ``auto`` (the default) selects one based on the locale.
  Overridable with :ref:`-t <options-text-table>`.

.. _configure-attributes-table:

``attributes-table`` *file*
  The :ref:`attributes table <table-attributes>` used when displaying
  screen-attribute information.
  Overridable with :ref:`-a <options-attributes-table>`.

.. _configure-contraction-table:

``contraction-table`` *file*
  The :ref:`contraction table <table-contraction>` used when 6-dot
  contracted braille is active (see the :ref:`SIXDOTS
  <command-SIXDOTS>` command and the :ref:`Text Style
  <preference-text-style>` preference).
  Overridable with :ref:`-c <options-contraction-table>`.

.. _configure-keyboard-table:

``keyboard-table`` *file*\ \|\ ``auto``
  The :ref:`key table <table-key>` mapping keyboard keys to BRLTTY
  commands. The default is not to use one.
  Overridable with :ref:`-k <options-keyboard-table>`.

.. _configure-keyboard-properties:

``keyboard-properties`` *name*\ ``=``\ *value*\ ``,``...
  Which keyboards BRLTTY should monitor (see :ref:`Keyboard Properties
  <keyboard-properties>`). The default is to monitor all of them.
  Overridable with :ref:`-K <options-keyboard-properties>`.

For the directives not covered here — speech, screen, MIDI, PCM,
BrlAPI, preferences-file, release-device, and a few others — see the
comments in ``Documents/brltty.conf``.


.. _options:

Command Line Options
--------------------

The options below are the ones first-time users most often reach for.
For the full list — including platform-specific, advanced, and rarely
used options — run ``brltty --help`` or read the ``brltty(1)`` man
page.

Display, driver, and device:

.. _options-braille-driver:

``-b``\ *driver*\ ``,``...|\ ``auto`` ``--braille-driver=``\ *driver*\ ``,``...
  Braille display driver (see :ref:`Driver Specification
  <operand-driver>`). ``auto`` autodetects.

.. _options-braille-device:

``-d``\ *device*\ ``,``... ``--braille-device=``\ *device*\ ``,``...
  Where the display is connected.

.. _options-braille-parameters:

``-B``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``... ``--braille-parameters=``\ ...
  Driver-specific parameters; see each driver's README.

``-s``\ *driver*\ ``,``...|\ ``auto`` ``--speech-driver=``\ *driver*\ ``,``...
  Speech synthesizer driver, or ``auto``.

``-x``\ *driver* ``--screen-driver=``\ *driver*
  Screen driver. Default: autodetected.

Tables:

.. _options-text-table:

``-t``\ *file* ``--text-table=``\ *file*
  :ref:`Text table <table-text>`. Default: locale-based.

.. _options-attributes-table:

``-a``\ *file* ``--attributes-table=``\ *file*
  :ref:`Attributes table <table-attributes>`.

.. _options-contraction-table:

``-c``\ *file* ``--contraction-table=``\ *file*
  :ref:`Contraction table <table-contraction>` for 6-dot mode.

.. _options-keyboard-table:

``-k``\ *file* ``--keyboard-table=``\ *file*
  :ref:`Key table <table-key>` mapping keyboard keys to commands.

.. _options-keyboard-properties:

``-K``\ *name*\ ``=``\ *value*\ ``,``... ``--keyboard-properties=``\ ...
  Restrict which keyboards BRLTTY monitors (see :ref:`Keyboard
  Properties <keyboard-properties>`).

Files and runtime control:

``-f``\ *file* ``--configuration-file=``\ *file*
  Use a non-default :ref:`configuration file <configure>`.

``-n`` ``--no-daemon``
  Stay in the foreground (don't detach).

``-N`` ``--no-api``
  Disable the BrlAPI server.

.. _options-message-timeout:

``-M``\ *csecs* ``--message-timeout=``\ *csecs*
  How long BRLTTY shows its own messages on the display, in hundredths
  of a second. Default: ``400`` (4 seconds).

Logging and diagnostics:

``-l``\ *level* ``--log-level=``\ *level*
  Severity threshold for log messages. ``level`` may be a number 0-7
  (0 = emergency, 7 = debug) or the name (which may be abbreviated).
  Default: ``information``.

``-q`` ``--quiet``
  Lower the default log level. With :ref:`-v <options-verify>` or
  :ref:`-V <options-version>` it becomes ``notice``; otherwise
  ``warning``.

``-e`` ``--standard-error``
  Send diagnostic messages to standard error instead of ``syslog``.

.. _options-help:

``-h`` ``--help``
  Print the option summary and exit.

.. _options-version:

``-V`` ``--version``
  Print version information for BRLTTY, its API, and the linked-in
  drivers, then exit. With ``-q`` only the versions are printed.

.. _options-verify:

``-v`` ``--verify``
  Print the resolved option values (after merging defaults, the
  configuration file, the command line, and — if :ref:`-E
  <options-environment-variables>` is in effect — environment
  variables), then exit. With multiple drivers or devices specified,
  performs autodetection as part of verification.

.. _options-environment-variables:

``-E`` ``--environment-variables``
  Default any unspecified option from a ``BRLTTY_*`` environment
  variable. The variable name for an option is its long name, in upper
  case, with hyphens replaced by underscores and a ``BRLTTY_`` prefix
  (e.g., ``--braille-device`` is read from ``BRLTTY_BRAILLE_DEVICE``).
  Useful for passing settings via Linux kernel boot parameters.


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
The file doesn't need to exist, and is created the first time the settings
are saved with the :ref:`PREFSAVE <command-PREFSAVE>` command. The most
recently saved settings can be restored at any time with the
:ref:`PREFLOAD <command-PREFLOAD>` command.

The file is named ``/etc/brltty-``\ *driver*\ ``.prefs``, where *driver* is
the two-letter :ref:`driver identification code <drivers>`.


.. _preferences-menu:

The Preferences Menu
~~~~~~~~~~~~~~~~~~~~

Preferences are saved as binary data, so they can't be edited by hand —
BRLTTY provides a built-in menu instead. The menu is activated by the
:ref:`PREFMENU <command-PREFMENU>` command; invoke it again to leave the
new settings in effect, exit the menu, and resume normal operation.
``PREFLOAD`` undoes any changes made since entering the menu. See
:ref:`Menu Navigation Commands <menu-navigation>` for the full set of
keys that select items and adjust settings; the routing keys can also
be used to pick a setting directly.

The menu is self-documenting and is the authoritative list of
available preferences — items not listed below behave as their menu
name implies. The remainder of this section covers the preferences
that the rest of this manual cross-references, the new ones added
since BRLTTY 6.5, and a handful that new users typically adjust early
on.


Selected Preferences
~~~~~~~~~~~~~~~~~~~~

.. _preference-text-style:

Text Style
  Display screen content using all eight dots (``8-dot``) or only dots
  1 through 6 (``6-dot``). When 6-dot mode is in effect and a
  contraction table has been selected, contracted braille is shown.
  Also changeable via the :ref:`SIXDOTS <command-SIXDOTS>` command.

.. _preference-skip-identical-lines:

Skip Identical Lines
  When moving up or down by one line, skip past lines whose content
  matches the current line. Also changeable via the
  :ref:`SKPIDLNS <command-SKPIDLNS>` command.

.. _preference-sliding-window:

Sliding Window
  When cursor tracking would otherwise push the cursor off the edge
  of the braille window, slide the window so the cursor stays nearer
  the centre instead of jumping by full window widths. Also
  changeable via the :ref:`SLIDEWIN <command-SLIDEWIN>` command.

.. _preference-show-cursor:

Show Cursor
  Whether to show the screen cursor on the braille display. Also
  changeable via the :ref:`CSRVIS <command-CSRVIS>` command.

.. _preference-cursor-style:

Cursor Style
  Represent the cursor with all eight dots (a solid block) or with
  just dots 7 and 8 (an underline). Also changeable via the
  :ref:`CSRSIZE <command-CSRSIZE>` command.

.. _preference-blinking-cursor:

Blinking Cursor
  Make the cursor alternately visible and invisible at a fixed rate.
  Also changeable via the :ref:`CSRBLINK <command-CSRBLINK>` command.

.. _preference-show-attributes:

Show Attributes
  Underline characters that have screen highlighting (bold, reverse,
  etc.). Also changeable via the :ref:`ATTRVIS <command-ATTRVIS>`
  command.

.. _preference-blinking-attributes:

Blinking Attributes
  Make the attribute underline blink. Also changeable via the
  :ref:`ATTRBLINK <command-ATTRBLINK>` command.

.. _preference-blinking-capitals:

Blinking Capitals
  Make capital letters blink so they stand out. Also changeable via
  the :ref:`CAPBLINK <command-CAPBLINK>` command.

Autorepeat
  While the key combination for an autorepeatable command remains
  pressed, repeat the command at a regular interval after an initial
  delay. Whether key-release events are reliable enough to support
  this depends on the braille display driver. Also changeable via the
  :ref:`AUTOREPEAT <command-AUTOREPEAT>` command.

.. _preference-window-follows-pointer:

Window Follows Pointer
  When the mouse pointer moves, drag the braille window along with
  it. Only available when GPM support is built in.

.. _preference-tune-device:

Tune Device
  Which audio device to play alert tunes through: the internal beeper,
  PCM (the sound card's digital audio interface), MIDI, or FM
  synthesis. Separate volume sub-settings are available for PCM, MIDI,
  and FM. The default is ``Beeper`` where supported, otherwise ``PCM``.

.. _preference-alert-dots:

Alert Dots
  When a significant event has an associated dot pattern (see
  :ref:`Alert Tunes <tunes>`), briefly display the pattern on every
  braille cell. A separate *Alert Dots Duration* setting (added after
  BRLTTY 6.5; default 0.4 seconds) controls how long the pattern is
  held — useful for displays whose actuators respond slowly.
  Suppressed if a tune fires for the same event.

.. _preference-alert-messages:

Alert Messages
  When a significant event has an associated message (see
  :ref:`Alert Tunes <tunes>`), display it on the braille display for
  a few seconds (see the :ref:`-M <options-message-timeout>` command
  line option). Suppressed if a tune or alert dots fire for the same
  event.

.. _preference-sayline-mode:

Say-Line Mode
  When the :ref:`SAY_LINE <command-SAY_LINE>` command runs, either
  discard pending speech (``Immediate``, the default) or queue the
  new line behind it (``Enqueue``).

Autospeak
  When enabled, automatically speak the new line when the braille
  window is moved vertically, characters as they're entered or
  deleted, and the character the cursor is moved to. Also changeable
  via the :ref:`AUTOSPEAK <command-AUTOSPEAK>` command.

.. _preference-speech-rate:

Speech Rate
  Adjust the speech rate (``0`` slowest, ``20`` fastest).
  Driver-dependent; also changeable via the
  :ref:`SAY_SLOWER/SAY_FASTER <command-SAY_SLOWER-SAY_FASTER>` commands.

.. _preference-speech-volume:

Speech Volume
  Adjust the speech volume (``0`` softest, ``20`` loudest).
  Driver-dependent; also changeable via the
  :ref:`SAY_SOFTER/SAY_LOUDER <command-SAY_SOFTER-SAY_LOUDER>` commands.

.. _preference-punctuation-level:

Punctuation Level
  How much punctuation the speech synthesizer reads aloud. The
  available levels are ``None``, ``Some``, ``Most``, and ``All`` —
  the ``Most`` level (added after BRLTTY 6.5) sits between ``Some``
  and ``All`` for users who want most punctuation but find ``All``
  too noisy. Driver-dependent.

Speak Empty Line
  When set to ``Yes`` (the default since being added after BRLTTY
  6.5), announce when the cursor reaches an empty line so the line
  break is audible. Set to ``No`` to skip empty lines silently.

.. _preference-text-table:

Text Table
  Select the text table at runtime. See :ref:`Text Tables <table-text>`
  and the :ref:`-t <options-text-table>` command line option. This
  preference isn't saved.

.. _preference-attributes-table:

Attributes Table
  Select the attributes table at runtime. See
  :ref:`Attributes Tables <table-attributes>` and the
  :ref:`-a <options-attributes-table>` command line option. This
  preference isn't saved.

Contraction Table
  Select the contraction table at runtime. See
  :ref:`Contraction Tables <table-contraction>` and the
  :ref:`-c <options-contraction-table>` command line option. This
  preference isn't saved.

.. _preference-keyboard-table:

Keyboard Table
  Select the keyboard table at runtime. See :ref:`Key Tables <table-key>`
  and the :ref:`-k <options-keyboard-table>` command line option. This
  preference isn't saved.


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


.. _getting-help:

Getting Help
============

Mailing list
------------

The fastest way to get a question answered is the project's mailing
list. Post to `<BRLTTY@brltty.app> <mailto:BRLTTY@brltty.app>`_; if
you're not subscribed, your post will be held briefly for moderator
approval. To subscribe, browse the archives, or change settings, go
to `http://brltty.app/mailman/listinfo/brltty
<http://brltty.app/mailman/listinfo/brltty>`_.

Bug reports and feature requests
--------------------------------

The issue tracker is on GitHub at
`https://github.com/brltty/brltty/issues
<https://github.com/brltty/brltty/issues>`_. A useful bug report
includes:

* The BRLTTY version (``brltty -V``).
* The braille display make and model.
* The host operating system and its version.
* The configuration file in use, if non-default.
* Relevant log output. Re-running BRLTTY with
  ``-n -e -l debug`` keeps it in the foreground and emits detailed
  diagnostics to standard error — paste the output that surrounds the
  failure.

Reading the source documentation
--------------------------------

For implementation questions or driver-specific behaviour the source
tree carries a family of topic READMEs in ``Documents/``: ``Bluetooth``,
``Devices``, ``Customize``, ``Profiles``, ``Polling``, ``Systemd``,
``X11``, ``CommandReference``, ``TextTables``, ``AttributesTables``,
``ContractionTables``, ``KeyTables``, ``BrailleDots``, and others.
``Documents/brltty.conf`` is the heavily-commented configuration
template — the fastest reference for any directive's syntax. The
BrlAPI manual covers the application interface separately.


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
(see the :ref:`-d <options-braille-device>` command line option
and the :ref:`braille-device <configure-braille-device>` configuration file directive)
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


Formalities
===========


License
-------

BRLTTY is free software, distributed under the terms of
`The GNU Lesser General Public License
<http://www.gnu.org/licenses/licenses.html#LGPL>`_, version 2.1 or
later. It comes with **absolutely no warranty** — not even the
implied warranty of merchantability or fitness for a particular
purpose. The full license text ships in ``LICENSE-LGPL`` at the top
of the source tree.


Authors
-------

BRLTTY started in the early 1990s as the work of Nikhil Nair,
Nicolas Pitre, and Stéphane Doyon, and is currently maintained by
Dave Mielke. The active team list, contributor history, and project
news live on the project's home page at
`brltty.app <http://brltty.app/>`_.
