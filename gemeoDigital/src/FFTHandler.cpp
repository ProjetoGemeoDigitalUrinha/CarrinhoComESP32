#include "FFTHandler.h"
#include "main_globals.h"
#include <arduinoFFT.h>
#include <Arduino.h>

static ArduinoFFT<double> FFT = ArduinoFFT<double>();

namespace FFTHandler {
    void processFFT(double* vReal, double* vImag, float& vibrationMagnitude, float& dominantFrequency, float& dominantAmplitude) {
        double media = 0;
        for (int i = 0; i < SAMPLES; i++) {
            media += vReal[i];
        }
        media /= SAMPLES;
        for (int i = 0; i < SAMPLES; i++) {
            vReal[i] -= media;
        }

        FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
        FFT.complexToMagnitude(vReal, vImag, SAMPLES);

        double peakAmplitude = 0.0;
        int peakIndex = 0;
        for (int i = 1; i < (SAMPLES/2); i++) {
            double currentAmplitude = vReal[i]*(2.0/SAMPLES);
            if (currentAmplitude > peakAmplitude) {
                peakAmplitude = currentAmplitude;
                peakIndex = i;
            }
        }
        dominantFrequency = (peakIndex*SAMPLING_FREQ)/SAMPLES;
        dominantAmplitude = peakAmplitude;
    }
}