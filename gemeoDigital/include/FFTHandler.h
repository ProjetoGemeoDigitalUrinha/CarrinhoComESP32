#ifndef FFTHANDLER_H
#define FFTHANDLER_H

namespace FFTHandler {
    void processFFT(double* vReal, double* vImag, float& vibrationMagnitude, float& dominantFrequency, float& dominantAmplitude);
}

#endif