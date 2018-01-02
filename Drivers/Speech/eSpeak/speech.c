/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "parse.h"

typedef enum {
	PARM_PATH,
	PARM_PUNCTLIST,
	PARM_VOICE,
	PARM_MAXRATE
} DriverParameter;
#define SPKPARMS "path", "punctlist", "voice", "maxrate"

#include "spk_driver.h"

#include <espeak/speak_lib.h>

#if ESPEAK_API_REVISION < 6
#define espeakRATE_MINIMUM	80
#define espeakRATE_MAXIMUM	450
#endif /* ESPEAK_API_REVISION < 6 */

static int maxrate = espeakRATE_MAXIMUM;

static void
spk_say(volatile SpeechSynthesizer *spk, const unsigned char *buffer, size_t length, size_t count, const unsigned char *attributes)
{
	int result;

	/* add 1 to the length in order to pass along the trailing zero */
	result = espeak_Synth(buffer, length+1, 0, POS_CHARACTER, 0,
			espeakCHARS_UTF8, NULL, (void *)spk);
	if (result != EE_OK)
		logMessage(LOG_ERR, "eSpeak: Synth() returned error %d", result);
}

static void
spk_mute(volatile SpeechSynthesizer *spk)
{
	espeak_Cancel();
}

static int SynthCallback(short *audio, int numsamples, espeak_EVENT *events)
{
	volatile SpeechSynthesizer *spk = events->user_data;

	while (events->type != espeakEVENT_LIST_TERMINATED) {
		if (events->type == espeakEVENT_WORD)
			tellSpeechLocation(spk, events->text_position - 1);
		if (events->type == espeakEVENT_MSG_TERMINATED)
			tellSpeechFinished(spk);
		events++;
	}
	return 0;
}

static void
spk_drain(volatile SpeechSynthesizer *spk)
{
	espeak_Synchronize();
}

static void
spk_setVolume(volatile SpeechSynthesizer *spk, unsigned char setting)
{
	int volume = getIntegerSpeechVolume(setting, 50);
	espeak_SetParameter(espeakVOLUME, volume, 0);
}

static void
spk_setRate(volatile SpeechSynthesizer *spk, unsigned char setting)
{
	int h_range = (maxrate - espeakRATE_MINIMUM)/2;
	int rate = getIntegerSpeechRate(setting, h_range) + espeakRATE_MINIMUM;
	espeak_SetParameter(espeakRATE, rate, 0);
}

static void
spk_setPitch(volatile SpeechSynthesizer *spk, unsigned char setting)
{
	int pitch = getIntegerSpeechPitch(setting, 50);
	espeak_SetParameter(espeakPITCH, pitch, 0);
}

static void
spk_setPunctuation(volatile SpeechSynthesizer *spk, SpeechPunctuation setting)
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

static int spk_construct(volatile SpeechSynthesizer *spk, char **parameters)
{
	char *data_path, *voicename, *punctlist;
	int result;

	spk->setVolume = spk_setVolume;
	spk->setRate = spk_setRate;
	spk->setPitch = spk_setPitch;
	spk->setPunctuation = spk_setPunctuation;
	spk->drain = spk_drain;

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

	return 1;
}

static void spk_destruct(volatile SpeechSynthesizer *spk)
{
	espeak_Cancel();
	espeak_Terminate();
}
