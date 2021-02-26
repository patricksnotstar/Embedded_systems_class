#ifndef _POTHEAD_H_
#define _POTHEAD_H_

// Module for interacting with the BBG potentiometer

// function to get current reading
int Pothead_getVoltage0Reading();

// function to calculate array size based on voltage, using PWL
int Pothead_calcArraySize(int A2Dinput);

#endif