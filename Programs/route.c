/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

/*
 * route.c - cursor routing functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "misc.h"
#include "scr.h"
#include "route.h"


/*
 * These control the performance of cursor routing.  The optimal settings
 * will depend heavily on system load, etc.  See the documentation for
 * further details.
 * NOTE: if you try to route the cursor to an invalid place, BRLTTY won't
 * give up until the timeout has elapsed!
 */
#define CSRJMP_NICENESS 10	/* niceness of cursor routing subprocess */
#define CSRJMP_TIMEOUT 2000	/* cursor routing idle timeout in ms */
#define CSRJMP_LOOP_DELAY 0	/* delay to use in csrjmp_sub() loops (ms) */
#define CSRJMP_SETTLE_DELAY 400	/* delay to use in csrjmp_sub() loops (ms) */


volatile int csr_active = 0;
volatile pid_t csr_pid = 0;


void csrjmp_sub(int x, int y, int curscr)
{
    ScreenDescription scr;		/* for screen state infos */
    int curx, cury;		/* current cursor position */
    int dif, t = 0;
    sigset_t new_mask, old_mask;		/* for blocking of SIGUSR1 */

    /* Set up signal mask: */
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &new_mask, NULL);

    /* Initialise second thread of screen reading: */
    if (!initializeRoutingScreen()) return;

    describeRoutingScreen(&scr);

    /* Deal with vertical movement first, ignoring horizontal jumping ... */
    while ((dif = y - scr.posy) && (curscr == scr.no)) {
	timeout_yet(0);		/* initialise stop-watch */
	sigprocmask(SIG_BLOCK, &new_mask, &old_mask);	/* block SIGUSR1 */
	insertKey(dif > 0 ? KEY_CURSOR_DOWN : KEY_CURSOR_UP);
	sigprocmask(SIG_SETMASK, &old_mask, NULL);	/* unblock SIGUSR1 */
	do {
#if CSRJMP_LOOP_DELAY > 0
	    delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif /* CSRJMP_LOOP_DELAY > 0 */
	    cury = scr.posy;
	    curx = scr.posx;
	    describeRoutingScreen(&scr);
	} while (!(t = timeout_yet(CSRJMP_TIMEOUT)) &&
		 scr.posy == cury && scr.posx == curx);
	if (t)
	    break;
	if ((scr.posy == cury && (scr.posx - curx) * dif <= 0) ||
	    (scr.posy != cury && (y - scr.posy) * (y - scr.posy) >= dif * dif))
	{
	    delay(CSRJMP_SETTLE_DELAY);
	    describeRoutingScreen(&scr);
	    if ((scr.posy == cury && (scr.posx - curx) * dif <= 0) ||
		(scr.posy != cury && (y - scr.posy) * (y - scr.posy) >= dif * dif))
	    {
		/* 
		 * We are getting farther from our target... Let's try to 
		 * go back to the previous position wich was obviously the 
		 * nearest ever reached to date before giving up.
		 */
		sigprocmask(SIG_BLOCK, &new_mask, &old_mask);	/* block SIGUSR1 */
		insertKey(dif < 0 ? KEY_CURSOR_DOWN : KEY_CURSOR_UP);
		sigprocmask(SIG_SETMASK, &old_mask, NULL);	/* unblock SIGUSR1 */
		break;
	    }
	}
    }

    if (x >= 0) {	/* don't do this for vertical-only routing (x=-1) */
	/* Now horizontal movement, quitting if the vertical position is wrong: */
	while ((dif = x - scr.posx) && (scr.posy == y) && (curscr == scr.no)) {
	    timeout_yet(0);	/* initialise stop-watch */
	    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);	/* block SIGUSR1 */
	    insertKey(dif > 0 ? KEY_CURSOR_RIGHT : KEY_CURSOR_LEFT);
	    sigprocmask(SIG_SETMASK, &old_mask, NULL);	/* unblock SIGUSR1 */
	    do {
#if CSRJMP_LOOP_DELAY > 0
		delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif /* CSRJMP_LOOP_DELAY > 0 */
		curx = scr.posx;
		describeRoutingScreen(&scr);
	    }
	    while (!(t = timeout_yet(CSRJMP_TIMEOUT)) &&
		   scr.posx == curx && scr.posy == y);
	    if (t)
		break;
	    if (scr.posy != y ||
		(x - scr.posx) * (x - scr.posx) >= dif * dif)
	    {
		delay(CSRJMP_SETTLE_DELAY);
		describeRoutingScreen(&scr);
		if (scr.posy != y ||
		    (x - scr.posx) * (x - scr.posx) >= dif * dif)
		{
		    /* 
		     * We probably wrapped on a short line... or are getting 
		     * farther from our target. Try to get back to the previous
		     * position which was obviously the nearest ever reached
		     * to date before we exit.
		     */
		    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);  /* block SIGUSR1 */
		    insertKey(dif > 0 ? KEY_CURSOR_LEFT : KEY_CURSOR_RIGHT);
		    sigprocmask(SIG_SETMASK, &old_mask, NULL);  /* unblock SIGUSR1 */
		    break;
		}
	    }
	}
    }

    closeRoutingScreen();		/* close second thread of screen reading */
}

int csrjmp(int x, int y, int scrno)
{
    int started = 0;
    sigset_t new_mask, old_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);	/* block SIGUSR1 */

    /*
     * Fork cursor routing subprocess. First, we must check if a
     * subprocess is already running: if so, we send it a SIGUSR1 and
     * wait for it to die. 
     */
    /* NB According to man 2 wait, setting SIGCHLD handler to SIG_IGN may mean
     * that wait can't catch the dying child.
     */
    if (csr_active) {
	kill(csr_pid, SIGUSR1);
        do {
            sigsuspend(&old_mask);
	} while (csr_active);
    }

    switch (csr_pid = fork()) {
      case -1:			/* fork failed */
        LogError("fork");
	break;
      default:			/* parent waits for child to return */
        csr_active++;
        started = 1;
	break;
      case 0:			/* child, cursor routing process */
	nice(CSRJMP_NICENESS);	/* reduce scheduling priority */
	csrjmp_sub(x, y, scrno);
	_exit(0);		/* terminate child process */
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);	/* unblock SIGUSR1 */
    return started;
}
