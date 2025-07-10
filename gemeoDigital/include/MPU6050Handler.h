#ifndef MPU6050HANDLER_H
#define MPU6050HANDLER_H

#include <Adafruit_MPU6050.h>

extern Adafruit_MPU6050 mpu1;
extern Adafruit_MPU6050 mpu2;

namespace MPU6050Handler {
    void setupMPU6050s();
}

#endif