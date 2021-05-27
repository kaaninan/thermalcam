#ifndef K210_H
#define K210_H

#include "Arduino.h"
#include <STM32FreeRTOS.h>

#define BAUDRATE 115200

class K210
{
private:

    HardwareSerial *serial;

    unsigned long lastIncomingDataMillis = 0;
    unsigned long lastMillis = 0;
    int msgCount = 0;

public:

    bool isBegin = false;

    // Just For Info
    int resolutionX = 320;
    int resolutionY = 240;

    typedef enum {
        status,
        x,
        y,
        w,
        h,
    }dataType;

    typedef enum {
        noFace,
        singleFace,
        multipleFace,
    }statusCode;

    K210(int, int);

    bool debug = false;
    int dataFrequency = -1;

    // [0] -> Status
    // [0] -> X
    // [0] -> Y
    // [0] -> W
    // [0] -> H
    long data[5] = {0, 0, 0, 0, 0};

    bool begin();
    void conf(double);
    void start();
    friend void readTaskK210(void*);
    String getValue(String, char, int);

};
 
#endif

