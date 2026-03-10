#include <Arduino.h>
#include <HX711.h>
#include <DHT.h>
#include <AccelStepper.h>
#include <lcd.h>
#include <defines.h>
#include <secrets.h>
#include <ToggleButton.h>
#include <ToggleSwitchISR.h>

// try to test pull requests

// #define RATIO (SALT / ALAMANG)  // COMPUTE RATIO BASED ON ALAMANG AND SALT WEIGHTS FROM TRIAL

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
DHT dht(DHT_PIN, DHT_TYPE);

AccelStepper shrimpStepper(1, STEPPER_SHRIMP_PUL_PIN, STEPPER_SHRIMP_DIR_PIN);
AccelStepper saltStepper(1, STEPPER_SALT_PUL_PIN, STEPPER_SALT_DIR_PIN);
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
  float sum = 0;
  byte count = 0;

  for (byte i = 0; i < validEntries; i++)
  {
    float h = dht.readHumidity();
    if (!isnan(h))
    {
      sum += h;
      count++;
    }
    delay(200);
  }

  if (count == 0)
    return NAN;

  return sum / count;
}
float readTemperature(byte validEntries = 5)
{
  float sum = 0;
  byte count = 0;

  for (byte i = 0; i < validEntries; i++)
  {
    float t = dht.readTemperature();
    if (!isnan(t))
    {
      sum += t;
      count++;
    }
    delay(200);
  }

  if (count == 0)
    return NAN;

  return sum / count;
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

  shrimpStepper.setSpeed(1000);       // set speed for shrimp stepper
  saltStepper.setSpeed(1000);         // set speed for salt stepper
  shrimpStepper.setAcceleration(500); // set acceleration for shrimp stepper
  saltStepper.setAcceleration(500);   // set acceleration for salt stepper
  saltStepper.setCurrentPosition(0);
  shrimpStepper.setCurrentPosition(0);
  saltStepper.moveTo(1000);   // example target
  shrimpStepper.moveTo(1000); // example target

  pushButton.begin(buttonOn, buttonOff);
  heaterSwitch.begin(turnOnHeater, turnOffHeater);

  dht.begin();

  // HX711 must always start
  myScale.begin(SCALE_DAT_PIN, SCALE_CLK_PIN);

#if !TO_CALIBRATE
  myScale.set_offset(SCALE_OFFSET);
  myScale.set_scale(SCALE_CALIBRATION_FACTOR);
#endif

  lcd.begin();
  lcd.welcome();
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

  // // 3️⃣ Other non-blocking tasks
  // CurrentWeight = readScale();
  // Temperature = readTemperature();
  // Humidity = readHumidity();

  // Serial.print("Weight: ");
  // Serial.print(CurrentWeight, 2);
  // Serial.print(" g\t");
  // Serial.print("Temp: ");
  // Serial.print(Temperature, 2);
  // Serial.print(" C\t");
  // Serial.print("Humidity: ");
  // Serial.println(Humidity, 2);
}