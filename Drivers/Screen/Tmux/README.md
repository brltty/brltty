# Tmux Screen Driver for BRLTTY

This driver provides braille access to tmux (terminal multiplexer) sessions
through BRLTTY.

## Overview

The Tmux screen driver interfaces with a running tmux session using tmux's
control mode (`-C` flag). This allows BRLTTY to:
- Read the content of the active tmux pane
- Track cursor position
- Inject keystrokes into the tmux session
- Navigate between panes and windows using braille display commands
- Provide braille access to terminal applications running inside tmux

## Features

- **Non-intrusive**: Uses tmux control mode for communication
- **Remote-capable**: Can work with tmux sessions over SSH
- **Navigation**: Full keyboard input support including special keys
- **Status aware**: Tracks cursor position and screen dimensions
- **Color support**: Captures and translates ANSI colors and attributes
- **Multi-pane aware**: Automatically tracks and displays the currently
  active pane

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

### Navigating Between Panes

The driver supports BRLTTY's virtual terminal navigation commands to cycle
through all panes across all windows:

- **Next pane**: Use your braille display's "switch to next virtual terminal"
  command
- **Previous pane**: Use your braille display's "switch to previous virtual
  terminal" command

When you switch panes:
- The driver queries all panes across all windows using `list-panes -a`
- Finds your current position in the pane list
- Cycles to the next/previous pane with wraparound
- Automatically switches windows if the target pane is in a different window

Each pane is identified by a globally unique ID (not per-window), so switching
between windows works seamlessly. The pane list cycles with wraparound,
so you can navigate through all panes in a loop.

## Requirements

- tmux must be installed and accessible in PATH
- An existing tmux session must be running
- BRLTTY must have permission to execute tmux commands

## How It Works

The driver:
1. Spawns `tmux -C attach-session` to connect in control mode
2. Subscribes to tmux notifications (pane changes, output, etc.)
3. Communicates with tmux through
   stdin/stdout pipes
4. Parses tmux control protocol messages
5. Queries the active pane using `list-panes -f '#{pane_active}'`
6. Captures pane content using `capture-pane -e` command (with ANSI escape
   sequences)
7. Parses ANSI escape sequences to extract colors and attributes
8. Translates ANSI colors to BRLTTY's attribute system
9. Sends keystrokes using `send-keys` command

## Color and Attribute Support

The driver captures and translates terminal colors and text attributes from
tmux panes.

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
- 8 basic colors: Black, Blue, Green, Cyan, Red, Magenta, Brown, Light Gray
- 8 bright variants using the bright/intensity flag

**Background Colors (8 colors):**
- 8 basic colors (no bright background support)

**Attributes:**
- Blink

### Translation

The driver maps ANSI escape sequences to BRLTTY attributes:

- **Standard colors (ESC[30-37m, ESC[40-47m)** → `SCR_COLOUR_FG_*` and
  `SCR_COLOUR_BG_*`
- **Bright colors (ESC[90-97m, ESC[100-107m)** → `SCR_COLOUR_FG_*` with
  bright flag
- **Bold (ESC[1m)** → Bright foreground flag
- **Blink (ESC[5m)** → `SCR_ATTR_BLINK`
- **256-color and RGB** → Mapped to nearest standard color
- **Other attributes** (italic, underline, etc.) → Not mapped (BRLTTY
  limitation)

Attributes are tracked per character and reset when ESC[0m is encountered.

## Limitations

The driver provides access to the **active pane content only**. The following
tmux UI elements are not accessible through this driver:

### What's NOT Available

- **Status bar**: The tmux status line (session name, window list, time, etc.)
  is not captured
- **Command prompt**: When entering commands with `:` (Ctrl-b :), the prompt
  is not visible
- **Copy mode UI**: While in copy-mode, selection indicators and position
  info are not shown and scrollback buffer can not be used
- **Pane borders**: Visual separators between panes are not displayed
- **Window list**: Cannot navigate or view the list of windows
- **Session information**: Cannot view or switch between sessions
- **Overlays**: choose-tree and display-menu do not work

### Workaround: Nested Tmux

For full access to all tmux UI elements (status bar, command prompts, copy
mode indicators, etc.), **run tmux inside tmux**:

1. **Outer tmux** (connected to BRLTTY): Displays everything as regular
   terminal content
2. **Inner tmux** (your working environment): Provides window/pane management

**Setup:**
```bash
# In your outer tmux session (connected to BRLTTY)
tmux set-option -g prefix C-a  # Use different prefix for inner tmux

# Start inner tmux
tmux new-session

# Now the outer tmux will display all UI elements from the inner tmux,
# including status bar, command prompts, and copy mode indicators
```

**Benefits:**
- Complete access to all tmux features and UI elements
- Status bar, command prompts, and mode indicators are visible
- No special driver modifications needed
- Works with any tmux configuration

**Note:** You'll need to configure different prefix keys for inner and outer
tmux (e.g., Ctrl-a for inner, Ctrl-b for outer) to control them independently.

## Future Enhancements

Potential improvements:
- Full tmux control mode protocol implementation
- Scrollback buffer access
- Better integration with copy-mode and view-mode

Note: For comprehensive UI access (status bar, command prompts, etc.), the
nested tmux approach described above is recommended over complex driver
modifications.

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

**Keys not working:**
- Verify the active virtual console corresponds to the tmux session
- Check if the pane is in a special mode (tmux prompt, copy-mode, etc.)

**Colors not displaying correctly:**
- Verify your terminal supports colors: `echo -e "\e[31mRed\e[0m"`
- Check tmux color support: `tmux info | grep Tc`
- Colors are approximated to BRLTTY's 16-color palette
- Background colors are limited to 8 basic colors (no bright backgrounds)
