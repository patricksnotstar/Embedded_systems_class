// This module handles getting all inputs from the Zen Cape.
#ifndef _GETINPUT_H_
#define _GETINPUT_H_

#define UP "/sys/class/gpio/gpio26/value"
#define DOWN "/sys/class/gpio/gpio46/value"
#define LEFT "/sys/class/gpio/gpio65/value"
#define RIGHT "/sys/class/gpio/gpio47/value"
#define MIDDLE "/sys/class/gpio/gpio27/value"
#define EXPORT "/sys/class/gpio/export"
#define UP_DIR "/sys/class/gpio/gpio26/direction"
#define DOWN_DIR "/sys/class/gpio/gpio46/direction"
#define LEFT_DIR "/sys/class/gpio/gpio65/direction"
#define RIGHT_DIR "/sys/class/gpio/gpio47/direction"
#define MIDDLE_DIR "/sys/class/gpio/gpio27/direction"

enum joystickDirections
{
    NEUTRAL,
    AUF,
    NIEDER,
    LINKS,
    RECHTS,
    CENTER
}; // Just Up, Down, Left, Right, Center in German. English words are used and since I'm learning German, I used those

void GetInput_initJoystick();
#endif