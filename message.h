/* message.h - send a message to Braille and speech */

/* Prototype: */
void message (unsigned char *, short);


/* Flags for the second argument: */
#define MSG_SILENT 1		/* Prevent output to speech */
#define MSG_WAITKEY 2		/* Wait for a key after the message is displayed */
