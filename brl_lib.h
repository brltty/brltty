
/* this header file is used to create the driver structure for dynamic loading */
/* DEF_MODE and HELPFILE must be defined - see brlconf.h */

/* Routines provided this braille driver */
void identbrl (const char *);/* print start-up messages */
void initbrl (brldim *, const char *); /* initialise Braille display */
void closebrl (brldim *); /* close braille display */
void writebrl (brldim *); /* write to braille display */
int readbrl (int);	/* get key press from braille display */
void setbrlstat (const unsigned char *);	/* set status cells */

driver brl_driver = 
{
  "BRLNAME",
  HELPNAME,
  DEF_MODE,
  identbrl,
  initbrl,
  closebrl,
  writebrl,
  readbrl,
  setbrlstat
};

