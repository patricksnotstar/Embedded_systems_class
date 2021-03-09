#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h> // for close()

#include "beatbox.h"
#include "networking.h"
#include "getInput.h"
#include "audioMixer.h"
#include "helper.h"

#define UPTIME "/proc/uptime"
#define HIHAT "beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define BASE "beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define SNARE "beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

static void processCommand(char *input, char *output);
static void getUptime(char *buff);
static void queueDrumBeat(char *drumBeat);
static void playRockBeat2();
static void playRockBeat1();
void *Sound_thread();
void *Networking_thread();

pthread_mutex_t networkingMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t networkId;
static pthread_t soundId;

int main(int argc, char **argv)
{
    time_t t;
    srand((unsigned)time(&t));
    GetInput_initJoystick();
    Networking_configNetwork();
    AudioMixer_init();
    AudioMixer_setVolume(DEFAULT_VOLUME);

    Beatbox_currentlyPlayingSound = ROCK1;
    if (pthread_mutex_init(&Beatbox_playbackMutex, NULL) != 0)
    {
        fprintf(stderr, "Unable to initialize playback mutex\n");
        exit(-1);
    }

    pthread_create(&networkId, NULL, &Networking_thread, NULL);
    pthread_create(&soundId, NULL, &Sound_thread, NULL);

    while (true)
    {
    }
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

void *Networking_thread()
{
    while (true)
    {
        char messageRx[MSG_MAX_LEN];
        memset(messageRx, '\0', sizeof(messageRx));
        socklen_t sin_len = sizeof(client);
        int bytesRx = recvfrom(socketDescriptor,
                               messageRx, MSG_MAX_LEN, 0,
                               (struct sockaddr *)&client, &sin_len);

        // Make it null terminated
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
        messageRx[terminateIdx] = '\0';
        char outResponse[MSG_MAX_LEN];
        processCommand(messageRx, outResponse);

        Networking_sendPacket(outResponse);
    }
}

static void processCommand(char *input, char *output)
{
    memset(output, '\0', MSG_MAX_LEN);
    if (strncmp(input, "volume_up", strlen(input)) == 0)
    {
        Helper_changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "volume_down", strlen(input)) == 0)
    {
        Helper_changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "bpm_up", strlen(input)) == 0)
    {
        Helper_changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "bpm_down", strlen(input)) == 0)
    {
        Helper_changeTempo("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "bpm_get", strlen(input)) == 0)
    {
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "volume_get", strlen(input)) == 0)
    {
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "hi-hat", strlen(input)) == 0)
    {
        sprintf(output, "hihat played");
        queueDrumBeat(HIHAT);
    }
    else if (strncmp(input, "snare", strlen(input)) == 0)
    {
        sprintf(output, "snare played");
        queueDrumBeat(SNARE);
    }
    else if (strncmp(input, "base", strlen(input)) == 0)
    {
        sprintf(output, "base played");
        queueDrumBeat(BASE);
    }
    else if (strncmp(input, "mode1", strlen(input)) == 0)
    {
        sprintf(output, "%d", 1);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = ROCK1;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "mode2", strlen(input)) == 0)
    {
        sprintf(output, "%d", 2);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = ROCK2;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "none", strlen(input)) == 0)
    {
        sprintf(output, "%d", 0);
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = NONE;
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
    }
    else if (strncmp(input, "uptime", strlen(input)) == 0)
    {
        char uptime[100];
        getUptime(uptime);
        sprintf(output, "uptime&%s", uptime);
    }
}

static void playRockBeat1()
{
    int bpm = AudioMixer_getBPM();
    long time_to_wait = ((60.0 / bpm) / 2) * 1000000000;
    queueDrumBeat(HIHAT);
    queueDrumBeat(BASE);
    Helper_sleep_thread(0, time_to_wait);
    queueDrumBeat(HIHAT);
    Helper_sleep_thread(0, time_to_wait);
    queueDrumBeat(HIHAT);
    queueDrumBeat(SNARE);
    Helper_sleep_thread(0, time_to_wait);
    queueDrumBeat(HIHAT);
    Helper_sleep_thread(0, time_to_wait);
}

static void playRockBeat2()
{
    int bpm = AudioMixer_getBPM();
    long time_to_wait = ((60.0 / bpm) / 2) * 1000000000;

    // queueDrumBeat(HIHAT);
    // Helper_sleep_thread(0, time_to_wait);
    // queueDrumBeat(BASE);
    // Helper_sleep_thread(0, time_to_wait);
    // queueDrumBeat(HIHAT);
    // Helper_sleep_thread(0, time_to_wait);
    // queueDrumBeat(BASE);
    // Helper_sleep_thread(0, time_to_wait);

    int n = rand() % 3;
    if (n == 0)
    {
        queueDrumBeat(HIHAT);
        Helper_sleep_thread(0, time_to_wait);
    }
    else if (n == 1)
    {
        queueDrumBeat(SNARE);
        Helper_sleep_thread(0, time_to_wait);
    }
    else
    {
        queueDrumBeat(BASE);
        Helper_sleep_thread(0, time_to_wait);
    }
}

static void queueDrumBeat(char *drumBeat)
{
    wavedata_t *pSound = malloc(sizeof(wavedata_t));
    AudioMixer_readWaveFileIntoMemory(drumBeat, pSound);
    AudioMixer_queueSound(pSound);
}

static void getUptime(char *buff)
{

    FILE *pFile = fopen(UPTIME, "r");
    if (pFile == NULL)
    {
        fprintf(stderr, "Unable to open path for writing\n");
        exit(-1);
    }
    fgets(buff, 100, pFile);
    fclose(pFile);
}
