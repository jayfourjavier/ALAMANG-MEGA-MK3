#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class LCDHelper
{

public:
    enum Activity
    {
        WAITING,
        ADDING_SHRIMP,
        ADDING_SALT,
        MIXER_ON,
        MIXER_OFF,
        HEATER_ON,
        HEATER_OFF,
        DONE,
        ERROR
    };

private:
    LiquidCrystal_I2C lcd;
    uint8_t cols;
    uint8_t rows;

    inline String toCaps(String msg)
    {
        msg.toUpperCase();
        return msg;
    }

public:
    inline LCDHelper(uint8_t addr, uint8_t c, uint8_t r)
        : lcd(addr, c, r), cols(c), rows(r) {}

    inline void begin()
    {
        lcd.init();
        lcd.backlight();
        lcd.clear();
    }

    inline void clear()
    {
        lcd.clear();
    }

    inline void print(uint8_t row, String message)
    {

        if (row >= rows)
            return;

        message = toCaps(message);

        lcd.setCursor(0, row);
        lcd.print("                    ");
        lcd.setCursor(0, row);
        lcd.print(message);
    }

    inline void printOnCenter(uint8_t row, String message)
    {

        if (row >= rows)
            return;

        message = toCaps(message);

        int len = message.length();
        int col = (cols - len) / 2;
        if (col < 0)
            col = 0;

        lcd.setCursor(0, row);
        lcd.print("                    ");

        lcd.setCursor(col, row);
        lcd.print(message);
    }

    /*
    Print temperature and humidity values on the LCD last row
    */
    inline void printVariables(float temperature, float humidity)
    {
        lcd.setCursor(0, rows - 1);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print("C  Hum: ");
        lcd.print(humidity, 1);
        lcd.print("%");
    }

    inline void welcome()
    {

        lcd.clear();
        printOnCenter(0, "ALAMANG MK3");
        printOnCenter(1, "SYSTEM READY");
    }

    inline void printActivity(Activity activity)
    {

        switch (activity)
        {

        case WAITING:
            printOnCenter(rows - 1, "WAITING");
            break;

        case ADDING_SHRIMP:
            printOnCenter(rows - 1, "ADDING SHRIMP");
            break;

        case ADDING_SALT:
            printOnCenter(rows - 1, "ADDING SALT");
            break;

        case MIXER_ON:
            printOnCenter(rows - 1, "MIXING");
            break;

        case MIXER_OFF:
            printOnCenter(rows - 1, "PROCESS DONE");
            break;

        case HEATER_ON:
            printOnCenter(rows - 1, "HEATER IS ON");
            break;

        case HEATER_OFF:
            printOnCenter(rows - 1, "HEATER IS OFF");
            break;

        case ERROR:
            printOnCenter(rows - 1, "SYSTEM ERROR");
            break;
        }
    }
};