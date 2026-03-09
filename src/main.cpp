#include <Arduino.h>
#include <HX711.h>
#include <DHT.h>
#include <AccelStepper.h>
#include <defines.h>
#include <secrets.h>

AccelStepper stepperShrimp(AccelStepper::DRIVER, STEPPER_SHRIMP_PUL_PIN, STEPPER_SHRIMP_DIR_PIN);
AccelStepper stepperSalt(AccelStepper::DRIVER, STEPPER_SALT_PUL_PIN, STEPPER_SALT_DIR_PIN);

void setup()
{
    Serial.begin(9600);
    stepperSalt.setMaxSpeed(1000.0);
    stepperSalt.setAcceleration(500.0);
    stepperShrimp.setMaxSpeed(1000.0);
    stepperShrimp.setAcceleration(500.0);
}

void loop()
{
    // Main control logic for the Alamang Mega MK3
    // Read sensors, control stepper motors, and manage timing for mixing
    if (stepperSalt.distanceToGo() == 0)
    {
        stepperSalt.move(10000); // Move to target position for salt
    }
    else
    {
        stepperSalt.run(); // Update salt stepper
    }

    if (stepperShrimp.distanceToGo() == 0)
    {
        stepperShrimp.move(10000); // Move to target position for shrimp
    }
    else
    {
        stepperShrimp.run(); // Update shrimp stepper
    }
}
