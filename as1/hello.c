#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define LED0 "/sys/class/leds/beaglebone:green:usr0/trigger"
#define LED1 "/sys/class/leds/beaglebone:green:usr1/trigger"
#define LED2 "/sys/class/leds/beaglebone:green:usr2/trigger"
#define LED3 "/sys/class/leds/beaglebone:green:usr3/trigger"
#define LED0_b "/sys/class/leds/beaglebone:green:usr0/brightness"
#define LED1_b "/sys/class/leds/beaglebone:green:usr1/brightness"
#define LED2_b "/sys/class/leds/beaglebone:green:usr2/brightness"
#define LED3_b "/sys/class/leds/beaglebone:green:usr3/brightness"
#define UP "/sys/class/gpio/gpio26/value"
#define DOWN "/sys/class/gpio/gpio46/value"
#define LEFT "/sys/class/gpio/gpio65/value"
#define RIGHT "/sys/class/gpio/gpio47/value"
#define ROUNDS_NUM 8

enum joystickDirections
{
    NEUTRAL,
    AUF,
    NIEDER,
    LINKS,
    RECHTS
}; // Just Up, Down, Left, Right in German. English words are used and since I'm learning German, I used those

enum LED
{
    TOP = 0,
    BOTTOM = 3
};

enum RESULT
{
    CORRECT = 1,
    INCORRECT = 5
};

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
static int initI2cBus(char *bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0)
    {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

int getJoyStickInput();
void changeLedBrightness(char *ledName, bool brightness);
void noTriggerLeds(char *ledName);
int randomLed();
void turnOffLeds();
void turnOnLeds();
void flashLeds(int times);

int main(int argc, char **argv)
{
    printf("Hello embedded world, from Patrick Nguyen\n");
    // Turn off all Leds at the start
    noTriggerLeds(LED0);
    noTriggerLeds(LED1);
    noTriggerLeds(LED2);
    noTriggerLeds(LED3);
    turnOffLeds();

    int score = 0;
    int counter = 0;
    int jInput = NEUTRAL;
    int led = 0;
    while (counter < ROUNDS_NUM)
    {
        printf("Press joystick; current score (%d / %d)\n", score, counter);
        led = randomLed();
        while (jInput == NEUTRAL)
        {
            jInput = getJoyStickInput();
        }
        if ((led == TOP && jInput == AUF) || (led == BOTTOM && jInput == NIEDER))
        {
            score++;
            printf("Correct!\n");
            flashLeds(CORRECT);
        }
        else if (jInput == LINKS || jInput == RECHTS)
        {
            break;
        }
        else
        {
            printf("Incorrect! :(\n");
            flashLeds(INCORRECT);
        }
        jInput = getJoyStickInput();
        while (jInput != NEUTRAL)
        {
            jInput = getJoyStickInput();
        }
        counter++;
    }

    printf("Your final score was (%d / %d)\n", score, counter);
    printf("Thank you for playing!\n");
    turnOffLeds();
    return 0;
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
    // save joystick values
    char up[2];
    char down[2];
    char left[2];
    char right[2];

    fgets(up, 2, pUpFile);
    fgets(down, 2, pDownFile);
    fgets(left, 2, pLeftFile);
    fgets(right, 2, pRightFile);
    // close files
    fclose(pUpFile);
    fclose(pDownFile);
    fclose(pLeftFile);
    fclose(pRightFile);
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
    else
    {
        return NEUTRAL;
    }
}

void changeLedBrightness(char *ledName, bool brightness)
{
    FILE *pLedBrightnessFile = fopen(ledName, "w");
    if (pLedBrightnessFile == NULL)
    {
        printf("ERROR OPENING FILE %s.\n", ledName);
        exit(1);
    }

    int charWritten;
    if (brightness)
    {
        charWritten = fprintf(pLedBrightnessFile, "1");
    }
    else
    {
        charWritten = fprintf(pLedBrightnessFile, "0");
    }
    if (charWritten <= 0)
    {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedBrightnessFile);
}

void noTriggerLeds(char *ledName)
{
    FILE *pLedTriggerFile = fopen(ledName, "w");
    if (pLedTriggerFile == NULL)
    {
        printf("ERROR OPENING FILE %s.\n", ledName);
        exit(1);
    }
    int charWritten = fprintf(pLedTriggerFile, "none");
    if (charWritten <= 0)
    {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedTriggerFile);
}

int randomLed()
{
    time_t t;
    srand((unsigned)time(&t));
    int n = rand() % 2;
    if (n == 1)
    {
        changeLedBrightness(LED0_b, true);
        return TOP;
    }
    else
    {
        changeLedBrightness(LED3_b, true);
        return BOTTOM;
    }
    return -1;
}

void turnOffLeds()
{
    changeLedBrightness(LED0_b, false);
    changeLedBrightness(LED1_b, false);
    changeLedBrightness(LED2_b, false);
    changeLedBrightness(LED3_b, false);
}

void turnOnLeds()
{
    changeLedBrightness(LED0_b, true);
    changeLedBrightness(LED1_b, true);
    changeLedBrightness(LED2_b, true);
    changeLedBrightness(LED3_b, true);
}

void flashLeds(int times)
{
    for (int i = 0; i < times; i++)
    {
        turnOnLeds();
        long seconds = 0;
        long nanoseconds = 100000000;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *)NULL);
        turnOffLeds();
        nanosleep(&reqDelay, (struct timespec *)NULL);
    }
}