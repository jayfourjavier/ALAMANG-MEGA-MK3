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
    bool _wasRotating; // Internal flag to detect the moment it stops

public:
    // Constructor
    inline StepperHelper(uint8_t dir, uint8_t pul, uint16_t micro = 1, uint16_t baseSteps = 200)
        : stepper(AccelStepper::DRIVER, pul, dir),
          dirPin(dir),
          pulPin(pul),
          microstep(micro),
          stepsPerRev(baseSteps),
          targetSteps(0),
          _wasRotating(false)
    {
    }

    inline void begin(float maxSpeed = 1000, float accel = 500)
    {
        stepper.setMaxSpeed(maxSpeed);
        stepper.setAcceleration(accel);
        stepper.setMinPulseWidth(5);
    }

    inline long getStepsPerRev()
    {
        return (long)stepsPerRev * microstep;
    }

    inline void rotate(float rev)
    {
        long steps = rev * getStepsPerRev();
        targetSteps = stepper.currentPosition() + steps;
        stepper.moveTo(targetSteps);
        _wasRotating = true; // Mark that we have started a movement
    }

    inline void rotateDeg(float deg)
    {
        rotate(deg / 360.0);
    }

    inline void run()
    {
        stepper.run();
    }

    // Returns true if the motor is currently moving to a target
    inline bool isRotating()
    {
        return stepper.distanceToGo() != 0;
    }

    // Returns true ONLY at the moment the rotation finishes
    inline bool isDone()
    {
        if (_wasRotating && stepper.distanceToGo() == 0)
        {
            _wasRotating = false; // Reset the latch
            return true;
        }
        return false;
    }

    inline void stop()
    {
        stepper.stop();
        _wasRotating = false;
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
        _wasRotating = false;
    }
};