#include <stdio.h>
#include "alva_api.h"
#include <unistd.h>

void main ()
{
   int i, len;
   unsigned char buf[100];

   if (ioperm (0x378, 3, 1))
   {
      perror ("ioperm ()");
      puts ("\7Only root may do this...");
      exit (1);
   } // if
   BrailleOpen ((unsigned char *)0x378);
   BrailleWrite ((unsigned char *)
      "\033B\000\022\000\000\000\000\000\123\021\007\007\025\000" \
      "\072\025\027\007\031\000\000\015", 27);
   BrailleProcess ();
   BrailleProcess ();
   BrailleProcess ();
   len = BrailleRead (buf, 50);
   BrailleClose ();
   printf ("\n");
   for (i = 0; i < 10; i++)
      printf ("%02x ", buf[i]);
   printf ("\n");
} // main
