
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


System Requirements
-------------------

To date, BRLTTY runs under Linux, Solaris, OpenBSD, FreeBSD, NetBSD, and Windows.
While ports to other Unix-like operating systems aren't currently planned,
we do welcome any interest in such projects.

Linux
  This software has been tested on a variety of Linux systems:

  - Desktops, laptops, and some PDAs.
  - Processors from a 386SX20 to a Pentium.
  - A huge range of memory sizes.
  - Several distributions including Debian, Red Hat, Slackware, and SuSE.
  - Many kernels, including 1.2.13, 2.0, 2.2, and 2.4.

Solaris
  This software has been tested on the following Solaris systems:

  - The Sparc architecture (releases 7, 8, and 9).
  - The Intel architecture (release 9).

OpenBSD
  This software has been tested on the following OpenBSD systems:

  - The Intel architecture (release 3.4).

FreeBSD
  This software has been tested on the following FreeBSD systems:

  - The Intel architecture (release 5.1).

NetBSD
  This software has been tested on the following NetBSD systems:

  - The Intel architecture (release 1.6).

.. _preference-skip-blank-windows:

Windows
  This software has been tested on Windows 95, 98, and XP.


On Linux, BRLTTY can inspect the content of the screen
completely independently of any logged in user.
It does this by using a special device
which provides easy access to the contents of the current virtual console.
This device was introduced in version 1.1.92 of the Linux kernel,
and is normally called either ``/dev/vcsa`` or ``/dev/vcsa0``
(on systems with ``devfs`` it's called ``/dev/vcc/a``).
For this reason, Linux kernel 1.1.92 or later is required
if BRLTTY is to be used in this way.
This capability:

- Allows BRLTTY to be started very early in the system boot sequence.
- Enables the braille display to be fully operational during the login prompt.
- Makes it much easier for a braille user to perform boot-time system administration tasks.


A patch for the ``screen`` program is provided
(see the ``Patches`` subdirectory).
It allows BRLTTY to access ``screen``'s screen image via shared memory,
and, therefore, allows BRLTTY to be used quite effectively on platforms
which don't have their own screen content inspection facility.
The main weakness of the ``screen`` approach is that
BRLTTY can't be started until the user has logged in.

BRLTTY only works with text-based consoles and applications.
It can be used with ``curses``-based applications,
but not with any application which
either uses special VGA features
or requires a graphics console (like the X Window system).

You must also, of course, possess a supported refreshable braille display
(see section :ref:`Supported Braille Displays <displays>` for the complete list).
We hope that additional displays will be supported in the future, so,
if you have any vaguely technical programming information
for a device which you'd like to see supported,
then please let us know (see section :ref:`Contact Information <contact>`).

Finally, you need tools to build the executable from its source:
``make``, ``C`` and ``C++`` compilers, ``yacc``, ``awk``, etc.
The development tools provided with standard Unix distributions should suffice.
If you have problems,
then contact us and we'll compile a binary for you.


The Build Procedure
===================

BRLTTY can be downloaded from its web site
(see section :ref:`Contact Information <contact>` for its location).
All releases are provided as compressed :ref:`tar balls <tar>`.
Newer releases are also provided as :ref:`RPM <rpm>` (RedHat Package Manager) files.

That tidbit of information has probably peaked your curiosity,
and now you just can't wait to get started.
It's a good idea, though,
to first become familiar with the files which will ultimately be installed.


.. _hierarchy:

Installed File Hierarchy
------------------------

The build procedure should result in the installation of the following files:
/bin/

  brltty
    The BRLTTY program.

  :ref:`brltty-install <utility-brltty-install>`
    A utility for copying BRLTTY's :ref:`installed file hierarchy <hierarchy>` from one location to another.

  :ref:`brltty-config <utility-brltty-config>`
    A utility which sets a number of environment variables to values which reflect the current installation of BRLTTY.

/lib/

  libbrlapi.a
    Static archive of the Application Programming Interface.

  libbrlapi.so
    Dynamically loadable object for the Application Programming Interface.

/lib/brltty/
  Your installation of BRLTTY may not have all of the following types of files. They're only created as needed based on the build options you select (see :ref:`Build Options <build>`).

  brltty-brl.lst
    A list of the braille display drivers which have been built as dynamically loadable shared objects, and, therefore, which can be selected at run-time. Each line consists of the two-letter identification code for a driver, a tab character, and a description of the braille display which that driver is for.

  libbrlttyb*driver*.so.1
    The dynamically loadable driver for a braille display, where *driver* is the two-letter :ref:`driver identification code <drivers>`.

  brltty-spk.lst
    A list of the speech synthesizer drivers which have been built as dynamically loadable shared objects, and, therefore, which can be selected at run-time. Each line consists of the two-letter identification code for a driver, a tab character, and a description of the speech synthesizer which that driver is for.

  libbrlttys*driver*.so.1
    The dynamically loadable driver for a speech synthesizer, where *driver* is the two-letter :ref:`driver identification code <drivers>`.

/lib/brltty/rw/
  Files created at run-time, e.g. needed but missing system resources.

/etc/

  brltty.conf
    System defaults for BRLTTY.

  brlapi.key
    The access key for BrlAPI.

/etc/brltty/
  Your installation of BRLTTY may not have all of the following types of files. They're only created as needed based on the build options you select (see :ref:`Build Options <build>`).

  \*.conf
    Driver-specific configuration data. Their names look more or less like ``brltty-``\ *driver*\ ``.conf``, where *driver* is the two-letter :ref:`driver identification code <drivers>`.

  \*.atb
    Attributes tables (see section :ref:`Attributes Tables <table-attributes>` for details). Their names look like *name*\ ``.atb``.

  \*.ati
    Include files for attributes tables.

  \*.ctb
    Contraction tables (see section :ref:`Contraction Tables <table-contraction>` for details). Their names look like *language*\ ``-``\ *country*\ ``-``\ *level*\ ``.ctb``.

  \*.cti
    Include files for contraction tables.

  \*.ktb
    Key tables (see section :ref:`Key Tables <table-key>` for details). Their names look like *name*\ ``.ktb``.

  \*.kti
    Include files for key tables.

  \*.ttb
    Text tables (see section :ref:`Text Tables <table-text>` for details). Their names look like *language*\ ``.ttb``.

  \*.tti
    Include files for text tables.

  \*.hlp
    Driver-specific help pages. Their names look more or less like ``brltty-``\ *driver*\ ``.hlp``, where *driver* is the two-letter :ref:`driver identification code <drivers>`.

/var/lib/BrlAPI/
  Local sockets for connecting to the Application Programming Interface.

/include/
  C header files for the Application Programming Interface. Their names look like ``brlapi-``\ *function*\ ``.h``. The main header is ``brlapi.h``.

/include/brltty/
  C header files for accessing braille hardware. Their names look like ``brldefs-``\ *driver*\ ``.h`` (where *driver* is the two-letter :ref:`driver identification code <drivers>`). The headers ``brldefs.h`` and ``api.h`` are provided for backward compatibility and shouldn't be used.

/man/
  Man pages.

  man1/*name*.1
    Man pages for BRLTTY-related user commands.

  man3/*name*.3
    Man pages for Application Programming Interface library routines.


Some optional files which you should be aware of,
although they aren't part of the installed file hierarchy, are:

/etc/brltty.conf
  The system defaults configuration file. It's created by the system administrator. See :ref:`The Configuration File <configure>` for details.

/etc/brltty-*driver*.prefs
  The saved preferences settings file (*driver* is a two-letter :ref:`driver identification code <drivers>`). It's created by the :ref:`PREFSAVE <command-PREFSAVE>` command. See :ref:`Preferences Settings <preferences>` for details.


.. _tar:

Installing from a TAR Ball
--------------------------

Here's what to do if you just want to install BRLTTY as quickly as possible,
trusting that all of our defaults are correct.
#. Download the source. It'll be a file named ``brltty-``\ *release*\ ``.tar.gz``, e.g. ``brltty-3.0.tar.gz``.
#. Unpack the source into its native hierarchical structure.


   .. parsed-literal::

     tar xzf brltty-*release*.tar.gz

#. This should create the directory ``brltty-``\ *release*.
#. Change to the source directory, configure, compile, and install BRLTTY.


   .. parsed-literal::

     cd brltty-*release*
     ./configure
     make install

#. This should be done as **root**.


To uninstall BRLTTY, do:

.. parsed-literal::

  cd brltty-*release*
  make uninstall


That's all there's to it.
Now, for those who really want to know what's going on, here are the details.


.. _build:

Build Options
~~~~~~~~~~~~~

The first step in building BRLTTY is to configure it
for your system and/or for your personal needs.
This is done by running the ``configure`` script in BRLTTY's top-level directory.
We've tried to make the defaults fit the most common case, so,
assuming that you're not attempting to do anything out of the ordinary,
you may not need to do anything more complicated than
invoke this script without specifying any options at all.

.. parsed-literal::

  ./configure

If, however, you have some special requirements,
or even if you're just adventurous,
you should find out what your choices are.

.. parsed-literal::

  ./configure --help

You should also check out the ``README`` file in the subdirectory
containing the driver for your braille display
for any additional display-specific instructions.


.. _build-defaults:

System Defaults
^^^^^^^^^^^^^^^

.. _build-braille-driver:

``--with-braille-driver=``\ *driver*
  Specify the braille display drivers which are to be linked into the BRLTTY binary. Those drivers which aren't listed via this option are built as dynamically loadable shared objects and can still be selected at run-time. Each driver must be identified either by its two-letter :ref:`driver identification code <drivers>` or by its proper name (full or abbreviated). The driver identifiers must be separated from one another by a single comma. If a driver identifier is prefixed by a minus sign (``-``), then that driver is excluded from the build. Any of the following words can also be used as the operand of this option:

  all
    Link all of the drivers into the binary. Don't build any of them as dynamically loadable shared objects. This word may also be specified as the final element of a driver list. This is how to specify the default driver when all the drivers are to be linked in.

  -all
    Only build those drivers which have been explicitly included via this option.

  no
    Don't build any drivers at all. This is equivalent to specifying ``--without-braille-driver``.

  yes
    Build all of the drivers as dynamically loadable shared objects. Don't link any of them into the binary. This is equivalent to specifying ``--with-braille-driver``.

  See the :ref:`braille-driver <configure-braille-driver>` configuration file directive and the :ref:`-b <options-braille-driver>` command line option for run-time selection.

.. _build-braille-parameters:

``--with-braille-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify the default parameter settings for the braille display drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`braille-parameters <configure-braille-parameters>` configuration file directive and the :ref:`-B <options-braille-parameters>` command line option for run-time selection.

.. _build-braille-device:

``--with-braille-device=``\ *device*\ ``,``...
  Specify the default device to which the braille display is connected (see section :ref:`Braille Device Specification <operand-braille-device>`). If this option isn't specified then ``usb:`` is assumed if USB support is available, ``bluetooth:`` is assumed if Bluetooth support is available, and ``usb:,bluetooth:`` is assumed if both are available. If neither USB nor Bluetooth support is available then an operating system appropriate path for the primary (first) serial port (device) is assumed. See the :ref:`braille-device <configure-braille-device>` configuration file directive and the :ref:`-d <options-braille-device>` command line option for run-time selection.

.. _build-libbraille:

``--with-libbraille=``\ *directory*
  Specify the installed location of the Libbraille package, and build the Libbraille braille display driver (see :ref:`Build Restrictions <restrictions-libbraille>`). Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-libbraille``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/Libbraille``, ``/usr/local/libbraille``, ``/opt/Libbraille``, or ``/opt/libbraille``. This is equivalent to specifying ``--with-libbraille``.

.. _build-text-table:

``--with-text-table=``\ *file*
  Specify the built-in (fallback) text table (see section :ref:`Text Tables <table-text>` for details). The specified table is linked into the BRLTTY binary, and is used either if locale-based autoselection fails or if the requested table can't be loaded. The absolute path to a table outside the source tree may be specified. The ``.ttb`` extension is optional. If this option isn't specified, then ``en-nabcc``, a commonly (in North America) used 8-dot variant of the :ref:`North American Braille Computer Code <nabcc>`, is assumed. See the :ref:`text-table <configure-text-table>` configuration file directive and the :ref:`-t <options-text-table>` command line option for run-time selection. This setting can be changed with the :ref:`Text Table <preference-text-table>` preference.

.. _build-attributes-table:

``--with-attributes-table=``\ *file*
  Specify the built-in (fallback) attributes table (see section :ref:`Attributes Translation <table-attributes>` for details). The specified table is linked into the BRLTTY binary, and is used if the requested table can't be loaded. The absolute path to a table outside the source tree may be specified. The ``.atb`` extension is optional. If this option isn't specified, then ``left_right`` is assumed. Change it to ``invleft_right`` if you'd like it done the old way. See the :ref:`attributes-table <configure-attributes-table>` configuration file directive and the :ref:`-a <options-attributes-table>` command line option for run-time selection. This setting can be changed with the :ref:`Attributes Table <preference-attributes-table>` preference.

.. _build-speech-driver:

``--with-speech-driver=``\ *driver*
  Specify the speech synthesizer drivers which are to be linked into the BRLTTY binary. Those drivers which aren't listed via this option are built as dynamically loadable shared objects and can still be selected at run-time. Each driver must be identified either by its two-letter :ref:`driver identification code <drivers>` or by its proper name (full or abbreviated). The driver identifiers must be separated from one another by a single comma. If a driver identifier is prefixed by a minus sign (``-``), then that driver is excluded from the build. Any of the following words can also be used as the operand of this option:

  all
    Link all of the drivers into the binary. Don't build any of them as dynamically loadable shared objects. This word may also be specified as the final element of a driver list. This is how to specify the default driver when all the drivers are to be linked in.

  -all
    Only build those drivers which have been explicitly included via this option.

  no
    Don't build any drivers at all. This is equivalent to specifying ``--without-speech-driver``.

  yes
    Build all of the drivers as dynamically loadable shared objects. Don't link any of them into the binary. This is equivalent to specifying ``--with-speech-driver``.

  See the :ref:`speech-driver <configure-speech-driver>` configuration file directive and the :ref:`-s <options-speech-driver>` command line option for run-time selection.

.. _build-speech-parameters:

``--with-speech-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify the default parameter settings for the speech synthesizer drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`speech-parameters <configure-speech-parameters>` configuration file directive and the :ref:`-S <options-speech-parameters>` command line option for run-time selection.

.. _build-flite:

``--with-flite=``\ *directory*
  Specify the installed location of the FestivalLite text-to-speech package, and build the FestivalLite speech synthesizer driver (see :ref:`Build Restrictions <restrictions-flite>`). Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-flite``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/FestivalLite``, ``/usr/local/flite``, ``/opt/FestivalLite``, or ``/opt/flite``. This is equivalent to specifying ``--with-flite``.

.. _build-flite-language:

``--with-flite-language=``\ *language*
  Specify the language which the FestivalLite text to speech engine is to use. The default language is ``usenglish``.

.. _build-flite-lexicon:

``--with-flite-lexicon=``\ *lexicon*
  Specify the lexicon which the FestivalLite text to speech engine is to use. The default lexicon is ``cmulex``.

.. _build-flite-voice:

``--with-flite-voice=``\ *voice*
  Specify the voice which the FestivalLite text to speech engine is to use. The default voice is ``cmu_us_kal16``.

.. _build-mikropuhe:

``--with-mikropuhe=``\ *directory*
  Specify the installed location of the Mikropuhe text-to-speech package, and build the Mikropuhe speech synthesizer driver (see :ref:`Build Restrictions <restrictions-mikropuhe>`). Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-mikropuhe``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/Mikropuhe``, ``/usr/local/mikropuhe``, ``/opt/Mikropuhe``, or ``/opt/mikropuhe``. This is equivalent to specifying ``--with-mikropuhe``.

.. _build-speechd:

``--with-speechd=``\ *directory*
  Specify the installed location of the speech-dispatcher text-to-speech package, and build the speech-dispatcher speech synthesizer driver. Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-speechd``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/speech-dispatcher``, ``/usr/local/speechd``, ``/opt/speech-dispatcher``, or ``/opt/speechd``. This is equivalent to specifying ``--with-speechd``.

.. _build-swift:

``--with-swift=``\ *directory*
  Specify the installed location of the Swift text-to-speech package, and build the Swift speech synthesizer driver. Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-swift``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/Swift``, ``/usr/local/swift``, ``/opt/Swift``, or ``/opt/swift``. This is equivalent to specifying ``--with-swift``.

.. _build-theta:

``--with-theta=``\ *directory*
  Specify the installed location of the Theta text-to-speech package, and build the Theta speech synthesizer driver (see :ref:`Build Restrictions <restrictions-theta>`). Any of the following words can also be used as the operand of this option:

  no
    Don't build the driver. This is equivalent to specifying ``--without-theta``.

  yes
    Build the driver if the package can be found in ``/usr``, ``/usr/local``, ``/usr/local/Theta``, ``/usr/local/theta``, ``/opt/Theta``, or ``/opt/theta``. This is equivalent to specifying ``--with-theta``.

.. _build-screen-driver:

``--with-screen-driver=``\ *driver*
  Specify the screen drivers which are to be linked into the BRLTTY binary. Those drivers which aren't listed via this option are built as dynamically loadable shared objects and can still be selected at run-time. Each driver must be identified either by its two-letter driver identification code (see section :ref:`Supported Screen Drivers <screen>`) or by its proper name (full or abbreviated). The driver identifiers must be separated from one another by a single comma. If a driver identifier is prefixed by a minus sign (``-``), then that driver is excluded from the build. Any of the following words can also be used as the operand of this option:

  all
    Link all of the drivers into the binary. Don't build any of them as dynamically loadable shared objects. This word may also be specified as the final element of a driver list. This is how to specify the default driver when all the drivers are to be linked in.

  -all
    Only build those drivers which have been explicitly included via this option.

  no
    Don't build any drivers at all. This is equivalent to specifying ``--without-screen-driver``.

  yes
    Build all of the drivers as dynamically loadable shared objects. Don't link any of them into the binary. This is equivalent to specifying ``--with-screen-driver``.

  The first non-excluded driver becomes the default driver. If this option isn't specified, or if no driver is specifically included, then an operating system appropriate default is selected. If a native driver for the current operating system is available, then that driver is selected; if not, then ``sc`` is selected. See the :ref:`screen-driver <configure-screen-driver>` configuration file directive and the :ref:`-x <options-screen-driver>` command line option for run-time selection.

.. _build-screen-parameters:

``--with-screen-parameters=``\ [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify the default parameter settings for the screen drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Supported Screen Drivers <screen>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`screen-parameters <configure-screen-parameters>` configuration file directive and the :ref:`-X <options-screen-parameters>` command line option for run-time selection.

.. _build-usb-package:

``--with-usb-package=``\ *package*\ ``,``...
  Specify the package which is to be used for USB I/O. The package names must be separated from one another by a single comma, and are processed from left to right. The first one which is installed on the system is selected. The following packages are supported:

  #. libusb
  #. libusb-1.0

  Any of the following words can also be used as the operand of this option:

  no
    Don't support USB I/O. This is equivalent to specifying ``--without-usb-package``.

  yes
    Use native support for USB I/O. If native support isn't available for the current platform then use the first available supported package (as per the order specified above). This is equivalent to specifying ``--with-usb-package``.

.. _build-bluetooth-package:

``--with-bluetooth-package=``\ *package*\ ``,``...
  Specify the package which is to be used for Bluetooth I/O. The package names must be separated from one another by a single comma, and are processed from left to right. The first one which is installed on the system is selected. The following packages are supported:

  #. (no packages are currently supported)

  Any of the following words can also be used as the operand of this option:

  no
    Don't support Bluetooth I/O. This is equivalent to specifying ``--without-bluetooth-package``.

  yes
    Use native support for Bluetooth I/O. If native support isn't available for the current platform then use the first available supported package (as per the order specified above). This is equivalent to specifying ``--with-bluetooth-package``.


.. _build-directoreis:

Directory Specification
^^^^^^^^^^^^^^^^^^^^^^^

.. _build-execute-root:

``--with-execute-root=``\ *directory*
  Specify the directory at which the :ref:`installed file hierarchy <hierarchy>` is to be rooted at run-time. The absolute path should be supplied. If this option isn't specified, then the system's root directory is assumed. Use this option if you need to install BRLTTY's run-time files in a non-standard location. You need to use this feature, for example, if you'd like to have more than one version of BRLTTY installed at the same time (see section :ref:`Installing Multiple Versions <multiple>` for an example of how to do this).

.. _build-install-root:

``--with-install-root=``\ *directory*
  Specify the directory beneath which the :ref:`installed file hierarchy <hierarchy>` is to be installed. The absolute path should be supplied. If this option isn't specified, then the run-time package root (see the :ref:`--with-execute-root <build-execute-root>` build option) is assumed. This directory is only used by :ref:`make install <make-install>` and :ref:`make uninstall <make-uninstall>`. Use this option if you need to install BRLTTY in a different location than the one from which it'll ultimately be executed. You need to use this feature, for example, if you're building BRLTTY on one system for use on another.

.. _build-portable-root:

``--prefix=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the default directories for the architecture-independent files are to be rooted. These directories include:

  - the :ref:`writable directory <build-writable-directory>`
  - the :ref:`data directory <build-data-directory>`
  - the :ref:`configuration directory <build-configuration-directory>`
  - the :ref:`manpage directory <build-manpage-directory>`
  - the :ref:`include directory <build-include-directory>`

  The absolute path should be supplied. If this option isn't specified, then the system's root directory is assumed. This directory is rooted at the directory specified by the :ref:`--with-execute-root <build-execute-root>` build option.

.. _build-architecture-root:

``--exec-prefix=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the default directories for the architecture-dependent files are to be rooted. These directories include:

  - the :ref:`program directory <build-program-directory>`
  - the :ref:`library directory <build-library-directory>`
  - the :ref:`API directory <build-api-directory>`

  The absolute path should be supplied. If this option isn't specified, then the directory specified via the :ref:`--prefix <build-portable-root>` build option is assumed. This directory is rooted at the directory specified by the :ref:`--with-execute-root <build-execute-root>` build option.

.. _build-api-directory:

``--libdir=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the static archive and the dynamically loadable object for the Application Programming Interface are to be installed. The absolute path should be supplied. If this option isn't specified, then the directory specified via the standard configure option ``--libdir`` (which defaults to ``/lib`` rooted at the directory specified by the :ref:`--exec-prefix <build-architecture-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-configuration-directory:

``--sysconfdir=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the configuration files are to be installed. The absolute path should be supplied. If this option isn't specified, then the directory specified via the standard configure option ``--sysconfdir`` (which defaults to ``/etc`` rooted at the directory specified by the :ref:`--prefix <build-portable-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-program-directory:

``--with-program-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the runnable programs (binaries, executables) are to be installed. The absolute path should be supplied. If this option isn't specified, then the directory specified via the standard configure option ``--bindir`` (which defaults to ``/bin`` rooted at the directory specified by the :ref:`--exec-prefix <build-architecture-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-library-directory:

``--with-library-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the drivers and other architecture-dependent files are to be installed. The absolute path should be supplied. If this option isn't specified, then the ``brltty`` subdirectory of the directory specified via the standard configure option ``--libdir`` (which defaults to ``/lib`` rooted at the directory specified by the :ref:`--exec-prefix <build-architecture-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-writable-directory:

``--with-writable-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` which may be written to. The absolute path should be supplied. Any of the following words can also be used as the operand of this option:

  no
    Don't define a writable directory. This is equivalent to specifying ``--without-writable-directory``.

  yes
    Use the default location. This is equivalent to specifying ``--with-writable-directory``.

  If this option isn't specified, then the ``rw`` subdirectory of the directory specified via the :ref:`--with-library-directory <build-library-directory>` build option is assumed. The directory is created if it doesn't exist.

.. _build-data-directory:

``--with-data-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the tables, help pages, and other architecture-independent files are to be installed. The absolute path should be supplied. If this option isn't specified, then the ``brltty`` subdirectory of the directory specified via the standard configure option ``--sysconfdir`` (which defaults to ``/etc`` rooted at the directory specified by the :ref:`--prefix <build-portable-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-manpage-directory:

``--with-manpage-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the man pages are to be installed. The absolute path should be supplied. If this option isn't specified, then the directory specified via the standard configure option ``--mandir`` (which defaults to ``/man`` rooted at the directory specified by the :ref:`--prefix <build-portable-root>` build option) is assumed. The directory is created if it doesn't exist.

.. _build-include-directory:

``--with-include-directory=``\ *directory*
  Specify the directory within the :ref:`installed file hierarchy <hierarchy>` where the C header files for the Application Programming Interface are to be installed. The absolute path should be supplied. If this option isn't specified, then the ``brltty`` subdirectory of the directory specified via the standard configure option ``--includedir`` (which defaults to ``/include`` rooted at the directory specified by the :ref:`--prefix <build-portable-root>` build option) is assumed. The directory is created if it doesn't exist.


.. _build-features:

Build Features
^^^^^^^^^^^^^^

These options are primarily useful when building BRLTTY for use on a boot disk.

.. _build-standalone-programs:

``--enable-standalone-programs``
  Create statically linked, rather than dynamically linked, programs. This option removes all dependencies on shared objects at run-time. Only the default drivers (see the :ref:`--with-braille-driver <build-braille-driver>`, :ref:`--with-speech-driver <build-speech-driver>`, and :ref:`--with-screen-driver <build-screen-driver>` build options) are compiled.

.. _build-relocatable-install:

``--enable-relocatable-install``
  If this feature is enabled then all internal paths are recalculated to be relative to the program directory. If it's disabled then all internal paths are absolute. This feature allows the entire installed file hierarchy to be copied or moved, in tact, from one place to another, and is primarily intended for use on Windows platforms.

.. _build-stripping:

``--disable-stripping``
  Don't remove the symbol tables from executables and shared objects when installing them.

.. _command-LEARN:

.. _build-learn-mode:

``--disable-learn-mode``
  Reduce program size by excluding command learn mode (see section :ref:`Command Learn Mode <learn>`).

.. _build-contracted-braille:

``--disable-contracted-braille``
  Reduce program size by excluding support for contracted braille (see section :ref:`Contraction Tables <table-contraction>`).

.. _build-speech-support:

``--disable-speech-support``
  Reduce program size by excluding support for speech synthesizers.

.. _build-iconv:

``--disable-iconv``
  Reduce program size by excluding support for character set conversion.

.. _build-icu:

``--disable-icu``
  Reduce program size by excluding support for Unicode-based internationalization.

.. _build-x:

``--disable-x``
  Reduce program size by excluding support for X11.

.. _build-beeper-support:

``--disable-beeper-support``
  Reduce program size by excluding support for the console tone generator.

.. _build-pcm-support:

``--disable-pcm-support``
  Reduce program size by excluding support for the digital audio interface on the sound card.

``--enable-pcm-support=``\ *interface*
  If a platform provides more than one digital audio interface then the one which is to be used may be specified.


  .. list-table::
     :header-rows: 1

     * - Platform
       - Interface
       - Description

     * - Linux
       - oss
       - Open Sound System

     * -
       - alsa
       - Advanced Linux Sound Architecture

.. _build-midi-support:

``--disable-midi-support``
  Reduce program size by excluding support for the Musical Instrument Digital Interface of the sound card.

``--enable-midi-support=``\ *interface*
  If a platform provides more than one Musical Instrument Digital Interface then the one which is to be used may be specified.


  .. list-table::
     :header-rows: 1

     * - Platform
       - Interface
       - Description

     * - Linux
       - oss
       - Open Sound System

     * -
       - alsa
       - Advanced Linux Sound Architecture

.. _build-fm-support:

``--disable-fm-support``
  Reduce program size by excluding support for the FM synthesizer on an AdLib, OPL3, Sound Blaster, or equivalent sound card.

.. _build-pm-configfile:

``--disable-pm-configfile``
  Reduce program size by excluding support for the Papenmeier driver's configuration file.

.. _build-gpm:

``--disable-gpm``
  Reduce program size by excluding the interface to the ``gpm`` application which allows BRLTTY to interact with the pointer (mouse) device (see section :ref:`Pointer (Mouse) Support via GPM <gpm>`).

.. _build-api:

``--disable-api``
  Reduce program size by excluding the Application Programming Interface.

.. _build-api-parameters:

``--with-api-parameters=``\ *name*\ ``=``\ *value*\ ``,``...
  Specify the default parameter settings for the Application Programming Interface. If the same parameter is specified more than once, then its rightmost assignment is used. For a description of the parameters accepted by the interface, please see the **BrlAPI** reference manual. See the :ref:`api-parameters <configure-api-parameters>` configuration file directive and the :ref:`-A <options-api-parameters>` command line option for run-time selection.

.. _build-caml-bindings:

``--disable-caml-bindings``
  Don't build the Caml bindings for the Application Programming Interface.

.. _build-java-bindings:

``--disable-java-bindings``
  Don't build the Java bindings for the Application Programming Interface.

.. _build-lisp-bindings:

``--disable-lisp-bindings``
  Don't build the Lisp bindings for the Application Programming Interface.

.. _build-python-bindings:

``--disable-python-bindings``
  Don't build the Python bindings for the Application Programming Interface.

.. _build-tcl-bindings:

``--disable-tcl-bindings``
  Don't build the Tcl bindings for the Application Programming Interface.

.. _build-tcl-config:

``--with-tcl-config=``\ *path*
  Specify the location of the Tcl configuration script (``tclConfig.sh``). Either the path to the script itself or to the directory containing it may be supplied. Any of the following words can also be used as the operand of this option:

  no
    Use other means to guess if Tcl is available, and, if so, where it has been installed. This is equivalent to specifying ``--without-tcl-config``.

  yes
    Search for the script in a few commonly used directories. This is equivalent to specifying ``--with-tcl-config``.


.. _build-miscellaneous:

Miscellaneous Options
^^^^^^^^^^^^^^^^^^^^^

.. _build-init-path:

``--with-init-path=``\ *path*
  Specify the path to the real ``init`` program for the system. The absolute path should be supplied. If this option is specified, then:

  #. The ``init`` program should be moved to a new location.
  #. ``brltty`` should be moved to the ``init`` program's original location.
  #. When the system runs ``init`` at startup, ``brltty`` is actually run. It puts itself into the background, and runs the real ``init`` in the foreground. This is one (somewhat sneaky) way to have braille right at the outset. It's especially useful for some install/rescue disks.

  If this option isn't specified, then this feature isn't activated. This option is primarily intended for building a braillified installer image.

.. _build-stderr-path:

``--with-stderr-path=``\ *path*
  Specify the path to the file or device where standard error output is to be written. The absolute path should be supplied. If this option isn't specified, then this feature isn't activated. This option is primarily intended for building a braillified installer image.


.. _make:

Make File Targets
~~~~~~~~~~~~~~~~~

Once BRLTTY has been configured,
the next steps are to compile and to install it.
These are done by applying the system's ``make`` command
to BRLTTY's main make file (``Makefile`` in the top-level directory).
BRLTTY's make file supports most of the common application maintenance targets.
They include:

.. _make-uninstall:

make
  A shortcut for ``make all``.

.. _make-all:

make all
  Compile and link the BRLTTY executable, its drivers and their help pages, its test programs, and a few other small utilities.

.. _make-install:

make install
  Complete the compile and link phase (see :ref:`make all <make-all>`), and then install the BRLTTY executable, its data files, drivers, and help pages, in the correct places and with the correct permissions.

make uninstall
  Remove the BRLTTY executable, its data files, drivers, and help pages, from the system.

.. _make-clean:

make clean
  Ensure that the next compile and link (see :ref:`make all <make-all>`) will be done from scratch by removing the results of compiling, linking, and testing from the source directory structure. This includes the removal of object files, executables, dynamically loadable shared objects, driver lists, help pages, temporary header files, and core files.

.. _make-distclean:

make distclean
  In addition to removing the results of compiling and linking (see :ref:`make clean <make-clean>`):

  - Remove the results of BRLTTY configuration (see :ref:`Build Options <build>`). This includes the removal of ``config.mk``, ``config.h``, ``config.cache``, ``config.status``, and ``config.log``.
  - Remove other files from the source directory structure which tend to accumulate over time but which don't belong there. This includes the removal of editor backup files, test case results, rejected patch hunks, and copies of original source files.


Testing BRLTTY
--------------

After compiling, linking, and installing BRLTTY,
it's a good idea to give it a quick test before activating it permanently.
To do so, invoke it with the command:

.. parsed-literal::

  brltty -b*driver* -d*device*

For *driver*, specify the two-letter
:ref:`driver identification code <drivers>`
corresponding to your braille display.
For *device*, specify the full path
for the device to which your braille display is connected.

If you don't want to explicitly identify the driver and device
each time you start BRLTTY, then you can take two approaches.
You can establish system defaults
via the :ref:`braille-driver <configure-braille-driver>`
and the :ref:`braille-device <configure-braille-device>`
configuration file directives,
and/or compile your needs right into BRLTTY
via the :ref:`--with-braille-driver <build-braille-driver>`
and the :ref:`--with-braille-device <build-braille-device>`
build options.

If all is well, BRLTTY's version identification message
should appear on the braille display for a few seconds
(see the :ref:`-M <options-message-timeout>` command line option).
After it goes away (which you can hasten by pressing any key on the display),
the area of the screen where the cursor is should appear.
This means that you should expect to see your shell's command prompt.
Then, as you enter your next command,
each character should appear on the display as it's typed on the keyboard.

If this is your experience, then leave BRLTTY running, and enjoy it.
If this isn't your experience, then it may be necessary
to test each driver separately in order to isolate the source of the problem.
The screen driver can be tested with :ref:`scrtest <utility-scrtest>`,
and the braille display driver can be tested with :ref:`brltest <utility-brltest>`.

If you experience a problem which requires a lot of digging,
then you may wish to use the following ``brltty`` command line options:

- :ref:`-ldebug <options-log-level>` to log lots of diagnostic messages.
- :ref:`-n <options-no-daemon>` to keep BRLTTY in the foreground.
- :ref:`-e <options-standard-error>` to direct diagnostic messages to standard error rather than to the system log.


Starting BRLTTY
---------------

BRLTTY, when properly installed, is invoked with the single command ``brltty``.
A configuration file
(see section :ref:`The Configuration File <configure>` for details)
can be created in order to establish system defaults for such things as
the location of the preferences file,
the braille display driver to be used,
the device to which the braille display is connected,

.. _preference-text-table:

and the text table to be used.
Many options
(see section :ref:`Command Line Options <options>` for details)
allow explicit run-time specification of such things as
the location of the configuration file,
any defaults established within the configuration file,
and some characteristics which have reasonable defaults
but which those who think they know what they're doing may wish to play with.
The :ref:`-h <options-help>` option
displays a summary of all the options.
The :ref:`-V <options-version>` option
displays the current version of the program, the API, and the selected drivers.
The :ref:`-v <options-verify>` option
displays the values of the options after all sources have been considered.

It's probably best to have the system automatically start BRLTTY
as part of the boot sequence
so that the braille display is already up and running when the login prompt appears.
Most (probably all) distributions provide a script wherein
user-supplied applications can be safely started near the end of the boot sequence.
The name of this script is distribution-dependent.
Here are the ones we know about so far:

Red Hat
  ``/etc/rc.d/rc.local``


Starting BRLTTY from this script is a good approach (especially for new users).
Just add a set of lines like these::

  if [ -x /bin/brltty -a -f /etc/brltty.conf ]
  then
  /bin/brltty
  fi

This can usually be abbreviated to the somewhat less readable form::

  [ -x /bin/brltty -a -f /etc/brltty.conf ] && /bin/brltty

Don't add these lines before the first line
(which usually looks like ``#!/bin/sh``).

If the braille display is to be used by a system administrator,
then it should probably be started as early as possible during the boot sequence
(like before the file systems are checked)
so that the display is usable
in the event that something goes wrong during these checks
and the system drops into single user mode.
Again, exactly where it's best to do this is distribution-dependent.
Here are the places we know about so far:

Debian
  ``/etc/init.d/boot`` (for older releases)

  ``/etc/init.d/`` (for newer releases)

  A ``brltty`` package is provided
  (see `http://packages.debian.org/brltty <http://packages.debian.org/brltty>`_)
  as of release ``3.0`` (``Woody``).
  Since this package takes care of starting BRLTTY,
  there's no need for user-supplied code to do so if it's installed.
  If you need the daemon to run with some command-line options,
  you can change the contents between quotes
  on the directive ``ARGUMENTS``
  in the ``/etc/default/brltty`` file.

RedHat
  ``/etc/rc.d/rc.sysinit``

  Beware that later releases,
  in order to support a more user-oriented system initialization procedure,
  have this script reinvoke itself such that
  it's under the control of ``initlog``.
  Look, probably right up near the top, for a set of lines like these::

    # Rerun ourselves through initlog
    if [ -z "$IN_INITLOG" ]; then
    [ -f /sbin/initlog ] && exec /sbin/initlog $INITLOG_ARGS -r /etc/rc.sysinit
    fi

  Starting BRLTTY before this reinvocation
  results in two BRLTTY processes running at the same time,
  and that'll give you no end of problems.
  If your version of this script has this feature,
  then make sure you start BRLTTY after the lines which implement it.

Slackware
  ``/etc/rc.d/rc.S``

SuSE
  ``/sbin/init.d/boot``

An alternative is to start BRLTTY from ``/etc/inittab``.
You have two choices if you choose this route.

- If you want it to be started really early
  but don't need it to be automatically restarted if it dies,
  then add a line like this before the first ``:sysinit:`` line which is already in there.

  ::

    brl::sysinit:/bin/brltty

- If you don't mind it being started later
  but do want it to be automatically restarted if it dies,
  then add a line like this anywhere within the file.

  .. parsed-literal::

    brl:12345:respawn:/bin/brltty -n

  The :ref:`-n <options-no-daemon>` (``--nodaemon``) option is very
  important when running BRLTTY with **init**'s ``respawn`` facility.
  You'll end up with hundreds of BRLTTY processes all running at the same time
  if you forget to specify it.

Check that the identifier (``brl`` in these examples)
isn't already being used by another entry,
and, if it is, choose a different one which isn't.

Note that a command like ``kill -TERM``
is sufficient to stop BRLTTY in its tracks.
If it dies during entry into single user mode, for example,
it may well be due to a problem of this nature.

Some systems, as part of the boot sequence, probe the serial ports
(usually in order to automatically find the mouse and deduce its type).
If your braille display is using a serial port,
this kind of probing may be enough to get it confused.
If this happens to you, then try restarting the braille driver

.. _command-RESTARTBRL:

(see the :ref:`RESTARTBRL <command-RESTARTBRL>` command).
Better yet, turn off the serial port probing.
Here's what we know so far about how to do this:

Red Hat
  The probing is done by a service named ``kudzu``. Use the command::

      chkconfig --list kudzu

  to see if it's been enabled. Use the command::

      chkconfig kudzu off

  to disable it. Later releases allow you to let ``kudzu`` run without probing the serial ports. To do this, edit the file ``/etc/sysconfig/kudzu``, and set ``SAFE`` to ``yes``.


If you want to start BRLTTY before any file systems are mounted, then
ensure that all of its components are installed within the root file system.
See the :ref:`--with-execute-root <build-execute-root>`,
:ref:`--bindir <build-program-directory>`,
:ref:`--libdir <build-library-directory>`,
:ref:`--with-writable-directory <build-writable-directory>`,
and :ref:`--with-data-directory <build-data-directory>`
build options.


Security Considerations
-----------------------

BRLTTY needs to run with root privileges because it needs
read and write access for the port to which the braille display is connected,
read access to ``/dev/vcsa`` or equivalent
(to query the screen dimensions and the cursor position,
and to review the current screen content and highlighting),
and read and write access to the system console
(for arrow key entry during cursor routing,
for input character insertion during paste,
for special key simulation using keys on the braille display,
for retrieving output character translation and screen font mapping tables,
and for activation of the internal beeper).
Access to the needed devices can, of course, be granted to a non-root user
by changing the file permissions associated with the devices.
Merely having access to the console, however, isn't enough because
activating the internal beeper and simulating key strokes still require root privilege.
So, if you're willing to give up Cursor Routing, Copy and Paste, beeps,
and all that, you can run BRLTTY without root privilege.


Build and Run-Time Restrictions
-------------------------------

.. _preference-alert-tunes:

.. _restrictions-tunes:

Alert Tunes
  Some platforms don't support all of the tune devices. See the :ref:`Tune Device <preference-tune-device>` preference for details.

.. _restrictions-flite:

FestivalLite Speech Synthesizer Driver
  The driver for the FestivalLite text to speech engine is only built if that package has been installed.

  This driver and the driver for the Theta text to speech engine (see the :ref:`--with-theta <build-theta>` build option) cannot be simultaneously linked into the BRLTTY binary (see the :ref:`--with-speech-driver <build-speech-driver>` build option) because their run-time libraries contain conflicting symbols.

.. _restrictions-libbraille:

Libbraille Braille Display Driver
  The driver for the Libbraille package is only built if that package has been installed.

.. _restrictions-mikropuhe:

Mikropuhe Speech Synthesizer Driver
  The driver for the Mikropuhe text to speech engine is only built if that package has been installed.

  This driver cannot be included if the BRLTTY binary is statically linked (see the :ref:`--enable-standalone-programs <build-standalone-programs>` build option) because a static archive isn't included with the package.

.. _restrictions-theta:

Theta Speech Synthesizer Driver
  The driver for the Theta text to speech engine is only built if that package has been installed.

  This driver and the driver for the FestivalLite text to speech engine (see the :ref:`--with-flite <build-flite>` build option) cannot be simultaneously linked into the BRLTTY binary (see the :ref:`--with-speech-driver <build-speech-driver>` build option) because their run-time libraries contain conflicting symbols.

  If this driver is built as a dynamically loadable shared object then ``$THETA_HOME/lib`` must be added to the ``LD_LIBRARY_PATH`` environment variable before BRLTTY is invoked because the shared objects within the package don't contain run-time search paths for their dependencies.

.. _restrictions-viavoice:

ViaVoice Speech Synthesizer Driver
  The driver for the ViaVoice text to speech engine is only built if that package has been installed.

  This driver cannot be included if the BRLTTY binary is statically linked (see the :ref:`--enable-standalone-programs <build-standalone-programs>` build option) because a static archive isn't included with the package.

.. _restrictions-videobraille:

VideoBraille Braille Display Driver
  The driver for the VideoBraille braille display is built on all systems, but only works on Linux.


.. _rpm:

Installing from an RPM File
---------------------------

To install BRLTTY from an RPM (RedHat Package Manager) file, do the following:
#. Download the binary package which corresponds to your hardware. It'll be a file named ``brltty-``\ *release*\ ``-``\ *version*\ ``.``\ *architecture*\ ``.rpm``, e.g. ``brltty-3.0-1.i386.rpm``.
#. Install the package.


   .. parsed-literal::

     rpm -Uvh brltty-*release*-*version*.*architecture*.rpm

#. This should be done as **root**. Strictly speaking, the ``-U`` (update) option is the only one which is necessary. The ``-v`` (verbose) option displays the name of the package as it's being installed. The ``-h`` (hashes) option displays a progress meter (using hash signs).

For the brave,
we also provide the source RPM (``.src.rpm``) file,
but that's beyond the scope of this document.

To uninstall BRLTTY, do:

.. parsed-literal::

  rpm -e brltty


Other Utilities
---------------

Building BRLTTY also results in the building of
a few small helper and diagnostic utilities.


.. _utility-brltty-config:

brltty-config
~~~~~~~~~~~~~

This utility sets a number of environment variables to values
which reflect the current installation of BRLTTY
(see :ref:`Build Options <build>`).
It should be executed within an existing shell environment,
i.e. not as a command in its own right,
and can only be used by scripts which support **Bourne Shell** syntax.

.. parsed-literal::

  . brltty-config


The following environment variables are set:

BRLTTY_VERSION
  The version number of the BRLTTY package.

BRLTTY_EXECUTE_ROOT
  Run-time root for the installed package. Configured via the :ref:`--with-execute-root <build-execute-root>` build option.

BRLTTY_PROGRAM_DIRECTORY
  Directory for runnable programs (binaries, executables). Configured via the :ref:`--with-program-directory <build-program-directory>` build option.

BRLTTY_LIBRARY_DIRECTORY
  Directory for drivers. Configured via the :ref:`--with-library-directory <build-library-directory>` build option.

BRLTTY_WRITABLE_DIRECTORY
  Directory which can be written to. Configured via the :ref:`--with-writable-directory <build-writable-directory>` build option.

BRLTTY_DATA_DIRECTORY
  Directory for tables and help pages. Configured via the :ref:`--with-data-directory <build-data-directory>` build option.

BRLTTY_MANPAGE_DIRECTORY
  Directory for manual pages. Configured via the :ref:`--with-manpage-directory <build-manpage-directory>` build option.

BRLTTY_INCLUDE_DIRECTORY
  Directory for BrlAPI's C header files. Configured via the :ref:`--with-include-directory <build-include-directory>` build option.

BRLAPI_VERSION
  The version number of BrlAPI (BRLTTY's Application Programming Interface).

BRLAPI_RELEASE
  The full release number of BrlAPI.

BRLAPI_AUTH
  The name of BrlAPI's key file.


In addition, the following standard **autoconf** environment variables are also set:

prefix
  Subroot for architecture-independent files. Configured via the :ref:`--prefix <build-portable-root>` build option.

exec_prefix
  Subroot for architecture-dependent files. Configured via the :ref:`--exec-prefix <build-architecture-root>` build option.

bindir
  Default location for :ref:`program directory <build-program-directory>`. Configured via the ``--bindir`` build option.

libdir
  Directory for BrlAPI's static archive and dynamically loadable object. Default anchor for :ref:`library directory <build-library-directory>`. Configured via the :ref:`--libdir <build-api-directory>` build option.

sysconfdir
  Directory for configuration files. Default anchor for :ref:`data directory <build-data-directory>`. Configured via the :ref:`--sysconfdir <build-configuration-directory>` build option.

mandir
  Default location for :ref:`manual pages directory <build-manpage-directory>`. Configured via the ``--mandir`` build option.

includedir
  Default anchor for :ref:`header files directory <build-include-directory>`. Configured via the ``--includedir`` build option.


.. _utility-brltty-install:

brltty-install
~~~~~~~~~~~~~~

This utility copies BRLTTY's
:ref:`installed file hierarchy <hierarchy>`
from one location to another.

.. parsed-literal::

  brltty-install *to* [*from*]

*to*
  The location to which the :ref:`installed file hierarchy <hierarchy>` is to be copied. It must be an existing directory.

*from*
  The location from which the :ref:`installed file hierarchy <hierarchy>` is to be taken. If it's specified, then it must be an existing directory. If it's not specified, then the location used for the build is assumed.


This utility can be used, for example, to copy BRLTTY to a root disk.
If a root floppy is mounted as ``/mnt``,
and BRLTTY is installed on the main system,
then typing

::

  brltty-install /mnt


copies BRLTTY, along with all of its data and library files,
to the root floppy.

Some problems have been experienced when copying BRLTTY
between systems with different versions of the shared C library.
This is worth investigating if you have difficulties.


.. _utility-brltest:

brltest
~~~~~~~

This utility tests a braille display driver,
and also provides an interactive way to learn
what the keys on the braille display do.
It should be run as root.

.. parsed-literal::

  brltest -*option* ... [*driver* [*name*=*value* ...]]

*driver*
  The driver for the braille display. It must be a two-letter :ref:`driver identification code <drivers>`. If it's not specified, then the first driver configured via the :ref:`--with-braille-driver <build-braille-driver>` build option is assumed.

*name*\ ``=``\ *value*
  Set a braille display driver parameter. For a description of the parameters accepted by a specific driver, please see the documentation for that driver.

``-d``\ *device* ``--device=``\ *device*
  The absolute path for the device to which the braille display is connected. If it's not specified, then the device configured via the :ref:`--with-braille-device <build-braille-device>` build option is assumed.

``-D``\ *directory* ``--data-directory=``\ *directory*
  The absolute path for the directory wherein the driver data files reside. If it's not specified, then the directory configured via the :ref:`--with-data-directory <build-data-directory>` build option is assumed.

``-L``\ *directory* ``--library-directory=``\ *directory*
  The absolute path for the directory wherein the drivers reside. If it's not specified, then the directory configured via the :ref:`--libdir <build-library-directory>` build option is assumed.

``-W``\ *directory* ``--writable-directory=``\ *directory*
  The absolute path for a directory which can be written to. If it's not specified, then the directory configured via the :ref:`--with-writable-directory <build-writable-directory>` build option is assumed.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


This utility uses BRLTTY's :ref:`Command Learn Mode <learn>`.
The key press timeout
(after which this utility exits)
is ``10`` seconds.
The message hold time
(used for non-final segments of long messages)
is ``4`` seconds.


.. _utility-spktest:

spktest
~~~~~~~

This utility tests a speech synthesizer driver.
It may need to be run as root.

.. parsed-literal::

  spktest -*option* ... [*driver* [*name*=*value* ...]]

*driver*
  The driver for the speech synthesizer. It must be a two-letter :ref:`driver identification code <drivers>`. If it's not specified, then the first driver configured via the :ref:`--with-speech-driver <build-speech-driver>` build option is assumed.

*name*\ ``=``\ *value*
  Set a speech synthesizer driver parameter. For a description of the parameters accepted by a specific driver, please see the documentation for that driver.

``-t``\ *string* ``--text-string=``\ *string*
  The text to be spoken. If it's not specified, then standard input is read.

``-D``\ *directory* ``--data-directory=``\ *directory*
  The absolute path for the directory wherein the driver data files reside. If it's not specified, then the directory configured via the :ref:`--with-data-directory <build-data-directory>` build option is assumed.

``-L``\ *directory* ``--library-directory=``\ *directory*
  The absolute path for the directory wherein the drivers reside. If it's not specified, then the directory configured via the :ref:`--libdir <build-library-directory>` build option is assumed.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


.. _utility-scrtest:

scrtest
~~~~~~~

This utility tests the screen driver.
It must be run as root.

.. parsed-literal::

  scrtest -*option* ... [*name*=*value* ...]

*name*\ ``=``\ *value*
  Set a screen driver parameter. For a description of the parameters accepted by a specific driver, please see the documentation for that driver.

``-l``\ *column* ``--left=``\ *column*
  Specify the starting (left) column (zero-origin) of the region. If this value isn't supplied, then a default value, based on the specified width, is selected such that the region is horizontally centred.

``-c``\ *count* ``--columns=``\ *count*
  Specify the width of the region (in columns). If this value isn't supplied, then a default value, based on the specified starting column, is selected such that the region is horizontally centred.

``-t``\ *row* ``--top=``\ *row*
  Specify the starting (top) row (zero-origin) of the region. If this value isn't supplied, then a default value, based on the specified height, is selected such that the region is vertically centred.

``-r``\ *count* ``--rows=``\ *count*
  Specify the height of the region (in rows). If this value isn't supplied, then a default value, based on the specified starting row, is selected such that the region is vertically centred.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


Notes:

- If neither a starting column nor a region width is specified, then the region is horizontally-centred and starts at column 5.
- If neither a starting row nor a region height is specified, then the region is vertically-centred and starts at row 5.


The following is written to standard output:
#. A line detailing the dimensions of the screen.


   .. parsed-literal::

     Screen: *width*x*height*

#. A line detailing the position (zero-origin) of the cursor.


   .. parsed-literal::

     Cursor: [*column*,*row*]

#. A line detailing the size of the selected screen region, and the position (zero-origin) of its top-left corner.


   .. parsed-literal::

     Region: *width*x*height*@[*column*,*row*]

#. The contents of the selected screen region. Unprintable characters are written as blanks.


.. _utility-ttbtest:

ttbtest
~~~~~~~

This utility tests a text table
(see section :ref:`Text Tables <table-text>`).

.. parsed-literal::

  ttbtest -*option* ... *input-table* *output-table*

*input-table*
  The file system path to the input text table. If it's relative then it's anchored at the directory configured via the :ref:`--with-data-directory <build-data-directory>` build option.

*output-table*
  The file system path to the output text table. If it's relative then it's anchored at the current working directory. If this parameter isn't supplied then no output table is written.

``-i``\ *format* ``--input-format=``\ *format*
  Specify the format of the input table. If this option isn't supplied then the format of the input table is deduced from the extension of the input table's file name.

``-o``\ *format* ``--output-format=``\ *format*
  Specify the format of the output table. If this option isn't supplied then the format of the output table is deduced from the extension of the output table's file name.

``-c``\ *charset* ``--charset=``\ *charset*
  Specify the name of the 8-bit character set to use when interpreting the tables. If this option isn't supplied then the host's character set is used.

``-e`` ``--edit``
  Invoke the text table editor. If the output table is specified then changes are written to it. If not then the input table is rewritten.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


If no special action is requested then the output table is optional.
If it is not specified then the input table is checked.
If it is specified then the input table is converted.

The following table formats are supported:

ttb
  BRLTTY

sbl
  SuSE Blinux

a2b
  Gnopernicus

gnb
  Gnome Braille


.. _utility-ctbtest:

ctbtest
~~~~~~~

.. _preference-contraction-table:

This utility tests a contraction table
(see section :ref:`Contraction Tables <table-contraction>`).
The text read from the input files (or standard input)
is rewritten to standard output as contracted braille.

.. parsed-literal::

  ctbtest -*option* ... *input-file* ...

*input-file*
  The list of files to be processed. Any number of files may be specified. They're processed from left to right. The special file name ``-`` is interpreted to mean standard input. If no files are specified then standard input is processed.

``-c``\ *file* ``--contraction-table=``\ *file*
  The file system path to the contraction table. If it's relative then it's anchored at the directory configured via the :ref:`--with-data-directory <build-data-directory>` build option. The ``.ctb`` extension is optional. If this option isn't supplied then ``en-us-g2`` is assumed.

``-t``\ *file*\ \|\ ``auto`` ``--text-table=``\ *file*\ \|\ ``auto``
  Specify the text table (see section :ref:`Text Tables <table-text>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ttb`` extension is optional. See the :ref:`text-table <configure-text-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Text Table <preference-text-table>` preference.

``-w``\ *columns* ``--output-width=``\ *columns*
  The maximum length of an output line. Each contracted input line is wrapped into as many output lines as necessary. If this option isn't specified then there's no limit, and there's a one-to-one correspondence between input and output lines.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


The text table is used:

- To define the output character set so that the contracted braille will be displayed correctly. The same table as will be used by BRLTTY when the output is read should be specified.
- To define the braille representations of those characters defined within the contraction table as ``=`` (see section :ref:`Character Translation <contraction-opcodes-translation>`).


The ``brf.ttb`` text table is provided for use with this utility.
It defines the format used within ``.brf`` files.
This is also the preferred format used
by most braille printers
and within electronically distributed braille documents.
This table effectively allows this utility to be used as a text to braille translator.


.. _utility-tunetest:

tunetest
~~~~~~~~

.. _command-TUNES:

This utility tests the alert tunes facility,
and also provides an easy way to compose new tunes.
It may need to be run as root.

.. parsed-literal::

  tunetest -*option* ... {*note* *duration*} ...

*note*
  A standard ``MIDI`` note number. It must be an integer from ``1`` through ``127``, with ``60`` representing ``Middle C``. Each value represents a standard chromatic semi-tone, with the next lower and higher values representing, respectively, the next lower and higher notes. The lowest value (``1``) represents the fifth ``C-Sharp`` below ``Middle C``, and the highest value (``127``) represents the sixth ``G`` above ``Middle C``.

*duration*
  The duration of the note in milliseconds. It must be an integer from ``1`` through ``255``.

``-d``\ *device* ``--device=``\ *device*
  The device on which to play the tune.

  beeper
    The internal beeper (console tone generator).

  pcm
    The digital audio interface on the sound card.

  midi
    The Musical Instrument Digital Interface on the sound card.

  fm
    The FM synthesizer on an AdLib, OPL3, Sound Blaster, or equivalent sound card.

  The device name may be abbreviated. See the :ref:`Tune Device <preference-tune-device>` preference for details regarding the default device and platform restrictions.

``-v``\ *loudness* ``--volume=``\ *loudness*
  Specify the output volume (loudness) as a percentage of the maximum. The default output volume is ``50``.

``-p``\ *device* ``--pcm-device=``\ *device*
  Specify the device to use for digital audio (see section :ref:`PCM Device Specification <operand-pcm-device>`). This option isn't available if the :ref:`--disable-pcm-support <build-pcm-support>` build option was specified.

``-m``\ *device* ``--midi-device=``\ *device*
  Specify the device to use for the Musical Instrument Digital Interface (see section :ref:`MIDI Device Specification <operand-midi-device>`). This option isn't available if the :ref:`--disable-midi-support <build-midi-support>` build option was specified.

``-i``\ *instrument* ``--instrument=``\ *instrument*
  The instrument to use if the selected device is ``midi``. For the complete list of instruments, see the :ref:`MIDI Instrument Table <midi>`. The default instrument is an ``acoustic grand piano``. The words comprising the instrument name must be separated from one another by a single minus sign rather than by spaces, and any of the words may be abbreviated. An ``acoustic grand piano``, for example, may be specified as ``a-gra-pi``.

``-h`` ``--help``
  Display a summary of the command line options, and then exit.


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
and the :ref:`--with-braille-device <build-braille-device>` build option
for alternatives regarding how to tell BRLTTY which device your display is connected to.
Check the
:ref:`-b <options-braille-driver>` command line option,
the :ref:`braille-driver <configure-braille-driver>` configuration file directive,
and the :ref:`--with-braille-driver <build-braille-driver>` build option
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
  Show the highlighting (the attributes) of each character within the braille window, rather than the characters themselves (the content). This feature is useful, for example, when you need to locate a highlighted item. When showing screen content, the text table is used (see the :ref:`-t <options-text-table>` command line option, the :ref:`text-table <configure-text-table>` configuration file directive, and the :ref:`--with-text-table <build-text-table>` build option). When showing screen attributes, the attributes table is used (see the :ref:`-a <options-attributes-table>` command line option, the :ref:`attributes-table <configure-attributes-table>` configuration file directive, and the :ref:`--with-attributes-table <build-attributes-table>` build option). This feature only affects the current virtual terminal.

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
  Skip past blank windows when reading either forward or backward. This feature affects the :ref:`FWINLT/FWINRT <command-FWINLT-FWINRT>` commands. This setting can also be changed with the :ref:`Skip Blank Windows <preference-skip-blank-windows>` preference.

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
  Play a short predefined tune (see :ref:`Alert Tunes <tunes>`) whenever a significant event occurs. This feature is initially **on**. This setting can also be changed with the :ref:`Alert Tunes <preference-alert-tunes>` preference.

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
  Switch to command learn mode (see section :ref:`Command Learn Mode <learn>` for full details). This is how you can interactively learn what your braille display's keys do. Invoke this command again to return to the screen. This command isn't available if the :ref:`--disable-learn-mode <build-learn-mode>` build option was specified.


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
  Specify parameters for the Application Programming Interface. If the same parameter is specified more than once, then its rightmost assignment is used. For a description of the parameters accepted by the interface, please see the **BrlAPI** reference manual. See the :ref:`--with-api-parameters <build-api-parameters>` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-A <options-api-parameters>` command line option.

.. _configure-attributes-table:

``attributes-table`` *file*
  Specify the attributes table (see section :ref:`Attributes Tables <table-attributes>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.atb`` extension is optional. The default is to use the built-in table (see the :ref:`--with-attributes-table <build-attributes-table>` build option). This directive can be overridden with the :ref:`-a <options-attributes-table>` command line option.

.. _configure-braille-device:

``braille-device`` *device*\ ``,``...
  Specify the device to which the braille display is connected (see section :ref:`Braille Device Specification <operand-braille-device>`). See the :ref:`--with-braille-device <build-braille-device>` build option for the default established during the build procedure. This directive can be overridden with the :ref:`-d <options-braille-device>` command line option.

.. _configure-braille-driver:

``braille-driver`` *driver*\ ``,``...|\ ``auto``
  Specify the braille display driver (see section :ref:`Driver Specification <operand-driver>`). The default is to perform autodetection. This directive can be overridden with the :ref:`-b <options-braille-driver>` command line option.

.. _configure-braille-parameters:

``braille-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the braille display drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`--with-braille-parameters <build-braille-parameters>` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-B <options-braille-parameters>` command line option.

.. _configure-contraction-table:

``contraction-table`` *file*
  Specify the contraction table (see section :ref:`Contraction Tables <table-contraction>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ctb`` extension is optional. The contraction table is used when the 6-dot braille feature is activated (see the :ref:`SIXDOTS <command-SIXDOTS>` command and the :ref:`Text Style <preference-text-style>` preference). The default is to display uncontracted 6-dot braille. This directive can be overridden with the :ref:`-c <options-contraction-table>` command line option. It isn't available if the :ref:`--disable-contracted-braille <build-contracted-braille>` build option was specified.

.. _configure-keyboard-table:

``keyboard-table`` *file*\ \|\ ``auto``
  Specify the keyboard table (see section :ref:`Key Tables <table-key>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ktb`` extension is optional. The default is not to use a keyboard table. This directive can be overridden with the :ref:`-k <options-keyboard-table>` command line option.

.. _configure-keyboard-properties:

``keyboard-properties`` *name*\ ``=``\ *value*\ ``,``...
  Specify the properties of the keyboard(s) to be monitored. If the same property is specified more than once, then its rightmost assignment is used. See section :ref:`Keyboard Properties <keyboard-properties>` for a list of the properties which may be specified. The default is to monitor all keyboards. This directive can be overridden with the :ref:`-K <options-keyboard-properties>` command line option.

.. _configure-midi-device:

``midi-device`` *device*
  Specify the device to use for the Musical Instrument Digital Interface (see section :ref:`MIDI Device Specification <operand-midi-device>`). This directive can be overridden with the :ref:`-m <options-midi-device>` command line option. It isn't available if the :ref:`--disable-midi-support <build-midi-support>` build option was specified.

.. _configure-pcm-device:

``pcm-device`` *device*
  Specify the device to use for digital audio (see section :ref:`PCM Device Specification <operand-pcm-device>`). This directive can be overridden with the :ref:`-p <options-pcm-device>` command line option. It isn't available if the :ref:`--disable-pcm-support <build-pcm-support>` build option was specified.

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
  Specify the screen driver (see section :ref:`Supported Screen Drivers <screen>`). See the :ref:`--with-screen-driver <build-screen-driver>` build option for the default established during the build procedure. This directive can be overridden with the :ref:`-x <options-screen-driver>` command line option.

.. _configure-screen-parameters:

``screen-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the screen drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Supported Screen Drivers <screen>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`--with-screen-parameters <build-screen-parameters>` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-X <options-screen-parameters>` command line option.

.. _configure-speech-driver:

``speech-driver`` *driver*\ ``,``...|\ ``auto``
  Specify the speech synthesizer driver (see section :ref:`Driver Specification <operand-driver>`). The default is to perform autodetection. This directive can be overridden with the :ref:`-s <options-speech-driver>` command line option. It isn't available if the :ref:`--disable-speech-support <build-speech-support>` build option was specified.

.. _configure-speech-input:

``speech-input`` *name*
  Specify the name of the file system object (FIFO, named pipe, named socket, etc) which can be used by other applications for text-to-speech conversion via BRLTTY's speech driver. This directive can be overridden with the :ref:`-i <options-speech-input>` command line option. It isn't available if the :ref:`--disable-speech-support <build-speech-support>` build option was specified.

.. _configure-speech-parameters:

``speech-parameters`` [*driver*\ ``:``]*name*\ ``=``\ *value*\ ``,``...
  Specify parameters for the speech synthesizer drivers. If the same parameter is specified more than once, then its rightmost assignment is used. If a parameter name is qualified by a driver (see section :ref:`Driver Identification Codes <drivers>`) then that setting only applies to that driver; if it isn't then it applies to all drivers. For a description of the parameters accepted by a specific driver, please see the documentation for that driver. See the :ref:`--with-speech-parameters <build-speech-parameters>` build option for the defaults established during the build procedure. This directive can be overridden with the :ref:`-S <options-speech-parameters>` command line option.

.. _configure-text-table:

``text-table`` *file*\ \|\ ``auto``
  Specify the text table (see section :ref:`Text Tables <table-text>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ttb`` extension is optional. The default is to perform locale-based autoselection, with fallback to the built-in table (see the :ref:`--with-text-table <build-text-table>` build option). This directive can be overridden with the :ref:`-t <options-text-table>` command line option.


.. _options:

Command Line Options
--------------------

Many settings can be explicitly specified when invoking BRLTTY.
The ``brltty`` command accepts the following options:

.. _options-attributes-table:

``-a``\ *file* ``--attributes-table=``\ *file*
  Specify the attributes table (see section :ref:`Attributes Tables <table-attributes>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.atb`` extension is optional. See the :ref:`attributes-table <configure-attributes-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Attributes Table <preference-attributes-table>` preference.

.. _options-braille-driver:

``-b``\ *driver*\ ``,``...|\ ``auto`` ``--braille-driver=``\ *driver*\ ``,``...|\ ``auto``
  Specify the braille display driver (see section :ref:`Driver Specification <operand-driver>`). See the :ref:`braille-driver <configure-braille-driver>` configuration file directive for the default run-time setting.

.. _options-contraction-table:

``-c``\ *file* ``--contraction-table=``\ *file*
  Specify the contraction table (see section :ref:`Contraction Tables <table-contraction>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ctb`` extension is optional. The contraction table is used when the 6-dot braille feature is activated (see the :ref:`SIXDOTS <command-SIXDOTS>` command and the :ref:`Text Style <preference-text-style>` preference). See the :ref:`contraction-table <configure-contraction-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Contraction Table <preference-contraction-table>` preference. This option isn't available if the :ref:`--disable-contracted-braille <build-contracted-braille>` build option was specified.

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
  Specify the name of the file system object (FIFO, named pipe, named socket, etc) which can be used by other applications for text-to-speech conversion via BRLTTY's speech driver. If not specified, the file system object is not created. See the :ref:`speech-input <configure-speech-input>` configuration file directive for the default run-time setting. This option isn't available if the :ref:`--disable-speech-support <build-speech-support>` build option was specified.

.. _options-keyboard-table:

``-k``\ *file* ``--keyboard-table=``\ *file*
  Specify the keyboard table (see section :ref:`Key Tables <table-key>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ktb`` extension is optional. See the :ref:`keyboard-table <configure-keyboard-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Keyboard Table <preference-keyboard-table>` preference.

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
  Specify the device to use for the Musical Instrument Digital Interface (see section :ref:`MIDI Device Specification <operand-midi-device>`). See the :ref:`midi-device <configure-midi-device>` configuration file directive for the default run-time setting. This option isn't available if the :ref:`--disable-midi-support <build-midi-support>` build option was specified.

.. _options-no-daemon:

``-n`` ``--no-daemon``
  Specify that BRLTTY is to remain in the foreground. If not specified, then BRLTTY becomes a background process (daemon) after initializing itself but before starting any of the selected drivers.

.. _options-pcm-device:

``-p``\ *device* ``--pcm-device=``\ *device*
  Specify the device to use for digital audio (see section :ref:`PCM Device Specification <operand-pcm-device>`). See the :ref:`pcm-device <configure-pcm-device>` configuration file directive for the default run-time setting. This option isn't available if the :ref:`--disable-pcm-support <build-pcm-support>` build option was specified.

.. _options-quiet:

``-q`` ``--quiet``
  Log less information. This option changes the default log level (see the :ref:`-l <options-log-level>` option) to ``notice`` if either the :ref:`-v <options-verify>` or the :ref:`-V <options-version>` option is specified, and to ``warning`` otherwise.

.. _options-release-device:

``-r`` ``--release-device``
  Release the device to which the braille display is connected when the current screen or window can't be read. See the :ref:`release-device <configure-release-device>` configuration file directive for the default run-time setting.

.. _options-speech-driver:

``-s``\ *driver*\ ``,``...|\ ``auto`` ``--speech-driver=``\ *driver*\ ``,``...|\ ``auto``
  Specify the speech synthesizer driver (see section :ref:`Driver Specification <operand-driver>`). See the :ref:`speech-driver <configure-speech-driver>` configuration file directive for the default run-time setting. This option isn't available if the :ref:`--disable-speech-support <build-speech-support>` build option was specified.

.. _options-text-table:

``-t``\ *file* ``--text-table=``\ *file*
  Specify the text table (see section :ref:`Text Tables <table-text>` for details). If a relative path is supplied, then it's anchored at ``/etc/brltty`` (see the :ref:`--with-data-directory <build-data-directory>` and the :ref:`--with-execute-root <build-execute-root>` build options for more details). The ``.ttb`` extension is optional. See the :ref:`text-table <configure-text-table>` configuration file directive for the default run-time setting. This setting can be changed with the :ref:`Text Table <preference-text-table>` preference.

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

If BRLTTY is configured with the :ref:`--enable-gpm <build-gpm>` build option
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
or the :ref:`Alert Tunes <preference-alert-tunes>` preference.
The tunes are played via the internal beeper by default,
but other alternatives can be selected
with the :ref:`Tune Device <preference-tune-device>` preference.

Each significant event is associated, from highest to lowest priority,
with one or more of the following:

a tune
  If a tune has been associated with the event, if the :ref:`Alert Tunes <preference-alert-tunes>` preference (see also the :ref:`TUNES <command-TUNES>` command) is active, and if the selected tune device (see the :ref:`Tune Device <preference-tune-device>` preference) can be opened, then the tune is played.

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

  This preference is only presented if the :ref:`--enable-gpm <build-gpm>` build option was specified.

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
    The internal beeper (console tone generator). This setting is supported on Linux, on OpenBSD, on FreeBSD, and on NetBSD. It's always safe to use, although it may be somewhat soft. This device isn't available if the :ref:`--disable-beeper-support <build-beeper-support>` build option was specified.

  PCM
    The digital audio interface on the sound card. This setting is supported on Linux (via ``/dev/dsp``), on Solaris (via ``/dev/audio``), on OpenBSD (via ``/dev/audio0``), on FreeBSD (via ``/dev/dsp``), and on NetBSD (via ``/dev/audio0``). It doesn't work when this device is already being used by another application. This device isn't available if the :ref:`--disable-pcm-support <build-pcm-support>` build option was specified.

  MIDI
    The Musical Instrument Digital Interface on the sound card This setting is supported on Linux (via ``/dev/sequencer``). It doesn't work when this device is already being used by another application. This device isn't available if the :ref:`--disable-midi-support <build-midi-support>` build option was specified.

  FM
    The FM synthesizer on an AdLib, OPL3, Sound Blaster, or equivalent sound card. This setting is supported on Linux. It works even if the FM synthesizer is already being used by another application. The results are unpredictable, and potentially not very good, if it's used with a sound card which doesn't support this feature. This device isn't available if the :ref:`--disable-fm-support <build-fm-support>` build option was specified.

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

  If alert tunes are to be played (see the :ref:`TUNES <command-TUNES>` command and the :ref:`Alert Tunes <preference-alert-tunes>` preference), if a tune has been associated with the event, and if the selected tune device can be opened, then, regardless of the setting of this preference, the dot pattern isn't displayed.

.. _preference-alert-messages:

Alert Messages
  Whenever a significant event with an associated message occurs (see :ref:`Alert Tunes <tunes>`):

  No
    Don't display the message.

  Yes
    Display the message.

  If alert tunes are to be played (see the :ref:`TUNES <command-TUNES>` command and the :ref:`Alert Tunes <preference-alert-tunes>` preference), if a tune has been associated with the event, and if the selected tune device can be opened, or if alert dot patterns are to be displayed (see the :ref:`Alert Dots <preference-alert-dots>` preference) and if a dot pattern has been associated with the event, then, regardless of the setting of this preference, the message isn't displayed.

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

Command Learn Mode
------------------

Command learn mode is an interactive way to learn
what the keys on the braille display do.
It can be accessed
either by the :ref:`LEARN <command-LEARN>` command
or via the :ref:`brltest <utility-brltest>` utility.
This feature isn't available if the
:ref:`--disable-learn-mode <build-learn-mode>` build option was specified.

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


.. _table-text:

Text Tables
-----------

Files with names of the form ``*.ttb`` are text tables,
and with names of the form ``*.tti`` are text subtables.
They are used by BRLTTY to translate the characters on the screen
into their corresponding 8-dot computer braille representations.

BRLTTY is initially configured to use the
:ref:`North American Braille Computer Code <nabcc>`
(NABCC) text table.
In addition to this default,
the following alternatives are provided:

.. include:: ../common/text-tables.rst

See the :ref:`-t <options-text-table>` command line option,
the :ref:`text-table <configure-text-table>` configuration file directive,
and the :ref:`--with-text-table <build-text-table>` build option
for details regarding how to use an alternate text table.


Text Table Format
~~~~~~~~~~~~~~~~~

A text table consists of a sequence of directives, one per line,
which define how each character is to be represented in braille.
``UTF-8`` character encoding must be used.
White-space (blanks, tabs) at the beginning of a line,
as well as before and/or after any operand of any directive,
is ignored.
Lines containing only white-space are ignored.
If the first non-white-space character of a line is "#"
then that line is a comment and is ignored.


Text Table Directives
~~~~~~~~~~~~~~~~~~~~~

The following directives are provided:

``char`` *character* *dots* # *comment*
  Use the ``char`` directive to specify how a Unicode character is to be represented in braille. Characters defined with this directive can also be entered from a braille keyboard. If several characters have the same braille representation then only one of them should be defined with the ``char`` directive - the others should be defined with the ``glyph`` directive (which has the same syntax). If more than one character with the same braille representation is defined with the ``char`` directive (which is currently allowed for backward compatibility) then the first one is selected.

  *character*
    The Unicode character being defined. It may be:

    - Any single character other than a backslash or a white-space character.
    - A backslash-prefixed special character. These are:

      ``\b``
        The backspace character.

      ``\f``
        The formfeed character.

      ``\n``
        The newline character.

      ``\o###``
        The three-digit octal representation of a character.

      ``\r``
        The carriage return character.

      ``\s``
        The space character.

      ``\t``
        The horizontal tab character.

      ``\u####``
        The four-digit hexadecimal representation of a character.

      ``\U########``
        The eight-digit hexadecimal representation of a character.

      ``\v``
        The vertical tab character.

      ``\x##``
        The two-digit hexadecimal representation of a character.

      ``\X##``
        ... (the case of the X and of the digits isn't significant)

      \#
        A literal number sign.

      \<name>
        The Unicode name of a character (use _ for space).

      \\
        A literal backslash.


  *dots*
    The braille representation of the Unicode character. It is a sequence of one to eight dot numbers. If the dot number sequence is enclosed within parentheses then the dot numbers may be separated from one another by white-space. A dot number is a digit within the range ``1``-``8`` as defined by the :ref:`Standard Braille Dot Numbering Convention <dots>`. The special dot number ``0`` is recognized when not enclosed within parentheses, and means no dots; it may not be used in conjunction with any other dot number.

  Examples:

  - ``char a 1``
  - ``char b (12)``
  - ``char c ( 4  1   )``
  - ``char \\ 12567``
  - ``char \s 0``
  - ``char \x20 ()``
  - ``char \<LATIN_SMALL_LETTER_D> 145``

``glyph`` *character* *dots* # *comment*
  Use the ``glyph`` directive to specify how a Unicode character is to be represented in braille. Characters defined with this directive are output-only. They cannot be entered from a braille keyboard.

  See the ``char`` directive for syntax details and for examples.

``byte`` *byte* *dots* # *comment*
  Use the ``byte`` directive to specify how a character in the local character set is to be represented in braille. It has been retained for backward compatibility but should not be used. Unicode characters should be defined (via the ``char`` directive) so that the text table remains valid regardless of what the local character set is.

  *byte*
    The local character being defined. It may be specified in the same ways as the *character* operand of the ``char`` directive except that the Unicode-specific forms (\u, \U, \<) may not be used.

  *dots*
    The braille representation of the local character. It may be specified in the same ways as the *dots* operand of the ``char`` directive.

``include`` *file* # *comment*
  Use the ``include`` directive to include the content of a text subtable. It is recursive, which means that any text subtable can itself include yet another text subtable. Care must be taken to ensure that an "include loop" is not created.

  *file*
    The file to be included. It may be either a relative or an absolute path. If relative, it is anchored at the directory containing the including file.


.. _table-attributes:

Attributes Tables
-----------------

Files with names of the form ``*.atb`` are attributes tables,
and with names of the form ``*.ati`` are attributes subtables.
They are used when BRLTTY
is displaying screen attributes rather than screen content
(see the :ref:`DISPMD <command-DISPMD>` command).
Each of the eight braille dots represents
one of the eight ``VGA`` attribute bits.

The following attributes tables are provided:

left_right
  The lefthand column represents the foreground colours:

  Dot 1
    Blue

  Dot 2
    Green

  Dot 3
    Red

  Dot 7
    Bright

  The righthand column represents the background colours:

  Dot 4
    Blue

  Dot 5
    Green

  Dot 6
    Red

  Dot 8
    Blink

  A dot is raised when its corresponding attribute bit is on. This is the default attributes table because it's the most intuitive. One of its problems, though, is that it's difficult to discern the difference between normal (white on black) and reverse (black on white) video.

invleft_right
  The lefthand column represents the foreground colours:

  Dot 1
    Blue

  Dot 2
    Green

  Dot 3
    Red

  Dot 7
    Bright

  The righthand column represents the background colours:

  Dot 4
    Blue

  Dot 5
    Green

  Dot 6
    Red

  Dot 8
    Blink

  A background bit being on triggers its corresponding dot, whereas a foreground bit being off triggers its corresponding dot. This unintuitive logic actually makes it easier to read the most commonly used attribute combinations.

upper_lower
  The upper square represents the foreground colours:

  Dot 1
    Red

  Dot 4
    Green

  Dot 2
    Blue

  Dot 5
    Bright

  The lower square represents the background colours:

  Dot 3
    Red

  Dot 6
    Green

  Dot 7
    Blue

  Dot 8
    Blink

  A dot is raised when its corresponding attribute bit is on.

See the :ref:`-a <options-attributes-table>` command line option,
the :ref:`attributes-table <configure-attributes-table>` configuration file directive,
and the :ref:`--with-attributes-table <build-attributes-table>` build option
for details regarding how to use an alternate attributes table.


Attributes Table Format
~~~~~~~~~~~~~~~~~~~~~~~

An attributes table consists of a sequence of directives, one per line,
which define how combinations of ``VGA`` attributes
are to be represented in braille.
``UTF-8`` character encoding must be used.
White-space (blanks, tabs) at the beginning of a line,
as well as before and/or after any operand of any directive,
is ignored.
Lines containing only white-space are ignored.
If the first non-white-space character of a line is "#"
then that line is a comment and is ignored.


Attributes Table Directives
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following directives are provided:

``dot`` *dot* *state* # *comment*
  Use the ``dot`` directive to specify what a specific dot represents.

  *dot*
    The dot being defined. It is a single digit within the range ``1``-``8`` as defined by the :ref:`Standard Braille Dot Numbering Convention <dots>`.

  *state*
    What the dot represents. It may be:

    ``=``\ *attribute*
      The dot is raised if the named attribute is on.

    ``~``\ *attribute*
      The dot is raised if the named attribute is off.

    The names of the attribute bits are:

    0X01
      ``fg-blue``

    0X02
      ``fg-green``

    0X04
      ``fg-red``

    0X08
      ``fg-bright``

    0X10
      ``bg-blue``

    0X20
      ``bg-green``

    0X40
      ``bg-red``

    0X80
      ``blink``


  Examples:

  - ``dot 1 =fg-red``
  - ``dot 2 ~bg-blue``

``include`` *file* # *comment*
  Use the ``include`` directive to include the content of an attributes subtable. It is recursive, which means that any attributes subtable can itself include yet another attributes subtable. Care must be taken to ensure that an "include loop" is not created.

  *file*
    The file to be included. It may be either a relative or an absolute path. If relative, it is anchored at the directory containing the including file.


.. _table-contraction:

Contraction Tables
------------------

Files with names of the form ``*.ctb`` are contraction tables,
and with names of the form ``*.cti`` are contraction subtables.
They are used by BRLTTY to translate character sequences on the screen
into their corresponding contracted braille representations.

BRLTTY presents contracted braille if:

- A contraction table has been selected. See the :ref:`-c <options-contraction-table>` command line option and the :ref:`contraction-table <configure-contraction-table>` configuration file directive for details.
- The 6-dot braille feature has been activated. See the :ref:`SIXDOTS <command-SIXDOTS>` command and the :ref:`Text Style <preference-text-style>` preference for details.

This feature isn't available if the
:ref:`--disable-contracted-braille <build-contracted-braille>` build option was specified.

The following contraction tables are provided:

.. include:: ../common/contraction-tables.rst

See the :ref:`-c <options-contraction-table>` command line option,
and the :ref:`contraction-table <configure-contraction-table>` configuration file directive
for details regarding how to use a contraction table.


Contraction Table Format
~~~~~~~~~~~~~~~~~~~~~~~~

A contraction table consists of a sequence of entries, one per line,
which define how character sequences are to be represented in braille.
``UTF-8`` character encoding must be used.
White-space (blanks, tabs) at the beginning of a line,
as well as before and/or after any operand,
is ignored.
Lines containing only white-space are ignored.
If the first non-white-space character of a line is "#"
then that line is a comment and is ignored.

The format of a contraction table entry is:

.. parsed-literal::

  *directive* *operand* ... [*comment*]

Each directive has a specific number of operands.
Any text beyond the last operand of a directive is interpreted as a comment.
The order of the entries within a contraction table is, in general,
anything that is convenient for its maintainer(s).
An entry which defines an entity, e.g. ``class``,
must precede all references to that entity.

Entries which match character sequences
are automatically rearranged from longest to shortest
so that longer matches are always preferred.
If more than one entry matches the same character sequence
then their original table ordering is maintained.
Thus, the same sequence may be translated differently under different circumstances.


Contraction Table Operands
~~~~~~~~~~~~~~~~~~~~~~~~~~

*characters*
  The first operand of a character sequence matching directive is the character sequence to be matched. Each character within the sequence may be:

  - Any single character other than a backslash or a white-space character.
  - A backslash-prefixed special character. These are:

    ``\b``
      The backspace character.

    ``\f``
      The formfeed character.

    ``\n``
      The newline character.

    ``\o###``
      The three-digit octal representation of a character.

    ``\r``
      The carriage return character.

    ``\s``
      The space character.

    ``\t``
      The horizontal tab character.

    ``\u####``
      The four-digit hexadecimal representation of a character.

    ``\U########``
      The eight-digit hexadecimal representation of a character.

    ``\v``
      The vertical tab character.

    ``\x##``
      The two-digit hexadecimal representation of a character.

    ``\X##``
      ... (the case of the X and of the digits isn't significant)

    \#
      A literal number sign.

    \<name>
      The Unicode name of a character (use _ for space).

    \\
      A literal backslash.


*representation*
  The second operand of those character sequence matching directives which have one is the braille representation of the sequence. Each braille cell is specified as a sequence of one to eight dot numbers. A dot number is a digit within the range ``1``-``8`` as defined by the :ref:`Standard Braille Dot Numbering Convention <dots>`. The special dot number ``0``, which may not be used in conjunction with any other dot number, means no dots.


.. _contraction-opcodes:

Opcodes
~~~~~~~

An opcode is a keyword which tells the translator how to interpret the operands.
The opcodes are grouped here by function.


.. _contraction-opcodes-administration:

Table Administration
^^^^^^^^^^^^^^^^^^^^

These opcodes make it easier to write contraction tables.
They have no direct effect on the character translation.

.. _contraction-opcode-include:

``include`` *path*
  Include the contents of another file. Nesting can be to any depth. Relative paths are anchored at the directory of the including file.

.. _contraction-opcode-locale:

``locale`` *locale*
  Define the locale for character interpretation (lowercase, uppercase, numeric, etc.). The locale may be specified as:

  *language*\ [``_``\ *country*\ ][``.``\ *charset*\ ][``@``\ *modifier*\ ]
    The *language* component is required and should be a two-letter ``ISO-639`` language code. The *country* component is optional and should be a two-letter ``ISO-3166`` country code. The *charset* component is optional and should be a character set name, e.g. ``ISO-8859-1``.

  C
    7-bit ASCII.

  -
    No locale.

  The last locale specification applies to the entire table. If this opcode isn't used then the ``C`` locale is assumed.


.. _contraction-opcodes-symbols:

Special Symbol Definition
^^^^^^^^^^^^^^^^^^^^^^^^^

These opcodes define special symbols which must be
inserted into the braille text in order to clarify it.

.. _contraction-opcode-capsign:

``capsign`` *dots*
  The symbol which capitalizes a single letter.

.. _contraction-opcode-begcaps:

``begcaps`` *dots*
  The symbol which begins a block of capital letters within a word.

.. _contraction-opcode-endcaps:

``endcaps`` *dots*
  The symbol which ends a block of capital letters within a word.

.. _contraction-opcode-letsign:

``letsign`` *dots*
  The symbol which marks a letter which isn't part of a word.

.. _contraction-opcode-numsign:

``numsign`` *dots*
  The symbol which marks the beginning of a number.


.. _contraction-opcodes-translation:

Character Translation
^^^^^^^^^^^^^^^^^^^^^

These opcodes define the braille representations for character sequences.
Each of them defines an entry within the contraction table.
These entries may be defined in any order except, as noted below,
when they define alternate representations for the same character sequence.

Each of these opcodes has
a *characters* operand (which must be specified as a *string*),
and a built-in condition governing its eligibility for use.
The text is processed strictly from left to right, character by character,
with the most eligible entry for each position being used.
If there's more than one eligible entry for a given position,
then the one with the longest character string is used.
If there's more than one eligible entry for the same character string,
then the one defined nearest to the beginning of the table is used
(this is the only order dependency).

Many of these opcodes have a *dots* operand
which defines the braille representation for its *characters* operand.
It may also be specified as an equals sign (``=``),
in which case it means one of two things.
If the entry is for a single character,
then it means that the currently selected computer braille representation
(see the :ref:`-t <options-text-table>` command line option
and the :ref:`text-table <configure-text-table>` configuration file directive)
for that character is to be used.
If it's for a multi-character sequence,
then the default representation for each character
(see :ref:`always <contraction-opcode-always>`)
within the sequence is to be used.

Some special terms are used within the descriptions of these opcodes.

word
  A maximal sequence of one or more consecutive letters.


Now, finally, here are the opcode descriptions themselves:

.. _contraction-opcode-literal:

``literal`` *characters*
  Translate the entire white-space-bounded containing character sequence into computer braille (see the :ref:`-t <options-text-table>` command line option and the :ref:`text-table <configure-text-table>` configuration file directive).

.. _contraction-opcode-replace:

``replace`` *characters* *characters*
  Replace the first set of characters, no matter where they appear, with the second. The replaced characters aren't reprocessed.

.. _contraction-opcode-always:

``always`` *characters* *dots*
  Translate the characters no matter where they appear. If there's only one character, then, in addition, define the default representation for that character.

.. _contraction-opcode-repeatable:

``repeatable`` *characters* *dots*
  Translate the characters no matter where they appear. Ignore any consecutive repetitions of the same sequence.

.. _contraction-opcode-largesign:

``largesign`` *characters* *dots*
  Translate the characters no matter where they appear. Remove white-space between consecutive words matched by this opcode.

.. _contraction-opcode-lastlargesign:

``lastlargesign`` *characters* *dots*
  Translate the characters no matter where they appear. Remove preceding white-space if the previous word was matched by the :ref:`largesign <contraction-opcode-largesign>` opcode.

.. _contraction-opcode-word:

``word`` *characters* *dots*
  Translate the characters if they're a word.

.. _contraction-opcode-joinword:

``joinword`` *characters* *dots*
  Translate the characters if they're a word. Remove the following white-space if the first character after it is a letter.

.. _contraction-opcode-lowword:

``lowword`` *characters* *dots*
  Translate the characters if they're a white-space-bounded word.

.. _contraction-opcode-contraction:

``contraction`` *characters*
  Prefix the characters with a letter sign (see :ref:`letsign <contraction-opcode-letsign>`) if they're a word.

.. _contraction-opcode-sufword:

``sufword`` *characters* *dots*
  Translate the characters if they're either a word or at the beginning of a word.

.. _contraction-opcode-prfword:

``prfword`` *characters* *dots*
  Translate the characters if they're either a word or at the end of a word.

.. _contraction-opcode-begword:

``begword`` *characters* *dots*
  Translate the characters if they're at the beginning of a word.

.. _contraction-opcode-begmidword:

``begmidword`` *characters* *dots*
  Translate the characters if they're either at the beginning or in the middle of a word.

.. _contraction-opcode-midword:

``midword`` *characters* *dots*
  Translate the characters if they're in the middle of a word.

.. _contraction-opcode-midendword:

``midendword`` *characters* *dots*
  Translate the characters if they're either in the middle or at the end of a word.

.. _contraction-opcode-endword:

``endword`` *characters* *dots*
  Translate the characters if they're at the end of a word.

.. _contraction-opcode-prepunc:

``prepunc`` *characters* *dots*
  Translate the characters if they're part of punctuation at the beginning of a word.

.. _contraction-opcode-postpunc:

``postpunc`` *characters* *dots*
  Translate the characters if they're part of punctuation at the end of a word.

.. _contraction-opcode-begnum:

``begnum`` *characters* *dots*
  Translate the characters if they're at the beginning of a number.

.. _contraction-opcode-midnum:

``midnum`` *characters* *dots*
  Translate the characters if they're in the middle of a number.

.. _contraction-opcode-endnum:

``endnum`` *characters* *dots*
  Translate the characters if they're at the end of a number.


.. _contraction-opcodes-classes:

Character Classes
^^^^^^^^^^^^^^^^^

These opcodes define and use character classes.
A character class associates a set of characters with a name.
The name then refers to any character within the class.
A character may belong to more than one class.

The following character classes are automatically predefined
based on the selected locale:

digit
  Numeric characters.

letter
  Both uppercase and lowercase alphabetic characters. Some locales have additional letters which are neither uppercase nor lowercase.

lowercase
  Lowercase alphabetic characters.

punctuation
  Printable characters which are neither white-space nor alphanumeric.

space
  White-space characters. In the default locale these are: space, horizontal tab, vertical tab, carriage return, new line, form feed.

uppercase
  Uppercase alphabetic characters.


The opcodes which define and use character classes are:

.. _contraction-opcode-class:

``class`` *name* *characters*
  Define a new character class. The *characters* operand must be specified as a *string*. A character class may not be used until it's been defined.

.. _contraction-opcode-after:

``after`` *class* *opcode* ...
  The specified opcode is further constrained in that the matched character sequence must be immediately preceded by a character belonging to the specified class. If this opcode is used more than once on the same line then the union of the characters in all the classes is used.

.. _contraction-opcode-before:

``before`` *class* *opcode* ...
  The specified opcode is further constrained in that the matched character sequence must be immediately followed by a character belonging to the specified class. If this opcode is used more than once on the same line then the union of the characters in all the classes is used.


.. _table-key:

Key Tables
----------

Files with names of the form ``*.ktb`` are key tables,
and with names of the form ``*.kti`` are key subtables.
They are used by BRLTTY to bind
braille display and keyboard key combinations
to BRLTTY commands.

The names of braille display key table files begin with ``brl-``\ *xx*\ ``-``",
where *xx* is the two-letter
:ref:`driver identification code <drivers>`.
The rest of the name identifies the model(s)
for which the key table is used.

The names of keyboard table files begin with ``kbd-``.
The rest of the name describes the kind of keyboard
for which the keyboard table has been designed.

The following keyboard tables are provided:

braille
  bindings for braille keyboards

desktop
  bindings for full keyboards

keypad
  bindings for keypad-based navigation

laptop
  bindings for keyboards without a keypad

sun_type6
  bindings for Sun Type 6 keyboards

See the :ref:`-k <options-keyboard-table>` command line option,
and the :ref:`keyboard-table <configure-keyboard-table>` configuration file directive
for details regarding how to select a keyboard table.


Key Table Directives
~~~~~~~~~~~~~~~~~~~~

A key table consists of a sequence of directives, one per line,
which define how keys and key combinations are to be interpreted.
``UTF-8`` character encoding must be used.
White-space (blanks, tabs) at the beginning of a line,
as well as before and/or after any operand,
is ignored.
Lines containing only white-space are ignored.
If the first non-white-space character of a line is a number (``#``) sign
then that line is a comment and is ignored.

The precedence for resolving each key press/release event is as follows:
#. A hotkey press or release defined within the current context. See the :ref:`hotkey <key-table-hotkey>` directive for details.
#. A key combination defined within the current context. See the :ref:`bind <key-table-bind>` directive for details.
#. A braille keyboard command defined within the current context. See the :ref:`map <key-table-map>` and :ref:`superimpose <key-table-superimpose>` directives for details.
#. A key combination defined within the default context. See the :ref:`bind <key-table-bind>` directive for details.


The following directives are provided:


.. _key-table-assign:

The Assign Directive
^^^^^^^^^^^^^^^^^^^^

Create or update a variable associated with the current include level.
The variable is visible to the current and to lower include levels,
but not to higher include levels.

``assign`` *variable* [*value*]

*variable*
  The name of the variable. If the variable doesn't already exist at the current include level then it is created.

*value*
  The value which is to be assigned to the variable. If it's not supplied then a zero-length (null) value is assigned.


The escape sequence \{variable} is substituted
with the value of the variable named within the braces.
The variable must have been defined
at the current or at a higher include level.

Examples:

- ``assign nullValue``
- ``assign ReturnKey Key1``
- ``bind \{ReturnKey} RETURN``


.. _key-table-bind:

The Bind Directive
^^^^^^^^^^^^^^^^^^

Define which BRLTTY command is executed
when a particular combination of one or more keys is pressed.
The binding is defined within the current context.

``bind`` *keys* *command*

*keys*
  The key combination which is to be bound. It's a sequence of one or more key names separated by plus (``+``) signs. The final (or only) key name may be optionally prefixed with an exclamation (``!``) point. The keys may be pressed in any order, with the exception that if the final key name is prefixed with an exclamation point then it must be pressed last. The exclamation point prefix means that the command is executed as soon as that key is pressed. If not used, the command is executed as soon as any of the keys is released.

*command*
  The name of a BRLTTY command. One or more modifiers may be optionally appended to the command name by using a plus (``+``) sign as the separator.

  - For commands which enable/disable a feature:
    - If the modifier ``+on`` is specified then the feature is enabled.
    - If the modifier ``+off`` is specified then the feature is disabled.
    - If neither ``+on`` nor ``+off`` is specified then the state of the feature is toggled on/off.

  - For commands which move the braille window:
    - If the modifier ``+route`` is specified then, if necessary, the cursor is automatically routed so that it's always visible on the braille display.

  - For commands which move the braille window to a specific line on the screen:
    - If the modifier ``+toleft`` is specified then the braille window is also moved to the beginning of that line.
    - If the modifier ``+scaled`` is specified then the set of keys bound to the command is interpreted as though it were a scroll bar. If it isn't, then there's a one-to-one correspondence between keys and lines.

  - For commands which require an offset:
    - The modifier +*offset*, where *offset* is a non-negative integer, may be specified. If it isn't supplied then ``+0`` is assumed.


Examples:

- ``bind Key1 CSRTRK``
- ``bind Key1+Key2 CSRTRK+off``
- ``bind Key1+Key3 CSRTRK+on``
- ``bind Key4 TOP``
- ``bind Key5 TOP+route``
- ``bind VerticalSensor GOTOLINE+toleft+scaled``
- ``bind Key6 CONTEXT+1``


.. _key-table-context:

The Context Directive
^^^^^^^^^^^^^^^^^^^^^

Define alternate ways to interpret certain key events and/or combinations.
A context contains definitions created by the
:ref:`bind <key-table-bind>`,
:ref:`hotkey <key-table-hotkey>`,
:ref:`ignore <key-table-ignore>`,
:ref:`map <key-table-map>`,
and
:ref:`superimpose <key-table-superimpose>`
directives.

``context`` *name* [*title*]

*name*
  Which context subsequent definitions are to be created within. These special contexts are predefined:

  default
    The default context. If a key combination hasn't been defined within the current context then its definition within the default context is used. This only applies to definitions created by the :ref:`bind <key-table-bind>` directive.

  menu
    This context is used when within BRLTTY's preferences menu.

*title*
  A person-readable description of the context. It may contain spaces, and standard capitalization conventions should be used. This operand is optional. If supplied when selecting a context which already has a title then the two must match. Special contexts already have internally-assigned titles.


A context is created the first time it's selected.
It may be reselected any number of times thereafter.

All subsequent definitions until
either the next :ref:`context <key-table-context>` directive
or the end of the current include level
are created within the selected context.
The initial context of the top-level key table is ``default``.
The initial context of an included key subtable
is the context which was selected when it was included.
Context changes within included key subtables
don't affect the context of the including key table or subtable.

If a context has a title
then it is persistent.
When a key event causes a persistent context to be activated,
that context remains current until a subsequent key event
causes a different persistent context to be activated.

If a context doesn't have a title
then it is temporary.
When a key event causes a temporary context to be activated,
that context is only used to interpret the very next key event.

Examples:

- ``context menu``
- ``context braille Braille Input``
- ``context DESCCHAR``


.. _key-table-hide:

The Hide Directive
^^^^^^^^^^^^^^^^^^

Specify whether or not certain definitions (see the
:ref:`bind <key-table-bind>`,
:ref:`hotkey <key-table-hotkey>`,
:ref:`map <key-table-map>`,
and
:ref:`superimpose <key-table-superimpose>`
directives) and notes (see the
:ref:`note <key-table-note>`
directive) are included within the key table's help text.

``hide`` *state*

*state*
  One of these keywords:

  on
    They're excluded.

  off
    They're included.


The specified state applies to all subsequent definitions and notes
until either the next ``hide`` directive
or the end of the current include level.
The initial state of the top-level key table is ``off``.
The initial state of an included key subtable
is the state which was selected when it was included.
State changes within included key subtables
don't affect the state of the including key table or subtable.

Examples:

- ``hide on``


.. _key-table-hotkey:

The Hotkey Directive
^^^^^^^^^^^^^^^^^^^^

Bind the press and release events of a specific key
to two separate BRLTTY commands.
The bindings are defined within the current context.

``hotkey`` *key* *press* *release*

*key*
  The name of the key which is to be bound.

*press*
  The name of the BRLTTY command which is to be executed whenever the key is pressed.

*release*
  The name of the BRLTTY command which is to be executed whenever the key is released.


Modifiers may be appended to the command names.
See the *command* operand
of the :ref:`bind <key-table-bind>` directive
for details.

Specify ``NOOP`` if no command is to be executed.
Specifying ``NOOP`` for both commands
effectively disables the key.

Examples:

- ``hotkey Key1 CSRVIS+off CSRVIS+on``
- ``hotkey Key2 NOOP NOOP``


.. _key-table-ifkey:

The IfKey Directive
^^^^^^^^^^^^^^^^^^^

Conditionally process a key table directive
only if the device has a particular key.

``ifkey`` *key* *directive*

*key*
  The name of the key whose availability is to be tested.

*directive*
  The key table directive which is to be conditionally processed.


Examples:

- ``ifkey Key1 ifkey Key2 bind Key1+Key2 HOME``


.. _key-table-include:

The Include Directive
^^^^^^^^^^^^^^^^^^^^^

Process the directives within a key subtable.
It's recursive, which means that
any key subtable can itself include yet another key subtable.
Care must be taken to ensure that an "include loop" is not created.

``include`` *file*

*file*
  The key subtable which is to be included. It may be either a relative or an absolute path. If relative, it's anchored at the directory containing the including key table or subtable.


Examples:

- ``include common.kti``
- ``include /path/to/my/keys.kti``


.. _key-table-ignore:

The Ignore Directive
^^^^^^^^^^^^^^^^^^^^

Ignore a specific key while within the current context.

``ignore`` *key*

*key*
  The name of the key which is to be ignored.


Examples:

- ``ignore Key1``


.. _key-table-map:

The Map Directive
^^^^^^^^^^^^^^^^^

Map a key to a braille keyboard function.
The mapping is defined within the current context.

``map`` *key* *function*

*key*
  The name of the key which is to be mapped. More than one key may be mapped to the same braille keyboard function.

*function*
  The name of the braille keyboard function. It may be one of the following keywords:

  DOT1
    The upper-left standard braille dot.

  DOT2
    The middle-left standard braille dot.

  DOT3
    The lower-left standard braille dot.

  DOT4
    The upper-right standard braille dot.

  DOT5
    The middle-right standard braille dot.

  DOT6
    The lower-right standard braille dot.

  DOT7
    The lower-left computer braille dot.

  DOT8
    The lower-right computer braille dot.

  SPACE
    The space bar.

  SHIFT
    The shift key.

  UPPER
    If a lowercase letter is being entered then translate it to its uppercase equivalent.

  CONTROL
    The control key.

  META
    The left alt key.


If a key combination consists only of keys
which have been mapped to braille keyboard functions,
and if those functions when combined form a valid braille keyboard command,
then that command is executed as soon as any of the keys is released.
A valid braille keyboard command must include
either any combination of dot keys or the space bar (but not both).
If at least one dot key is included
then the braille keyboard functions specified by the
:ref:`superimpose <key-table-superimpose>`
directives within the same context are also implicitly included.

Examples:

- ``map Key1 DOT1``


.. _key-table-note:

The Note Directive
^^^^^^^^^^^^^^^^^^

Add a person-readable explanation to the key table's help text.
Notes are commonly used, for example,
to describe the placement, sizes, and shapes of the keys on the device.

``note`` *text*

*text*
  The explanation which is to be added. It may contain spaces, and should be grammatically correct.


Each note specifies exactly one line of explanatory text.
Leading space is ignored so indentation cannot be specified.

There's no limit to the number of notes which may be specified.
All of them are gathered together
and presented in a single block
at the start of the key table's help text.

Examples:

- ``note Key1 is the round key at the far left on the front surface.``


.. _key-table-superimpose:

The Superimpose Directive
^^^^^^^^^^^^^^^^^^^^^^^^^

Implicitly include a braille keyboard function whenever
a braille keyboard command consisting of at least one dot is executed.
The implicit inclusion is defined within the current context.
Any number of them may be specified.

``superimpose`` *function*

*function*
  The name of the braille keyboard function. See the *function* operand of the :ref:`map <key-table-map>` directive for details.


Examples:

- ``superimpose DOT7``


.. _key-table-title:

The Title Directive
^^^^^^^^^^^^^^^^^^^

Provide a person-readable summary of the key table's purpose.

``title`` *text*

*text*
  A one-line summary of what the key table is used for. It may contain spaces, and standard capitalization conventions should be used.


The title of the key table may be specified only once.

Examples:

- ``title Bindings for Keypad-based Navigation``


.. _keyboard-properties:

Keyboard Properties
~~~~~~~~~~~~~~~~~~~

The default is that all keyboards are monitored.
A subset of the keyboards may be selected by specifying one or more of the following properties
(see the :ref:`-K <options-keyboard-properties>` command line option,
and the :ref:`keyboard-properties <configure-keyboard-properties>` configuration file directive):

type
  The bus type, specified as one of the following keywords: ``any``, ``ps2``, ``usb``, ``bluetooth``.

vendor
  The vendor identifier, specified as a 16-bit unsigned integer.

product
  The product identifier, specified as a 16-bit unsigned integer.


The vendor and product identifiers may be specified in
decimal (no prefix),
octal (prefixed by ``0``),
or hexadecimal (prefixed by ``0x``).
Specifying ``0`` means match any value
(as if the property weren't specified).


Advanced Topics
===============


.. _multiple:

Installing Multiple Versions
----------------------------

It's easy to have more than one version of BRLTTY
installed on the same system at the same time.
This capability allows you to test a new version before removing the old one.

The :ref:`--with-execute-root <build-execute-root>` build option allows you
to install the complete :ref:`installed file hierarchy <hierarchy>`
anywhere you want such that it's entirely self-contained.
Remembering that it's best to keep all of BRLTTY's components
within the root file system, you can build it like this:

::

  ./configure --with-execute-root=/brltty-3.1
  make install
  You can then run it like this:


::

  /brltty-3.1/bin/brltty

When version 3.2 is released, just install it in a different location
and run the new executable from there.

::

  ./configure --with-execute-root=/brltty-3.2
  make install
  /brltty-3.2/bin/brltty

  So far, this paradigm is somewhat awkward for at least two reasons.
  One is that these long path names are too hard to type,
  and the other is that you don't want to fiddle with your system's boot sequence
  each time you want to switch to a different version of BRLTTY.
  These problems are easily solved by adding a symbolic link for the executable.


::

  ln -s /brltty-3.1/bin/brltty /bin/brltty

When it's time to switch to the newer version, just repoint the symbolic link.

.. parsed-literal::

  ln -s /brltty-3.2/bin/brltty /bin/brltty


If you'd like to get really fancy, then introduce another level of indirection
in order to make all of BRLTTY's files for any given version
look like they're in all of the standard places.
First, create a symbolic link through a common repointable location
from each of BRLTTY's standard locations.

::

  ln -s /brltty/bin/brltty /bin/brltty
  ln -s /brltty/etc/brltty /etc/brltty
  ln -s /brltty/lib/brltty /lib/brltty

Then all you need to do is to point ``/brltty`` to the desired version.

::

  ln -s /brltty-3.1 /brltty


Installation/Rescue Root Disks for Linux
----------------------------------------

BRLTTY can run as a stand-alone executable.
Everything it needs to know can be explicitly configured at build-time
(see :ref:`Build Options <build>`).
If the data directory
(see the :ref:`--with-data-directory <build-data-directory>`
and :ref:`--with-execute-root <build-execute-root>` build options)
doesn't exist, then BRLTTY looks in ``/etc`` for the files it needs.
Even if any of these files don't exist at all, BRLTTY still works!

If, for some reason, you ever create the data directory
(usually ``/etc/brltty``) by hand, it's important
to set its permissions so that only root can create files within it.

.. parsed-literal::

  chmod 755 /etc/brltty


The screen content inspection device (usually ``/dev/vcsa``) is required.
It should already exist unless your Linux distribution is quite old.
If necessary, you can create it with:

::

  mknod /dev/vcsa c 7 128
  chmod 660 /dev/vcsa
  chown root.tty /dev/vcsa

  One problem often encountered when trying to use BRLTTY
  in an uncertain environment like a root disk or an incomplete system is
  that it might not find the shared libraries (or parts thereof) which it needs.
  Root disks often use subset and/or outdated versions of the libraries
  which may be inadequate.
  The solution is to configure BRLTTY with the
  <ref id="build-standalone-programs" name="--enable-standalone-programs"> build option.
  This removes all dependencies on shared libraries,
  but, unfortunately, also creates a larger executable.
  There are a number of build options which can be used
  to selectively remove unneeded features from BRLTTY
  in order to somewhat mitigate this problem
  (see section <ref id="build-features" name="Build Features">).

  The executable is stripped during the <ref id="make-install" name="make install">.
  This significantly reduces its size by removing its symbol table.
  You'll get a much smaller executable, therefore,
  if you complete the full build procedure,
  and then copy it from its installed location.
  If, however, you copy it from the build directory, it'll be way too big.
  Don't forget to strip it.


::

  strip brltty


Future Enhancements
-------------------

Apart from fixing bugs and supporting more types of braille displays,
we hope, time permitting, to work on the following:
Better Attribute Handling

  - Attribute tracking.
  - Mixed text and attribute mode.

Scroll Tracking
  Locking the braille window to one line as it scrolls on the screen.

Better Speech Support

  - Mixed braille and speech for faster reading of text.
  - Better speech navigation.
  - More speech synthesizers.

Screen Subregions
  Ignore cursor motion outside the region, and set soft navigational boundaries at the edges of the region.

See the file ``TODO`` for a more complete list.


Known Bugs
----------

At the time of writing (December 2001), the following problems are known:

Cursor routing is implemented as a looping sub-process
which runs at reduced priority to avoid using too much cpu time.
Different system loads require different settings of its parameters.
The defaults work very well
in a typical Unix editor on a fairly lightly loaded system,
but very poorly in some other situations,
e.g. over a slow serial link to a remote host.


.. _displays:

Supported Braille Displays
==========================

BRLTTY supports the following braille displays:

.. include:: ../common/braille-drivers.rst


.. _synthesizers:

Supported Speech Synthesizers
=============================

BRLTTY supports the following speech synthesizers:

.. include:: ../common/speech-drivers.rst


.. _drivers:

Driver Identification Codes
===========================


.. include:: ../common/driver-codes.rst


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
and the :ref:`--with-braille-device <build-braille-device>` build option)
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

A standard braille cell consists of six dots
arranged in three rows and two columns.
Each dot can be specifically identified by its number as follows:

1
  Top-left (row 1, column 1).

2
  Middle-left (row 2, column 1).

3
  Bottom-left (row 3, column 1).

4
  Top-right (row 1, column 2).

5
  Middle-right (row 2, column 2).

6
  Bottom-right (row 3, column 2).


Computer braille has introduced a fourth row at the bottom.

7
  Below-left (row 4, column 1).

8
  Below-right (row 4, column 2).


Perhaps a picture will make this numbering convention easier to understand.

::

  1 o o 4
  2 o o 5
  3 o o 6
  7 o o 8

.. _nabcc:

North American Braille Computer Code
=====================================

::

  Num Hex     Dots     Description
  +-----------------------------------------------------------------+
  |   0  00  [7   4  8]  NUL (null)                                 |
  |   1  01  [7  1   8]  SOH (start of header)                      |
  |   2  02  [7 21   8]  STX (start of text)                        |
  |   3  03  [7  14  8]  ETX (end of text)                          |
  |   4  04  [7  145 8]  EOT (end of transmission)                  |
  |   5  05  [7  1 5 8]  ENQ (enquiry)                              |
  |   6  06  [7 214  8]  ACK (acknowledge)                          |
  |   7  07  [7 2145 8]  BEL (bell)                                 |
  |   8  08  [7 21 5 8]  BS (back space)                            |
  |   9  09  [7 2 4  8]  HT (horizontal tab)                        |
  |  10  0A  [7 2 45 8]  LF (line feed)                             |
  |  11  0B  [73 1   8]  VT (vertical tab)                          |
  |  12  0C  [7321   8]  FF (form feed)                             |
  |  13  0D  [73 14  8]  CR (carriage return)                       |
  |  14  0E  [73 145 8]  SO (shift out)                             |
  |  15  0F  [73 1 5 8]  SI (shift in)                              |
  |  16  10  [73214  8]  DLE (data link escape)                     |
  |  17  11  [732145 8]  DC1 (direct control 1)                     |
  |  18  12  [7321 5 8]  DC2 (direct control 2)                     |
  |  19  13  [732 4  8]  DC3 (direct control 3)                     |
  |  20  14  [732 45 8]  DC4 (direct control 4)                     |
  |  21  15  [73 1  68]  NAK (negative acknowledge)                 |
  |  22  16  [7321  68]  SYN (synchronize)                          |
  |  23  17  [7 2 4568]  ETB (end of text block)                    |
  |  24  18  [73 14 68]  CAN (cancel)                               |
  |  25  19  [73 14568]  EM (end of medium)                         |
  |  26  1A  [73 1 568]  SUB (substitute)                           |
  |  27  1B  [7 2 4 68]  ESC (escape)                               |
  |  28  1C  [7 21 568]  FS (file separator)                        |
  |  29  1D  [7 214568]  GS (group separator)                       |
  |  30  1E  [7   45 8]  RS (record separator)                      |
  |  31  1F  [7   4568]  US (unit separator)                        |
  |  32  20  [        ]  space                                      |
  |  33  21  [ 32 4 6 ]  exclamation point                          |
  |  34  22  [     5  ]  quotation mark                             |
  |  35  23  [ 3  456 ]  number sign                                |
  |  36  24  [  214 6 ]  dollar sign                                |
  |  37  25  [   14 6 ]  percent sign                               |
  |  38  26  [ 3214 6 ]  ampersand                                  |
  |  39  27  [ 3      ]  acute accent                               |
  |  40  28  [ 321 56 ]  left parenthesis                           |
  |  41  29  [ 32 456 ) right parenthesis                          |
  |  42  2A  [   1  6 ]  asterisk                                   |
  |  43  2B  [ 3  4 6 ]  plus sign                                  |
  |  44  2C  [      6 ]  comma                                      |
  |  45  2D  [ 3    6 ]  minus sign                                 |
  |  46  2E  [    4 6 ]  period                                     |
  |  47  2F  [ 3  4   ]  forward slash                              |
  |  48  30  [ 3   56 ]  zero                                       |
  |  49  31  [  2     ]  one                                        |
  |  50  32  [ 32     ]  two                                        |
  |  51  33  [  2  5  ]  three                                      |
  |  52  34  [  2  56 ]  four                                       |
  |  53  35  [  2   6 ]  five                                       |
  |  54  36  [ 32  5  ]  six                                        |
  |  55  37  [ 32  56 ]  seven                                      |
  |  56  38  [ 32   6 ]  eight                                      |
  |  57  39  [ 3   5  ]  nine                                       |
  |  58  3A  [   1 56 ]  colon                                      |
  |  59  3B  [     56 ]  semicolon                                  |
  |  60  3C  [  21  6 ]  less-than sign                             |
  |  61  3D  [ 321456 ]  equals sign                                |
  |  62  3E  [ 3  45  ]  greater-than sign                          |
  |  63  3F  [   1456 ]  question mark                              |
  |  64  40  [7   4   ]  commercial at                              |
  |  65  41  [7  1    ]  capital a                                  |
  |  66  42  [7 21    ]  capital b                                  |
  |  67  43  [7  14   ]  capital c                                  |
  |  68  44  [7  145  ]  capital d                                  |
  |  69  45  [7  1 5  ]  capital e                                  |
  |  70  46  [7 214   ]  capital f                                  |
  |  71  47  [7 2145  ]  capital g                                  |
  |  72  48  [7 21 5  ]  capital h                                  |
  |  73  49  [7 2 4   ]  capital i                                  |
  |  74  4A  [7 2 45  ]  capital j                                  |
  |  75  4B  [73 1    ]  capital k                                  |
  |  76  4C  [7321    ]  capital l                                  |
  |  77  4D  [73 14   ]  capital m                                  |
  |  78  4E  [73 145  ]  capital n                                  |
  |  79  4F  [73 1 5  ]  capital o                                  |
  |  80  50  [73214   ]  capital p                                  |
  |  81  51  [732145  ]  capital q                                  |
  |  82  52  [7321 5  ]  capital r                                  |
  |  83  53  [732 4   ]  capital s                                  |
  |  84  54  [732 45  ]  capital t                                  |
  |  85  55  [73 1  6 ]  capital u                                  |
  |  86  56  [7321  6 ]  capital v                                  |
  |  87  57  [7 2 456 ]  capital w                                  |
  |  88  58  [73 14 6 ]  capital x                                  |
  |  89  59  [73 1456 ]  capital y                                  |
  |  90  5A  [73 1 56 ]  capital z                                  |
  |  91  5B  [7 2 4 6 ]  left bracket                               |
  |  92  5C  [7 21 56 ]  backward slash                             |
  |  93  5D  [7 21456 ]  right bracket                              |
  |  94  5E  [7   45  ]  circumflex accent                          |
  |  95  5F  [    456 ]  underscore                                 |
  |  96  60  [    4   ]  grave accent                               |
  |  97  61  [   1    ]  small a                                    |
  |  98  62  [  21    ]  small b                                    |
  |  99  63  [   14   ]  small c                                    |
  | 100  64  [   145  ]  small d                                    |
  | 101  65  [   1 5  ]  small e                                    |
  | 102  66  [  214   ]  small f                                    |
  | 103  67  [  2145  ]  small g                                    |
  | 104  68  [  21 5  ]  small h                                    |
  | 105  69  [  2 4   ]  small i                                    |
  | 106  6A  [  2 45  ]  small j                                    |
  | 107  6B  [ 3 1    ]  small k                                    |
  | 108  6C  [ 321    ]  small l                                    |
  | 109  6D  [ 3 14   ]  small m                                    |
  | 110  6E  [ 3 145  ]  small n                                    |
  | 111  6F  [ 3 1 5  ]  small o                                    |
  | 112  70  [ 3214   ]  small p                                    |
  | 113  71  [ 32145  ]  small q                                    |
  | 114  72  [ 321 5  ]  small r                                    |
  | 115  73  [ 32 4   ]  small s                                    |
  | 116  74  [ 32 45  ]  small t                                    |
  | 117  75  [ 3 1  6 ]  small u                                    |
  | 118  76  [ 321  6 ]  small v                                    |
  | 119  77  [  2 456 ]  small w                                    |
  | 120  78  [ 3 14 6 ]  small x                                    |
  | 121  79  [ 3 1456 ]  small y                                    |
  | 122  7A  [ 3 1 56 ]  small z                                    |
  | 123  7B  [  2 4 6 ]  left brace                                 |
  | 124  7C  [  21 56 ]  vertical bar                               |
  | 125  7D  [  21456 ]  right brace                                |
  | 126  7E  [    45  ]  tilde accent                               |
  | 127  7F  [7   456 ]  DEL (delete)                               |
  | 128  80  [    4  8]  <control>                                  |
  | 129  81  [   1   8]  <control>                                  |
  | 130  82  [  21   8]  BPH (break permitted here)                 |
  | 131  83  [   14  8]  NBH (no break here)                        |
  | 132  84  [   145 8]  <control>                                  |
  | 133  85  [   1 5 8]  NL (next line)                             |
  | 134  86  [  214  8]  SSA (start of selected area)               |
  | 135  87  [  2145 8]  ESA (end of selected area)                 |
  | 136  88  [  21 5 8]  CTS (character tabulation set)             |
  | 137  89  [  2 4  8]  CTJ (character tabulation justification)   |
  | 138  8A  [  2 45 8]  LTS (line tabulation set)                  |
  | 139  8B  [ 3 1   8]  PLD (partial line down)                    |
  | 140  8C  [ 321   8]  PLU (partial line up)                      |
  | 141  8D  [ 3 14  8]  RLF (reverse line feed)                    |
  | 142  8E  [ 3 145 8]  SS2 (single shift two)                     |
  | 143  8F  [ 3 1 5 8]  SS3 (single shift three)                   |
  | 144  90  [ 3214  8]  DCS (device control string)                |
  | 145  91  [ 32145 8]  PU1 (private use one)                      |
  | 146  92  [ 321 5 8]  PU2 (private use two)                      |
  | 147  93  [ 32 4  8]  STS (set transmit state)                   |
  | 148  94  [ 32 45 8]  CC (cancel character)                      |
  | 149  95  [ 3 1  68]  MW (message waiting)                       |
  | 150  96  [ 321  68]  SGA (start of guarded area)                |
  | 151  97  [  2 4568]  EGA (end of guarded area)                  |
  | 152  98  [ 3 14 68]  SS (start of string)                       |
  | 153  99  [ 3 14568]  <control>                                  |
  | 154  9A  [ 3 1 568]  SCI (single character introducer)          |
  | 155  9B  [  2 4 68]  CSI (control sequence introducer)          |
  | 156  9C  [  21 568]  ST (string terminator)                     |
  | 157  9D  [  214568]  OSC (operating system command)             |
  | 158  9E  [    45 8]  PM (privacy message)                       |
  | 159  9F  [    4568]  APC (application program command)          |
  | 160  A0  [7      8]  no-break space                             |
  | 161  A1  [732 4 6 ]  inverted exclamation mark                  |
  | 162  A2  [7 214 6 ]  cent sign                                  |
  | 163  A3  [73  456 ]  pound sign                                 |
  | 164  A4  [7  14 6 ]  currency sign                              |
  | 165  A5  [73214 6 ]  yen sign                                   |
  | 166  A6  [7  1 56 ]  broken bar                                 |
  | 167  A7  [73   5  ]  section sign                               |
  | 168  A8  [7    5  ]  diaeresis                                  |
  | 169  A9  [732  56 ]  copyright sign                             |
  | 170  AA  [       8]  feminine ordinal indicator                 |
  | 171  AB  [7 21  6 ]  left-pointing double angle quotation mark  |
  | 172  AC  [7 2  56 ]  not sign                                   |
  | 173  AD  [73    6 ]  soft hyphen                                |
  | 174  AE  [732   6 ]  registered sign                            |
  | 175  AF  [7 2   6 ]  macron                                     |
  | 176  B0  [73   56 ]  degree sign                                |
  | 177  B1  [73  4 6 ]  plus-minus sign                            |
  | 178  B2  [732     ]  superscript two                            |
  | 179  B3  [7 2  5  ]  superscript three                          |
  | 180  B4  [73      ]  acute accent                               |
  | 181  B5  [7    56 ]  micro sign                                 |
  | 182  B6  [732  5  ]  pilcrow sign                               |
  | 183  B7  [7   4 6 ]  middle dot                                 |
  | 184  B8  [7     6 ]  cedilla                                    |
  | 185  B9  [7 2     ]  superscript one                            |
  | 186  BA  [7       ]  masculine ordinal indicator                |
  | 187  BB  [73  45  ]  right-pointing double angle quotation mark |
  | 188  BC  [7321 56 ]  vulgar fraction one quarter                |
  | 189  BD  [7321456 ]  vulgar fraction one half                   |
  | 190  BE  [732 456 ]  vulgar fraction three quarters             |
  | 191  BF  [7  1456 ]  inverted question mark                     |
  | 192  C0  [732  5 8]  capital a grave                            |
  | 193  C1  [7  1  68]  capital a acute                            |
  | 194  C2  [7 2    8]  capital a circumflex                       |
  | 195  C3  [7    5 8]  capital a tilde                            |
  | 196  C4  [73214 68]  capital a diaeresis                        |
  | 197  C5  [73  45 8]  capital a ring above                       |
  | 198  C6  [73     8]  capital ae                                 |
  | 199  C7  [73  4 68]  capital c cedilla                          |
  | 200  C8  [732  568]  capital e grave                            |
  | 201  C9  [7 21  68]  capital e acute                            |
  | 202  CA  [732    8]  capital e circumflex                       |
  | 203  CB  [73214568]  capital e diaeresis                        |
  | 204  CC  [732   68]  capital i grave                            |
  | 205  CD  [7  14 68]  capital i acute                            |
  | 206  CE  [7 2  5 8]  capital i circumflex                       |
  | 207  CF  [7321 568]  capital i diaeresis                        |
  | 208  D0  [7     68]  capital eth                                |
  | 209  D1  [7   4 68]  capital n tilde                            |
  | 210  D2  [73   5 8]  capital o grave                            |
  | 211  D3  [7  14568]  capital o acute                            |
  | 212  D4  [7 2  568]  capital o circumflex                       |
  | 213  D5  [7    568]  capital o tilde                            |
  | 214  D6  [732 4 68]  capital o diaeresis                        |
  | 215  D7  [7  1  6 ]  multiplication sign                        |
  | 216  D8  [73  4  8]  capital o stroke                           |
  | 217  D9  [73   568]  capital u grave                            |
  | 218  DA  [7  1 568]  capital u acute                            |
  | 219  DB  [7 2   68]  capital u circumflex                       |
  | 220  DC  [732 4568]  capital u diaeresis                        |
  | 221  DD  [7 214 68]  capital y acute                            |
  | 222  DE  [73    68]  capital thorn                              |
  | 223  DF  [73  4568]  small sharp s                              |
  | 224  E0  [ 32  5 8]  small a grave                              |
  | 225  E1  [   1  68]  small a acute                              |
  | 226  E2  [  2    8]  small a circumflex                         |
  | 227  E3  [     5 8]  small a tilde                              |
  | 228  E4  [ 3214 68]  small a diaeresis                          |
  | 229  E5  [ 3  45 8]  small a ring above                         |
  | 230  E6  [ 3     8]  small ae                                   |
  | 231  E7  [ 3  4 68]  small c cedilla                            |
  | 232  E8  [ 32  568]  small e grave                              |
  | 233  E9  [  21  68]  small e acute                              |
  | 234  EA  [ 32    8]  small e circumflex                         |
  | 235  EB  [ 3214568]  small e diaeresis                          |
  | 236  EC  [ 32   68]  small i grave                              |
  | 237  ED  [   14 68]  small i acute                              |
  | 238  EE  [  2  5 8]  small i circumflex                         |
  | 239  EF  [ 321 568]  small i diaeresis                          |
  | 240  F0  [      68]  small eth                                  |
  | 241  F1  [    4 68]  small n tilde                              |
  | 242  F2  [ 3   5 8]  small o grave                              |
  | 243  F3  [   14568]  small o acute                              |
  | 244  F4  [  2  568]  small o circumflex                         |
  | 245  F5  [     568]  small o tilde                              |
  | 246  F6  [ 32 4 68]  small o diaeresis                          |
  | 247  F7  [73  4   ]  division sign                              |
  | 248  F8  [ 3  4  8]  small o stroke                             |
  | 249  F9  [ 3   568]  small u grave                              |
  | 250  FA  [   1 568]  small u acute                              |
  | 251  FB  [  2   68]  small u circumflex                         |
  | 252  FC  [ 32 4568]  small u diaeresis                          |
  | 253  FD  [  214 68]  small y acute                              |
  | 254  FE  [ 3    68]  small thorn                                |
  | 255  FF  [ 3  4568]  small y diaeresis                          |
  +-----------------------------------------------------------------+

.. _midi:

MIDI Instrument Table
=====================

.. list-table::
   :header-rows: 1
   :widths: 10 10 80

   * - Number
     - Category
     - Instrument
   * - 0
     - Piano
     - Acoustic Grand Piano
   * - 1
     - Piano
     - Bright Acoustic Piano
   * - 2
     - Piano
     - Electric Grand Piano
   * - 3
     - Piano
     - Honkytonk Piano
   * - 4
     - Piano
     - Electric Piano 1
   * - 5
     - Piano
     - Electric Piano 2
   * - 6
     - Piano
     - Harpsichord
   * - 7
     - Piano
     - Clavi
   * - 8
     - Chromatic Percussion
     - Celesta
   * - 9
     - Chromatic Percussion
     - Glockenspiel
   * - 10
     - Chromatic Percussion
     - Music Box
   * - 11
     - Chromatic Percussion
     - Vibraphone
   * - 12
     - Chromatic Percussion
     - Marimba
   * - 13
     - Chromatic Percussion
     - Xylophone
   * - 14
     - Chromatic Percussion
     - Tubular Bells
   * - 15
     - Chromatic Percussion
     - Dulcimer
   * - 16
     - Organ
     - Drawbar Organ
   * - 17
     - Organ
     - Percussive Organ
   * - 18
     - Organ
     - Rock Organ
   * - 19
     - Organ
     - Church Organ
   * - 20
     - Organ
     - Reed Organ
   * - 21
     - Organ
     - Accordion
   * - 22
     - Organ
     - Harmonica
   * - 23
     - Organ
     - Tango Accordion
   * - 24
     - Guitar
     - Nylon Acoustic Guitar
   * - 25
     - Guitar
     - Steel Acoustic Guitar
   * - 26
     - Guitar
     - Jazz Electric Guitar
   * - 27
     - Guitar
     - Clean Electric Guitar
   * - 28
     - Guitar
     - Muted Electric Guitar
   * - 29
     - Guitar
     - Overdriven Guitar
   * - 30
     - Guitar
     - Distortion Guitar
   * - 31
     - Guitar
     - Guitar Harmonics
   * - 32
     - Bass
     - Acoustic Bass
   * - 33
     - Bass
     - Finger Electric Bass
   * - 34
     - Bass
     - Pick Electric Bass
   * - 35
     - Bass
     - Fretless Bass
   * - 36
     - Bass
     - Slap Bass 1
   * - 37
     - Bass
     - Slap Bass 2
   * - 38
     - Bass
     - Synth Bass 1
   * - 39
     - Bass
     - Synth Bass 2
   * - 40
     - Strings
     - Violin
   * - 41
     - Strings
     - Viola
   * - 42
     - Strings
     - Cello
   * - 43
     - Strings
     - Contrabass
   * - 44
     - Strings
     - Tremolo Strings
   * - 45
     - Strings
     - Pizzicato Strings
   * - 46
     - Strings
     - Orchestral Harp
   * - 47
     - Strings
     - Timpani
   * - 48
     - Ensemble
     - String Ensemble 1
   * - 49
     - Ensemble
     - String Ensemble 2
   * - 50
     - Ensemble
     - Synth Strings 1
   * - 51
     - Ensemble
     - Synth Strings 2
   * - 52
     - Ensemble
     - Choir Aahs
   * - 53
     - Ensemble
     - Voice Oohs
   * - 54
     - Ensemble
     - Synth Voice
   * - 55
     - Ensemble
     - Orchestra Hit
   * - 56
     - Brass
     - Trumpet
   * - 57
     - Brass
     - Trombone
   * - 58
     - Brass
     - Tuba
   * - 59
     - Brass
     - Muted Trumpet
   * - 60
     - Brass
     - French Horn
   * - 61
     - Brass
     - Brass Section
   * - 62
     - Brass
     - Synth Brass 1
   * - 63
     - Brass
     - Synth Brass 2
   * - 64
     - Reed
     - Soprano Saxophone
   * - 65
     - Reed
     - Alto Saxophone
   * - 66
     - Reed
     - Tenor Saxophone
   * - 67
     - Reed
     - Baritone Saxophone
   * - 68
     - Reed
     - Oboe
   * - 69
     - Reed
     - English Horn
   * - 70
     - Reed
     - Bassoon
   * - 71
     - Reed
     - Clarinet
   * - 72
     - Pipe
     - Piccolo
   * - 73
     - Pipe
     - Flute
   * - 74
     - Pipe
     - Recorder
   * - 75
     - Pipe
     - Pan Flute
   * - 76
     - Pipe
     - Blown Bottle
   * - 77
     - Pipe
     - Shakuhachi
   * - 78
     - Pipe
     - Whistle
   * - 79
     - Pipe
     - Ocarina
   * - 80
     - Synth Lead
     - Lead 1 (square)
   * - 81
     - Synth Lead
     - Lead 2 (sawtooth)
   * - 82
     - Synth Lead
     - Lead 3 (calliope)
   * - 83
     - Synth Lead
     - Lead 4 (chiff)
   * - 84
     - Synth Lead
     - Lead 5 (charang)
   * - 85
     - Synth Lead
     - Lead 6 (voice)
   * - 86
     - Synth Lead
     - Lead 7 (fifths)
   * - 87
     - Synth Lead
     - Lead 8 (bass + lead)
   * - 88
     - Synth Pad
     - Pad 1 (new age)
   * - 89
     - Synth Pad
     - Pad 2 (warm)
   * - 90
     - Synth Pad
     - Pad 3 (polysynth)
   * - 91
     - Synth Pad
     - Pad 4 (choir)
   * - 92
     - Synth Pad
     - Pad 5 (bowed)
   * - 93
     - Synth Pad
     - Pad 6 (metallic)
   * - 94
     - Synth Pad
     - Pad 7 (halo)
   * - 95
     - Synth Pad
     - Pad 8 (sweep)
   * - 96
     - Synth FM
     - FX 1 (rain)
   * - 97
     - Synth FM
     - FX 2 (soundtrack)
   * - 98
     - Synth FM
     - FX 3 (crystal)
   * - 99
     - Synth FM
     - FX 4 (atmosphere)
   * - 100
     - Synth FM
     - FX 5 (brightness)
   * - 101
     - Synth FM
     - FX 6 (goblins)
   * - 102
     - Synth FM
     - FX 7 (echoes)
   * - 103
     - Synth FM
     - FX 8 (science-fiction)
   * - 104
     - Ethnic
     - Sitar
   * - 105
     - Ethnic
     - Banjo
   * - 106
     - Ethnic
     - Shamisen
   * - 107
     - Ethnic
     - Koto
   * - 108
     - Ethnic
     - Kalimba
   * - 109
     - Ethnic
     - Bag Pipe
   * - 110
     - Ethnic
     - Fiddle
   * - 111
     - Ethnic
     - Shanai
   * - 112
     - Percussive
     - Tinkle Bell
   * - 113
     - Percussive
     - Agogo
   * - 114
     - Percussive
     - Steel Drum
   * - 115
     - Percussive
     - Wooden Block
   * - 116
     - Percussive
     - Taiko Drum
   * - 117
     - Percussive
     - Melodic Tom
   * - 118
     - Percussive
     - Synth Drum
   * - 119
     - Percussive
     - Reverse Cymbal
   * - 120
     - Sound Effects
     - Guitar Fret Noise
   * - 121
     - Sound Effects
     - Breath Noise
   * - 122
     - Sound Effects
     - Seashore
   * - 123
     - Sound Effects
     - Bird Tweet
   * - 124
     - Sound Effects
     - Telephone Ring
   * - 125
     - Sound Effects
     - Helicopter
   * - 126
     - Sound Effects
     - Applause
   * - 127
     - Sound Effects
     - Gunshot
