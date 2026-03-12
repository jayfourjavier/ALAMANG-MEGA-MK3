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

#define RATIO .5 // OR DEFINE EXPLICITLY IN CODE

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
bool ShrimpAdded = false;
bool SaltAdded = false;
bool AbortProcess = false;
unsigned long lastPrintTime = 0;

float WeightCurrentValue = 0.0;
float SaltWeight = 0.0;
float ShrimpWeight = 0.0;
float WeightSampleSum = 0;
int WeightSampleCount = 0;
unsigned long WeightLastAverageTime = 0;

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
  Serial.println("START BUTTON PRESSED");
  IsWaitingToStart = false;
}

void buttonOff()
{
  Serial.println("STOP BUTTON PRESSED");
  IsWaitingToStart = true;
  AbortProcess = true;
  // IsWaitingToStart = true;
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
  if (ShrimpAdded)
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
    ShrimpAdded = true;
    Serial.println("Target shrimp weight reached.");
  }
}

void saltController()
{
  if (!ShrimpAdded)
    return;
  if (SaltAdded)
    return;

  if (!GotTargetSaltWeight)
  {
    TargetSaltWeight = getTargetSaltWeight(); // Ensure target salt weight is updated based on shrimp weight
    GotTargetSaltWeight = true;
    Serial.println("TARGET SALT WEIGHT IS COMPUTED");
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
    shrimpStepper.stop();
    IsAddingSalt = false;
    Serial.println("Target salt weight reached.");
    SaltAdded = true;
  }
}

void mixerController()
{
  if (IsMixerOn)
  {
    return;
  }
  turnOnMixer();
  IsMixerOn = true;
}

void abortProcess()
{
  //
  Serial.println("PROCESS CANCELLED BY USER");
  AbortProcess = false;
  IsAddingShrimp = false;
  IsAddingSalt = false;

  ShrimpAdded = false;
  SaltAdded = false;

  ShrimpWeight = 0.0;
  SaltWeight = 0.0;
  TargetSaltWeight = 0.0;

  if (shrimpStepper.isRotating())
  {
    shrimpStepper.stop();
  }
  if (saltStepper.isRotating())
  {
    saltStepper.stop();
  }

  turnOffMixer();

  // S
}

void mainController()
{
  if (AbortProcess)
  {
    abortProcess();
    return;
  }

  if (IsWaitingToStart)
  {
    return;
  }

  if (!IsProcessStarted)
  {
    Serial.println("PROCESS INITIATED.");
    IsProcessStarted = true;
  }
  else
  {
    if (!IsAddingShrimp && !isShrimpEnough())
    {
      IsAddingShrimp = true;
    }

    if (isShrimpEnough() && !IsAddingSalt)
    {
      IsAddingSalt = true;
    }

    if (ShrimpAdded && SaltAdded)
    {
      IsMixing = true;
    }
  }

  // 1. Add shrimp until target weight is reached. Calculate target salt weight based on ratio.
  if (IsAddingShrimp)
  {
    shrimpController();
  }

  // 2. Add salt until target weight is reached
  if (IsAddingSalt)
  {
    saltController();
  }

  // 3. Start mixer for defined duration
  if (IsMixing)
  {
    mixerController();
  }
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
    Serial.print("STATUS: ");
    Serial.print(IsWaitingToStart ? "WAITING" : "RUNNING");
    Serial.print(" | ADDING SHRIMP : ");
    Serial.print(IsAddingShrimp ? "YES" : "NO ");
    Serial.print(" | ADDING SALT : ");
    Serial.print(IsAddingSalt ? "YES" : "NO ");
    Serial.print(" | MIXING : ");
    Serial.print(IsMixing ? "ON " : "OFF");
    Serial.print(" || \t|| HEATER : ");
    Serial.print(heaterSwitch.getState() ? "ON " : "OFF");

    // DISPLAY WEIGHT
    // WeightCurrentValue = readScale();

    // format SHRIMP: (CURRENT/TARGET)g AND SALT: (CURRENT/TARGET)g
    // variable names: SaltWeight, ShrimpWeight, TargetShrimpWeight, TargetSaltWeight
    Serial.print("|| \t|| SCALE WEIGHT: ");
    Serial.print(WeightCurrentValue, 2);
    Serial.print(" g  | SHRIMP: (");
    Serial.print(ShrimpWeight, 2);
    Serial.print("/");
    Serial.print(TargetShrimpWeight, 2);
    Serial.print(")g | SALT: (");
    Serial.print(SaltWeight, 2);
    Serial.print("/");
    Serial.print(TargetSaltWeight, 2);
    Serial.println(")g");

    lastPrintTime = millis();
  }
}