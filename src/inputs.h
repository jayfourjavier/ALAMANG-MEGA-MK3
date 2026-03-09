#pragma once
#include <Arduino.h>

class Button
{

private:
    uint8_t pin;
    volatile bool pressedFlag;

    void (*callback)();

    static void IRAM_ATTR isr(void *arg)
    {
        Button *instance = static_cast<Button *>(arg);
        instance->pressedFlag = true;
    }

public:
    inline Button(uint8_t p)
        : pin(p), pressedFlag(false), callback(nullptr) {}

    inline void begin()
    {
        pinMode(pin, INPUT);
        attachInterruptArg(digitalPinToInterrupt(pin), isr, this, RISING);
    }

    inline void setCallback(void (*cb)())
    {
        callback = cb;
    }

    inline void update()
    {
        if (pressedFlag)
        {
            pressedFlag = false;

            if (callback)
            {
                callback();
            }
        }
    }
};