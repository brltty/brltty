/* this header file is used to create the driver structure */
/* for a dynamically loadable braille display driver. */
/* BRLNAME, HELPNAME, and PREFSTYLE must be defined - see brlconf.h */

/* Routines provided by this braille display driver. */
static void identbrl (void);/* print start-up messages */
static void initbrl (brldim *, const char *); /* initialise Braille display */
static void closebrl (brldim *); /* close braille display */
static void writebrl (brldim *); /* write to braille display */
static int readbrl (int);	/* get key press from braille display */
static void setbrlstat (const unsigned char *);	/* set status cells */

braille_driver brl_driver = 
{
  BRLNAME,
  BRLDRIVER,
  HELPNAME,
  PREFSTYLE,
  identbrl,
  initbrl,
  closebrl,
  writebrl,
  readbrl,
  setbrlstat
};

