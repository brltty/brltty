/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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
 * brl_load.c - handling of dynamic braille drivers
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#include "brl.h"
#include "misc.h"

braille_driver *braille = NULL;	/* filled by dynamic libs */
char* braille_libname = NULL;	/* name of library */
static void* library = NULL;	/* handle to driver */
#define BRL_SYMBOL "brl_driver"

/* load driver from library */
/* return true (nonzero) on success */
int load_braille_driver(void)
{
  const char* error;

  #ifdef BRL_BUILTIN
    extern braille_driver brl_driver;
    if (braille_libname != NULL)
      if (strcmp(braille_libname, brl_driver.identifier) == 0)
        braille_libname = NULL;
    if (braille_libname == NULL)
      {
	braille = &brl_driver;
	braille_libname = "built-in";
	return 1;
      }
  #else
    if (braille_libname == NULL)
      return 0;
  #endif

  /* allow shortcuts */
  if (strlen(braille_libname) == 2)
    {
      static char name[] = "libbrlttyb??.so.1"; /* two ? for shortcut */
      char * pos = strchr(name, '?');
      pos[0] = braille_libname[0];
      pos[1] = braille_libname[1];
      braille_libname = name;
    }

  library = dlopen(braille_libname, RTLD_NOW|RTLD_GLOBAL);
  if (library == NULL) 
    {
      LogAndStderr(LOG_ERR, "%s", dlerror()); 
      LogAndStderr(LOG_ERR, "Cannot open braille driver library: %s", braille_libname);
      return 0;
    }

  braille = dlsym(library, BRL_SYMBOL); /* locate struct driver - filled with all the data */
  error = dlerror();
  if (error)
    {
      LogAndStderr(LOG_ERR, "%s", error);
      LogAndStderr(LOG_ERR, "Braille driver symbol not found: %s", BRL_SYMBOL);
      exit(10);
    }

  return 1;
}


int list_braille_drivers(void)
{
	char buf[64];
	static const char *list_file = LIB_PATH "/brltty-brl.lst";
	int cnt, fd = open( list_file, O_RDONLY );
	if (fd < 0) {
		fprintf( stderr, "Error: can't access braille driver list file\n" );
		perror( list_file );
		return 0;
	}
	fprintf( stderr, "Available Braille Drivers:\n\n" );
	fprintf( stderr, "XX\tDescription\n" );
	fprintf( stderr, "--\t-----------\n" );
	while( (cnt = read( fd, buf, sizeof(buf) )) )
		fwrite( buf, cnt, 1, stderr );
	close(fd);
	return 1;
}


