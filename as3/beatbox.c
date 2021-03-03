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

pthread_mutex_t jInputMutex = PTHREAD_MUTEX_INITIALIZER;

static void sleep_thread(long seconds, long nanoseconds);
static void configurePins();
void *readInput();

static pthread_t tid;

int main(int argc, char **argv)
{
    configurePins();
    Networking_configNetwork();

    int jInput;

    pthread_create(&tid, NULL, &readInput, &jInput);

    while (true)
    {
    }
}

void *readInput()
{
    int jInput = NEUTRAL;
    while (true)
    {
        pthread_mutex_lock(&jInputMutex);
        {
            jInput = GetInput_getJoyStickInput();
        }
        pthread_mutex_unlock(&jInputMutex);
        sleep_thread(0, 10000000);

        while (jInput != NEUTRAL)
        {
            pthread_mutex_lock(&jInputMutex);
            {
                jInput = GetInput_getJoyStickInput();
            }
            pthread_mutex_unlock(&jInputMutex);
        }
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