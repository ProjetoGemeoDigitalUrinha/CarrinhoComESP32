#include "Debug.h"
#include "main_globals.h"
#include <Arduino.h>

void printDebug() {
    Serial.println("------ ENCODERS ------");
    Serial.print("RPM Direita : ");
    Serial.print(rpmDir, 2);
    Serial.print(" | RPM Esquerda : ");
    Serial.print(rpmEsq, 2);
    Serial.println(" rpm");
    Serial.println("----------------------");

    Serial.println("---- ULTRASSÔNICO ----");
    Serial.print("Distância: ");
    Serial.print(distance_cm, 2);
    Serial.println(" cm");
    Serial.println("----------------------");

    Serial.println("------ MPU 1 ------");
    Serial.print("Pitch: ");
    Serial.print(orientationX_1, 2);
    Serial.print("° | Roll: ");
    Serial.print(orientationY_1, 2);
    Serial.print("° | Yaw: ");
    Serial.print(orientationZ_1, 2);
    Serial.println("°");
    Serial.print("Vibração: ");
    Serial.print(currentVibrationMagnitude1, 2);
    Serial.print(" m/s² | Freq. dominante: ");
    Serial.print(dominantVibrationFrequency1, 2);
    Serial.print(" Hz | Amplitude dominante: ");
    Serial.print(dominantVibrationAmplitude1, 2);
    Serial.println(" m/s²");
    Serial.println("-------------------");

    Serial.println("------ MPU 2 ------");
    Serial.print("Pitch: ");
    Serial.print(orientationX_2, 2);
    Serial.print("° | Roll: ");
    Serial.print(orientationY_2, 2);
    Serial.print("° | Yaw: ");
    Serial.print(orientationZ_2, 2);
    Serial.println("°");
    Serial.print("Vibração: ");
    Serial.print(currentVibrationMagnitude2, 2);
    Serial.print(" m/s² | Freq. dominante: ");
    Serial.print(dominantVibrationFrequency2, 2);
    Serial.print(" Hz | Amplitude dominante: ");
    Serial.print(dominantVibrationAmplitude2, 2);
    Serial.println(" m/s²");
    Serial.println("-------------------\n");
}