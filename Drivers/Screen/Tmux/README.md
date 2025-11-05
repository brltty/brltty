# Tmux Screen Driver for BRLTTY

This driver provides braille access to tmux (terminal multiplexer) sessions through BRLTTY.

## Overview

The Tmux screen driver interfaces with a running tmux session using tmux's control mode (`-C` flag). This allows BRLTTY to:
- Read the content of the active tmux pane
- Track cursor position
- Inject keystrokes into the tmux session
- Provide braille access to terminal applications running inside tmux

## Features

- **Non-intrusive**: Uses tmux control mode for communication
- **Remote-capable**: Can work with tmux sessions over SSH
- **Navigation**: Full keyboard input support including special keys
- **Status aware**: Tracks cursor position and screen dimensions
- **Color support**: Captures and translates ANSI colors and attributes

## Usage

### Basic Usage

To use the tmux screen driver with BRLTTY:

```bash
brltty -x tx
```

This will attach to the default tmux session.

### Parameters

The driver supports the following parameters:

- **session**: Specify which tmux session to attach to
  ```bash
  brltty -x tx -X session=mysession
  ```

- **socket**: Specify a custom tmux socket path
  ```bash
  brltty -x tx -X socket=/tmp/tmux-1000/default
  ```

### Example Configurations

1. **Attach to a specific session:**
   ```bash
   brltty -x tx -X session=work
   ```

2. **Use a custom socket:**
   ```bash
   brltty -x tx -X socket=/tmp/tmux-custom
   ```

3. **Both parameters:**
   ```bash
   brltty -x tx -X session=mysession,socket=/tmp/tmux-1000/default
   ```

## Requirements

- tmux must be installed and accessible in PATH
- An existing tmux session must be running
- BRLTTY must have permission to execute tmux commands

## How It Works

The driver:
1. Spawns `tmux -C attach-session` to connect in control mode
2. Communicates with tmux through stdin/stdout pipes
3. Parses tmux control protocol messages
4. Captures pane content using `capture-pane -e` command (with ANSI escape sequences)
5. Parses ANSI escape sequences to extract colors and attributes
6. Translates ANSI colors to BRLTTY's attribute system
7. Sends keystrokes using `send-keys` command

## Color and Attribute Support

The driver captures and translates terminal colors and text attributes from tmux panes.

### What Tmux Provides

Tmux's `capture-pane -e` command outputs ANSI escape sequences that include:

**Colors:**
- 8 standard colors (black, red, green, yellow, blue, magenta, cyan, white)
- 8 bright/intense colors (bright versions of the standard colors)
- Support for 256-color and 24-bit RGB (when terminal supports it)

**Text Attributes:**
- Bold (often rendered as bright/intense)
- Dim/faint
- Italic
- Underline (including styles in recent tmux versions)
- Blink (slow and fast)
- Reverse video (swap foreground/background)
- Invisible/hidden
- Strikethrough

### What BRLTTY Supports

BRLTTY's attribute system provides:

**Foreground Colors (16 colors):**
- 8 basic colors: black, blue, green, cyan, red, magenta, brown, light grey
- 8 bright variants using the bright/intensity flag

**Background Colors (8 colors):**
- 8 basic colors (no bright background support)

**Attributes:**
- Blink

### Translation

The driver maps ANSI escape sequences to BRLTTY attributes:

- **Standard colors (ESC[30-37m, ESC[40-47m)** → `SCR_COLOUR_FG_*` and `SCR_COLOUR_BG_*`
- **Bright colors (ESC[90-97m, ESC[100-107m)** → `SCR_COLOUR_FG_*` with bright flag
- **Bold (ESC[1m)** → Bright foreground flag
- **Blink (ESC[5m)** → `SCR_ATTR_BLINK`
- **256-color and RGB** → Mapped to nearest standard color
- **Other attributes** (italic, underline, etc.) → Not mapped (BRLTTY limitation)

Attributes are tracked per character and reset when ESC[0m is encountered.

## Limitations

- Currently supports only the active pane (no pane switching yet)
- Control mode output parsing is basic
- Screen updates may have slight delay
- Only 8 background colors supported (BRLTTY limitation - no bright backgrounds)
- Text attributes beyond blink (italic, underline, strikethrough) are not preserved
- 256-color and 24-bit RGB colors are approximated to nearest 16-color equivalent

## Future Enhancements

Potential improvements:
- Full tmux control mode protocol implementation
- Support for pane switching via braille commands
- Status line integration
- Window/session management
- Copy mode support
- Scrollback buffer access

## Building

The driver is automatically built when BRLTTY is configured with:
```bash
./autogen
./configure
make
```

The driver code is `tx` and can be specified with `-x tx`.

## Troubleshooting

**Driver fails to start:**
- Ensure tmux is installed: `which tmux`
- Verify a tmux session is running: `tmux ls`
- Check BRLTTY logs for error messages

**No screen content:**
- The driver may need a screen refresh
- Try switching to another window and back in tmux
- Check if tmux control mode is working: `tmux -C attach`

**Keys not working:**
- Verify the tmux session is responsive
- Check if the pane is in a special mode (copy-mode, etc.)

**Colors not displaying correctly:**
- Verify your terminal supports colors: `echo -e "\e[31mRed\e[0m"`
- Check tmux color support: `tmux info | grep Tc`
- Colors are approximated to BRLTTY's 16-color palette
- Background colors are limited to 8 basic colors (no bright backgrounds)

## Development

The driver is located in `Drivers/Screen/Tmux/` and consists of:
- `screen.c` - Main driver implementation
- `Makefile.in` - Build configuration
- `README.md` - This file

## License

This driver is part of BRLTTY and is licensed under the GNU Lesser General Public License (LGPL) 2.1 or later.
