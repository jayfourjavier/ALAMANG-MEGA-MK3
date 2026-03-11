#pragma once
#include <Arduino.h>

class ToggleSwitchISR
{
private:
    uint8_t pin;
    volatile bool changedFlag;
    bool currentState;
    unsigned long lastInterrupt;
    uint16_t debounceTime;

    void (*onCallback)();
    void (*offCallback)();

    // Support for multiple instances (e.g., Heater and a Safety Switch)
    static ToggleSwitchISR *instances[4];
    static uint8_t instanceCount;

    // The shared ISR wrapper
    static void isrWrapper()
    {
        for (uint8_t i = 0; i < instanceCount; i++)
        {
            instances[i]->changedFlag = true;
        }
    }

public:
    ToggleSwitchISR(uint8_t p, uint16_t debounce = 50)
        : pin(p),
          changedFlag(false),
          currentState(false),
          lastInterrupt(0),
          debounceTime(debounce),
          onCallback(nullptr),
          offCallback(nullptr)
    {
        if (instanceCount < 4)
        {
            instances[instanceCount++] = this;
        }
    }

    void begin(void (*onCb)(), void (*offCb)())
    {
        onCallback = onCb;
        offCallback = offCb;

        pinMode(pin, INPUT); // Assumes external pull-down
        currentState = digitalRead(pin);

        attachInterrupt(digitalPinToInterrupt(pin), isrWrapper, CHANGE);
    }

    void listen()
    {
        // If the ISR hasn't flagged a change, do nothing
        if (!changedFlag)
            return;

        unsigned long now = millis();
        // Debounce: Only process if enough time has passed since the last change
        if (now - lastInterrupt >= debounceTime)
        {
            bool latestRead = digitalRead(pin);

            // If the state has actually settled into a different value
            if (latestRead != currentState)
            {
                currentState = latestRead;

                if (currentState && onCallback)
                {
                    onCallback();
                }
                else if (!currentState && offCallback)
                {
                    offCallback();
                }
            }
            lastInterrupt = now;
            changedFlag = false; // Reset flag after processing
        }
    }

    bool getState() { return currentState; }
};

// Initialize static members
ToggleSwitchISR *ToggleSwitchISR::instances[4] = {nullptr};
uint8_t ToggleSwitchISR::instanceCount = 0;