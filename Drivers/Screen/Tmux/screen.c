/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Tmux screen driver
 * This driver interfaces with a running tmux session using tmux control mode
 * to provide braille access to tmux panes.
 *
 * Tmux Control Mode Protocol Documentation:
 * https://github.com/tmux/tmux/wiki/Control-Mode
 *
 * Protocol Overview:
 * - Commands produce output wrapped in %begin/%end guard lines
 * - Format: %begin [timestamp] [command-number] [flags]
 * - Each command generates one output block
 * - Asynchronous notifications prefixed with % (e.g., %output, %layout-change)
 * - Use -C flag for control mode (single -C for testing, -CC for applications)
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <wchar.h>
#include <locale.h>

#include "log.h"
#include "parse.h"
#include "async_handle.h"
#include "async_io.h"

/* From scr_main.h */
extern void mainScreenUpdated(void);

typedef enum {
  PARM_SESSION,
  PARM_SOCKET,
} ScreenParameters;

#define SCRPARMS "session", "socket"

#include "scr_driver.h"

/* Parameters */
static char *sessionParameter = NULL;
static char *socketParameter = NULL;

/* Tmux control mode state */
static pid_t tmuxPid = -1;
static int tmuxStdout = -1;
static int tmuxStdin = -1;
static AsyncHandle tmuxMonitorHandle = NULL;

/* Buffering for reading from tmux */
static char tmuxReadBuffer[4096];
static size_t tmuxReadBufferUsed = 0;

/* Screen state */
static int screenRows = 24;
static int screenCols = 80;
static int cursorRow = 0;
static int cursorCol = 0;
static wchar_t *screenContent = NULL;
static ScreenAttributes *screenAttrs = NULL;
static int screenAllocated = 0;
static int currentPaneNumber = 0;

/* Pane switching state */
static int pendingPaneSwitchDirection = 0;  /* 0=none, 1=next, -1=previous */

/* Expected response types */
typedef enum {
  RESPONSE_NONE,        /* Not expecting anything */
  RESPONSE_IGNORE,      /* Ignore this response (e.g., send-keys) */
  RESPONSE_DIMENSIONS,  /* list-panes - expecting 1 line */
  RESPONSE_CONTENT,     /* capture-pane - expecting screenRows lines */
  RESPONSE_PANE_LIST,   /* list-panes -a - list of all panes for navigation */
} ResponseType;

/* Response queue entry */
typedef struct {
  ResponseType type;
  int repeatCount;  /* How many consecutive responses of this type */
} ResponseQueueEntry;

/* Response queue - static circular buffer */
#define RESPONSE_QUEUE_SIZE 8
static ResponseQueueEntry responseQueue[RESPONSE_QUEUE_SIZE];
static int responseQueueHead = 0;
static int responseQueueTail = 0;

/* Current response being processed */
static int insideBeginEnd = -1;  /* Sequence number of current %begin/%end block, -1 if not inside */
static ResponseType currentResponseType = RESPONSE_NONE;
static char **responseLines = NULL;
static int responseLineCount = 0;
static int responseLineCapacity = 0;

/* Screen update tracking */
static int screenNeedsUpdate = 0;
static int updateInProgress = 0;  /* Flag to prevent duplicate update sequences */

/* Tmux commands */
static const char *TMUX_CMD_ENABLE_NOTIFICATIONS =
  "refresh-client -A pane:window-pane-changed,session:session-window-changed";
static const char *TMUX_CMD_LIST_PANES =
  "list-panes -f '#{pane_active}' -F '#{pane_width} #{pane_height} #{cursor_x} #{cursor_y} #{pane_id}'";
static const char *TMUX_CMD_LIST_ALL_PANES =
  "list-panes -a -F '#{window_index} #{pane_index} #{pane_id}'";
static const char *TMUX_CMD_CAPTURE_PANE =
  "capture-pane -e -p";

/* Forward declarations */
static int startTmuxControlMode(void);
static void stopTmuxControlMode(void);
static int updateScreenFromTmux(void);
static void enqueueExpectedResponse(ResponseType type);
static void clearResponseQueue(void);
static int sendTmuxCommand(const char *command, ResponseType expectedResponse);
static int parseTmuxOutput(const char *line);
static void clearResponseLines(void);
static int addResponseLine(const char *line);
static void processScreenContent(void);
static void processPaneList(void);
static int parsePaneId(const char *paneIdStr);
static ScreenAttributes mapAnsiToAttribute(int code, ScreenAttributes current);
static int parseAnsiSequence(const char **ptr, ScreenAttributes *attr);

/* Async callback */
static ASYNC_MONITOR_CALLBACK(tmuxMonitorCallback);

/* ANSI Color Mapping */
static ScreenAttributes
mapAnsiForegroundColor(int color) {
  /* Map ANSI color codes (0-15) to BRLTTY foreground colors */
  switch (color) {
    case 0: return SCR_COLOUR_FG_BLACK;
    case 1: return SCR_COLOUR_FG_RED;
    case 2: return SCR_COLOUR_FG_GREEN;
    case 3: return SCR_COLOUR_FG_BROWN;
    case 4: return SCR_COLOUR_FG_BLUE;
    case 5: return SCR_COLOUR_FG_MAGENTA;
    case 6: return SCR_COLOUR_FG_CYAN;
    case 7: return SCR_COLOUR_FG_LIGHT_GREY;
    case 8: return SCR_COLOUR_FG_DARK_GREY;
    case 9: return SCR_COLOUR_FG_LIGHT_RED;
    case 10: return SCR_COLOUR_FG_LIGHT_GREEN;
    case 11: return SCR_COLOUR_FG_YELLOW;
    case 12: return SCR_COLOUR_FG_LIGHT_BLUE;
    case 13: return SCR_COLOUR_FG_LIGHT_MAGENTA;
    case 14: return SCR_COLOUR_FG_LIGHT_CYAN;
    case 15: return SCR_COLOUR_FG_WHITE;
    default: return SCR_COLOUR_FG_LIGHT_GREY;
  }
}

static ScreenAttributes
mapAnsiBackgroundColor(int color) {
  /* Map ANSI color codes (0-7) to BRLTTY background colors
   * Note: BRLTTY only supports 8 background colors (no bright backgrounds) */
  switch (color % 8) {
    case 0: return SCR_COLOUR_BG_BLACK;
    case 1: return SCR_COLOUR_BG_RED;
    case 2: return SCR_COLOUR_BG_GREEN;
    case 3: return SCR_COLOUR_BG_BROWN;
    case 4: return SCR_COLOUR_BG_BLUE;
    case 5: return SCR_COLOUR_BG_MAGENTA;
    case 6: return SCR_COLOUR_BG_CYAN;
    case 7: return SCR_COLOUR_BG_LIGHT_GREY;
    default: return SCR_COLOUR_BG_BLACK;
  }
}

static ScreenAttributes
mapAnsiToAttribute(int code, ScreenAttributes current) {
  /* Process a single ANSI SGR (Select Graphic Rendition) parameter */
  if (code == 0) {
    /* Reset all attributes */
    return SCR_COLOUR_DEFAULT;
  } else if (code == 1) {
    /* Bold - set bright flag on foreground */
    return current | SCR_ATTR_FG_BRIGHT;
  } else if (code == 5 || code == 6) {
    /* Blink (slow or fast) */
    return current | SCR_ATTR_BLINK;
  } else if (code == 22) {
    /* Normal intensity - clear bright flag */
    return current & ~SCR_ATTR_FG_BRIGHT;
  } else if (code == 25) {
    /* Blink off */
    return current & ~SCR_ATTR_BLINK;
  } else if (code >= 30 && code <= 37) {
    /* Standard foreground colors - preserve bright flag */
    ScreenAttributes fg = mapAnsiForegroundColor(code - 30);
    return (current & ~(SCR_MASK_FG & ~SCR_ATTR_FG_BRIGHT)) | fg;
  } else if (code == 39) {
    /* Default foreground */
    return (current & ~SCR_MASK_FG) | SCR_COLOUR_FG_LIGHT_GREY;
  } else if (code >= 40 && code <= 47) {
    /* Standard background colors */
    ScreenAttributes bg = mapAnsiBackgroundColor(code - 40);
    return (current & ~SCR_MASK_BG) | bg;
  } else if (code == 49) {
    /* Default background */
    return (current & ~SCR_MASK_BG) | SCR_COLOUR_BG_BLACK;
  } else if (code >= 90 && code <= 97) {
    /* Bright foreground colors */
    ScreenAttributes fg = mapAnsiForegroundColor(code - 90 + 8);
    return (current & ~SCR_MASK_FG) | fg;
  } else if (code >= 100 && code <= 107) {
    /* Bright background colors - map to standard (BRLTTY limitation) */
    ScreenAttributes bg = mapAnsiBackgroundColor(code - 100);
    return (current & ~SCR_MASK_BG) | bg;
  }

  /* Ignore other codes (italic, underline, etc. - not supported by BRLTTY) */
  return current;
}

static int
parseAnsiSequence(const char **ptr, ScreenAttributes *attr) {
  /* Parse an ANSI escape sequence: ESC[...m
   * Returns 1 if a sequence was parsed, 0 otherwise
   * Updates *ptr to point past the sequence
   * Updates *attr with new attributes */

  const char *p = *ptr;

  if (p[0] != '\033' || p[1] != '[') {
    return 0;
  }

  p += 2; /* Skip ESC[ */

  /* Parse semicolon-separated numeric parameters */
  int params[16];
  int paramCount = 0;

  while (*p && *p != 'm' && paramCount < 16) {
    if (*p >= '0' && *p <= '9') {
      int value = 0;
      while (*p >= '0' && *p <= '9') {
        value = value * 10 + (*p - '0');
        p++;
      }
      params[paramCount++] = value;
    } else if (*p == ';') {
      p++;
      /* Handle empty parameter (default to 0) */
      if (*p == ';' || *p == 'm') {
        params[paramCount++] = 0;
      }
    } else {
      /* Invalid character, abort */
      return 0;
    }
  }

  if (*p != 'm') {
    /* Not an SGR sequence, ignore */
    return 0;
  }

  p++; /* Skip 'm' */

  /* If no parameters, default to 0 (reset) */
  if (paramCount == 0) {
    params[0] = 0;
    paramCount = 1;
  }

  /* Apply all parameters */
  for (int i = 0; i < paramCount; i++) {
    int code = params[i];

    /* Handle 256-color and RGB sequences */
    if (code == 38 && i + 2 < paramCount && params[i + 1] == 5) {
      /* 256-color foreground: ESC[38;5;Nm */
      int color = params[i + 2];
      /* Map to 16-color palette (approximate) */
      if (color < 16) {
        *attr = (*attr & ~SCR_MASK_FG) | mapAnsiForegroundColor(color);
      } else if (color < 232) {
        /* 216-color cube - approximate to nearest 16-color */
        int idx = color - 16;
        int r = (idx / 36) > 2 ? 1 : 0;
        int g = ((idx / 6) % 6) > 2 ? 1 : 0;
        int b = (idx % 6) > 2 ? 1 : 0;
        int bright = ((idx / 36) > 4 || ((idx / 6) % 6) > 4 || (idx % 6) > 4) ? 1 : 0;
        int c = r * 4 + g * 2 + b;
        *attr = (*attr & ~SCR_MASK_FG) | mapAnsiForegroundColor(c + (bright ? 8 : 0));
      } else {
        /* Grayscale - approximate */
        int gray = color - 232;
        *attr = (*attr & ~SCR_MASK_FG) | mapAnsiForegroundColor(gray < 12 ? 0 : (gray < 24 ? 8 : 15));
      }
      i += 2;
      continue;
    } else if (code == 48 && i + 2 < paramCount && params[i + 1] == 5) {
      /* 256-color background: ESC[48;5;Nm */
      int color = params[i + 2];
      if (color < 16) {
        *attr = (*attr & ~SCR_MASK_BG) | mapAnsiBackgroundColor(color);
      } else if (color < 232) {
        int idx = color - 16;
        int r = (idx / 36) > 2 ? 1 : 0;
        int g = ((idx / 6) % 6) > 2 ? 1 : 0;
        int b = (idx % 6) > 2 ? 1 : 0;
        int c = r * 4 + g * 2 + b;
        *attr = (*attr & ~SCR_MASK_BG) | mapAnsiBackgroundColor(c);
      } else {
        int gray = color - 232;
        *attr = (*attr & ~SCR_MASK_BG) | mapAnsiBackgroundColor(gray < 12 ? 0 : 7);
      }
      i += 2;
      continue;
    } else if ((code == 38 || code == 48) && i + 4 < paramCount && params[i + 1] == 2) {
      /* RGB color: ESC[38;2;R;G;Bm or ESC[48;2;R;G;Bm */
      int r = params[i + 2] > 127 ? 1 : 0;
      int g = params[i + 3] > 127 ? 1 : 0;
      int b = params[i + 4] > 127 ? 1 : 0;
      int bright = (params[i + 2] > 192 || params[i + 3] > 192 || params[i + 4] > 192) ? 1 : 0;
      int c = r * 4 + g * 2 + b;
      if (code == 38) {
        *attr = (*attr & ~SCR_MASK_FG) | mapAnsiForegroundColor(c + (bright ? 8 : 0));
      } else {
        *attr = (*attr & ~SCR_MASK_BG) | mapAnsiBackgroundColor(c);
      }
      i += 4;
      continue;
    }

    /* Standard SGR parameter */
    *attr = mapAnsiToAttribute(code, *attr);
  }

  *ptr = p;
  return 1;
}

static void
setParameter(char **variable, char **parameters, ScreenParameters parameter) {
  char *value = parameters[parameter];
  if (value && !*value) value = NULL;
  *variable = value;
}

static int
processParameters_TmuxScreen(char **parameters) {
  setParameter(&sessionParameter, parameters, PARM_SESSION);
  setParameter(&socketParameter, parameters, PARM_SOCKET);
  return 1;
}

static void
releaseParameters_TmuxScreen(void) {
  /* Parameters are managed by BRLTTY */
}

static int
allocateScreenBuffer(void) {
  size_t size = screenRows * screenCols;

  if (screenAllocated) {
    wchar_t *newContent = realloc(screenContent, size * sizeof(wchar_t));
    ScreenAttributes *newAttrs = realloc(screenAttrs, size * sizeof(ScreenAttributes));

    if (!newContent || !newAttrs) {
      logMessage(LOG_ERR, "Failed to reallocate screen buffer");
      return 0;
    }

    screenContent = newContent;
    screenAttrs = newAttrs;
  } else {
    screenContent = malloc(size * sizeof(wchar_t));
    screenAttrs = malloc(size * sizeof(ScreenAttributes));

    if (!screenContent || !screenAttrs) {
      logMessage(LOG_ERR, "Failed to allocate screen buffer");
      if (screenContent) free(screenContent);
      if (screenAttrs) free(screenAttrs);
      return 0;
    }

    screenAllocated = 1;
  }

  /* Clear the buffer */
  for (size_t i = 0; i < size; i++) {
    screenContent[i] = L' ';
    screenAttrs[i] = SCR_COLOUR_DEFAULT;
  }

  return 1;
}

static void
freeScreenBuffer(void) {
  if (screenAllocated) {
    free(screenContent);
    free(screenAttrs);
    screenContent = NULL;
    screenAttrs = NULL;
    screenAllocated = 0;
  }
}

static int
construct_TmuxScreen(void) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Constructing Tmux screen driver");

  /* Set locale for wide character support */
  setlocale(LC_ALL, "");

  if (!allocateScreenBuffer()) {
    return 0;
  }

  if (!startTmuxControlMode()) {
    freeScreenBuffer();
    return 0;
  }

  /* Tmux sends a dummy %begin/%end block on startup - queue an IGNORE response for it */
  enqueueExpectedResponse(RESPONSE_IGNORE);

  /* Enable pane change notifications */
  sendTmuxCommand(TMUX_CMD_ENABLE_NOTIFICATIONS, RESPONSE_IGNORE);

  screenNeedsUpdate = 1;
  updateInProgress = 0;

  /* Register async monitor for tmux output */
  if (!asyncMonitorFileInput(&tmuxMonitorHandle, tmuxStdout, tmuxMonitorCallback, NULL)) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Failed to register async monitor for tmux output");
    stopTmuxControlMode();
    freeScreenBuffer();
    return 0;
  }
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Tmux async monitor registered");

  return 1;
}

static void
destruct_TmuxScreen(void) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Destructing Tmux screen driver");

  /* Unregister async monitor */
  if (tmuxMonitorHandle) {
    asyncCancelRequest(tmuxMonitorHandle);
    tmuxMonitorHandle = NULL;
  }

  stopTmuxControlMode();
  freeScreenBuffer();

  /* Free response queue */
  clearResponseQueue();

  /* Free response lines */
  clearResponseLines();
  if (responseLines) {
    free(responseLines);
    responseLines = NULL;
    responseLineCapacity = 0;
  }
}

static int
startTmuxControlMode(void) {
  int pipeStdout[2];
  int pipeStdin[2];

  if (pipe(pipeStdout) < 0 || pipe(pipeStdin) < 0) {
    logMessage(LOG_ERR, "Failed to create pipes: %s", strerror(errno));
    return 0;
  }

  tmuxPid = fork();

  if (tmuxPid < 0) {
    logMessage(LOG_ERR, "Failed to fork: %s", strerror(errno));
    close(pipeStdout[0]);
    close(pipeStdout[1]);
    close(pipeStdin[0]);
    close(pipeStdin[1]);
    return 0;
  }

  if (tmuxPid == 0) {
    /* Child process - exec tmux */
    close(pipeStdout[0]); /* Close read end */
    close(pipeStdin[1]);  /* Close write end */

    dup2(pipeStdin[0], STDIN_FILENO);
    dup2(pipeStdout[1], STDOUT_FILENO);
    dup2(pipeStdout[1], STDERR_FILENO);

    close(pipeStdin[0]);
    close(pipeStdout[1]);

    /* Build tmux command */
    if (sessionParameter) {
      if (socketParameter) {
        execlp("tmux", "tmux", "-C", "-S", socketParameter, "attach-session", "-t", sessionParameter, NULL);
      } else {
        execlp("tmux", "tmux", "-C", "attach-session", "-t", sessionParameter, NULL);
      }
    } else {
      if (socketParameter) {
        execlp("tmux", "tmux", "-C", "-S", socketParameter, "attach-session", NULL);
      } else {
        execlp("tmux", "tmux", "-C", "attach-session", NULL);
      }
    }

    logMessage(LOG_ERR, "Failed to exec tmux: %s", strerror(errno));
    _exit(1);
  }

  /* Parent process */
  close(pipeStdout[1]); /* Close write end */
  close(pipeStdin[0]);  /* Close read end */

  tmuxStdout = pipeStdout[0];
  tmuxStdin = pipeStdin[1];

  /* Make stdout non-blocking */
  int flags = fcntl(tmuxStdout, F_GETFL, 0);
  fcntl(tmuxStdout, F_SETFL, flags | O_NONBLOCK);

  /* Initialize input buffer */
  tmuxReadBufferUsed = 0;

  logMessage(LOG_DEBUG, "Tmux control mode started");
  return 1;
}

static void
stopTmuxControlMode(void) {
  if (tmuxPid > 0) {
    kill(tmuxPid, SIGTERM);
    waitpid(tmuxPid, NULL, 0);
    tmuxPid = -1;
  }

  if (tmuxStdout >= 0) {
    close(tmuxStdout);
    tmuxStdout = -1;
  }

  if (tmuxStdin >= 0) {
    close(tmuxStdin);
    tmuxStdin = -1;
  }
}

static int
sendTmuxCommand(const char *command, ResponseType expectedResponse) {
  if (tmuxStdin < 0) {
    return 0;
  }

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Sending tmux command: [%s]", command);

  size_t len = strlen(command);
  ssize_t written = write(tmuxStdin, command, len);

  if (written < 0 || (size_t)written != len) {
    logMessage(LOG_ERR, "Failed to write to tmux: %s", strerror(errno));
    return 0;
  }

  /* Write newline */
  if (write(tmuxStdin, "\n", 1) != 1) {
    logMessage(LOG_ERR, "Failed to write newline to tmux: %s", strerror(errno));
    return 0;
  }

  /* Enqueue the expected response type */
  enqueueExpectedResponse(expectedResponse);

  return 1;
}

static void
enqueueExpectedResponse(ResponseType type) {
  /* Check if queue is empty */
  if (responseQueueHead == responseQueueTail) {
    /* Empty queue - add first entry */
    responseQueue[responseQueueTail].type = type;
    responseQueue[responseQueueTail].repeatCount = 1;
    responseQueueTail = (responseQueueTail + 1) % RESPONSE_QUEUE_SIZE;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
               "Enqueued response type %d", type);
    return;
  }

  /* Get the index of the last entry */
  int lastIndex = (responseQueueTail - 1 + RESPONSE_QUEUE_SIZE) % RESPONSE_QUEUE_SIZE;

  if (responseQueue[lastIndex].type == type) {
    /* Same type as last entry - increment repeat count */
    responseQueue[lastIndex].repeatCount++;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
               "Incremented repeat count for type %d to %d", type, responseQueue[lastIndex].repeatCount);
    return;
  }

  /* Check if queue is full */
  int nextTail = (responseQueueTail + 1) % RESPONSE_QUEUE_SIZE;
  if (nextTail == responseQueueHead) {
    logMessage(LOG_ERR, "Response queue full, cannot enqueue type %d", type);
    return;
  }

  /* Add new entry */
  responseQueue[responseQueueTail].type = type;
  responseQueue[responseQueueTail].repeatCount = 1;
  responseQueueTail = nextTail;

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
             "Enqueued response type %d", type);
}

static ResponseType
dequeueExpectedResponse(void) {
  /* Check if queue is empty */
  if (responseQueueHead == responseQueueTail) {
    return RESPONSE_NONE;
  }

  /* Get the entry at head */
  ResponseType type = responseQueue[responseQueueHead].type;

  if (responseQueue[responseQueueHead].repeatCount > 1) {
    /* Decrement repeat count instead of removing entry */
    responseQueue[responseQueueHead].repeatCount--;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
               "Decremented repeat count for type %d to %d", type, responseQueue[responseQueueHead].repeatCount);
    return type;
  }

  /* Remove entry from queue by advancing head */
  responseQueueHead = (responseQueueHead + 1) % RESPONSE_QUEUE_SIZE;

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
             "Dequeued response type %d", type);
  return type;
}

static void
clearResponseQueue(void) {
  responseQueueHead = 0;
  responseQueueTail = 0;
  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Response queue cleared");
}

static void
clearResponseLines(void) {
  for (int i = 0; i < responseLineCount; i++) {
    free(responseLines[i]);
  }
  responseLineCount = 0;
}

static int
addResponseLine(const char *line) {
  if (responseLineCount >= responseLineCapacity) {
    int newCapacity = responseLineCapacity == 0 ? 32 : responseLineCapacity * 2;
    char **newLines = realloc(responseLines, newCapacity * sizeof(char *));
    if (!newLines) {
      logMessage(LOG_ERR, "Failed to allocate memory for response lines");
      return 0;
    }
    responseLines = newLines;
    responseLineCapacity = newCapacity;
  }

  responseLines[responseLineCount] = strdup(line);
  if (!responseLines[responseLineCount]) {
    logMessage(LOG_ERR, "Failed to duplicate line");
    return 0;
  }
  responseLineCount++;
  return 1;
}

static void
processScreenContent(void) {
  /* Process response pane content and fill screen buffer */
  int row = 0;

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Processing %d response lines for %dx%d screen",
             responseLineCount, screenCols, screenRows);

  for (int i = 0; i < responseLineCount && row < screenRows; i++) {
    const char *line = responseLines[i];
    int col = 0;
    ScreenAttributes currentAttr = SCR_COLOUR_DEFAULT;

    /* Convert line to wide characters and fill buffer, parsing ANSI sequences */
    for (const char *p = line; *p && col < screenCols; ) {
      /* Check for ANSI escape sequence */
      if (parseAnsiSequence(&p, &currentAttr)) {
        /* Sequence parsed, currentAttr updated, continue */
        continue;
      }

      /* Regular character - convert to wide char */
      wchar_t wc;
      int bytes = mbtowc(&wc, p, MB_CUR_MAX);

      if (bytes <= 0) {
        /* Invalid character, skip it */
        p++;
        continue;
      }

      int index = row * screenCols + col;
      screenContent[index] = wc;
      screenAttrs[index] = currentAttr;

      p += bytes;
      col++;
    }

    /* Fill remaining columns with spaces */
    for (; col < screenCols; col++) {
      int index = row * screenCols + col;
      screenContent[index] = L' ';
      screenAttrs[index] = SCR_COLOUR_DEFAULT;
    }

    row++;
  }

  /* Fill remaining rows with spaces */
  for (; row < screenRows; row++) {
    for (int col = 0; col < screenCols; col++) {
      int index = row * screenCols + col;
      screenContent[index] = L' ';
      screenAttrs[index] = SCR_COLOUR_DEFAULT;
    }
  }

  mainScreenUpdated();
}

static int
parsePaneId(const char *paneIdStr) {
  /* Parse pane ID string (format: %N) and return the numeric part
   * Returns -1 if parsing fails
   */
  if (!paneIdStr || paneIdStr[0] != '%') {
    return -1;
  }
  return atoi(paneIdStr + 1);
}

static void
processPaneList(void) {
  /* Process list of all panes and switch to next/previous pane */
  if (responseLineCount == 0 || pendingPaneSwitchDirection == 0) {
    return;
  }

  int currentIndex = -1;
  int targetIndex = -1;

  /* Find current pane in the list */
  for (int i = 0; i < responseLineCount; i++) {
    char paneId[32];
    int windowIdx, paneIdx;
    if (sscanf(responseLines[i], "%d %d %31s", &windowIdx, &paneIdx, paneId) == 3) {
      int paneNum = parsePaneId(paneId);
      if (paneNum == currentPaneNumber) {
        currentIndex = i;
        break;
      }
    }
  }

  /* Calculate target index with wraparound */
  if (currentIndex >= 0) {
    if (pendingPaneSwitchDirection > 0) {
      targetIndex = (currentIndex + 1) % responseLineCount;
    } else {
      targetIndex = (currentIndex - 1 + responseLineCount) % responseLineCount;
    }

    /* Parse target pane info and switch to it */
    char paneId[32];
    int windowIdx, paneIdx;
    if (sscanf(responseLines[targetIndex], "%d %d %31s", &windowIdx, &paneIdx, paneId) == 3) {
      char selectWindowCmd[64];
      char selectPaneCmd[64];
      snprintf(selectWindowCmd, sizeof(selectWindowCmd), "select-window -t :%d", windowIdx);
      snprintf(selectPaneCmd, sizeof(selectPaneCmd), "select-pane -t :%d.%d", windowIdx, paneIdx);

      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Switching to window %d pane %d (%s)",
                 windowIdx, paneIdx, paneId);

      sendTmuxCommand(selectWindowCmd, RESPONSE_IGNORE);
      sendTmuxCommand(selectPaneCmd, RESPONSE_IGNORE);
    }
  }

  pendingPaneSwitchDirection = 0;
}

static int
parseTmuxOutput(const char *line) {
  /* Parse tmux control mode output
   * In control mode, output is either:
   * - Control messages starting with % (like %begin, %end, %output, etc.)
   * - Regular output lines (including pane IDs like %0, %1)
   */

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
             "parsing tmux line \"%s\" (currentType %d)", line, currentResponseType);

  if (insideBeginEnd >= 0 &&
      currentResponseType == RESPONSE_CONTENT &&
      responseLineCount < screenRows) {
    /*
     * The capture command returns the screen content which may contain
     * the equivalent of control messages such as "%end". Don't consider
     * any additional parsing until we've gathered at least the expected
     * number of lines.
     */
    addResponseLine(line);
    return 1;
  }

  /* Check for control messages */
  if (line[0] == '%') {
    /* Check if it's a known control message */
    if (strncmp(line, "%begin ", 7) == 0) {
      /* Start of command output
       * Format: %begin timestamp command-number flags */
      int timestamp, cmdNumber, flags;
      int parsed = sscanf(line, "%%begin %d %d %d", &timestamp, &cmdNumber, &flags);

      /* Track the sequence number of this block */
      if (parsed >= 2) {
        insideBeginEnd = cmdNumber;

        /* Dequeue the next expected response type */
        currentResponseType = dequeueExpectedResponse();

        logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
                   "Tmux begin: cmd=%d, dequeued type=%d",
                   cmdNumber, currentResponseType);
      } else {
        insideBeginEnd = -1;
        currentResponseType = RESPONSE_NONE;
      }
    } else if (strncmp(line, "%end ", 5) == 0 || strncmp(line, "%error ", 7) == 0) {
      /* End of command output - process captured data
       * Format: %end timestamp command-number flags
       * Or: %error timestamp command-number flags */
      int isError = (strncmp(line, "%error ", 7) == 0);
      int timestamp, cmdNumber, flags;
      int parsed = sscanf(line + (isError ? 7 : 5), "%d %d %d", &timestamp, &cmdNumber, &flags);

      logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Tmux %s: cmd=%d currentType=%d lines=%d",
                 isError ? "error" : "end", cmdNumber, currentResponseType, responseLineCount);

      /* Check if this matches our begin sequence */
      int isMatchingEnd = (parsed >= 2 && cmdNumber == insideBeginEnd);
      if (!isMatchingEnd) {
        logMessage(LOG_ERR, "Sequence mismatch: %%begin had %d, %s has %d",
                   insideBeginEnd, isError ? "%error" : "%end", cmdNumber);
      }

      insideBeginEnd = -1;

      if (isError) {
        /* Log the error message */
        logMessage(LOG_ERR, "Tmux command %d failed:", cmdNumber);
        for (int i = 0; i < responseLineCount; i++) {
          logMessage(LOG_ERR, "  %s", responseLines[i]);
        }
        currentResponseType = RESPONSE_NONE;
      } else if (isMatchingEnd) {
        if (currentResponseType == RESPONSE_IGNORE) {
          /* Command response we don't care about (e.g., send-keys) */
          logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Ignoring response");
        } else if (currentResponseType == RESPONSE_DIMENSIONS) {
          /* Dimensions received from list-panes - parse first line */
          if (responseLineCount == 1) {
            const char *line = responseLines[0];
            char paneId[32];
            int parsed = sscanf(line, "%d %d %d %d %31s",
                               &screenCols, &screenRows, &cursorCol, &cursorRow, paneId);
            if (parsed == 5) {
              /* pane_id format is %<number> - extract the numeric part */
              currentPaneNumber = parsePaneId(paneId);
              if (currentPaneNumber < 0) {
                currentPaneNumber = 0;
              }
              logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
                         "list-panes: %dx%d, cursor: (%d,%d), pane: %s (number: %d)",
                         screenCols, screenRows, cursorCol, cursorRow, paneId, currentPaneNumber);
              allocateScreenBuffer();
            } else {
              logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Failed to parse list-panes response: %s", line);
            }
          }
        } else if (currentResponseType == RESPONSE_CONTENT) {
          /* Content received from capture-pane - process it */
          if (responseLineCount > 0) {
            processScreenContent();
          }
          /* Update sequence complete */
          updateInProgress = 0;
        } else if (currentResponseType == RESPONSE_PANE_LIST) {
          /* Received list of all panes - process and switch */
          processPaneList();
        }
        currentResponseType = RESPONSE_NONE;
      }

      /* We're done with this response */
      clearResponseLines();
    } else if (strncmp(line, "%output ", 8) == 0) {
      /* Pane output notification - screen has changed!
       * Format: %output %pane_id data...
       * Only update if it's for the current pane
       */
      int paneNum = parsePaneId(line + 8);
      if (paneNum == currentPaneNumber) {
        logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
                   "Tmux output notification for current pane %d", paneNum);
        screenNeedsUpdate = 1;
      } else {
        logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG,
                   "Tmux output notification for other pane %d (current: %d)", paneNum, currentPaneNumber);
      }
    } else if (strncmp(line, "%layout-change", 14) == 0) {
      /* Layout changed - screen dimensions may have changed */
      logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Tmux layout change: %s", line);
      screenNeedsUpdate = 1;
    } else if (strncmp(line, "%window-pane-changed", 20) == 0) {
      /* Active pane changed */
      logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Tmux pane change: %s", line);
      screenNeedsUpdate = 1;
    } else if (strncmp(line, "%session-window-changed", 23) == 0) {
      /* Active window changed */
      logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Tmux window change: %s", line);
      screenNeedsUpdate = 1;
    } else if (strncmp(line, "%session-changed", 16) == 0) {
      /* Session changed notification */
      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Tmux session change: %s", line);
      screenNeedsUpdate = 1;
    } else if (strncmp(line, "%sessions-changed", 17) == 0) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Tmux sessions change: %s", line);
      screenNeedsUpdate = 1;
    } else if (strncmp(line, "%exit", 5) == 0) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Tmux exit: %s", line);
    } else {
      /* Starts with % but not a known control message */
      logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Line starts with %% but not a control message: %s", line);
    }
  } else if (insideBeginEnd >= 0) {
    addResponseLine(line);
  } else {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER), "unexpected data from tmux: %s", line);
  }

  return 1;
}

ASYNC_MONITOR_CALLBACK(tmuxMonitorCallback) {
  /* Called by BRLTTY's event loop when data is available on tmux stdout */
  ssize_t bytesRead;
  char *lineStart = tmuxReadBuffer;

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "Async callback called");

  if (tmuxStdout < 0) return 0;

  /* Read all available data without blocking */
  do {
    bytesRead = read(tmuxStdout, tmuxReadBuffer + tmuxReadBufferUsed,
                     sizeof(tmuxReadBuffer) - tmuxReadBufferUsed);

    /* Check for errors */
    if (bytesRead < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        /* No more data available - this is normal for non-blocking I/O */
        bytesRead = 0;
      } else {
        logMessage(LOG_ERR, "Error reading from tmux: %s", strerror(errno));
        return 0;
      }
    }

    /* Look for line termination */
    for (ssize_t i = tmuxReadBufferUsed; i < tmuxReadBufferUsed + bytesRead; i++) {
      if (tmuxReadBuffer[i] != '\n') continue;

      /* End of line - null terminate and process */
      tmuxReadBuffer[i] = '\0';
      parseTmuxOutput(lineStart);
      lineStart = tmuxReadBuffer + i + 1;
    }

    /* Compute size of incomplete line data */
    tmuxReadBufferUsed = tmuxReadBuffer + tmuxReadBufferUsed + bytesRead - lineStart;
    if (tmuxReadBufferUsed >= sizeof(tmuxReadBuffer)) {
      logMessage(LOG_ERR, "Error reading from tmux: line exceeds %zu bytes",
                 sizeof(tmuxReadBuffer));
      return 0;
    }

    /* Move incomplete line data to the beginning */
    if (tmuxReadBufferUsed)
      memmove(tmuxReadBuffer, lineStart, tmuxReadBufferUsed);
    lineStart = tmuxReadBuffer;
  } while (bytesRead > 0);

  if (screenNeedsUpdate)
    if (updateScreenFromTmux())
      screenNeedsUpdate = 0;

  return 1;
}

static int
updateScreenFromTmux(void) {
  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "updateScreenFromTmux called (updateInProgress=%d)", updateInProgress);

  if (updateInProgress) {
    /* An update sequence is already queued */
    return 1;
  }

  /* Send both commands back-to-back - responses will arrive asynchronously
   * and be processed by tmuxMonitorCallback in the right order.
   *
   * First: list-panes (without -a) to get dimensions and cursor of active pane only
   * Second: capture-pane to get the content
   */
  if (sendTmuxCommand(TMUX_CMD_LIST_PANES, RESPONSE_DIMENSIONS) &&
      sendTmuxCommand(TMUX_CMD_CAPTURE_PANE, RESPONSE_CONTENT)) {
    updateInProgress = 1;
    return 1;
  }
  return 0;
}

static void
describe_TmuxScreen(ScreenDescription *description) {
  description->cols = screenCols;
  description->rows = screenRows;
  description->posx = cursorCol;
  description->posy = cursorRow;
  description->number = currentPaneNumber;
  description->hasCursor = 1;
  description->hasSelection = 0;
  description->unreadable = NULL;
  description->quality = SCQ_GOOD;

  logMessage(LOG_CATEGORY(SCREEN_DRIVER) | LOG_DEBUG, "describe: %dx%d, cursor: (%d,%d), pane: %d",
             screenCols, screenRows, cursorCol, cursorRow, currentPaneNumber);
}

static int
readCharacters_TmuxScreen(const ScreenBox *box, ScreenCharacter *buffer) {
  if (!validateScreenBox(box, screenCols, screenRows)) {
    logMessage(LOG_ERR, "Invalid screen box");
    return 0;
  }

  for (int row = 0; row < box->height; row++) {
    int screenRow = box->top + row;
    if (screenRow >= screenRows) break;

    for (int col = 0; col < box->width; col++) {
      int screenCol = box->left + col;
      if (screenCol >= screenCols) break;

      int screenIndex = screenRow * screenCols + screenCol;
      int bufferIndex = row * box->width + col;

      buffer[bufferIndex].text = screenContent[screenIndex];
      buffer[bufferIndex].attributes = screenAttrs[screenIndex];
    }
  }

  return 1;
}

static int
insertKey_TmuxScreen(ScreenKey key) {
  char command[256];

  /* Map ScreenKey to tmux key names */
  if (key == SCR_KEY_CURSOR_UP) {
    snprintf(command, sizeof(command), "send-keys Up");
  } else if (key == SCR_KEY_CURSOR_DOWN) {
    snprintf(command, sizeof(command), "send-keys Down");
  } else if (key == SCR_KEY_CURSOR_LEFT) {
    snprintf(command, sizeof(command), "send-keys Left");
  } else if (key == SCR_KEY_CURSOR_RIGHT) {
    snprintf(command, sizeof(command), "send-keys Right");
  } else if (key == SCR_KEY_HOME) {
    snprintf(command, sizeof(command), "send-keys Home");
  } else if (key == SCR_KEY_END) {
    snprintf(command, sizeof(command), "send-keys End");
  } else if (key == SCR_KEY_PAGE_UP) {
    snprintf(command, sizeof(command), "send-keys PageUp");
  } else if (key == SCR_KEY_PAGE_DOWN) {
    snprintf(command, sizeof(command), "send-keys PageDown");
  } else if (key == SCR_KEY_ENTER) {
    snprintf(command, sizeof(command), "send-keys Enter");
  } else if (key == SCR_KEY_TAB) {
    snprintf(command, sizeof(command), "send-keys Tab");
  } else if (key == SCR_KEY_BACKSPACE) {
    snprintf(command, sizeof(command), "send-keys BSpace");
  } else if (key == SCR_KEY_DELETE) {
    snprintf(command, sizeof(command), "send-keys DC");
  } else if (key == SCR_KEY_ESCAPE) {
    snprintf(command, sizeof(command), "send-keys Escape");
  } else if (key >= SCR_KEY_F1 && key <= SCR_KEY_F12) {
    int fnum = key - SCR_KEY_F1 + 1;
    snprintf(command, sizeof(command), "send-keys F%d", fnum);
  } else {
    /* Regular character */
    wchar_t ch = key & SCR_KEY_CHAR_MASK;

    if (ch < 0x80) {
      /* ASCII character */
      if (key & SCR_KEY_CONTROL) {
        snprintf(command, sizeof(command), "send-keys C-%c", (char)ch);
      } else if (key & (SCR_KEY_ALT_LEFT | SCR_KEY_ALT_RIGHT)) {
        snprintf(command, sizeof(command), "send-keys M-%c", (char)ch);
      } else {
        snprintf(command, sizeof(command), "send-keys '%c'", (char)ch);
      }
    } else {
      /* Unicode character - just send as literal */
      char utf8[8];
      snprintf(utf8, sizeof(utf8), "%lc", (wint_t)ch);
      snprintf(command, sizeof(command), "send-keys '%s'", utf8);
    }
  }

  return sendTmuxCommand(command, RESPONSE_IGNORE);
}

static int
poll_TmuxScreen(void) {
  /* With async I/O, BRLTTY's event loop handles monitoring the tmux FD.
   * We don't need to poll - return 0 to indicate no polling needed. */
  return 0;
}

static int
refresh_TmuxScreen(void) {
  /* All the refreshing is done through the async I/O callback. */
  return 1;
}

static ScreenPasteMode
getPasteMode_TmuxScreen(void) {
  /* Tmux always accepts bracketed paste input */
  return SPM_BRACKETED;
}

static int
currentVirtualTerminal_TmuxScreen(void) {
  /* Return the current pane number (extracted from pane_id) */
  return currentPaneNumber;
}

static int
switchVirtualTerminal_TmuxScreen(int vt) {
  /* Switch to a different pane by cycling through all panes
   * vt parameter indicates target pane number
   * We need to query list-panes -a to find next/previous pane
   */

  if (vt > currentPaneNumber) {
    pendingPaneSwitchDirection = 1;  /* Next */
  } else if (vt < currentPaneNumber) {
    pendingPaneSwitchDirection = -1;  /* Previous */
  } else {
    /* Already on this pane */
    return 1;
  }

  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "Querying pane list for switch (direction: %d)",
             pendingPaneSwitchDirection);
  return sendTmuxCommand(TMUX_CMD_LIST_ALL_PANES, RESPONSE_PANE_LIST);
}

static void
scr_initialize(MainScreen *main) {
  initializeRealScreen(main);

  main->base.poll = poll_TmuxScreen;
  main->base.refresh = refresh_TmuxScreen;
  main->base.describe = describe_TmuxScreen;
  main->base.readCharacters = readCharacters_TmuxScreen;
  main->base.insertKey = insertKey_TmuxScreen;
  main->base.getPasteMode = getPasteMode_TmuxScreen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_TmuxScreen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_TmuxScreen;

  main->processParameters = processParameters_TmuxScreen;
  main->releaseParameters = releaseParameters_TmuxScreen;
  main->construct = construct_TmuxScreen;
  main->destruct = destruct_TmuxScreen;
}
