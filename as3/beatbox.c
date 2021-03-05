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

pthread_mutex_t jInputMutex = PTHREAD_MUTEX_INITIALIZER;

static void sleep_thread(long seconds, long nanoseconds);
static void configurePins();
void *readInput();
static void processCommand(char *input, char *output);
static void changeVolume(char *direction);
static void getUptime(char *buff);
static void processJoystickInput(int input);
static void changeTempo(char *direction);

static pthread_t tid;

int main(int argc, char **argv)
{
    configurePins();
    Networking_configNetwork();
    AudioMixer_setVolume(DEFAULT_VOLUME);

    int jInput;

    pthread_create(&tid, NULL, &readInput, &jInput);

    // int volume = AudioMixer_getVolume();
    // AudioMixer_setVolume(newVolume);

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
        changeVolume("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
    }
    else if (strncmp(input, "volume_get", strlen(input)) == 0)
    {
        sprintf(output, "volume&%d", AudioMixer_getVolume());
    }
    else if (strncmp(input, "uptime", strlen(input)) == 0)
    {
        char uptime[100];
        getUptime(uptime);
        sprintf(output, "uptime&%s", uptime);
    }
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
        // pthread_mutex_lock(&jInputMutex);
        // {
        jInput = GetInput_getJoyStickInput();
        // }
        // pthread_mutex_unlock(&jInputMutex);
        sleep_thread(0, 10000000);

        while (jInput != NEUTRAL)
        {
            // pthread_mutex_lock(&jInputMutex);
            // {
            // if user holds down for longer than 1 seconds
            // assume they are doing a repeated command
            jInput = GetInput_getJoyStickInput();
            processJoystickInput(jInput);
            sleep_thread(1, 0);

            // }
            // pthread_mutex_unlock(&jInputMutex);
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
    // do we have to memset it to empty everytime?

    char output[MSG_MAX_LEN];
    memset(output, '\0', sizeof(output));
    switch (input)
    {
    // NEUTRAL
    case 0:
        break;
    // UP
    case 1:
        // increase volume
        changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Networking_sendPacket(output);
        break;
    // DOWN
    case 2:
        // decrease volume
        changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Networking_sendPacket(output);
        break;
    // LEFT
    case 3:
        // decrease bpm
        changeTempo("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Networking_sendPacket(output);
        break;
    //RIGHT
    case 4:
        // decrease bpm
        changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Networking_sendPacket(output);
        break;
    // CENTER
    case 5:
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
    writeToFile(EXPORT, "46");
    writeToFile(EXPORT, "65");
    writeToFile(EXPORT, "47");
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

int getJoyStickInput()
{
    FILE *pUpFile = fopen(UP, "r");
    if (pUpFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", UP);
        exit(-1);
    }
    FILE *pDownFile = fopen(DOWN, "r");
    if (pDownFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", DOWN);
        exit(-1);
    }
    FILE *pLeftFile = fopen(LEFT, "r");
    if (pLeftFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", LEFT);
        exit(-1);
    }
    FILE *pRightFile = fopen(RIGHT, "r");
    if (pRightFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", RIGHT);
        exit(-1);
    }

    FILE *pMidFile = fopen(MIDDLE, "r");
    if (pMidFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", MIDDLE);
        exit(-1);
    }
    // save joystick values
    char up[2];
    char down[2];
    char left[2];
    char right[2];
    char mid[2];

    fgets(up, 2, pUpFile);
    fgets(down, 2, pDownFile);
    fgets(left, 2, pLeftFile);
    fgets(right, 2, pRightFile);
    fgets(mid, 2, pMidFile);
    // close files
    fclose(pUpFile);
    fclose(pDownFile);
    fclose(pLeftFile);
    fclose(pRightFile);
    fclose(pMidFile);
    // check if any direction is pressed
    if (up[0] == '0')
    {
        return AUF;
    }
    else if (down[0] == '0')
    {
        return NIEDER;
    }
    else if (left[0] == '0')
    {
        return LINKS;
    }
    else if (right[0] == '0')
    {
        return RECHTS;
    }
    else if (mid[0] == '0')
    {
        return CENTER;
    }
    else
    {
        return NEUTRAL;
    }
}