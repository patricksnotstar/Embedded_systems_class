// Module for controlling 14-seg display
#ifndef _HANDLE14SEGDISPLAY_H_
#define _HANDLE14SEGDISPLAY_H_

// startup and shutdown functions
int Seg_configureI2C();
void Seg_shutDownI2C(int i2cFileDesc);
// function to write number to display
void Seg_writeNumber(int i2cFileDesc, long long num);

#endif