/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_MESSAGES
#define BRLTTY_INCLUDED_MESSAGES

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int setMessagesLanguage (const char *code);
extern const char *getMessagesLanguage (void);

extern int setMessagesDomain (const char *name);
extern const char *getMessagesDomain (void);

extern int setMessagesDirectory (const char *directory);
extern const char *getMessagesDirectory (void);

extern void ensureAllMessagesProperties (void);

extern int loadMessagesData (void);
extern void releaseMessagesData (void);

typedef struct MessageStruct Message;
extern const char *getMessageText (const Message *message);
extern uint32_t getMessageLength (const Message *message);

extern uint32_t getMessageCount (void);
extern const Message *getOriginalMessage (unsigned int index);
extern const Message *getTranslatedMessage (unsigned int index);

extern int findOriginalMessage (const char *text, size_t textLength, unsigned int *index);
extern const Message *findSimpleTranslation (const char *text, size_t length);
extern const Message *findPluralTranslation (const char *const *strings);

extern const char *getSimpleTranslation (const char *text);
extern const char *getPluralTranslation (const char *singular, const char *plural, unsigned long int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MESSAGES */
