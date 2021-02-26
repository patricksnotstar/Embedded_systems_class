// Module for controlling 14-seg display
#ifndef _HANDLE14SEGDISPLAY_H_
#define _HANDLE14SEGDISPLAY_H_

// startup and shutdown functions
int configureI2C();
void shutDownI2C(int i2cFileDesc);
// function to write number to display
void Seg_writeNumber(int i2cFileDesc, long long num);

#endif