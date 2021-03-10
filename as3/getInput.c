#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "getInput.h"
#include "server.h"
#include "audioMixer.h"
#include "beatbox.h"
#include "helper.h"

pthread_mutex_t jInputMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t tid;
void *readInput();
static int GetInput_getJoyStickInput();
static void processJoystickInput(int input);

void GetInput_initJoystick()
{
    // set pins to be exported
    Helper_writeToFile(EXPORT, "26");
    Helper_sleep_thread(0, 50000000);

    Helper_writeToFile(EXPORT, "46");
    Helper_sleep_thread(0, 50000000);

    Helper_writeToFile(EXPORT, "65");
    Helper_sleep_thread(0, 50000000);

    Helper_writeToFile(EXPORT, "47");
    Helper_sleep_thread(0, 50000000);

    Helper_writeToFile(EXPORT, "27");
    Helper_sleep_thread(0, 50000000);
    // set joystick as input
    Helper_writeToFile(UP_DIR, "in");
    Helper_writeToFile(DOWN_DIR, "in");
    Helper_writeToFile(LEFT_DIR, "in");
    Helper_writeToFile(MIDDLE_DIR, "in");
    Helper_writeToFile(RIGHT_DIR, "in");

    // sleep for 400ms before attempting to use any of these pins
    Helper_sleep_thread(0, 40000000);

    pthread_create(&tid, NULL, &readInput, NULL);
}

void *readInput()
{
    int jInput;
    while (true)
    {
        jInput = GetInput_getJoyStickInput();
        processJoystickInput(jInput);
        while (jInput != NEUTRAL)
        {
            Helper_sleep_thread(0, 500000000);
            jInput = GetInput_getJoyStickInput();
            processJoystickInput(jInput);
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
        Helper_changeVolume("up");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Server_sendPacket(output);
        break;
    case NIEDER:
        // decrease volume
        Helper_changeVolume("down");
        sprintf(output, "volume&%d", AudioMixer_getVolume());
        Server_sendPacket(output);
        break;
    case LINKS:
        // decrease bpm
        Helper_changeTempo("down");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Server_sendPacket(output);
        break;
    case RECHTS:
        // decrease bpm
        Helper_changeTempo("up");
        sprintf(output, "bpm&%d", AudioMixer_getBPM());
        Server_sendPacket(output);
        break;
    case CENTER:
        // cycle through beat type
        pthread_mutex_lock(&Beatbox_playbackMutex);
        {
            Beatbox_currentlyPlayingSound = (Beatbox_currentlyPlayingSound + 1) % 3;
            sprintf(output, "%d", Beatbox_currentlyPlayingSound);
        }
        pthread_mutex_unlock(&Beatbox_playbackMutex);
        Server_sendPacket(output);
        break;
    }
}

static int GetInput_getJoyStickInput()
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
    Helper_sleep_thread(0, 30000000);
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
