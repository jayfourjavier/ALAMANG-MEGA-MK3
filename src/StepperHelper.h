#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class StepperHelper
{
private:
    AccelStepper stepper;

    uint8_t dirPin;
    uint8_t pulPin;

    uint16_t microstep;
    uint16_t stepsPerRev;

    long targetSteps;

public:
    // Constructor
    inline StepperHelper(uint8_t dir, uint8_t pul, uint16_t micro = 1, uint16_t baseSteps = 200)
        : stepper(AccelStepper::DRIVER, pul, dir),
          dirPin(dir),
          pulPin(pul),
          microstep(micro),
          stepsPerRev(baseSteps),
          targetSteps(0)
    {
    }

    inline void begin(float maxSpeed = 1000, float accel = 500)
    {
        stepper.setMaxSpeed(maxSpeed);
        stepper.setAcceleration(accel);
        stepper.setMinPulseWidth(5);
    }

    // compute real steps per revolution
    inline long getStepsPerRev()
    {
        return (long)stepsPerRev * microstep;
    }

    // rotate given revolutions (non-blocking)
    inline void rotate(float rev)
    {
        long steps = rev * getStepsPerRev();
        targetSteps = stepper.currentPosition() + steps;
        stepper.moveTo(targetSteps);
    }

    // rotate degrees
    inline void rotateDeg(float deg)
    {
        rotate(deg / 360.0);
    }

    // call inside loop()
    inline void run()
    {
        stepper.run();
    }

    inline bool isRunning()
    {
        return stepper.distanceToGo() != 0;
    }

    inline void stop()
    {
        stepper.stop();
    }

    inline void setSpeed(float speed)
    {
        stepper.setMaxSpeed(speed);
    }

    inline void setAcceleration(float accel)
    {
        stepper.setAcceleration(accel);
    }

    inline long position()
    {
        return stepper.currentPosition();
    }

    inline void resetPosition()
    {
        stepper.setCurrentPosition(0);
    }
};