#include <EEPROM.h>
#include <TimerOne.h>

float flow0 = 0, flow1 = 0, flow2 = 0, flow3 = 0;
float tot0 = 0, tot1 = 0, tot2 = 0, tot3 = 0;
float leakFlow0 = 0, leakFlow1 = 0, leakFlow2 = 0, leakFlow3 = 0;
volatile int count1 = 0, count2 = 0;
int count0 = 0, count3 = 0;

const int meterpin0 = 5, meterpin1 = 2, meterpin2 = 3, meterpin3 = 4;

int laststate0 = LOW, laststate3 = LOW;
float calibrationFactor = 7.5;
const int leakduration = 600; // 600 seconds (10 minutes) leak detection duration
bool flag0 = false, flag1 = false, flag2 = false, flag3 = false;
int flagcounter0 = 0, flagcounter1 = 0, flagcounter2 = 0, flagcounter3 = 0;

unsigned long previousMillis = 0;
unsigned long previousEEPROMReset = 0;  // To store the last EEPROM reset time
const unsigned long eepromResetInterval = 3600000; // 1 hours in milliseconds
const unsigned long interval = 1000;         // 1 second

void setup() {
  pinMode(meterpin1, INPUT);
  pinMode(meterpin2, INPUT);
  pinMode(meterpin3, INPUT);
  pinMode(meterpin0, INPUT);

  Timer1.initialize(1000000);        // 1-second interval
  Timer1.attachInterrupt(calibration);

  attachInterrupt(digitalPinToInterrupt(meterpin1), countpulse1, RISING);
  attachInterrupt(digitalPinToInterrupt(meterpin2), countpulse2, RISING);

  Serial.begin(9600);
}

void loop() {
  int flow3state = digitalRead(meterpin3);
  if (flow3state == HIGH && laststate3 == LOW) {
    count3++;
  }
  laststate3 = flow3state;

  int flow0state = digitalRead(meterpin0);
  if (flow0state == HIGH && laststate0 == LOW) {
    count0++;
  }
  laststate0 = flow0state;

  unsigned long currentMillis = millis();

  // Ensure non-blocking and robust leak detection
  if (currentMillis - previousMillis >= interval) {
    previousMillis += interval; // Avoid timer drift
    leak();                     // Periodic leak detection
  }

  // Reset EEPROM every 2 hours
  if (currentMillis - previousEEPROMReset >= eepromResetInterval) {
    resetEEPROM();  // Reset EEPROM values
    previousEEPROMReset = currentMillis;  // Store the time of the last EEPROM reset
  }
}

void countpulse1() {
  count1++;
}

void countpulse2() {
  count2++;
}

void calibration() {
  flow1 = count1 / calibrationFactor;
  tot1 += flow1 / 60;
  flow2 = count2 / calibrationFactor;
  tot2 += flow2 / 60;
  flow3 = count3 / calibrationFactor;
  tot3 += flow3 / 60;
  flow0 = count0 / calibrationFactor;
  tot0 += flow0 / 60;

  // Log flow and totals for debugging
  Serial.print("Flow0: "); Serial.println(flow0);
  Serial.print("Total0: "); Serial.println(tot0);
  Serial.print("Flow1: "); Serial.println(flow1);
  Serial.print("Total1: "); Serial.println(tot1);
  Serial.print("Flow2: "); Serial.println(flow2);
  Serial.print("Total2: "); Serial.println(tot2);
  Serial.print("Flow3: "); Serial.println(flow3);
  Serial.print("Total3: "); Serial.println(tot3);

  count0 = 0;
  count1 = 0;
  count2 = 0;
  count3 = 0;
}

void leak() {
  // Check for each sensor individually
  if (flow1 > 0) {
    flagcounter1++;
    if (flagcounter1 >= leakduration && !flag1) {
      flag1 = true;
      leakFlow1 = tot1 - (tot1 - (flow1 / 60) * leakduration); 
      Serial.print("LEAK DETECTED in Flow Sensor 1, Leak Flow: ");
      Serial.println(leakFlow1);
      flagcounter1 = 0;
      flag1 = false;
    }
  } else {
    flagcounter1 = 0;
    flag1 = false;
  }

  if (flow2 > 0) {
    flagcounter2++;
    if (flagcounter2 >= leakduration && !flag2) {
      flag2 = true;
      leakFlow2 = tot2 - (tot2 - (flow2 / 60) * leakduration);
      Serial.print("LEAK DETECTED in Flow Sensor 2, Leak Flow: ");
      Serial.println(leakFlow2);
      flagcounter2 = 0;
      flag2 = false;
    }
  } else {
    flagcounter2 = 0;
    flag2 = false;
  }

  if (flow3 > 0) {
    flagcounter3++;
    if (flagcounter3 >= leakduration && !flag3) {
      flag3 = true;
      leakFlow3 = tot3 - (tot3 - (flow3 / 60) * leakduration);
      Serial.print("LEAK DETECTED in Flow Sensor 3, Leak Flow: ");
      Serial.println(leakFlow3);
      flagcounter3 = 0;
      flag3 = false;
    }
  } else {
    flagcounter3 = 0;
    flag3 = false;
  }

  if (flow0 > 0) {
    flagcounter0++;
    if (flagcounter0 >= leakduration && !flag0) {
      flag0 = true;
      leakFlow0 = tot0 - (tot0 - (flow0 / 60) * leakduration);
      Serial.print("LEAK DETECTED in Flow Sensor 0, Leak Flow: ");
      Serial.println(leakFlow0);
      flagcounter0 = 0;
      flag0 = false;
    }
  } else {
    flagcounter0 = 0;
    flag0 = false;
  }
}

void resetEEPROM() {
  // Reset EEPROM values to default (e.g., 0)
  EEPROM.write(0, 0);
  EEPROM.write(1, 0);
  EEPROM.write(2, 0);
  EEPROM.write(3, 0);
  EEPROM.write(4, 0);
  EEPROM.write(5, 0);
  // Add any other EEPROM reset logic you need here
  Serial.println("EEPROM values reset.");
}
