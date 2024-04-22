#ifndef PWMController_h
#define PWMController_h
#define DEFAULT_SPEED 255
#define MAX_PWM_PINS 6

#include "Arduino.h"

typedef void (*controller_callback)(void *, int);
typedef unsigned long time_ms;

class PWMController
{
public:
    PWMController();

    bool addPin(byte pin);

    void begin();

    void setSpeed(int unitsPerSecond);

    void setTarget(byte target);
    byte getTarget();
    byte getCurrent();

    bool moveToTarget();

    bool reachedTarget();

    void jumpToTarget();

private:
    byte pwmPins[MAX_PWM_PINS];
    byte enabledPins = 0;
    float unitsPerMs;
    byte targetValue;
    byte currentValue;
    unsigned long lastUpdate;

    void writeOutput();
};

#endif