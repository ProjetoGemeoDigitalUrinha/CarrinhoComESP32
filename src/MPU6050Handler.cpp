#include "MPU6050Handler.h"
#include <Wire.h>
#include <Arduino.h>
#include "main_globals.h"

namespace MPU6050Handler {
    void setupMPU6050s() {
        Wire.begin();
        Serial.println("Initializing MPU6050 sensors...");
        if (!mpu1.begin()) {
            Serial.println("Failed to find MPU6050 #1. Check connections!");
            while (1) delay(10);
        }
        mpu1.setAccelerometerRange(MPU6050_RANGE_8_G); mpu1.setGyroRange(MPU6050_RANGE_500_DEG); mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("MPU6050 #1 connected and configured.");
        if (!mpu2.begin((uint8_t)105U)) {
            Serial.println("Failed to find MPU6050 #2. Check connections and AD0 jumper!");
            while (1) delay(10);
        }
        mpu2.setAccelerometerRange(MPU6050_RANGE_8_G); mpu2.setGyroRange(MPU6050_RANGE_500_DEG); mpu2.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("MPU6050 #2 connected and configured.");
    }
}