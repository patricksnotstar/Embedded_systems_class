#ifndef _LITTYDASEGGY_H_
#define _LITTYDASEGGY_H_

int i2cFileDesc;
void Seg_writeNumber(int i2cFileDesc, long long num);
void configureI2C(int i2cFileDesc);
void shutDownI2C(int i2cFileDesc);
#endif