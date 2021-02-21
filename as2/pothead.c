#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include "sorter.h"
#include "networking.h"
#include "pothead.h"

#define PIECEWISE_NUM_POINTS 10
#define A2D_FILE_VOLTAGE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095

const int PIECEWISE_A2D[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4100};
const int PIECEWISE_SIZE[] = {1, 20, 60, 120, 250, 300, 500, 800, 1200, 2100};

int Pothead_calcArraySize(int A2Dinput)
{
    if (A2Dinput < 0)
    {
        A2Dinput = 0;
    }
    else if (A2Dinput >= 4095)
    {
        A2Dinput = 4095;
    }
    // find neighbouring points on either side of input
    for (int i = 0; i < PIECEWISE_NUM_POINTS; i++)
    {
        if (A2Dinput == PIECEWISE_A2D[i])
        {
            printf("match\n");
            return PIECEWISE_SIZE[i];
        }
        else if (A2Dinput > PIECEWISE_A2D[i] && A2Dinput < PIECEWISE_A2D[i + 1])
        {
            float a = PIECEWISE_A2D[i];
            float b = PIECEWISE_A2D[i + 1];
            float m = PIECEWISE_SIZE[i];
            float n = PIECEWISE_SIZE[i + 1];

            return (((A2Dinput - a) / (b - a)) * (n - m)) + m;
        }
    }

    return 0;
}

int Pothead_getVoltage0Reading()
{
    // Open file
    FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
    if (!f)
    {
        printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    // Get reading
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0)
    {
        printf("ERROR: Unable to read values from voltage input file.\n");
        exit(-1);
    }
    // Close file
    fclose(f);
    return a2dReading;
}