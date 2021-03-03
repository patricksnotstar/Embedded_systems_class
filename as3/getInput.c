#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "getInput.h"

int GetInput_getJoyStickInput()
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