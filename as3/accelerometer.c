#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "helper.h"
#include "audioMixer.h"
#include "beatbox.h"

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x1C

#define REG_XMSB 0x01
#define REG_XLSB 0x02
#define REG_YMSB 0x03
#define REG_YLSB 0x04
#define REG_ZMSB 0x05
#define REG_ZLSB 0x06
#define CTRL_REG1 0x2A // Data Rate, ACTIVE Mode

#define THRESHOLD 1.0
int i2cFileDesc;
wavedata_t *sounds[3];
int initI2cBus(char *bus, int address);
void *readG_thread();
bool accelManager_xMoved();
bool accelManager_yMoved();
bool accelManager_zMoved();

enum drums
{
    _HIHAT_,
    _SNARE_,
    _BASE_,
    _NEUTRAL_
};

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void processAccelInput(int accelInput);
static int getAxis();

void Accel_initAccelerometer()
{
    i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    // set to active mode
    writeI2cReg(i2cFileDesc, CTRL_REG1, 0x01);

    pthread_t tid;
    pthread_create(&tid, NULL, &readG_thread, NULL);
}

void *readG_thread()
{
    int accelInput;
    while (true)
    {
        Helper_sleep_thread(0, 10000000);
        accelInput = getAxis();
        processAccelInput(accelInput);
    }
}

static void processAccelInput(int accelInput)
{
    switch (accelInput)
    {
    case _HIHAT_:
        AudioMixer_queueSound(hihat);
        Helper_sleep_thread(0, 300000000);
        break;
    case _SNARE_:
        AudioMixer_queueSound(snare);
        Helper_sleep_thread(0, 300000000);
        break;
    case _BASE_:
        AudioMixer_queueSound(base);
        Helper_sleep_thread(0, 300000000);
        break;
    default:
        break;
    }
}

static int getAxis()
{
    float prevReadings[3] = {0, 0, 0};
    char buff[7];

    // read 7 registers of data, 0x00 to 0x06
    if (read(i2cFileDesc, buff, 7) != 7)
    {
        printf("Error reading accelerometer data\n");
    }
    float currentReadings[3] = {0, 0, 0};

    int16_t x = (buff[REG_XMSB] << 8) | (buff[REG_XLSB]);
    x >>= 4;
    currentReadings[0] = x / 1024.0;
    int16_t y = (buff[REG_YMSB] << 8) | (buff[REG_YLSB]);
    y >>= 4;
    currentReadings[1] = y / 1024.0;
    int16_t z = (buff[REG_ZMSB] << 8) | (buff[REG_ZLSB]);
    z >>= 4;
    currentReadings[2] = (z / 1024.0) - 1;

    if ((abs(currentReadings[0] - prevReadings[0]) >= 1.0))
    {
        printf("Acceleration in X-Axis : %f \n", currentReadings[0]);
        return _HIHAT_;
    }

    if ((abs(currentReadings[1] - prevReadings[1]) >= 1.0))
    {
        printf("Acceleration in Y-Axis : %f \n", currentReadings[1]);
        return _SNARE_;
    }
    if ((abs(currentReadings[2] - prevReadings[2]) >= 1.0))
    {
        printf("Acceleration in Z-Axis : %f \n", currentReadings[2]);
        return _BASE_;
    }
    prevReadings[0] = currentReadings[0];
    prevReadings[1] = currentReadings[1];
    prevReadings[2] = currentReadings[2];

    return _NEUTRAL_;
}

int initI2cBus(char *bus, int address)
{
    // Create i2c bus
    int i2cFileDesc = open(bus, O_RDWR);
    if (i2cFileDesc < 0)
    {
        printf("I2C: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is");
        exit(1);
    }

    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0)
    {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }

    return i2cFileDesc;
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