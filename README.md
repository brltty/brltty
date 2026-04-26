# BRLTTY

BRLTTY is a background daemon that gives a blind user access to text
consoles on Linux and similar systems via a refreshable braille
display. It can drive the display from very early in the boot
sequence, so the system is usable in single-user mode, during
recovery, and for ordinary day-to-day work.

Version 6.9.1, April 2026 · Copyright © 1995-2026 The BRLTTY
Developers · <http://brltty.app/>

## About refreshable braille

A refreshable braille display is a piece of hardware containing a
row of cells, each made up of small pins that can be raised or
lowered to render braille characters. BRLTTY drives the cells to
reflect a rectangular portion of the screen — referred to in the
documentation as *the window* — so the user can read what would
normally appear visually. Buttons or routing keys on the display
itself send commands back to BRLTTY: move the window around the
screen, toggle viewing options, copy text, switch virtual consoles,
and so on.

Synthesized speech is the more widely known accessibility technology
for blind computer users; the two complement each other rather than
competing, and BRLTTY can drive a speech synthesizer alongside the
braille display.

## What BRLTTY does

* Reflects a configurable rectangular region of the screen as
  braille, with intelligent cursor tracking and routing.
* Translates output through pluggable text and contraction tables —
  English and French ship in the box; many more languages are
  available as add-on tables. Eight-dot computer braille and six-dot
  contracted braille are both supported.
* Implements a full screen-review feature set: highlighted-text
  inspection (attribute mode), cut-and-paste, smart copy of URLs and
  e-mail addresses, configurable cursor styles and blinking, and a
  preferences menu.
* Provides an on-line learn mode for discovering commands, optional
  speech output, an interactive on-line help facility, and a
  programmable C API (BrlAPI) for client applications.

## Hardware support

BRLTTY supports 50+ braille display vendors and 12 speech-synthesis
engines. The authoritative lists ship as machine-readable CSV files
in this repository:

* [`Documents/braille-driver.csv`](Documents/braille-driver.csv) —
  supported braille displays.
* [`Documents/speech-driver.csv`](Documents/speech-driver.csv) —
  supported speech synthesizers.
* [`Documents/screen-driver.csv`](Documents/screen-driver.csv) —
  supported screen drivers (Linux console, AT-SPI desktops, tmux,
  Android, etc.).

Linux is the primary platform. Android, Windows, and Hurd are
supported with varying degrees of completeness — see
[`Documents/README.Android`](Documents/README.Android),
[`Documents/README.Windows`](Documents/README.Windows), and the
other platform-specific READMEs in [`Documents/`](Documents/).

## Documentation

* The **[BRLTTY Reference Manual](Documents/Manual-BRLTTY/English/BRLTTY.rst)**
  is the user-facing reference: installation, command list, key
  bindings, preferences, configuration-file syntax, and feature
  descriptions. A French translation lives in
  [`Documents/Manual-BRLTTY/French/`](Documents/Manual-BRLTTY/French/).
* The **[BrlAPI Reference Manual](Documents/Manual-BrlAPI/English/BrlAPI.rst)**
  covers the C API for client applications.
* The **[`Documents/README.*`](Documents/) topic family** drills
  deeper into specific subjects: text tables, contraction tables,
  attributes tables, key tables, Bluetooth, devices, customization,
  systemd integration, and platform-specific notes.
* **[`Documents/brltty.conf`](Documents/brltty.conf)** is the
  comment-rich configuration-file template most distributions
  install as `/etc/brltty.conf`. It is the authoritative
  directive-syntax reference.
* The **`brltty(1)`** and **`brlapi(3)`** man pages document the
  command-line interface and the C API.

## Getting help

* **Mailing list:** post to <BRLTTY@brltty.app>; subscribe at
  <http://brltty.app/mailman/listinfo/brltty>.
* **Bug reports and feature requests:** the project's GitHub issue
  tracker. A useful bug report typically includes the BRLTTY version
  (`brltty -V`), your braille-display model, the host OS and version,
  your configuration file, and a debug-level startup log captured
  with `brltty -n -e -l debug`.

## Building from source

Most Linux distributions ship BRLTTY as a package; install it with
your distribution's package manager unless you specifically need a
custom build.

To build from a fresh source checkout:

```sh
./autogen
./configure
make
make install
```

`./configure --help` lists every available build option; defaults
are appropriate for most environments. Driver-specific notes live in
the `README` file inside each `Drivers/Braille/<X>/` and
`Drivers/Speech/<X>/` subdirectory.

## License

BRLTTY is free software, distributed under version 2.1 or later of
the [GNU Lesser General Public License](http://www.gnu.org/licenses/licenses.html#LGPL).
It comes with **absolutely no warranty**. The full license text
ships in [`LICENSE-LGPL`](LICENSE-LGPL) at the top of the source
tree.

## Maintainers

BRLTTY is currently maintained by Dave Mielke. The active team list
and contributor history live on the project's home page at
<http://brltty.app/>.
