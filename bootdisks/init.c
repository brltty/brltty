/*
 * Compile this with:
 * gcc -o init -s -static init.c
 *
 * This should replace /sbin/init on a Linux installation ramdisk or initrd.
 * IIRC, /linuxrc is used in the context of initrd, while an ordinary
 * ramdisk boots from /sbin/init. Anyway the safest thing is to leave the
 * symlink from /linuxrc -> /sbin/init and replace /sbin/init.
 * So rename current /sbin/init -> /sbin/real_init, and put
 * this program as -> /sbin/init (relative to the ramdisk, of course).
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BRLTTY "/sbin/brltty"
#define REAL_INIT "/sbin/real_init"

int main( int argc, char *argv[] )
{
	static char *noarg[] = { BRLTTY, NULL };

	switch( fork() ) {
	    case -1:  /* can't fork */
		perror( "fork()" );
		exit( -1 );
	    case 0:  /* child */
		execv( BRLTTY, noarg );
		/* execv() shoudn't return */
		perror( "execv(): " BRLTTY );
		exit( -1 );
	    default:  /* parent */
		wait( NULL );
		execv( REAL_INIT, argv );
		/* execv() shouldn't return */
		perror( "execve(): " REAL_INIT );
		exit( -1 );
	}
	return 0;
}
