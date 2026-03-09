// defines.h
/*
 * DEFINES
 * Define constants and global variables for the Alamang Mega MK3 project
 */

#include <Arduino.h>

#define MIXING_DURATION_MINUTES 5 // duration to run mixer after adding shrimp and salt, adjust as needed

#define TO_CALIBRATE false // set to true to run calibration routine, false to run normal operation

#define MIXER_SPEED 20 // 0-100%

// adjust pins if needed
#define SCALE_CLK_PIN 6
#define SCALE_DAT_PIN 7
#define MOTOR_PWM_PIN 8
#define HEATER_RELAY_PIN 9
#define DHT_PIN 18
#define DHT_TYPE DHT22

#define SCALE_OFFSET 96462
#define SCALE_CALIBRATION_FACTOR 168.916992

#define ALAMANG 2000.0
#define SALT 1000.0

#define STEPPER_SHRIMP_DIR_PIN 48
#define STEPPER_SHRIMP_PUL_PIN 49
#define STEPPER_SALT_DIR_PIN 50
#define STEPPER_SALT_PUL_PIN 51

#define LCD_I2C_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4

#define BUTTON_PIN 19
#define DEBOUNCE_TIME 50
#define TOGGLE_SWITCH_PIN 18
