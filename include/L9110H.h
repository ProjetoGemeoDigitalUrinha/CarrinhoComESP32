#ifndef L9110H_H

#define L9110H_H

#include <Arduino.h>

class L9110H {
    
    public:
      L9110H();

      // Coloca as entradas da ponte H em LOW (motores parados). Chamar no início do setup().
      void begin();

      void motor(int motorNumber, String instruction, int speed);

      void forward(int i, int seconds, bool hasStop);

      void backward(int i, int seconds, bool hasStop);

      void right(int i, int seconds, bool hasStop);

      void left(int i, int seconds, bool hasStop);

      void stop();

    private:

    int pinA = -1;
    int pinB = -1;
};

#endif