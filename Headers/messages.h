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

extern int setMessagesLocale (const char *locale);
extern const char *getMessagesLocale (void);

extern int setMessagesDomain (const char *domain);
extern const char *getMessagesDomain (void);

extern int setMessagesDirectory (const char *directory);
extern const char *getMessagesDirectory (void);

extern void ensureAllMessagesProperties (void);

extern int loadMessagesData (void);
extern void releaseMessagesData (void);

typedef struct MessagesStringStruct MessagesString;
extern const char *getStringText (const MessagesString *string);
extern uint32_t getStringLength (const MessagesString *string);

extern uint32_t getStringCount (void);
extern const MessagesString *getOriginalString (unsigned int index);
extern const MessagesString *getTranslatedString (unsigned int index);

extern int findOriginalString (const char *text, size_t textLength, unsigned int *index);
extern const char *findBasicTranslation (const char *text, size_t length);
extern const char *findPluralTranslation (unsigned int index, const char *const *strings);

extern const char *getBasicTranslation (const char *text);
extern const char *getPluralTranslation (const char *singular, const char *plural, unsigned long int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MESSAGES */
