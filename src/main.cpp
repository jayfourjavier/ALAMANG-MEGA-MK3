#include <Arduino.h>
#include <HX711.h>
#include <DHT.h>
#include <AccelStepper.h>
#include <lcd.h>
#include <defines.h>
#include <secrets.h>
#include <ToggleButton.h>
#include <ToggleSwitchISR.h>
#include <StepperHelper.h>

#define RATIO .5 // OR DEFINE EXPLICITLY IN CODE

float CurrentWeight = 0.0;
float SaltWeight = 0.0;
float ShrimpWeight = 0.0;
float Humidity = 0.0;
float Temperature = 0.0;

float TargetShrimpWeight = 1000.0; // target weight of shrimp in grams, adjust as needed

bool IsWaiting = false;
bool IsAddingSalt = false;
bool IsAddingShrimp = false;
bool IsMixing = false;
bool IsHeating = false;

HX711 myScale;
DHT dht1(DHT_PIN1, DHT_TYPE);
DHT dht2(DHT_PIN2, DHT_TYPE);

StepperHelper shrimpStepper(STEPPER_SHRIMP_DIR_PIN, STEPPER_SHRIMP_PUL_PIN, SHRIMP_MICROSTEPS, STEPS_PER_REV);
StepperHelper saltStepper(STEPPER_SALT_DIR_PIN, STEPPER_SALT_PUL_PIN, SALT_MICROSTEPS, STEPS_PER_REV);
LCDHelper lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
ToggleButton pushButton(BUTTON_PIN, DEBOUNCE_TIME);
ToggleSwitchISR heaterSwitch(HEATER_SWITCH_PIN, DEBOUNCE_TIME);

void calibrate()
{
  Serial.println("\n\nCALIBRATION\n===========");
  Serial.println("Remove all weight from the load cell.");

  while (Serial.available())
    Serial.read();
  Serial.println("Press ENTER when ready.");
  while (Serial.available() == 0)
    ;

  Serial.println("Determining zero offset...");
  myScale.tare(20);
  int32_t offset = myScale.get_offset();

  Serial.print("OFFSET: ");
  Serial.println(offset);
  Serial.println();

  while (Serial.available())
    Serial.read();
  Serial.println("Place a known weight and enter its value in grams:");

  uint32_t weight = 0;

  while (Serial.peek() != '\n')
  {
    if (Serial.available())
    {
      char ch = Serial.read();
      if (isdigit(ch))
      {
        weight = weight * 10 + (ch - '0');
      }
    }
  }

  Serial.print("WEIGHT: ");
  Serial.println(weight);

  myScale.calibrate_scale(weight, 20);
  float scale = myScale.get_scale();

  Serial.print("SCALE: ");
  Serial.println(scale, 6);

  Serial.println("\nUse these in your code:");
  Serial.print("#define SCALE_OFFSET ");
  Serial.println(offset);

  Serial.print("#define SCALE_CALIBRATION_FACTOR ");
  Serial.println(scale, 6);

  Serial.println("\nCalibration complete.");
}

float readScale()
{
  return myScale.get_units(10);
}

void turnOnMixer()
{
  Serial.print("Turning on mixer...\t| ");
  analogWrite(MOTOR_PWM_PIN, map(MIXER_SPEED, 0, 100, 0, 255));
  Serial.println("Mixer is on.");
}

void turnOffMixer()
{
  Serial.print("Turning off mixer...\t| ");
  analogWrite(MOTOR_PWM_PIN, 0);
  Serial.println("Mixer is off.");
}

void turnOnHeater()
{
  Serial.print("Turning on heater...\t| ");
  digitalWrite(HEATER_RELAY_PIN, LOW); // LOW to turn on
  Serial.println("Heater is on.");
}

void turnOffHeater()
{
  Serial.print("Turning off heater...\t| ");
  digitalWrite(HEATER_RELAY_PIN, HIGH);
  Serial.println("Heater is off.");
}

// Read humidity several times, average valid readings
float readHumidity(byte validEntries = 5)
{
  return 0.0;
}
float readTemperature(byte validEntries = 5)
{
  return 0.0;
}

void buttonOn()
{
  Serial.println("PROCESS STARTED");
  // IsWaiting = false;
}

void buttonOff()
{
  Serial.println("PROCESS ABORTED");
  // IsWaiting = true;
}

void testStepper()
{
  saltStepper.rotate(1);
  shrimpStepper.rotate(1);
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println(__FILE__);
  Serial.print("HX711_LIB_VERSION: ");
  Serial.println(HX711_LIB_VERSION);
  Serial.println();

  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, HIGH); // turn off heater

  saltStepper.begin(1000, 500);
  shrimpStepper.begin(1000, 500);

  pushButton.begin(buttonOn, buttonOff);
  heaterSwitch.begin(turnOnHeater, turnOffHeater);

  dht1.begin();
  dht2.begin();

  // HX711 must always start
  myScale.begin(SCALE_DAT_PIN, SCALE_CLK_PIN);

#if !TO_CALIBRATE
  myScale.set_offset(SCALE_OFFSET);
  myScale.set_scale(SCALE_CALIBRATION_FACTOR);
#endif

  lcd.begin();
  lcd.welcome();

  testStepper();
}

bool isShrimpEnough()
{
  return CurrentWeight >= TargetShrimpWeight;
}

bool isSaltEnough()
{
  return SaltWeight >= (TargetShrimpWeight * RATIO);
}

void addShrimp()
{
  Serial.println("Adding shrimp...");
  // Code to control hardware for adding shrimp goes here
  Serial.println("Shrimp added.");
}

void addSalt()
{
  Serial.println("Adding salt...");
  // Code to control hardware for adding salt goes here
  Serial.println("Salt added.");
}

void mainController()
{
  if (IsWaiting)
  {
    // Handle waiting state
    // UPDATE LCD TO SHOW WAITING STATUS
    // lcdPrintActivity("PLACE CONTAINER AND PRESS START");
    return;
  }
  else
  {
    // ADD SHRIMP
    if (!isShrimpEnough())
    {
      if (!IsAddingShrimp)
      {
        IsAddingShrimp = true;
        addShrimp();
        IsAddingShrimp = false;
      }
      return; // wait until shrimp is enough before proceeding
    }

    // ADD SALT
    if (!isSaltEnough())
    {
      if (!IsAddingSalt)
      {
        IsAddingSalt = true;
        addSalt();
        IsAddingSalt = false;
      }
      return; // wait until salt is enough before proceeding
    }

    if (!IsMixing && isShrimpEnough() && isSaltEnough())
    {
      IsMixing = true;
      turnOnMixer();
      // delay for mixing duration, adjust as needed
      delay(5000);
      turnOffMixer();
      IsMixing = false;
    }

    // MIX

    // HEAT
  }
}

void loop()
{
  pushButton.listen();
  heaterSwitch.listen();
  saltStepper.run();
  shrimpStepper.run();

  testStepper();

  // float temp1 = dht1.readTemperature();
  // float hum1 = dht1.readHumidity();
  // Serial.print("Temp1: ");
  // Serial.print(temp1);
  // Serial.print(" C\t");
  // Serial.print("Humidity1: ");
  // Serial.print(hum1);
  // Serial.print(" %");

  // float temp2 = dht2.readTemperature();
  // float hum2 = dht2.readHumidity();
  // Serial.print(" \t | Temp2: ");
  // Serial.print(temp2);
  // Serial.print(" C\t");
  // Serial.print("Humidity2: ");
  // Serial.print(hum2);
  // Serial.println(" %");

  // delay(1000);

  // // Other non-blocking tasks
  // CurrentWeight = readScale();
  // Temperature = readTemperature();
  // Humidity = readHumidity();

  // Serial.print("Weight: ");
  // Serial.print(CurrentWeight, 2);
  // Serial.print(" g\n");
  // Serial.print("Temp: ");
  // Serial.print(Temperature, 2);
  // Serial.print(" C\t");
  // Serial.print("Humidity: ");
  // Serial.println(Humidity, 2);
}