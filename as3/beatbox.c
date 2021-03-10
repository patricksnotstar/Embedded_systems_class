#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h> // for close()

#include "beatbox.h"
#include "server.h"
#include "getInput.h"
#include "helper.h"
#include "accelerometer.h"

#define UPTIME "/proc/uptime"
#define HIHAT "beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define BASE "beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define SNARE "beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

static void allocate_Sounds();

static void playRockBeat2();
static void playRockBeat1();
void *Sound_thread();

static pthread_t soundId;

int main(int argc, char **argv)
{
    Beatbox_currentlyPlayingSound = ROCK1;
    if (pthread_mutex_init(&Beatbox_playbackMutex, NULL) != 0)
    {
        fprintf(stderr, "Unable to initialize playback mutex\n");
        exit(-1);
    }
    time_t t;
    srand((unsigned)time(&t));

    allocate_Sounds();

    GetInput_initJoystick();
    Server_configNetwork();
    AudioMixer_init();
    Accel_initAccelerometer();

    pthread_create(&soundId, NULL, &Sound_thread, NULL);

    pthread_join(soundId, NULL);
}

static void allocate_Sounds()
{
    hihat = malloc(sizeof(wavedata_t));
    AudioMixer_readWaveFileIntoMemory(HIHAT, hihat);
    snare = malloc(sizeof(wavedata_t));
    AudioMixer_readWaveFileIntoMemory(SNARE, snare);
    base = malloc(sizeof(wavedata_t));
    AudioMixer_readWaveFileIntoMemory(BASE, base);
}

void *Sound_thread()
{
    while (true)
    {
        int sound;

        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            sound = Beatbox_currentlyPlayingSound;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
        switch (sound)
        {
        case ROCK1:
            playRockBeat1();
            break;
        case ROCK2:
            playRockBeat2();
            break;
        case NONE:
            break;
        }
    }
}

static void playRockBeat1()
{
    int bpm = AudioMixer_getBPM();
    long time_to_wait = ((60.0 / bpm) / 2) * 1000000000;
    AudioMixer_queueSound(hihat);
    AudioMixer_queueSound(base);
    Helper_sleep_thread(0, time_to_wait);
    AudioMixer_queueSound(hihat);
    Helper_sleep_thread(0, time_to_wait);
    AudioMixer_queueSound(hihat);
    AudioMixer_queueSound(snare);
    Helper_sleep_thread(0, time_to_wait);
    AudioMixer_queueSound(hihat);
    Helper_sleep_thread(0, time_to_wait);
}

static void playRockBeat2()
{
    int bpm = AudioMixer_getBPM();
    long time_to_wait = ((60.0 / bpm) / 2) * 1000000000;

    int n = rand() % 3;
    if (n == 0)
    {
        AudioMixer_queueSound(hihat);
        Helper_sleep_thread(0, time_to_wait);
    }
    else if (n == 1)
    {
        AudioMixer_queueSound(snare);
        Helper_sleep_thread(0, time_to_wait);
    }
    else
    {
        AudioMixer_queueSound(base);
        Helper_sleep_thread(0, time_to_wait);
    }
}
