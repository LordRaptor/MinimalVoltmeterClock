#include <Arduino.h>
#include <TimeLib.h>
#include <DCF77.h>
#include <controller/PWMController.h>

#define DCF_PIN 21
#define DCF_UPDATE_INTERVAL 3600000

#define VOLTMETER_PIN 3
#define VOLTMETER_STEP_PER_SECOND 255

// put function declarations here:
void startup();
void displayTime();
void receiveTime();
void enterCalibration();
void calibration();
void exitCalibration();
int calculateTarget(int hour, int minute, int second);

void writeTimetoSerial(uint8_t hours, uint8_t minutes, uint8_t seconds);

DCF77 dcfReceiver = DCF77(DCF_PIN, digitalPinToInterrupt(DCF_PIN));
time_t lastTimeReceived = 0;

PWMController voltmeter = PWMController();

enum ClockState
{
  startupState,
  displayTimeState,
  receiveTimeState,
  calibrationState
};
ClockState state = startupState;

void setup() {
  Serial.begin(9600);
  voltmeter.addPin(VOLTMETER_PIN);
  voltmeter.setSpeed(VOLTMETER_STEP_PER_SECOND);
  voltmeter.setTarget(0);

  pinMode(DCF_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // put your setup code here, to run once:
  dcfReceiver.Start();


}

void loop() {
  // put your main code here, to run repeatedly:
  switch (state)
  {
  case startupState:
    startup();
    break;
  case displayTimeState:
    receiveTime(); // Maybe only update once an hour?
    displayTime();
    break;
  case receiveTimeState:
  case calibrationState:
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

  state = displayTimeState;
  Serial.println(F("Startup finished"));

}

void displayTime() {
  voltmeter.setTarget(calculateTarget(hour(), minute(), second()));
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