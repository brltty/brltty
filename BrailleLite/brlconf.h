/* BrailleLite/brlconf.h - Configurable definitions for the Braille Lite driver
 * N. Nair, 6 September 1998
 *
 * Edit as necessary for your system.
 */


/* BrailleLite size (18/40) */
/* #define BLITE_SIZE 18 */

/* We always expect 8 data bits, no parity, 1 stop bit. */
#define BAUDRATE B9600		/* baud rate */


/* prefered/default status cells mode */
#define DEF_MODE  ST_None

