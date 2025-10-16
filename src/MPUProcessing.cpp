#include "MPUProcessing.h"
#include "main_globals.h"
#include <QMC5883LCompass.h>
#include <math.h>
#include "FFTHandler.h"

QMC5883LCompass compass; // É QMC5883L NA VERDADE. VERSÃO CHINESA FALSIFICADA DO HMC5883L. PASSARAM A PERNA

void processMPU1() {
    if (micros() - lastSampleTime1 >= (1000000 / SAMPLING_FREQ)) {
        lastSampleTime1 = micros();
        sensors_event_t a1, g1, temp1;
        mpu1.getEvent(&a1, &g1, &temp1);
        accelerationX_1 = a1.acceleration.x;
        accelerationY_1 = a1.acceleration.y;
        accelerationZ_1 = a1.acceleration.z;
        rotationX_1 = g1.gyro.x;
        rotationY_1 = g1.gyro.y;
        rotationZ_1 = g1.gyro.z;
        float raw_pitch_1 = atan2(-accelerationX_1, sqrt(accelerationY_1 * accelerationY_1 + accelerationZ_1 * accelerationZ_1)) * 180.0 / PI;
        float raw_roll_1  = atan2(accelerationY_1, accelerationZ_1) * 180.0 / PI;
        orientationX_1 = (raw_pitch_1 * PITCH_MULTIPLIER_1) + PITCH_OFFSET_1;
        orientationY_1 = (raw_roll_1 * ROLL_MULTIPLIER_1) + ROLL_OFFSET_1;
        static bool compassInitialized = false;
        static float fusedYaw1 = 0.0f;
        static unsigned long lastYawTime1 = 0;
        static bool yawOffsetCalibrated = false;
        static float yawOffset = 0.0f;
        if (!compassInitialized) {
            Wire.begin();
            compass.init();
            compass.setCalibration(-418, 1222, 1026, 2838, -76, 1337);
            compassInitialized = true;
        }
        unsigned long now1 = micros();
        float dt1 = (lastYawTime1 == 0) ? 0.0f : (now1 - lastYawTime1) / 1e6f;
        lastYawTime1 = now1;
        compass.read();
        float magYaw1 = compass.getAzimuth();  // (0-360)
        if (!yawOffsetCalibrated && !isnan(magYaw1)) {
            yawOffset = magYaw1;
            yawOffsetCalibrated = true;
        }
        if (dt1 > 0 && fabs(rotationZ_1) < 100.0f) {
            fusedYaw1 += rotationZ_1 * dt1 * 180.0f / PI;
        }
        if (!isnan(magYaw1)) {
            const float alpha = 0.98f;
            fusedYaw1 = alpha * fusedYaw1 + (1.0f - alpha) * magYaw1;
        }
        float correctedYaw = fusedYaw1 - yawOffset;
        while (correctedYaw < 0.0f) correctedYaw += 360.0f;
        while (correctedYaw >= 360.0f) correctedYaw -= 360.0f;
        orientationZ_1 = (correctedYaw * YAW_MULTIPLIER_1) + YAW_OFFSET_1;
        currentVibrationMagnitude1 = sqrt(pow(a1.acceleration.x, 2) + pow(a1.acceleration.y, 2) + pow(a1.acceleration.z, 2));
        vReal1[sampleCounter1] = currentVibrationMagnitude1;
        vImag1[sampleCounter1] = 0.0;
        sampleCounter1++;
        if (sampleCounter1 >= SAMPLES) {
            FFTHandler::processFFT(vReal1, vImag1, currentVibrationMagnitude1, dominantVibrationFrequency1, dominantVibrationAmplitude1);
            sampleCounter1 = 0;
        }
    }
}


void processMPU2() {
    if (micros() - lastSampleTime2 >= (1000000 / SAMPLING_FREQ)) {
        lastSampleTime2 = micros();
        sensors_event_t a2, g2, temp2;
        mpu2.getEvent(&a2, &g2, &temp2);
        accelerationX_2 = a2.acceleration.x;
        accelerationY_2 = a2.acceleration.y;
        accelerationZ_2 = a2.acceleration.z;
        rotationX_2 = g2.gyro.x;
        rotationY_2 = g2.gyro.y;
        rotationZ_2 = g2.gyro.z;
        float raw_pitch_2 = atan2(-accelerationX_2, sqrt(accelerationY_2 * accelerationY_2 + accelerationZ_2 * accelerationZ_2)) * 180.0 / PI;
        float raw_roll_2  = atan2(accelerationY_2, accelerationZ_2) * 180.0 / PI;
        orientationX_2 = (raw_pitch_2 * PITCH_MULTIPLIER_2) + PITCH_OFFSET_2;
        orientationY_2 = (raw_roll_2 * ROLL_MULTIPLIER_2) + ROLL_OFFSET_2;
        float raw_yaw_2 = rotationZ_2 * (180.0f / PI);
        orientationZ_2 = (raw_yaw_2 * YAW_MULTIPLIER_2) + YAW_OFFSET_2;
        currentVibrationMagnitude2 = sqrt(pow(a2.acceleration.x, 2) + pow(a2.acceleration.y, 2) + pow(a2.acceleration.z, 2));
        vReal2[sampleCounter2] = currentVibrationMagnitude2;
        vImag2[sampleCounter2] = 0.0;
        sampleCounter2++;
        if (sampleCounter2 >= SAMPLES) {
            FFTHandler::processFFT(vReal2, vImag2, currentVibrationMagnitude2, dominantVibrationFrequency2, dominantVibrationAmplitude2);
            sampleCounter2 = 0;
        }
    }
}