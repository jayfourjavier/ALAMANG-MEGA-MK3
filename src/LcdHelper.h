#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class LCDHelper
{

public:
    enum Activity
    {
        ABORT,
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
    uint8_t activityRow = 1;

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

    inline void welcome()
    {

        lcd.clear();
        printOnCenter(0, "ALAMANG MK3");
        printOnCenter(activityRow, "SYSTEM READY");
    }

    /*
    Print temperature and humidity values on Row 2
    */
    inline void printVariables(float temperature, float humidity)
    {
        lcd.setCursor(0, 2); // Using Row 2 to avoid conflict with activityRow (1)
        lcd.print("T: ");
        lcd.print(temperature, 1);
        lcd.write(223); // Degree symbol for LCD
        lcd.print("C  H: ");
        lcd.print(humidity, 1);
        lcd.print("% ");
    }

    inline void printActivity(Activity activity)
    {
        switch (activity)
        {
        case ABORT:
            printOnCenter(activityRow, "PROCESS CANCELLED");
            printOnCenter(3, "                     ");
            break;
        case WAITING:
            printOnCenter(activityRow, "WAITING");
            printOnCenter(3, "PRESS START BUTTON");
            break;
        case ADDING_SHRIMP:
            printOnCenter(activityRow, "ADDING SHRIMP");
            printOnCenter(3, "LONG PRESS CANCEL ");
            break;
        case ADDING_SALT:
            printOnCenter(activityRow, "ADDING SALT");
            printOnCenter(3, "LONG PRESS CANCEL ");
            break;
        case MIXER_ON:
            printOnCenter(activityRow, "MIXING");
            printOnCenter(3, "LONG PRESS CANCEL ");
            break;
        case MIXER_OFF:
            printOnCenter(activityRow, "PROCESS DONE");
            printOnCenter(3, "      ---       ");
            break;
        case HEATER_ON:
            printOnCenter(activityRow, "HEATER IS ON");
            break;
        case HEATER_OFF:
            printOnCenter(activityRow, "HEATER IS OFF");
            break;
        case DONE:
            printOnCenter(activityRow, "BATCH COMPLETE");
            printOnCenter(2, "REMOVE PRODUCT");
            printOnCenter(3, "PRESS START BUTTON");
            break;
        case ERROR:
            printOnCenter(activityRow, "SYSTEM ERROR");
            break;
        default:
            Serial.println("INVALID ACTIVITY"); // Fixed missing quote
            break;
        }
    }
};