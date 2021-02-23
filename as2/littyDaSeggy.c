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

// Assume pins already configured for I2C:
//   (bbg)$ config-pin P9_18 i2c
//   (bbg)$ config-pin P9_17 i2c

#define I2C_DEVICE_ADDRESS 0x20
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15

#define LEFT_DIGIT "/sys/class/gpio/gpio61/value"
#define RIGHT_DIGIT "/sys/class/gpio/gpio44/value"

// turns the right digit on or off
static void changeState(char *digit, char *state)
{
    FILE *f = fopen(digit, "w");
    if (f == NULL)
    {
        printf("ERROR: Unable to open %s.\n", digit);
        exit(-1);
    }
    int charWritten = fprintf(f, state);
    if (charWritten <= 0)
    {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(f);
}

int main()
{
    // printf("Drive display (assumes GPIO #61 and #44 are output and 1\n");
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);
    // Drive an hour-glass looking character
    // (Like an X with a bar on top & bottom)
    // writeI2cReg(i2cFileDesc, REG_OUTA, 0x2A);
    // writeI2cReg(i2cFileDesc, REG_OUTB, 0x54);

    while (true)
    {
        changeState(LEFT_DIGIT, "0");
        changeState(RIGHT_DIGIT, "1");
        writeNumber(i2cFileDesc, 9);
        changeState(RIGHT_DIGIT, "0");
        changeState(LEFT_DIGIT, "1");
        writeNumber(i2cFileDesc, 6);
    }

    // Read a register:
    unsigned char regVal = readI2cReg(i2cFileDesc, REG_OUTA);
    printf("Reg OUT-A = 0x%02x\n", regVal);
    // Cleanup I2C access;
    close(i2cFileDesc);
    return 0;
}

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

static void writeNumber(int i2cFile, int num)
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