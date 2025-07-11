#include "main_globals.h"
#include <Adafruit_MPU6050.h>

// --- ENCODERS ---
volatile long encoderDirCount = 0;
volatile long encoderEsqCount = 0;
float rpmDir = 0.0;
float rpmEsq = 0.0;

// --- CONSTANTES ---
const float PITCH_MULTIPLIER_1 = 1.0;
const float ROLL_MULTIPLIER_1  = 1.0;
const float YAW_MULTIPLIER_1   = 1.0;
const float PITCH_OFFSET_1 = -3.6;
const float ROLL_OFFSET_1  = 1.3;
const float YAW_OFFSET_1   = 0.0;
const float PITCH_MULTIPLIER_2 = -1.0;
const float ROLL_MULTIPLIER_2  = -1.0;
const float YAW_MULTIPLIER_2   = -1.0;
const float PITCH_OFFSET_2 = 1.0;
const float ROLL_OFFSET_2  = -0.8;
const float YAW_OFFSET_2   = 0.0;
const uint16_t SAMPLES = 64;
const double SAMPLING_FREQ = 100.0;

// --- MPU 1 ---
double vReal1[SAMPLES];
double vImag1[SAMPLES];
float currentVibrationMagnitude1 = 0.0;
float dominantVibrationFrequency1 = 0.0;
float dominantVibrationAmplitude1 = 0.0;
unsigned long lastSampleTime1 = 0;
uint16_t sampleCounter1 = 0;
float accelerationX_1 = 0, accelerationY_1 = 0, accelerationZ_1 = 0;
float rotationX_1 = 0, rotationY_1 = 0, rotationZ_1 = 0;
float orientationX_1 = 0, orientationY_1 = 0, orientationZ_1 = 0;

// --- MPU 2 ---
double vReal2[SAMPLES];
double vImag2[SAMPLES];
float currentVibrationMagnitude2 = 0.0;
float dominantVibrationFrequency2 = 0.0;
float dominantVibrationAmplitude2 = 0.0;
unsigned long lastSampleTime2 = 0;
uint16_t sampleCounter2 = 0;
float accelerationX_2 = 0, accelerationY_2 = 0, accelerationZ_2 = 0;
float rotationX_2 = 0, rotationY_2 = 0, rotationZ_2 = 0;
float orientationX_2 = 0, orientationY_2 = 0, orientationZ_2 = 0;

// --- ULTRASSÔNICO ---
float distance_cm = 0.0;
unsigned long last_ultrasonic_read_time = 0;

// --- OBJETOS GLOBAIS ---
Adafruit_MPU6050 mpu1;
Adafruit_MPU6050 mpu2;