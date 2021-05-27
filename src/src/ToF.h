#ifndef ToF_H
#define ToF_H

#include "Arduino.h"
#include <STM32FreeRTOS.h>
#include <Wire.h>
#include <VL53L1X.h>

class ToF
{
private:
    VL53L1X vl53l1x;
    TwoWire* Wire2;

    unsigned long lastMillis = 0;
    int msgCount = 0;

public:

    bool isBegin = false;

    typedef enum {
        range,
        status,
        peakSignal,
        ambient,
    }dataType;

    bool debug = false;
    int dataFrequency = -1;

    double data[4] = {0, 0, 0, 0};

    ToF(int, int);

    bool begin();
    void start();
    void conf(double);
    void read(double*);

    friend void readTaskTOF(void*);

    // HELPER
    const char* rangeStatusToString(double);

};
 
#endif

