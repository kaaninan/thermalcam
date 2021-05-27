#ifndef AMG_H
#define AMG_H

#include "Arduino.h"
#include <STM32FreeRTOS.h>
#include <Wire.h>
#include <Math.h>
#include <Adafruit_AMG88xx.h>

#define AMG_COLS 8
#define AMG_ROWS 8

#define INTERPOLATED_COLS 24
#define INTERPOLATED_ROWS 24

#define PIXEL_TEMP_OFFSET 15.5 // Celcius

class AMG
{
private:

    Adafruit_AMG88xx amg;
    float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
    float pixels_adapted[AMG88xx_PIXEL_ARRAY_SIZE];

    float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
    void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);
    void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
    void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
    float cubicInterpolate(float p[], float x);
    float bicubicInterpolate(float p[], float x, float y);
    void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, float *dest, uint8_t dest_rows, uint8_t dest_cols);

    float dest_2d[INTERPOLATED_ROWS * INTERPOLATED_COLS];

    unsigned long lastMillis = 0;
    int msgCount = 0;

    int SDA_PIN;
    int SCL_PIN;

public:

    bool isBegin = false;

    typedef enum {
        status,
        thermistor,
        maxTemp,
        minTemp,
        averageTemp,
    }dataType;

    typedef enum {
        ok,
        fail,
    }statusCode;

    bool debug = false;
    int dataFrequency = -1;

    double data[4] = {0, 0, 0, 0};
    double calculatedData[4] = {0, 0, 0, 0};

    AMG(int, int);

    bool begin();
    void start();
    void conf(double);
    void read(double*);

    void printPixels();
    void printInterpolatedPixels();

    friend void readTaskAMG(void*);

    // HELPER
    void calculatePixels(long, long, long, long, long);

};
 
#endif

