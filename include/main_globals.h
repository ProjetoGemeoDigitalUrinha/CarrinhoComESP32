#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include "L9110H.h"

// --- ENCODERS ---
extern volatile long encoderDirCount;
extern volatile long encoderEsqCount;
extern float rpmDir, rpmEsq;

// --- MPU 1 ---
extern double vReal1[];
extern double vImag1[];
extern float currentVibrationMagnitude1;
extern float dominantVibrationFrequency1;
extern float dominantVibrationAmplitude1;
extern unsigned long lastSampleTime1;
extern uint16_t sampleCounter1;
extern float accelerationX_1, accelerationY_1, accelerationZ_1;
extern float rotationX_1, rotationY_1, rotationZ_1;
extern float orientationX_1, orientationY_1, orientationZ_1;

// --- MPU 2 ---
extern double vReal2[];
extern double vImag2[];
extern float currentVibrationMagnitude2;
extern float dominantVibrationFrequency2;
extern float dominantVibrationAmplitude2;
extern unsigned long lastSampleTime2;
extern uint16_t sampleCounter2;
extern float accelerationX_2, accelerationY_2, accelerationZ_2;
extern float rotationX_2, rotationY_2, rotationZ_2;
extern float orientationX_2, orientationY_2, orientationZ_2;

// --- ULTRASSÔNICO ---
extern float distance_cm;
extern unsigned long last_ultrasonic_read_time;

// --- CONSTANTES ---
extern const float PITCH_MULTIPLIER_1, ROLL_MULTIPLIER_1, YAW_MULTIPLIER_1, PITCH_OFFSET_1, ROLL_OFFSET_1, YAW_OFFSET_1;
extern const float PITCH_MULTIPLIER_2, ROLL_MULTIPLIER_2, YAW_MULTIPLIER_2, PITCH_OFFSET_2, ROLL_OFFSET_2, YAW_OFFSET_2;
extern const uint16_t SAMPLES;
extern const double SAMPLING_FREQ;

// --- OBJETOS GLOBAIS ---
extern Adafruit_MPU6050 mpu1;
extern Adafruit_MPU6050 mpu2;
extern L9110H robot;

// --- MQTT ---
extern const char* mqtt_server;
extern const int mqtt_port;
extern const char* mqtt_user;
extern const char* mqtt_password;
extern const char* topic_status;
extern const char* topic_sensors;
extern const char* topic_command;

// --- Network ---
extern const char* ssid;
extern const char* password;
extern const char* mdns_hostname;
extern const uint16_t mdns_port;
extern const char* mdns_service;
extern const char* mdns_proto;
extern const char* softap_ssid;
extern const char* softap_password;
#endif