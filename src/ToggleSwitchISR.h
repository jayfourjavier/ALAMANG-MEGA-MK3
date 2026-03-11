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

    // Feedback variables
    uint8_t _feedbackPin;
    unsigned long feedbackTimeout;
    const uint16_t feedbackDuration = 1000; // 1 second as requested

    void (*onCallback)();
    void (*offCallback)();

    static ToggleSwitchISR *instances[4];
    static uint8_t instanceCount;

    static void isrWrapper()
    {
        for (uint8_t i = 0; i < instanceCount; i++)
        {
            instances[i]->changedFlag = true;
        }
    }

    void triggerFeedback()
    {
        digitalWrite(_feedbackPin, HIGH);
        feedbackTimeout = millis() + feedbackDuration;
    }

public:
    ToggleSwitchISR(uint8_t p, uint16_t debounce = 50)
        : pin(p),
          changedFlag(false),
          currentState(false),
          lastInterrupt(0),
          debounceTime(debounce),
          _feedbackPin(LED_BUILTIN), // Default feedback
          feedbackTimeout(0),
          onCallback(nullptr),
          offCallback(nullptr)
    {
        if (instanceCount < 4)
        {
            instances[instanceCount++] = this;
        }
    }

    void setFeedbackPin(uint8_t pin)
    {
        digitalWrite(_feedbackPin, LOW); // Clean up old pin
        _feedbackPin = pin;
        pinMode(_feedbackPin, OUTPUT);
        digitalWrite(_feedbackPin, LOW);
    }

    void begin(void (*onCb)(), void (*offCb)())
    {
        onCallback = onCb;
        offCallback = offCb;

        pinMode(pin, INPUT);
        pinMode(_feedbackPin, OUTPUT); // Ensure feedback pin is output
        currentState = digitalRead(pin);

        attachInterrupt(digitalPinToInterrupt(pin), isrWrapper, CHANGE);
    }

    void listen()
    {
        // 1. Handle non-blocking feedback timer
        if (feedbackTimeout > 0 && millis() >= feedbackTimeout)
        {
            digitalWrite(_feedbackPin, LOW);
            feedbackTimeout = 0;
        }

        // 2. Handle Switch logic
        if (!changedFlag)
            return;

        unsigned long now = millis();
        if (now - lastInterrupt >= debounceTime)
        {
            bool latestRead = digitalRead(pin);

            if (latestRead != currentState)
            {
                currentState = latestRead;
                triggerFeedback(); // Trigger 1s pulse on state change

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
            changedFlag = false;
        }
    }

    bool getState() { return currentState; }
};

ToggleSwitchISR *ToggleSwitchISR::instances[4] = {nullptr};
uint8_t ToggleSwitchISR::instanceCount = 0;
