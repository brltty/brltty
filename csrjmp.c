/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
 *
 * Web Page: http://www.cam.org/~nico/brltty
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/*
 * csrjmp.c - cursor routing functions
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "csrjmp.h"
#include "scr.h"
#include "inskey.h"
#include "misc.h"


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
    scrstat scr;		/* for screen state infos */
    int curx, cury;		/* current cursor position */
    int dif, t = 0;
    sigset_t mask;		/* for blocking of SIGUSR1 */

    /* Set up signal mask: */
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);

    /* Initialise second thread of screen reading: */
    if (initscr_phys())
	return;

    getstat_phys(&scr);

    /* Deal with vertical movement first, ignoring horizontal jumping ... */
    dif = y - scr.posy;
    while (dif != 0 && curscr == scr.no) {
	timeout_yet(0);		/* initialise stop-watch */
	sigprocmask(SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
	inskey(dif > 0 ? DN_CSR : UP_CSR);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
	do {
#if CSRJMP_LOOP_DELAY > 0
	    delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
	    cury = scr.posy;
	    curx = scr.posx;
	    getstat_phys(&scr);
	} while (!(t = timeout_yet(CSRJMP_TIMEOUT)) &&
		 scr.posy == cury && scr.posx == curx);
	if (t)
	    break;
	if ((scr.posy == cury && (scr.posx - curx) * dif <= 0) ||
	    (scr.posy != cury && (y - scr.posy) * (y - scr.posy) >= dif * dif))
	{
	    delay(CSRJMP_SETTLE_DELAY);
	    getstat_phys(&scr);
	    if ((scr.posy == cury && (scr.posx - curx) * dif <= 0) ||
		(scr.posy != cury && (y - scr.posy) * (y - scr.posy) >= dif * dif))
	    {
		/* 
		 * We are getting farther from our target... Let's try to 
		 * go back to the previous position wich was obviously the 
		 * nearest ever reached to date before giving up.
		 */
		sigprocmask(SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
		inskey(dif < 0 ? DN_CSR : UP_CSR);
		sigprocmask(SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
		break;
	    }
	}
	dif = y - scr.posy;
    }

    if (x >= 0) {	/* don't do this for vertical-only routing (x=-1) */
	/* Now horizontal movement, quitting if the vertical position is wrong: */
	dif = x - scr.posx;
	while (dif != 0 && scr.posy == y && curscr == scr.no) {
	    timeout_yet(0);	/* initialise stop-watch */
	    sigprocmask(SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */
	    inskey(dif > 0 ? RT_CSR : LT_CSR);
	    sigprocmask(SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
	    do {
#if CSRJMP_LOOP_DELAY > 0
		delay(CSRJMP_LOOP_DELAY);	/* sleep a while ... */
#endif
		curx = scr.posx;
		getstat_phys(&scr);
	    }
	    while (!(t = timeout_yet(CSRJMP_TIMEOUT)) &&
		   scr.posx == curx && scr.posy == y);
	    if (t)
		break;
	    if (scr.posy != y ||
		(x - scr.posx) * (x - scr.posx) >= dif * dif)
	    {
		delay(CSRJMP_SETTLE_DELAY);
		getstat_phys(&scr);
		if (scr.posy != y ||
		    (x - scr.posx) * (x - scr.posx) >= dif * dif)
		{
		    /* 
		     * We probably wrapped on a short line... or are getting 
		     * farther from our target. Try to get back to the previous
		     * position which was obviously the nearest ever reached
		     * to date before we exit.
		     */
		    sigprocmask(SIG_BLOCK, &mask, NULL);  /* block SIGUSR1 */
		    inskey(dif > 0 ? LT_CSR : RT_CSR);
		    sigprocmask(SIG_UNBLOCK, &mask, NULL);  /* unblock SIGUSR1 */
		    break;
		}
	    }
	    dif = x - scr.posx;
	}
    }

    closescr_phys();		/* close second thread of screen reading */
}

void csrjmp(int x, int y, int scrno)
{
    sigset_t mask;
    /*
     * Fork cursor routing subprocess. * First, we must check if a
     * subprocess is already running: if so, we * send it a SIGUSR1 and
     * wait for it to die. 
     */
    /* NB According to man 2 wait, setting SIGCHLD handler to SIG_IGN may mean
       that wait can't catch the dying child. */
    if (csr_active) {
	kill(csr_pid, SIGUSR1);
	while (csr_active)
	    pause();
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);	/* block SIGUSR1 */

    csr_active++;

    switch (csr_pid = fork()) {
    case -1:			/* fork failed */
	csr_active--;
	break;
    case 0:			/* child, cursor routing process */
	nice(CSRJMP_NICENESS);	/* reduce scheduling priority */
	csrjmp_sub(x, y, scrno);
	exit(0);		/* terminate child process */
    default:			/* parent waits for child to return */
	break;
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);	/* unblock SIGUSR1 */
}
