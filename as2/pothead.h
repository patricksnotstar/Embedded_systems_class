// Module for interacting with the BBG potentiometer and smooth out noise with piecewise linear function
#ifndef _POTHEAD_H_
#define _POTHEAD_H_

// function to get current reading
int Pothead_getVoltage0Reading();

// function to calculate array size based on voltage, using PWL
int Pothead_calcArraySize(int A2Dinput);

#endif