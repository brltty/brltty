/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include <signal.h>
#include <sys/wait.h>

int
executeHostCommand (const char *const *arguments) {
  int result = 0XFF;
  sigset_t newMask, oldMask;
  pid_t pid;

  sigemptyset(&newMask);
  sigaddset(&newMask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &newMask, &oldMask);

  switch ((pid = fork())) {
    case -1: /* error */
      LogError("fork");
      break;

    case 0: /* child */
      sigprocmask(SIG_SETMASK, &oldMask, NULL);
      execvp(arguments[0], (char *const*)arguments);
      LogError("execvp");
      _exit(1);

    default: { /* parent */
      int status;
      if (waitpid(pid, &status, 0) == -1) {
        LogError("waitpid");
      } else if (WIFEXITED(status)) {
        result = WEXITSTATUS(status);
        LogPrint(LOG_DEBUG, "exit status: %d", result);
      } else if (WIFSIGNALED(status)) {
        result = WTERMSIG(status);
        LogPrint(LOG_DEBUG, "termination signal: %d", result);
        result += 0X80;
      } else if (WIFSTOPPED(status)) {
        result = WSTOPSIG(status);
        LogPrint(LOG_DEBUG, "stop signal: %d", result);
        result += 0X80;
      } else {
        LogPrint(LOG_DEBUG, "unknown status: 0X%X", status);
      }
    }
  }

  sigprocmask(SIG_SETMASK, &oldMask, NULL);
  return result;
}
