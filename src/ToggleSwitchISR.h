// ToggleSwitchISR.h

#pragma once
#include <Arduino.h>

class ToggleSwitchISR
{
private:
    uint8_t pin;
    volatile bool changedFlag;
    volatile bool currentState;
    volatile unsigned long lastInterrupt;
    uint16_t debounceTime;

    void (*onCallback)();
    void (*offCallback)();

    static ToggleSwitchISR *instance;

    static void isrWrapper()
    {
        if (!instance)
            return;

        unsigned long now = millis();

        if (now - instance->lastInterrupt < instance->debounceTime)
            return;

        instance->lastInterrupt = now;

        bool state = digitalRead(instance->pin);

        if (state != instance->currentState)
        {
            instance->currentState = state;
            instance->changedFlag = true;
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
        instance = this;
    }

    void begin(void (*onCb)(), void (*offCb)())
    {
        onCallback = onCb;
        offCallback = offCb;

        pinMode(pin, INPUT); // external pulldown
        currentState = digitalRead(pin);

        attachInterrupt(digitalPinToInterrupt(pin), isrWrapper, CHANGE);
    }

    void listen()
    {
        if (!changedFlag)
            return;

        noInterrupts();
        bool state = currentState;
        changedFlag = false;
        interrupts();

        if (state)
        {
            if (onCallback)
                onCallback();
        }
        else
        {
            if (offCallback)
                offCallback();
        }
    }

    bool getState()
    {
        return currentState;
    }
};

ToggleSwitchISR *ToggleSwitchISR::instance = nullptr;