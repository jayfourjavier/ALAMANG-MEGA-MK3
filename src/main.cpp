#include <Arduino.h>
#include <HX711.h>
#include <DHT.h>
#include <AccelStepper.h>
#include <LcdHelper.h>
#include <defines.h>
#include <secrets.h>
#include <ToggleButton.h>
#include <ToggleSwitchISR.h>
#include <StepperHelper.h>

float Humidity = 0.0;
float Temperature = 0.0;

float TargetShrimpWeight = 1000.0; // target weight of shrimp in grams, adjust as needed
float TargetSaltWeight = 0.0;      // target weight of salt in grams, adjust as needed

bool IsWaitingToStart = true;
bool IsProcessStarted = false;
bool IsAddingSalt = false;
bool IsAddingShrimp = false;
bool IsMixing = false;
bool IsMixerOn = false;
bool IsHeating = false;
bool GotTargetSaltWeight = false;
bool IsShrimpAdded = false;
bool IsSaltAdded = false;
bool IsMixingDone = false;
bool AbortProcess = false;
unsigned long lastPrintTime = 0;
unsigned long durationMillis = (unsigned long)MIXING_DURATION_MINUTES * 60 * 1000;

float WeightCurrentValue = 0.0;
float SaltWeight = 0.0;
float ShrimpWeight = 0.0;
float WeightSampleSum = 0;
int WeightSampleCount = 0;
unsigned long WeightLastAverageTime = 0;
unsigned long MixerOnMillis = 0;
unsigned long MixerOnEllapsed = 0;

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
  IsMixerOn = true;
  Serial.println("Mixer is on.");
}

void turnOffMixer()
{
  Serial.print("Turning off mixer...\t| ");
  analogWrite(MOTOR_PWM_PIN, 0);
  Serial.println("Mixer is off.");
  IsMixerOn = false;
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

// Read humidity from both sensors and average them, handling fails
float readHumidity(byte validEntries = 5)
{
  float h1 = dht1.readHumidity();
  float h2 = dht2.readHumidity();

  bool h1Valid = !isnan(h1);
  bool h2Valid = !isnan(h2);

  // Case 1: Both are valid - return average
  if (h1Valid && h2Valid)
  {
    return (h1 + h2) / 2.0;
  }
  // Case 2: Only sensor 1 is valid
  if (h1Valid)
  {
    return h1;
  }
  // Case 3: Only sensor 2 is valid
  if (h2Valid)
  {
    return h2;
  }
  // Case 4: Both failed
  return NAN;
}

float readTemperature(byte validEntries = 5)
{
  float t1 = dht1.readTemperature();
  float t2 = dht2.readTemperature();

  bool t1Valid = !isnan(t1);
  bool t2Valid = !isnan(t2);

  if (t1Valid && t2Valid)
  {
    return (t1 + t2) / 2.0;
  }
  if (t1Valid)
  {
    return t1;
  }
  if (t2Valid)
  {
    return t2;
  }
  return NAN;
}

void buttonOn()
{
  Serial.println("START BUTTON PRESSED");
  IsWaitingToStart = false;
}

void buttonOff()
{
  Serial.println("STOP BUTTON PRESSED");
  IsWaitingToStart = true;
  AbortProcess = true;
}

void testStepper()
{
  saltStepper.rotate(1);
  shrimpStepper.rotate(1);
}

void GetWeight()
{
  if (myScale.is_ready())
  {
    float currentSample = myScale.get_units(1);            // get_units(1) automatically applies OFFSET and SCALE factor
    if (currentSample > -5000.0 && currentSample < 5000.0) // perform data sanity check
    {
      WeightSampleSum += currentSample; // Note: WeightSampleSum should be float now
      WeightSampleCount++;
    }
  }

  if (millis() - WeightLastAverageTime >= 1000)
  {
    if (WeightSampleCount > 0)
    {
      WeightCurrentValue = (float)WeightSampleSum / WeightSampleCount;
    }
    else
    {
      Serial.println("CRITICAL: Weight Sensor Not Responding!");
    }

    WeightSampleSum = 0;
    WeightSampleCount = 0;
    WeightLastAverageTime = millis();
  }
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

  if (digitalRead(HEATER_SWITCH_PIN))
  {
    Serial.println("HEATER SWITCH IS ON POSITION");
    turnOnHeater();
  }

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

  lcd.printActivity(LCDHelper::WAITING);
}

bool isShrimpEnough()
{
  return ShrimpWeight >= TargetShrimpWeight;
}

bool isSaltEnough()
{
  return SaltWeight >= TargetSaltWeight;
}

float getTargetSaltWeight()
{
  return ShrimpWeight * RATIO;
}

void shrimpController()
{

  if (IsShrimpAdded)
  {
    return;
  }

  if (!IsAddingShrimp)
  {
    return;
  }

  ShrimpWeight = WeightCurrentValue;

  if (!isShrimpEnough())
  {
    if (!shrimpStepper.isRotating())
    {
      Serial.println("Adding shrimp...");
      shrimpStepper.rotate(SHRIMP_ROTATE_PER_CYCLE); // Rotate a small amount to add shrimp, adjust as needed
    }
  }
  else
  {
    shrimpStepper.stop();
    IsAddingShrimp = false;
    IsShrimpAdded = true;
    Serial.println("Target shrimp weight reached.");
  }
}

void saltController()
{
  if (!IsShrimpAdded || IsSaltAdded)
    return;

  if (!GotTargetSaltWeight)
  {
    TargetSaltWeight = getTargetSaltWeight();
    GotTargetSaltWeight = true;
  }

  SaltWeight = WeightCurrentValue - ShrimpWeight;

  if (!isSaltEnough())
  {
    if (!saltStepper.isRotating())
    {
      saltStepper.rotate(SALT_ROTATE_PER_CYCLE);
    }
  }
  else
  {
    saltStepper.stop();
    IsAddingSalt = false;
    IsSaltAdded = true;
    Serial.println("Target salt weight reached.");
  }
}

void mixerController()
{
  if (IsMixingDone)
  {
    if (IsMixerOn)
    {
      turnOffMixer();
      IsMixing = false; // Pinapatay ang flag sa mainController
      Serial.println("MIXING PROCESS FINISHED.");
    }
    return;
  }

  if (!IsMixerOn)
  {
    turnOnMixer();
    MixerOnMillis = millis();
    Serial.println("MIXER STARTED.");
  }

  MixerOnEllapsed = millis() - MixerOnMillis;
  if (MixerOnEllapsed >= durationMillis)
  {
    IsMixingDone = true;
  }
}

bool isStepperRunning()
{
  return shrimpStepper.isRotating() || saltStepper.isRotating();
}

void stopSteppers()
{
  shrimpStepper.stop();
  saltStepper.stop();
}

void resetVariables()
{
  WeightCurrentValue = 0;
  ShrimpWeight = 0.0;
  SaltWeight = 0.0;
  TargetSaltWeight = 0.0;
  MixerOnMillis = 0;
  MixerOnEllapsed = 0;
}

void resetFlags()
{
  IsWaitingToStart = true;
  IsProcessStarted = false;
  IsAddingShrimp = false;
  IsShrimpAdded = false;
  GotTargetSaltWeight = false;
  IsAddingSalt = false;
  IsSaltAdded = false;
  IsMixing = false;
  IsMixingDone = false;
  AbortProcess = false;
}

void abortProcess()
{
  //
  resetFlags();
  resetVariables();

  if (isStepperRunning())
  {
    stopSteppers();
  }

  if (IsMixerOn)
  {
    turnOffMixer();
  }
}

void completeProcess()
{
  pushButton.setState(false);
  abortProcess();
}

void mainController()
{
  if (AbortProcess)
  {
    lcd.printActivity(LCDHelper::WAITING);
    lcd.printActivity(LCDHelper::ABORT);
    abortProcess();
    return;
  }
  if (IsWaitingToStart)
    return;

  // 1. STARTUP
  if (!IsProcessStarted)
  {
    Serial.println("PROCESS INITIATED.");
    Serial.println("ADDING SHRIMP NOW");
    lcd.printActivity(LCDHelper::ADDING_SHRIMP);
    IsProcessStarted = true;
    IsAddingShrimp = true;
    IsShrimpAdded = false;
    IsSaltAdded = false;
    return;
  }

  // 2. PHASE TRANSITIONS (The Logic Gate)
  if (IsShrimpAdded && !IsSaltAdded && !IsAddingSalt && !IsMixing)
  {
    lcd.printActivity(LCDHelper::ADDING_SALT);
    IsAddingShrimp = false;
    IsAddingSalt = true;
    Serial.println("SHRIMP IS ADDED. ADDING SALT NOW.");
  }

  if (IsSaltAdded && !IsMixingDone && !IsMixing)
  {
    lcd.printActivity(LCDHelper::MIXER_ON);
    IsAddingSalt = false;
    IsMixing = true;
    Serial.println("SHRIMP AND SALT ADDED. MIXING NOW.");
  }

  // 3. EXECUTION (The Workers)
  if (IsAddingShrimp)
    shrimpController();
  if (IsAddingSalt)
    saltController();
  if (IsMixing)
    mixerController();

  // 4. COMPLETION
  if (IsMixingDone)
  {
    lcd.printActivity(LCDHelper::DONE);
    completeProcess();
  }
}

String formatMillis(unsigned long ms)
{
  unsigned long minutes = ms / 60000;
  unsigned long seconds = (ms % 60000) / 1000;
  unsigned long mils = ms % 1000;

  char buffer[13];
  sprintf(buffer, "%02lu:%02lu:%03lu", minutes, seconds, mils);

  return String(buffer);
}

void loop()
{
#if (TO_CALIBRATE)
  calibrate();
  return;
#endif

  pushButton.listen();
  heaterSwitch.listen();
  saltStepper.run();
  shrimpStepper.run();

  GetWeight();
  mainController();

  // Only print twice a second
  if (millis() - lastPrintTime >= 500)
  {
    Temperature = readTemperature();
    Humidity = readHumidity();

    Serial.print("|| STATUS: ");
    Serial.print(IsWaitingToStart ? "WAITING" : "RUNNING");
    Serial.print(" | ADDING SHRIMP : ");
    Serial.print(IsAddingShrimp ? "YES" : "NO ");
    Serial.print(" | ADDING SALT : ");
    Serial.print(IsAddingSalt ? "YES" : "NO ");
    Serial.print(" | MIXING : ");
    Serial.print(IsMixing ? "YES " : "N0 ");
    Serial.print(" | HEATER : ");
    Serial.print(heaterSwitch.getState() ? "ON " : "OFF");
    Serial.print(" || \t|| ");

    Serial.print("TEMP: ");
    Serial.print(Temperature, 1);
    Serial.print(" C");
    Serial.print(" HUMIDITY: ");
    Serial.print(Humidity, 1);
    Serial.print(" % ");

    Serial.print("|| \t|| ");
    Serial.print("SCALE WEIGHT: ");
    Serial.print(WeightCurrentValue, 2);
    Serial.print(" g  | SHRIMP: (");
    Serial.print(ShrimpWeight, 2);
    Serial.print("/");
    Serial.print(TargetShrimpWeight, 2);
    Serial.print(")g | SALT: (");
    Serial.print(SaltWeight, 2);
    Serial.print("/");
    Serial.print(TargetSaltWeight, 2);
    Serial.print(")g ||");

    if (IsMixerOn)
    {
      Serial.print("\t|| MIXER ELLAPSED: ");
      Serial.print(formatMillis(MixerOnEllapsed));
      Serial.print(" ||");

      lcd.printOnCenter(2, "MIXER : " + formatMillis(MixerOnEllapsed));
    }

    Serial.print("\n");
    lastPrintTime = millis();

    if (IsAddingShrimp)
    {
      lcd.printOnCenter(2, "SHRIMP : " + String(ShrimpWeight));
    }
    if (IsAddingSalt)
    {
      lcd.printOnCenter(2, "SALT : " + String(SaltWeight));
    }
  }
}