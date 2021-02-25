#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "sorter.h"
#include "pothead.h"
#include "littyDaSeggy.h"

#define I2C_DEVICE_ADDRESS 0x20
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15

#define EXPORT "/sys/class/gpio/export"
#define LEFT_DIRECTION "/sys/class/gpio/gpio61/direction"
#define RIGHT_DIRECTION "/sys/class/gpio/gpio44/direction"
#define LEFT_DIGIT "/sys/class/gpio/gpio61/value"
#define RIGHT_DIGIT "/sys/class/gpio/gpio44/value"

static void changeState(char *digit, char *state);
static int initI2cBus(char *bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void writeDigit(int i2cFile, int num);
// static void writeNumber(int i2cFileDesc, int num);
// static unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr);

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

void configureI2C(int i2cFileDesc)
{

    i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    system("config-pin P9_17 i2c");
    system("config-pin P9_18 i2c");

    writeToFile(EXPORT, "61");

    long seconds = 0;
    long nanoseconds = 30000000;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);

    writeToFile(EXPORT, "44");

    nanosleep(&reqDelay, (struct timespec *)NULL);

    writeToFile(LEFT_DIRECTION, "out");
    writeToFile(RIGHT_DIRECTION, "out");

    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    changeState(LEFT_DIGIT, "0");
    changeState(RIGHT_DIGIT, "0");
}

void shutDownI2C(int i2cFileDesc)
{

    changeState(LEFT_DIGIT, "0");
    changeState(RIGHT_DIGIT, "0");
    close(i2cFileDesc);
}

// turns the right digit on or off
static void changeState(char *digit, char *state)
{
    FILE *f = fopen(digit, "w");
    if (f == NULL)
    {
        printf("ERROR: Unable to open %s.\n", digit);
        exit(-1);
    }
    fprintf(f, "%s", state);
    fclose(f);
}

// int main()
// {
//     // printf("Drive display (assumes GPIO #61 and #44 are output and 1\n");
//     int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

//     configureI2C(i2cFileDesc);

//     for(int i=0; i<200; i++){
//         Seg_writeNumber(i2cFileDesc, 69);

//         // long seconds = 0;
//         // long nanoseconds = 50000000;
//         // struct timespec reqDelay = {seconds, nanoseconds};
//         // nanosleep(&reqDelay, (struct timespec *)NULL);

//     }

//     shutDownI2C(i2cFileDesc);

//     return 0;
// }

static int initI2cBus(char *bus, int address)
{
    int i2cFile = open(bus, O_RDWR);
    int result = ioctl(i2cFile, I2C_SLAVE, address);
    if (result < 0)
    {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFile;
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2)
    {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

// static unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr)
// {
//     // To read a register, must first write the address
//     int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));

//     if (res != sizeof(regAddr))
//     {
//         perror("I2C: Unable to write to i2c register.");
//         exit(1);
//     }
//     // Now read the value and return it
//     char value = 0;
//     res = read(i2cFileDesc, &value, sizeof(value));
//     if (res != sizeof(value))
//     {
//         perror("I2C: Unable to read from i2c register");
//         exit(1);
//     }
//     return value;
// }

static void writeDigit(int i2cFile, int num)
{
    switch (num)
    {
    case 0:
        writeI2cReg(i2cFile, REG_OUTA, 0xa1);
        writeI2cReg(i2cFile, REG_OUTB, 0x86);
        break;
    case 1:
        writeI2cReg(i2cFile, REG_OUTA, 0x80);
        writeI2cReg(i2cFile, REG_OUTB, 0x02);
        break;
    case 2:
        writeI2cReg(i2cFile, REG_OUTA, 0x31);
        writeI2cReg(i2cFile, REG_OUTB, 0xe);
        break;
    case 3:
        writeI2cReg(i2cFile, REG_OUTA, 0xb0);
        writeI2cReg(i2cFile, REG_OUTB, 0x0e);
        break;
    case 4:
        writeI2cReg(i2cFile, REG_OUTA, 0x90);
        writeI2cReg(i2cFile, REG_OUTB, 0x8a);
        break;
    case 5:
        writeI2cReg(i2cFile, REG_OUTA, 0xb0);
        writeI2cReg(i2cFile, REG_OUTB, 0x8c);
        break;
    case 6:
        writeI2cReg(i2cFile, REG_OUTA, 0xb1);
        writeI2cReg(i2cFile, REG_OUTB, 0x8d);
        break;
    case 7:
        writeI2cReg(i2cFile, REG_OUTA, 0x4);
        writeI2cReg(i2cFile, REG_OUTB, 0x14);
        break;
    case 8:
        writeI2cReg(i2cFile, REG_OUTA, 0xb1);
        writeI2cReg(i2cFile, REG_OUTB, 0x8e);
        break;
    case 9:
        writeI2cReg(i2cFile, REG_OUTA, 0xb0);
        writeI2cReg(i2cFile, REG_OUTB, 0x8e);
        break;
    }
}

void Seg_writeNumber(int i2cFileDesc, int num)
{
    int tens;
    int ones;
    if (num > 99)
    {
        tens = 9;
        ones = 9;
    }
    else
    {
        tens = num / 10; // get first digit of number
        ones = num % 10; // get second digit of number
    }

    long seconds = 0;
    long nanoseconds = 10000000;
    struct timespec reqDelay = {seconds, nanoseconds};
    // turn both numbers of display off
    changeState(LEFT_DIGIT, "0");
    changeState(RIGHT_DIGIT, "0");

    // write to left and turn on, then sleep
    writeDigit(i2cFileDesc, tens);
    changeState(LEFT_DIGIT, "1");

    nanosleep(&reqDelay, (struct timespec *)NULL);
    //turn off left
    changeState(LEFT_DIGIT, "0");

    // write to right and turn on, then sleep
    writeDigit(i2cFileDesc, ones);
    changeState(RIGHT_DIGIT, "1");

    nanosleep(&reqDelay, (struct timespec *)NULL);
}