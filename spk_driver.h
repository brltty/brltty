/* this header file is used to create the driver structure */
/* for a dynamically loadable speech driver. */
/* SPKNAME must be defined - see spkconf.h */

/* Routines provided by this speech driver. */
static void identspk (void);/* print start-up messages */
static void initspk (char *parm); /* initialize speech device */
static void say (unsigned char *buffer, int len); /* speak text */
#ifdef SPK_HAVE_SAYATTRIBS
static void sayWithAttribs (unsigned char *buffer, int len); /* speak text */
#endif
static void mutespk (void); /* mute speech */
#ifdef SPK_HAVE_TRACK
static void processSpkTracking (void); /* Get current speaking position */
static int trackspk (void); /* Get current speaking position */
static int isSpeaking (void);
#else
static void processSpkTracking (void) { }
static int trackspk () { return 0; }
static int isSpeaking () { return 0; }
#endif
static void closespk (void); /* close speech device */

speech_driver spk_driver = 
{
  SPKNAME,
  SPKDRIVER,
  identspk,
  initspk,
  say,
#ifdef SPK_HAVE_SAYATTRIBS
  sayWithAttribs,
#else
  0,
#endif
  mutespk,
  processSpkTracking,
  trackspk,
  isSpeaking,
  closespk
};
