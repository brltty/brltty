/* BrailleLite/brlconf.h - Configurable definitions for the Braille Lite driver
 * N. Nair, 6 September 1998
 *
 * Edit as necessary for your system.
 */


#define BRLNAME "BrailleLite"

/* We always expect 8 data bits, no parity, 1 stop bit. */
/* Select baudrate to use */
#define BAUDRATE B9600
//#define BAUDRATE B38400

/* Define the preferred/default status cells mode. */
#define PREFSTYLE ST_None
