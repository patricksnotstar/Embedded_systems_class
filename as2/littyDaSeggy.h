#ifndef _LITTYDASEGGY_H_
#define _LITTYDASEGGY_H_

// Module for controlling I2C display

// startup and shutdown functions
int configureI2C();
void shutDownI2C(int i2cFileDesc);
// function to write number to display
void Seg_writeNumber(int i2cFileDesc, long long num);

#endif