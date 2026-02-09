#include <Arduino.h>
#include "driver/gpio.h"
#include <cmath>

float BASE = 1.2;
float CONSTANT = 30;  // 10
#define COL_FOR_COUNTERS_COUNT 8

// ESP32 pin numbers
byte row[] =    {5, 10, 14, 8,  1, 13,  2, 11};
byte col[] =    {9,  3,  4, 6, 12,  7, 17, 18};

static uint8_t pattern[8][8] = {};  
uint32_t FAST_INTERVAL = 10;  // In us
uint32_t MULS[7];
bool toggle = true;
uint32_t mode_start_num = 0;

// macros for direct register manipulation, should reduce no. ticks drastically
// http://www.billporter.info/2010/08/18/ready-set-oscillate-the-fastest-way-to-change-arduino-pins/
#define SET_PIN(PIN)    (GPIO.out_w1ts = ((uint32_t)1 << PIN))  // HIGH
#define CLR_PIN(PIN)    (GPIO.out_w1tc = ((uint32_t)1 << PIN))  // LOW

hw_timer_t *timerFast = NULL;
hw_timer_t *timerChange = NULL;
hw_timer_t *timerGrey = NULL;

volatile uint32_t isrCount = 0;
volatile uint32_t isrCount0 = 0;
volatile uint32_t isrCount1 = 0;
volatile uint32_t isrCount2 = 0;
volatile uint32_t isrCount3 = 0;
volatile uint32_t isrCount4 = 0;
volatile uint32_t isrCount5 = 0;
volatile uint32_t isrCount6 = 0;
volatile uint32_t isrCount7 = 0;
volatile uint32_t isrCountGrey = 0;
volatile bool secondFlag = false;
volatile uint8_t gray_code = 0;

void IRAM_ATTR onTimerFast();
void IRAM_ATTR ontimerChange();
void IRAM_ATTR onTimerGrey();
void IRAM_ATTR displayMatrixColOpt(const uint8_t pattern[8][8]);


// Display pattern on LED matrix
void IRAM_ATTR displayMatrixColOpt(const uint8_t pattern[8][8]) {
  for (int c = 0; c < 8; c++) {
    uint8_t colBits = 0;
    for (int r = 0; r < 8; r++) {
      colBits |= (pattern[r][c] << r);
    }
    if (!colBits)
      continue;

    CLR_PIN(col[c]);
    for (int r = 0; r < 8; r++)
      if (colBits & (1 << r)) SET_PIN(row[r]);
    delayMicroseconds(1);
    for (int r = 0; r < 8; r++)
      if (colBits & (1 << r)) CLR_PIN(row[r]);
    SET_PIN(col[c]);
  }
}

// Update the fast counters
void IRAM_ATTR onTimerFast() {
  // Pattern all zeros
  for (int i=1; i<8; i+=2)
    for (int j=0; j<8; j++)
      pattern[i][j] = 0;

  isrCount++;
  isrCount0++;

  // Update counters
  isrCount0 = uint32_t(isrCount / MULS[0]);
  isrCount2 = uint32_t(isrCount / MULS[2]);
  isrCount4 = uint32_t(isrCount / MULS[4]);
  isrCount6 = uint32_t(isrCount / MULS[6]);
  
  // Based on counter activate LEDs
  pattern[0][isrCount0 & 0x7] = 1;
  pattern[2][isrCount1 & 0x7] = 1;
  pattern[4][isrCount2 & 0x7] = 1;
  pattern[6][isrCount3 & 0x7] = 1;

  // Change pattern with Gray code counter values
  for(int i=0; i<8; i++)
    pattern[1][i] = (uint8_t)(toGray((uint8_t)(isrCount0 / 8) % 256) >> i) & 1;
  for(int i=0; i<8; i++)
    pattern[3][i] = (uint8_t)(toGray((uint8_t)(isrCount2 / 8) % 256) >> i) & 1;
  for(int i=0; i<8; i++)
    pattern[5][i] = (uint8_t)(toGray((uint8_t)(isrCount4 / 8) % 256) >> i) & 1;
  for(int i=0; i<8; i++)
    pattern[7][i] = (uint8_t)(toGray((uint8_t)(isrCount6 / 8) % 256) >> i) & 1;
  
}

void IRAM_ATTR ontimerChange() {
  secondFlag = true;
}

uint8_t toGray(uint8_t n) {
    return n ^ (n >> 1);
}

void setup() {
  // Init pins for digital output
  for (int i = 0; i < 8; i++) {
    pinMode(col[i], OUTPUT);
    pinMode(row[i], OUTPUT);
    CLR_PIN(col[i]);
    SET_PIN(row[i]);
  }
  
  // Begin serial communication
  Serial.begin(115200);
  delay(5000);

  Serial.print(FAST_INTERVAL);
  Serial.println(" us");
  Serial.println("\n---------------------------------------------------------------------\nPattern 1");
  Serial.println("Counter Intervals: ");
  for (int i=0; i<7; i++) {
    MULS[i] = uint32_t(((pow(BASE, i+1)) * 30) / FAST_INTERVAL);  // In fastest counter cycles (NOT us nor ms)
    Serial.print(MULS[i] * FAST_INTERVAL);
    Serial.print(" us | ");
  }
  Serial.print("\nGray Code Interval: ");
  Serial.print((COL_FOR_COUNTERS_COUNT * MULS[6] * FAST_INTERVAL));
  Serial.println(" us");
  Serial.print("---------------------------------------------------------------------\n");

  // Timer for mode switch
  timerChange = timerBegin(1, 80, true);
  if (timerChange == NULL) {
    Serial.println("Timer failed to initialize!");
  }
  timerAttachInterrupt(timerChange, &ontimerChange, true);
  timerAlarmWrite(timerChange, 2000000, true);
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
  displayMatrixColOpt(pattern);
  
  if (secondFlag) {
    secondFlag = false;

    uint32_t countCopy = isrCount;
    isrCount = 0;

    mode_start_num = (mode_start_num < 35) ?  mode_start_num + 7 : 0;

    Serial.print("Pattern "); Serial.println(mode_start_num / 7 + 1);
    Serial.print("Counter Intervals: ");
    int iter = 0;
    for (int i=mode_start_num; i<mode_start_num+7; i++) {
      // 30 is a constant see this table: https://docs.google.com/spreadsheets/d/1n0wAZg8HFRTRlkmVXQ9rEmlsJPpDABjiiHWexLP9GDY/edit?gid=0#gid=0
      // MULS(i) = Time in us / time of one fast cycle => no. iterations for counter i to take before updating
      MULS[i - mode_start_num] = uint32_t(((pow(BASE, i+1)) * 30) / FAST_INTERVAL);  // In fastest counter cycles (NOT us nor ms)
      if (iter % 2 == 0) {  
        Serial.print(MULS[i - mode_start_num] * FAST_INTERVAL);
        Serial.print(" us | ");
      }
      iter++;
    }
    Serial.print("\n---------------------------------------------------------------------\n");
  }
}
