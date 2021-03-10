#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "helper.h"
#include "audioMixer.h"

void Helper_sleep_thread(long seconds, long nanoseconds)
{
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

void Helper_writeToFile(const char *fileName, const char *value)
{
    FILE *pFile = fopen(fileName, "w");
    if (pFile == NULL)
    {
        fprintf(stderr, "Unable to open path for writing %s\n", fileName);
        exit(-1);
    }
    fprintf(pFile, "%s", value);
    fclose(pFile);
}

void Helper_changeVolume(char *direction)
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

// takes in a string telling it which way to change tempo (up or down)
void Helper_changeTempo(char *direction)
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