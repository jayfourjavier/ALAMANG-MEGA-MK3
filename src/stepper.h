#pragma once
#include <Arduino.h>

class StepperTB6600 {
private:
    uint8_t pulPin;
    uint8_t dirPin;
    uint32_t stepDelay; // microseconds

public:

    inline StepperTB6600(uint8_t pul, uint8_t dir)
        : pulPin(pul), dirPin(dir), stepDelay(500) {}

    inline void begin() {
        pinMode(pulPin, OUTPUT);
        pinMode(dirPin, OUTPUT);
        digitalWrite(pulPin, LOW);
    }

    inline void setSpeedRPM(float rpm, uint16_t stepsPerRev = 200) {
        float stepsPerMin = rpm * stepsPerRev;
        float stepsPerSec = stepsPerMin / 60.0;
        stepDelay = 1000000.0 / stepsPerSec;
    }

    inline void setDirection(bool dir) {
        digitalWrite(dirPin, dir);
    }

    inline void step() {
        digitalWrite(pulPin, HIGH);
        delayMicroseconds(2);
        digitalWrite(pulPin, LOW);
        delayMicroseconds(stepDelay);
    }

    inline void move(long steps) {
        for(long i = 0; i < steps; i++) {
            step();
        }
    }
};