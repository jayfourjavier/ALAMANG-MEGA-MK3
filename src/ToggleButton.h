#pragma once
#include <Arduino.h>
#include <OneButton.h>

class ToggleButton
{
private:
    OneButton button;
    bool state;
    uint8_t _feedbackPin;
    unsigned long feedbackTimeout;
    const uint16_t feedbackDuration = 150;

    void (*onCallback)();
    void (*offCallback)();

    void triggerFeedback()
    {
        digitalWrite(_feedbackPin, HIGH);
        feedbackTimeout = millis() + feedbackDuration;
    }

    static void handleClick(void *scope)
    {
        ToggleButton *self = static_cast<ToggleButton *>(scope);
        if (!self->state)
        {
            self->state = true;
            self->triggerFeedback();
            if (self->onCallback)
                self->onCallback();
        }
    }

    static void handleLongPressStop(void *scope)
    {
        ToggleButton *self = static_cast<ToggleButton *>(scope);
        if (self->state)
        {
            self->state = false;
            self->triggerFeedback();
            if (self->offCallback)
                self->offCallback();
        }
    }

public:
    // Constructor initializes the default feedback pin immediately
    ToggleButton(uint8_t p, uint16_t debounce = 50)
        : button(p, false, false), state(false),
          _feedbackPin(LED_BUILTIN), feedbackTimeout(0),
          onCallback(nullptr), offCallback(nullptr)
    {
        button.setDebounceMs(debounce);
        button.setPressMs(1000);

        // Ensure the default pin (LED_BUILTIN) is ready even if setFeedbackPin is never called
        pinMode(_feedbackPin, OUTPUT);
        digitalWrite(_feedbackPin, LOW);
    }

    void setFeedbackPin(uint8_t pin)
    {
        // Turn off old pin before switching
        digitalWrite(_feedbackPin, LOW);

        _feedbackPin = pin;
        pinMode(_feedbackPin, OUTPUT);
        digitalWrite(_feedbackPin, LOW);
    }

    void begin(void (*onCb)(), void (*offCb)(), void (*isrFunc)() = nullptr)
    {
        onCallback = onCb;
        offCallback = offCb;
        button.attachClick(handleClick, this);
        button.attachLongPressStop(handleLongPressStop, this);
    }

    void listen()
    {
        button.tick();

        // Handle the pulse duration for the feedback pin
        if (feedbackTimeout > 0)
        {
            if (millis() >= feedbackTimeout)
            {
                digitalWrite(_feedbackPin, LOW);
                feedbackTimeout = 0;
            }
        }
    }

    void setState(bool s) { state = s; }
    bool getState() { return state; }
};