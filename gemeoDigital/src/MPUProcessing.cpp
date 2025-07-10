#include "MPUProcessing.h"
#include "main_globals.h"
#include <Adafruit_HMC5883_U.h>
#include <math.h>
#include "FFTHandler.h"

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
        static Adafruit_HMC5883_Unified mag1 = Adafruit_HMC5883_Unified(12345);
        static bool magInitialized1 = false;
        static float fusedYaw1 = 0.0f;
        static unsigned long lastYawTime1 = 0;
        if (!magInitialized1) {
            if (mag1.begin()) {
                magInitialized1 = true;
            }
        }
        unsigned long now1 = micros();
        float dt1 = (lastYawTime1 == 0) ? 0.0f : (now1 - lastYawTime1) / 1e6f;
        lastYawTime1 = now1;
        float magYaw1 = NAN;
        if (magInitialized1) {
            sensors_event_t mag_event1;
            mag1.getEvent(&mag_event1);
            magYaw1 = atan2(mag_event1.magnetic.y, mag_event1.magnetic.x);
            if (magYaw1 < 0) magYaw1 += 2 * PI;
            magYaw1 = magYaw1 * 180.0f / PI;
        }
        if (dt1 > 0 && fabs(rotationZ_1) < 100.0f) {
            fusedYaw1 += rotationZ_1 * dt1 * 180.0f / PI;
        }
        if (!isnan(magYaw1)) {
            const float alpha = 0.98f;
            fusedYaw1 = alpha * fusedYaw1 + (1.0f - alpha) * magYaw1;
        }
        orientationZ_1 = (fusedYaw1 * YAW_MULTIPLIER_1) + YAW_OFFSET_1;
        while (orientationZ_1 < 0.0f) orientationZ_1 += 360.0f;
        while (orientationZ_1 >= 360.0f) orientationZ_1 -= 360.0f;

        currentVibrationMagnitude1 = sqrt(pow(a1.acceleration.x, 2) + pow(a1.acceleration.y, 2) + pow(a1.acceleration.z, 2));
        vReal1[sampleCounter1] = currentVibrationMagnitude1;
        vImag1[sampleCounter1] = 0.0;

        sampleCounter1++;
        if (sampleCounter1 >= SAMPLES) {
            // Print orientações do MPU1
            Serial.print("MPU 1: Pitch: ");
            Serial.print(orientationX_1, 2);
            Serial.print(" | Roll: ");
            Serial.print(orientationY_1, 2);
            Serial.print(" | Yaw: ");
            Serial.println(orientationZ_1, 2);

            FFTHandler::processFFT(vReal1, vImag1, currentVibrationMagnitude1, dominantVibrationFrequency1, dominantVibrationAmplitude1);

            // Print vibração MPU1
            Serial.print("MPU 1: Vibração: ");
            Serial.print(currentVibrationMagnitude1, 2);
            Serial.print(" | Freq. dominante: ");
            Serial.print(dominantVibrationFrequency1, 2);
            Serial.print(" Hz | Amplitude dominante: ");
            Serial.println(dominantVibrationAmplitude1, 2);

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
            // Print orientações do MPU2
            Serial.print("MPU 2: Pitch: ");
            Serial.print(orientationX_2, 2);
            Serial.print(" | Roll: ");
            Serial.print(orientationY_2, 2);
            Serial.print(" | Yaw: ");
            Serial.println(orientationZ_2, 2);

            FFTHandler::processFFT(vReal2, vImag2, currentVibrationMagnitude2, dominantVibrationFrequency2, dominantVibrationAmplitude2);

            // Print vibração MPU2
            Serial.print("MPU 2: Vibração: ");
            Serial.print(currentVibrationMagnitude2, 2);
            Serial.print(" | Freq. dominante: ");
            Serial.print(dominantVibrationFrequency2, 2);
            Serial.print(" Hz | Amplitude dominante: ");
            Serial.println(dominantVibrationAmplitude2, 2);

            sampleCounter2 = 0;
        }
    }
}