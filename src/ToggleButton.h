#pragma once
#include <Arduino.h>

class ToggleButton
{
private:
    uint8_t pin;
    volatile bool pressedFlag;
    volatile unsigned long lastInterrupt;
    bool state;
    uint16_t debounceTime;

    void (*onCallback)();
    void (*offCallback)();

    // AVR only: static array to handle multiple buttons
    static ToggleButton *instances[6];
    static uint8_t instanceCount;

    // ISR wrapper for AVR
    static void isrWrapper()
    {
        for (uint8_t i = 0; i < instanceCount; i++)
        {
            ToggleButton *btn = instances[i];
            unsigned long now = millis();
            if (now - btn->lastInterrupt >= btn->debounceTime)
            {
                if (digitalRead(btn->pin) == HIGH)
                { // only trigger on press
                    btn->lastInterrupt = now;
                    btn->pressedFlag = true;
                }
            }
        }
    }

public:
    inline ToggleButton(uint8_t p, uint16_t debounce = 50)
        : pin(p), pressedFlag(false), lastInterrupt(0),
          state(false), debounceTime(debounce),
          onCallback(nullptr), offCallback(nullptr)
    {
        instances[instanceCount++] = this;
    }

    // begin and attach callbacks
    inline void begin(void (*onCb)(), void (*offCb)())
    {
        onCallback = onCb;
        offCallback = offCb;
        pinMode(pin, INPUT); // assume external pulldown
        attachInterrupt(digitalPinToInterrupt(pin), isrWrapper, RISING);
    }

    // call frequently in loop
    inline void listen()
    {
        if (!pressedFlag)
            return;
        pressedFlag = false;

        state = !state; // toggle on press

        if (state && onCallback)
            onCallback();
        else if (!state && offCallback)
            offCallback();
    }

    inline bool getState() { return state; }
};

// static members
ToggleButton *ToggleButton::instances[6] = {nullptr};
uint8_t ToggleButton::instanceCount = 0;