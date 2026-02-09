#include <Arduino.h>
#include "driver/gpio.h"
#include <cmath>

// Mode switch interval = 10 s
uint32_t SWITCH_INTERVAL = 1000000;

// ESP32 pin numbers
byte row[] =    {5, 10, 14, 8,  1, 13,  2, 11};
byte col[] =    {9,  3,  4, 6, 12,  7, 17, 18};

uint32_t FAST_INTERVAL = 1000;
uint32_t n_read = 1;
uint8_t i_read = 0;
volatile uint32_t isrCount = 0;
volatile bool secondFlag = false;

hw_timer_t *timerFast = NULL;
hw_timer_t *timerChange = NULL;

// macros for direct register manipulation, should reduce no. ticks drastically
// http://www.billporter.info/2010/08/18/ready-set-oscillate-the-fastest-way-to-change-arduino-pins/
#define SET_PIN(PIN)    (GPIO.out_w1ts = ((uint32_t)1 << PIN))  // HIGH
#define CLR_PIN(PIN)    (GPIO.out_w1tc = ((uint32_t)1 << PIN))  // LOW

void IRAM_ATTR onTimerFast();
void IRAM_ATTR onTimerChange();

// Update the fast counter
void IRAM_ATTR onTimerFast() {
  SET_PIN(col[i_read-1]); // deactivate
  if (i_read > 7)
    i_read = 0;

  CLR_PIN(col[i_read]); // activate
  i_read++;
}

// Swith interval mode
void IRAM_ATTR onTimerChange() {
  secondFlag = true;
}

void setup() {
  // Init pins for digital output
  for (int i = 0; i < 8; i++) {
    pinMode(col[i], OUTPUT);
    pinMode(row[i], OUTPUT);
    digitalWrite(col[i], HIGH);
    digitalWrite(row[i], HIGH);
  }
  
  // Begin serial communication
  Serial.begin(115200);
  delay(5000);
  Serial.println("Starting readout test...");

  Serial.print("Mode: ");
  Serial.println(n_read);

  // Timer for mode switch
  timerChange = timerBegin(1, 80, true);
  if (timerChange == NULL) {
    Serial.println("Timer failed to initialize!");
  }
  timerAttachInterrupt(timerChange, &onTimerChange, true);
  timerAlarmWrite(timerChange, SWITCH_INTERVAL, true);
  timerAlarmEnable(timerChange);

  // Timer 2: fires every FAST_INTERVAL
  // 80 MHz / 80 = 1 MHz → 1 tick = 1 µs
  timerFast = timerBegin(2, 80, true);
  if (timerFast == NULL) {
    Serial.println("Timer failed to initialize!");
  }
  timerAttachInterrupt(timerFast, &onTimerFast, true);
  timerAlarmWrite(timerFast, FAST_INTERVAL, true);
  timerAlarmEnable(timerFast);
}

void loop() {
  if (secondFlag) {
    secondFlag = false;

    n_read = n_read + 2;
    if (n_read > 50)
      n_read = 1;
    
    // Change the FAST_INTERVAL so the column blinking interval changes
    timerAlarmDisable(timerFast);
    timerDetachInterrupt(timerFast);
    timerAttachInterrupt(timerFast, &onTimerFast, true);
    timerAlarmWrite(timerFast, n_read * FAST_INTERVAL, true);
    timerAlarmEnable(timerFast);

    // Print current blinking interval
    Serial.print("Mode: ");
    Serial.println(n_read);
  }
}
