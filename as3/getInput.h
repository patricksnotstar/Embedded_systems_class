// This module handles getting all inputs from the Zen Cape. Assumes all pins are exported
#ifndef _GETINPUT_H_
#define _GETINPUT_H_

#define UP "/sys/class/gpio/gpio26/value"
#define DOWN "/sys/class/gpio/gpio46/value"
#define LEFT "/sys/class/gpio/gpio65/value"
#define RIGHT "/sys/class/gpio/gpio47/value"
#define MIDDLE "/sys/class/gpio/gpio27/value"

enum joystickDirections
{
    NEUTRAL,
    AUF,
    NIEDER,
    LINKS,
    RECHTS,
    CENTER
}; // Just Up, Down, Left, Right, Center in German. English words are used and since I'm learning German, I used those

int GetInput_getJoyStickInput();
#endif