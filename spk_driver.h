/* this header file is used to create the driver structure */
/* for a dynamically loadable speech driver. */
/* SPKNAME must be defined - see spkconf.h */

/* Routines provided by this speech driver. */
static void identspk (void);/* print start-up messages */
static void initspk (void); /* initialize speech device */
static void say (unsigned char *buffer, int len); /* speak text */
static void mutespk (void); /* mute speech */
static void closespk (void); /* close speech device */

speech_driver spk_driver = 
{
  SPKNAME,
  SPKDRIVER,
  identspk,
  initspk,
  say,
  mutespk,
  closespk
};

