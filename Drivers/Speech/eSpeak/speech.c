/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_POSIX_THREADS
#include <pthread.h>
#endif /* HAVE_POSIX_THREADS */

#include "log.h"
#include "parse.h"

typedef enum {
	PARM_PATH,
	PARM_PUNCTLIST,
	PARM_VOICE,
	PARM_MAXRATE
} DriverParameter;
#define SPKPARMS "path", "punctlist", "voice", "maxrate"

#define SPK_HAVE_TRACK
#define SPK_HAVE_RATE
#define SPK_HAVE_VOLUME
#define SPK_HAVE_PITCH
#define SPK_HAVE_PUNCTUATION
#include "spk_driver.h"

#include <espeak/speak_lib.h>

#if ESPEAK_API_REVISION < 6
#define espeakRATE_MINIMUM	80
#define espeakRATE_MAXIMUM	450
#endif /* ESPEAK_API_REVISION < 6 */

static int maxrate = espeakRATE_MAXIMUM;
static volatile int IndexPos;

#ifdef HAVE_POSIX_THREADS

static pthread_t request_thread;
static pthread_mutex_t queue_lock;
static pthread_cond_t queue_cond;
static volatile int alive;

struct request {
	struct request *next;
	unsigned buflength;
	unsigned char buffer[0];
};

static struct request *request_queue;

static void free_queue(void)
{
	struct request *curr = request_queue;
	request_queue = NULL;
	while (curr) {
		struct request *next = curr->next;
		free(curr);
		curr = next;
	}
}
	
static void *process_request(void *unused)
{
	struct request *curr;
	int result;

	pthread_mutex_lock(&queue_lock);
	while (alive) {
		curr = request_queue;
		if (!curr) {
			pthread_cond_wait(&queue_cond, &queue_lock);
			continue;
		}
		request_queue = curr->next;
		pthread_mutex_unlock(&queue_lock);
		if (curr->buflength) {
			result = espeak_Synth(curr->buffer, curr->buflength, 0,
					POS_CHARACTER, 0, espeakCHARS_UTF8,
					NULL, NULL);
			if (result != EE_OK)
				logMessage(LOG_ERR, "eSpeak: Synth() returned error %d", result);
		} else {
			espeak_Cancel();
		}
		free(curr);
		pthread_mutex_lock(&queue_lock);
	}
	pthread_mutex_unlock(&queue_lock);

	return NULL;
}

#endif /* HAVE_POSIX_THREADS */

static void
spk_say(SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes)
{
#ifdef HAVE_POSIX_THREADS
	struct request *new, **prev_next;

	IndexPos = 0;

	if (!length)
		return;

	new = malloc(sizeof(*new) + length + 1);
	if (!new) {
		logSystemError("eSpeak: malloc");
		return;
	}
	new->next = NULL;
	new->buflength = length + 1;
	memcpy(new->buffer, buffer, length);
	new->buffer[length] = 0;

	pthread_mutex_lock(&queue_lock);
	prev_next = &request_queue;
	while (*prev_next)
		prev_next = &(*prev_next)->next;
	*prev_next = new;
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_lock);
#else /* HAVE_POSIX_THREADS */
	int result;

	IndexPos = 0;

	/* add 1 to the length in order to pass along the trailing zero */
	result = espeak_Synth(buffer, length+1, 0, POS_CHARACTER, 0,
			espeakCHARS_UTF8, NULL, NULL);
	if (result != EE_OK)
		logMessage(LOG_ERR, "eSpeak: Synth() returned error %d", result);
#endif /* HAVE_POSIX_THREADS */
}

static void
spk_mute(SpeechSynthesizer *spk)
{
#ifdef HAVE_POSIX_THREADS
	struct request *stop = malloc(sizeof(*stop));
	if (!stop) {
		logSystemError("eSpeak: malloc");
		return;
	}
	stop->next = NULL;
	stop->buflength = 0;
	pthread_mutex_lock(&queue_lock);
	free_queue();
	request_queue = stop;
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_lock);
#else /* HAVE_POSIX_THREADS */
	espeak_Cancel();
#endif /* HAVE_POSIX_THREADS */
}

static int SynthCallback(short *audio, int numsamples, espeak_EVENT *events)
{
	while (events->type != espeakEVENT_LIST_TERMINATED) {
		if (events->type == espeakEVENT_WORD)
			IndexPos = events->text_position - 1;
		events++;
	}
	return 0;
}

static void
spk_doTrack(SpeechSynthesizer *spk)
{
}

static int
spk_getTrack(SpeechSynthesizer *spk)
{
	return IndexPos;
}

static int
spk_isSpeaking(SpeechSynthesizer *spk)
{
	return espeak_IsPlaying();
}

static void
spk_setVolume(SpeechSynthesizer *spk, unsigned char setting)
{
	int volume = getIntegerSpeechVolume(setting, 50);
	espeak_SetParameter(espeakVOLUME, volume, 0);
}

static void
spk_setRate(SpeechSynthesizer *spk, unsigned char setting)
{
	int h_range = (maxrate - espeakRATE_MINIMUM)/2;
	int rate = getIntegerSpeechRate(setting, h_range) + espeakRATE_MINIMUM;
	espeak_SetParameter(espeakRATE, rate, 0);
}

static void
spk_setPitch(SpeechSynthesizer *spk, unsigned char setting)
{
	int pitch = getIntegerSpeechPitch(setting, 50);
	espeak_SetParameter(espeakPITCH, pitch, 0);
}

static void
spk_setPunctuation(SpeechSynthesizer *spk, SpeechPunctuation setting)
{
	espeak_PUNCT_TYPE punct;
	if (setting <= SPK_PUNCTUATION_NONE)
		punct = espeakPUNCT_NONE;
	else if (setting >= SPK_PUNCTUATION_ALL)
		punct = espeakPUNCT_ALL;
	else
		punct = espeakPUNCT_SOME;
	espeak_SetParameter(espeakPUNCTUATION, punct, 0);
}

static int spk_construct(SpeechSynthesizer *spk, char **parameters)
{
	char *data_path, *voicename, *punctlist;
	int result;

	logMessage(LOG_INFO, "eSpeak version %s", espeak_Info(NULL));

	data_path = parameters[PARM_PATH];
	if (data_path && !*data_path)
		data_path = NULL;
	result = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, data_path, 0);
	if (result < 0) {
		logMessage(LOG_ERR, "eSpeak: initialization failed");
		return 0;
	}

	voicename = parameters[PARM_VOICE];
	if(!voicename || !*voicename)
		voicename = "default";
	result = espeak_SetVoiceByName(voicename);
	if (result != EE_OK) {
		espeak_VOICE voice_select;
		memset(&voice_select, 0, sizeof(voice_select));
		voice_select.languages = voicename;
		result = espeak_SetVoiceByProperties(&voice_select);
	}
	if (result != EE_OK) {
		logMessage(LOG_ERR, "eSpeak: unable to load voice '%s'", voicename);
		return 0;
	}

	punctlist = parameters[PARM_PUNCTLIST];
	if (punctlist && *punctlist) {
		wchar_t w_punctlist[strlen(punctlist) + 1];
		int i = 0;
		while ((w_punctlist[i] = punctlist[i]) != 0) i++;
		espeak_SetPunctuationList(w_punctlist);
	}

	if (parameters[PARM_MAXRATE]) {
		int val = atoi(parameters[PARM_MAXRATE]);
		if (val > espeakRATE_MINIMUM) maxrate = val;
	}

	espeak_SetSynthCallback(SynthCallback);

#ifdef HAVE_POSIX_THREADS

	pthread_mutex_init(&queue_lock, NULL);
	pthread_cond_init(&queue_cond, NULL);

	alive = 1;
	result = pthread_create(&request_thread, NULL, process_request, NULL);
	if (result) {
		logMessage(LOG_ERR, "eSpeak: unable to create thread");
		espeak_Terminate();
		pthread_mutex_destroy(&queue_lock);
		pthread_cond_destroy(&queue_cond);
		return 0;
	}

#endif /* HAVE_POSIX_THREADS */

	return 1;
}

static void spk_destruct(SpeechSynthesizer *spk)
{
#ifdef HAVE_POSIX_THREADS
	pthread_mutex_lock(&queue_lock);
	alive = 0;
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_lock);
	pthread_join(request_thread, NULL);
	pthread_mutex_destroy(&queue_lock);
	pthread_cond_destroy(&queue_cond);
	free_queue();
#endif /* HAVE_POSIX_THREADS */

	espeak_Cancel();
	espeak_Terminate();
}
