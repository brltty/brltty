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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BRLTTY "/sbin/brltty"
#define REAL_INIT "/sbin/real_init"

static char *argumentVector[16];
static int argumentCount = 0;

static void
addArgument(char *argument) {
   argumentVector[argumentCount++] = argument;
}

static void
addOption(char *variable, char *option) {
   char *value = getenv(variable);
   if (value) {
      if (*value) {
         addArgument(option);
	 addArgument(value);
      }
   }
}

int
main(int argc, char *argv[]) {

   addArgument(BRLTTY);
   addOption("BRLTTY_DEVICE", "-d");
   addOption("BRLTTY_DRIVER", "-b");
   addOption("BRLTTY_TABLE", "-t");
   addArgument(NULL);

   switch (fork()) {
      case -1:  /* can't fork */
         perror("fork");
         exit(3);
      case 0:  /* child */
         execv(BRLTTY, argumentVector);
         /* execv() shoudn't return */
         perror("execv: " BRLTTY);
         exit(4);
      default:  /* parent */
         wait(NULL);
         execv(REAL_INIT, argv);
         /* execv() shouldn't return */
         perror("execv: " REAL_INIT);
         exit(5);
   }
   return 0;
}
