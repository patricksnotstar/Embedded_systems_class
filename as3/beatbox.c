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

#define EXPORT "/sys/class/gpio/export"
#define UP_DIR "/sys/class/gpio/gpio26/direction"
#define DOWN_DIR "/sys/class/gpio/gpio46/direction"
#define LEFT_DIR "/sys/class/gpio/gpio65/direction"
#define RIGHT_DIR "/sys/class/gpio/gpio47/direction"
#define MIDDLE_DIR "/sys/class/gpio/gpio27/direction"
#define UPTIME "/proc/uptime"
#define HIHAT "beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define BASE "beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define SNARE "beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

pthread_mutex_t jInputMutex = PTHREAD_MUTEX_INITIALIZER;

static void sleep_thread(long seconds, long nanoseconds);
static void configurePins();
void *readInput();
static void processCommand(char *input, char *output);
static void changeVolume(char *direction);
static void getUptime(char *buff);
static void processJoystickInput(int input);
static void changeTempo(char *direction);
static void queueDrumBeat(char *drumBeat);
static void playRockBeat2();
static void playRockBeat1();

static pthread_t tid;

int main(int argc, char **argv)
{
    configurePins();
    Networking_configNetwork();
    AudioMixer_init();
    AudioMixer_setVolume(DEFAULT_VOLUME);

    int jInput;

    pthread_create(&tid, NULL, &readInput, &jInput);

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

static void changeVolume(char *direction)
{
    int currentVol = AudioMixer_getVolume();

    int max = 100;
    int min = 0;

    if (strncmp(direction, "up", strlen(direction)) == 0)
    {
        if (currentVol != max)
        {
            int newVolume = currentVol + 5;
            AudioMixer_setVolume(newVolume);
        }
    }
    else if (strncmp(direction, "down", strlen(direction)) == 0)
    {
        if (currentVol != min)
        {
            int newVolume = currentVol - 5;
            AudioMixer_setVolume(newVolume);
        }
    }
}

static void processCommand(char *input, char *output)
{
    memset(output, '\0', MSG_MAX_LEN);
    if (strncmp(input, "volume_up", strlen(input)) == 0)
    {
        changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "volume_down", strlen(input)) == 0)
    {
        changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "bpm_up", strlen(input)) == 0)
    {
        changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "bpm_down", strlen(input)) == 0)
    {
        changeTempo("down");
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
        sprintf(output, "mode1");
        playRockBeat1();
    }
    else if (strncmp(input, "mode2", strlen(input)) == 0)
    {
        sprintf(output, "mode2");
        playRockBeat2();
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
    for (int i = 0; i <= 1; i++)
    {
        queueDrumBeat(HIHAT);
        queueDrumBeat(BASE);
        sleep_thread(0, time_to_wait);
        queueDrumBeat(HIHAT);
        sleep_thread(0, time_to_wait);
        queueDrumBeat(HIHAT);
        queueDrumBeat(SNARE);
        sleep_thread(0, time_to_wait);
        queueDrumBeat(HIHAT);
    }
}

static void playRockBeat2()
{
    int bpm = AudioMixer_getBPM();
    long time_to_wait = ((60.0 / bpm) / 2) * 1000000000;
    for (int i = 0; i < MAX_SOUND_BITES; i++)
    {
        if (i % 3 == 0)
        {
            queueDrumBeat(HIHAT);
            sleep_thread(0, time_to_wait);
            continue;
        }
        if (i % 2 == 0)
        {
            queueDrumBeat(BASE);
            sleep_thread(0, time_to_wait);
        }
        else if (i % 2 == 1)
        {
            queueDrumBeat(SNARE);
            sleep_thread(0, time_to_wait);
        }
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

void *readInput()
{
    int jInput = NEUTRAL;
    while (true)
    {
        jInput = GetInput_getJoyStickInput();
        processJoystickInput(jInput);
        while (jInput != NEUTRAL)
        {
            sleep_thread(0, 500000000);
            jInput = GetInput_getJoyStickInput();
            processJoystickInput(jInput);
        }
    }
}

// takes in a string telling it which way to change tempo (up or down)
static void changeTempo(char *direction)
{
    int currentBPM = AudioMixer_getBPM();

    int max = 300;
    int min = 40;

    if (strncmp(direction, "up", strlen(direction)) == 0)
    {
        if (currentBPM < max)
        {
            int newBPM = currentBPM + 5;
            AudioMixer_setBPM(newBPM);
        }
    }
    else if (strncmp(direction, "down", strlen(direction)) == 0)
    {
        if (currentBPM > min)
        {
            int newBPM = currentBPM - 5;
            AudioMixer_setBPM(newBPM);
        }
    }
}

static void processJoystickInput(int input)
{
    char output[MSG_MAX_LEN];
    memset(output, '\0', sizeof(output));
    switch (input)
    {
    case NEUTRAL:
        break;
    case AUF:
        // increase volume
        changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Networking_sendPacket(output);
        break;
    case NIEDER:
        // decrease volume
        changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Networking_sendPacket(output);
        break;
    case LINKS:
        // decrease bpm
        changeTempo("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Networking_sendPacket(output);
        break;
    case RECHTS:
        // decrease bpm
        changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Networking_sendPacket(output);
        break;
    case CENTER:
        // cycle through beat type
        break;
    }
}

static void writeToFile(const char *fileName, const char *value)
{
    FILE *pFile = fopen(fileName, "w");
    if (pFile == NULL)
    {
        fprintf(stderr, "Unable to open path for writing\n");
        exit(-1);
    }
    fprintf(pFile, "%s", value);
    fclose(pFile);
}

static void configurePins()
{
    // set pins to be exported
    writeToFile(EXPORT, "26");
    sleep_thread(0, 33000000);

    writeToFile(EXPORT, "46");
    sleep_thread(0, 33000000);

    writeToFile(EXPORT, "65");
    sleep_thread(0, 33000000);

    writeToFile(EXPORT, "47");
    sleep_thread(0, 33000000);

    writeToFile(EXPORT, "27");

    // set joystick as input
    writeToFile(UP_DIR, "in");
    writeToFile(DOWN_DIR, "in");
    writeToFile(LEFT_DIR, "in");
    writeToFile(MIDDLE_DIR, "in");
    writeToFile(RIGHT_DIR, "in");

    // sleep for 400ms before attempting to use any of these pins
    sleep_thread(0, 40000000);
}

static void sleep_thread(long seconds, long nanoseconds)
{
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}
