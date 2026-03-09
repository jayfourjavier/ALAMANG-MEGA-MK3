#ifndef IBT2MOTOR_H
#define IBT2MOTOR_H

#include <Arduino.h>

class IBT2Motor
{

private:
    enum Mode
    {
        MODE_4WIRE,
        MODE_2WIRE,
        MODE_1WIRE
    };

    Mode _mode;

    uint8_t _rpwm = 255;
    uint8_t _lpwm = 255;
    uint8_t _ren = 255;
    uint8_t _len = 255;

#ifdef ESP32
    uint8_t _ch_r = 0;
    uint8_t _ch_l = 1;
    const uint32_t _freq = 20000;
    const uint8_t _resolution = 8;
#endif

    inline void writeR(uint8_t val)
    {
#ifdef ESP32
        ledcWrite(_ch_r, val);
#else
        analogWrite(_rpwm, val);
#endif
    }

    inline void writeL(uint8_t val)
    {
#ifdef ESP32
        ledcWrite(_ch_l, val);
#else
        analogWrite(_lpwm, val);
#endif
    }

public:
    // =========================
    // 4-WIRE CONSTRUCTOR
    // =========================
    inline IBT2Motor(uint8_t rpwm, uint8_t lpwm,
                     uint8_t ren, uint8_t len)
        : _mode(MODE_4WIRE),
          _rpwm(rpwm), _lpwm(lpwm),
          _ren(ren), _len(len)
    {
    }

    // =========================
    // 2-WIRE CONSTRUCTOR
    // EN shorted to VCC
    // =========================
    inline IBT2Motor(uint8_t rpwm, uint8_t lpwm)
        : _mode(MODE_2WIRE),
          _rpwm(rpwm), _lpwm(lpwm)
    {
    }

    // =========================
    // 1-WIRE CONSTRUCTOR
    // Single PWM only
    // =========================
    inline IBT2Motor(uint8_t pwmPin)
        : _mode(MODE_1WIRE),
          _rpwm(pwmPin)
    {
    }

    // =========================
    // INIT
    // =========================
    inline void init()
    {

        if (_mode == MODE_4WIRE)
        {
            pinMode(_ren, OUTPUT);
            pinMode(_len, OUTPUT);
            digitalWrite(_ren, HIGH);
            digitalWrite(_len, HIGH);
        }

#ifndef ESP32
        pinMode(_rpwm, OUTPUT);
        if (_mode != MODE_1WIRE)
            pinMode(_lpwm, OUTPUT);
#endif

#ifdef ESP32
        ledcSetup(_ch_r, _freq, _resolution);
        ledcAttachPin(_rpwm, _ch_r);

        if (_mode != MODE_1WIRE)
        {
            ledcSetup(_ch_l, _freq, _resolution);
            ledcAttachPin(_lpwm, _ch_l);
        }
#endif

        turnOff();
    }

    // =========================
    // ENABLE CONTROL
    // =========================
    inline void turnOn()
    {
        if (_mode == MODE_4WIRE)
        {
            digitalWrite(_ren, HIGH);
            digitalWrite(_len, HIGH);
        }
    }

    inline void turnOff()
    {
        stopRight();
        stopLeft();

        if (_mode == MODE_4WIRE)
        {
            digitalWrite(_ren, LOW);
            digitalWrite(_len, LOW);
        }
    }

    // =========================
    // RUN RIGHT CHANNEL
    // =========================
    inline void runRight(uint8_t speed = 255)
    {
        speed = constrain(speed, 0, 255);
        writeR(speed);

        if (_mode != MODE_1WIRE)
            writeL(0);
    }

    inline void stopRight()
    {
        writeR(0);
    }

    // =========================
    // RUN LEFT CHANNEL
    // =========================
    inline void runLeft(uint8_t speed = 255)
    {
        if (_mode == MODE_1WIRE)
            return; // not supported

        speed = constrain(speed, 0, 255);
        writeL(speed);
        writeR(0);
    }

    inline void stopLeft()
    {
        if (_mode == MODE_1WIRE)
            return;
        writeL(0);
    }

    // =========================
    // RAMP UP (blocking)
    // =========================
    inline void rampUp(bool rightSide = true,
                       uint8_t target = 255,
                       uint8_t step = 5,
                       uint16_t delayMs = 10)
    {

        target = constrain(target, 0, 255);

        for (uint8_t s = 0; s <= target; s += step)
        {
            if (rightSide)
                runRight(s);
            else
                runLeft(s);

            delay(delayMs);
        }
    }

    // =========================
    // RAMP DOWN (blocking)
    // =========================
    inline void rampDown(bool rightSide = true,
                         uint8_t start = 255,
                         uint8_t step = 5,
                         uint16_t delayMs = 10)
    {

        start = constrain(start, 0, 255);

        for (int s = start; s >= 0; s -= step)
        {
            if (rightSide)
                runRight(s);
            else
                runLeft(s);

            delay(delayMs);
        }

        if (rightSide)
            stopRight();
        else
            stopLeft();
    }
};

#endif