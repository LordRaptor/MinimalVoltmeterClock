#include <Arduino.h>
#include <TimeLib.h>
#include <DCF77.h>
#include <controller/PWMController.h>
#include <avdweb_Switch.h>

#define DCF_PIN 21
#define DCF_UPDATE_INTERVAL 3600000

#define VOLTMETER_PIN 3
#define VOLTMETER_STEP_PER_SECOND 255

#define BUTTON_PIN 20

// put function declarations here:
void startup();
void displayTime();
void receiveTime();
void waitForTime();
void enterCalibration();
void calibration();
void exitCalibration();
int calculateTarget(int hour, int minute, int second);

void buttonLongPressedCallback(void *ref);

void writeTimetoSerial(uint8_t hours, uint8_t minutes, uint8_t seconds);

DCF77 dcfReceiver = DCF77(DCF_PIN, digitalPinToInterrupt(DCF_PIN));
time_t lastTimeReceived = 0;

PWMController voltmeter = PWMController();

Switch button = Switch(BUTTON_PIN);

enum ClockState
{
  startupState,
  displayTimeState,
  waitForTimeState,
  calibrationState
};
ClockState state = startupState;

int calibrationTargetHour = 12;

void setup() {
  Serial.begin(9600);
  voltmeter.addPin(VOLTMETER_PIN);
  voltmeter.setSpeed(VOLTMETER_STEP_PER_SECOND);
  voltmeter.setTarget(0);

  // TCB1.CTRLA &= TCB_ENABLE_bm;
  // // Set waveform generation mode to single slope PWM
  // TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;

  // // Set period (TOP) value to define frequency
  // // For example, 0x00FF gives ~62.5 kHz at 16 MHz clock
  // TCB1.CCMP = 0x00FF;

  // // Set duty cycle (0-255)
  // TCB1.CNT = 0;
  
  // // Use CLK_PER (same as CPU clock, typically 16 MHz)
  // TCB1.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;

  pinMode(DCF_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // put your setup code here, to run once:
  dcfReceiver.Start();

  int ref;
  button.setLongPressCallback(&buttonLongPressedCallback, &ref);
}

void loop() {
  button.poll();
  receiveTime(); // Maybe only update once an hour?

  switch (state)
  {
  case startupState:
    startup();
    break;
  case displayTimeState:
    displayTime();
    break;
  case waitForTimeState:
    waitForTime();
    break;
  case calibrationState:
    calibration();
    break;
  default:
    break;
  }

}

void startup() {
  voltmeter.setSpeed(64);
  voltmeter.setTarget(255);

  while(!voltmeter.moveToTarget());

  voltmeter.setTarget(0);

  while(!voltmeter.moveToTarget());

  voltmeter.setSpeed(VOLTMETER_STEP_PER_SECOND);

  if (lastTimeReceived > 0) {
    state = displayTimeState;
    Serial.println(F("Received time already"));
  } else {
    state = waitForTimeState;
    Serial.println(F("No time received yet"));
  }

  Serial.println(F("Startup finished"));

}

void displayTime() {
  byte target = calculateTarget(hour(), minute(), second());
  voltmeter.setTarget(target);
  voltmeter.moveToTarget();
}

void receiveTime() {
  time_t dcfTime = dcfReceiver.getTime();
  if (dcfTime != 0) 
  {
    Serial.print("New time received ");
    setTime(dcfTime);
    writeTimetoSerial(hour(), minute(), second());
    lastTimeReceived = dcfTime;
  }
}

void waitForTime() {
  voltmeter.setSpeed(64);
  
  if (voltmeter.moveToTarget()) {
    voltmeter.setTarget(~voltmeter.getTarget());
  }

  if (lastTimeReceived > 0) {
    voltmeter.setSpeed(VOLTMETER_STEP_PER_SECOND);
    state = displayTimeState;
  }
}

void enterCalibration() {
  state = calibrationState;
  calibrationTargetHour = 12;
  voltmeter.setTarget(255);
  Serial.println(F("Entering calibration state"));
}

void calibration() {
  voltmeter.moveToTarget();
  if (button.pushed()) {
    calibrationTargetHour = (calibrationTargetHour + 1) % 13;
    byte target = calculateTarget(12 + calibrationTargetHour, 0, 0);
    Serial.print(F("Hour "));
    Serial.print(calibrationTargetHour);
    Serial.print(F(" ("));
    Serial.print(target);
    Serial.println(")");
    voltmeter.setTarget(target);
  }
}

void exitCalibration() {
    if (lastTimeReceived > 0) {
    state = displayTimeState;
  } else {
    voltmeter.setTarget(0);
    state = waitForTimeState;
  }
  Serial.println(F("Exiting calibration state"));
}

void buttonLongPressedCallback(void* ref) {
  Serial.println("Button Pressed");
  if (state != calibrationState) {
    enterCalibration();
  } else {
    exitCalibration();
  }
}

int calculateTarget(int hour, int minute, int second) {
  if (hour >= 12) {
    hour -= 12;
  }
  float decimal12HTime = (hour + (minute / 60.f) + (second / 3600.f)) / 12.f;
  return constrain(round(decimal12HTime * 252), 0, 255);
}

void writeTimetoSerial(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  if (hours < 10)
  {
    Serial.print("0");
  }
  Serial.print(hours);
  Serial.print(":");
  if (minutes < 10)
  {
    Serial.print("0");
  }
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10)
  {
    Serial.print("0");
  }
  Serial.print(seconds);

  Serial.print(" (");
  Serial.print(calculateTarget(hours, minutes, seconds));
  Serial.println(")");
}