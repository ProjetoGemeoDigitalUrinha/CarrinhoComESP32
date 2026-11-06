#ifndef PINS_H
#define PINS_H

// Pinagem do shield "Projeto Gêmeo Digital" REV 4.0 (EasyEDA, URA) para a
// placa ESP32-S3-DevKitC-1 (U12). Única fonte de verdade da pinagem do firmware.
//
//  Sinal no esquemático | GPIO | Pino do header da DevKitC-1 (U12)
//  ---------------------+------+----------------------------------
//  B1_A (L9110S)        |   4  | 4
//  B1_B (L9110S)        |   5  | 5
//  A1_B (L9110S)        |   6  | 6
//  A1_A (L9110S)        |   7  | 7
//  SDA  (I2C)           |   8  | 12
//  SCL  (I2C)           |   9  | 15
//  TRIG (HC-SR04)       |  13  | 19
//  ECHO (HC-SR04)       |  14  | 20  (via divisor 1k/2k: 5 V -> 3,3 V)
//
// Barramento I2C (SDA/SCL): MPU-1 (J2, 0x68), MPU-2 (J3, AD0 em 3V3 -> 0x69)
// e bússola U5 (footprint HMC5883L; o módulo usado é QMC5883L, 0x0D).
//
// Encoders das rodas: o shield REV 4.0 NÃO possui conexão para encoders,
// por isso nenhum pino é definido para eles (ver Encoders.cpp).

// --- Ponte H L9110S (U3) ---
// Motor 1 = canal A, Motor 2 = canal B
constexpr int PIN_MOTOR1_A = 7;  // A1_A
constexpr int PIN_MOTOR1_B = 6;  // A1_B
constexpr int PIN_MOTOR2_A = 4;  // B1_A
constexpr int PIN_MOTOR2_B = 5;  // B1_B

// --- I2C (J2 MPU-1, J3 MPU-2, U5 bússola) ---
constexpr int PIN_I2C_SDA = 8;
constexpr int PIN_I2C_SCL = 9;

// --- Ultrassônico HC-SR04 (U2) ---
constexpr int PIN_US_TRIG = 13;
constexpr int PIN_US_ECHO = 14;

#endif
