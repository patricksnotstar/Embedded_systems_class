#ifndef _BEATBOX_H_
#define _BEATBOX_H_

#include <alsa/asoundlib.h>
#include <ctype.h>

#include "audioMixer.h"

#define HIHAT "beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define BASE "beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define SNARE "beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

enum sounds
{
    NONE,
    ROCK1,
    ROCK2
};

int Beatbox_currentlyPlayingSound;
pthread_mutex_t Beatbox_playbackMutex;
wavedata_t *hihat;
wavedata_t *snare;
wavedata_t *base;

void Beatbox_queueDrumBeat(wavedata_t *pSound);

#endif