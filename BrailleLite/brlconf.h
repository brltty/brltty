/* BrailleLite/brlconf.h - Configurable definitions for the Braille Lite driver
 * N. Nair, 6 September 1998
 *
 * Edit as necessary for your system.
 */

#ifndef BLITE_SIZE
   /* Set the display size here if it has not been set in the make file. */
   /* BrailleLite size (18/40) */
   #define BLITE_SIZE 18
   //#define BLITE_SIZE 40
#endif

#undef BRLDRIVER
#if BLITE_SIZE == 18
  #define BRLNAME "BrailleLite18"
  #define BRLDRIVER "b1"
#else
  #if BLITE_SIZE == 40
    #define BRLNAME "BrailleLite40"
    #define BRLDRIVER "b4"
  #else
    #error "BrailleLite size not specified."
  #endif
#endif

/* We always expect 8 data bits, no parity, 1 stop bit. */
/* Select baudrate to use */
#define BAUDRATE B9600
//#define BAUDRATE B38400

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_None
