#include <Arduino.h>
#include "driver/gpio.h"
#include <cmath>

// Mode switch interval = 10 s
uint32_t SWITCH_INTERVAL = 1000000;

float BASE = 1.1;
float CONSTANT = 20;  // 10
#define COL_FOR_COUNTERS_COUNT 8
#define NUM_COUNTERS 4
#define MAX_PATTERNS 20  // Number of patterns

// ESP32 pin numbers
byte row[] =    {5, 10, 14, 8,  1, 13,  2, 11};
byte col[] =    {9,  3,  4, 6, 12,  7, 17, 18};

static uint8_t pattern[8][8] = {};
uint32_t FAST_INTERVAL = 10;  // In us
uint32_t MULS[NUM_COUNTERS];
uint32_t mode_start_num = 0;

// macros for direct register manipulation
#define SET_PIN(PIN)    (GPIO.out_w1ts = ((uint32_t)1 << PIN))  // HIGH
#define CLR_PIN(PIN)    (GPIO.out_w1tc = ((uint32_t)1 << PIN))  // LOW

hw_timer_t *timerFast = NULL;
hw_timer_t *timerChange = NULL;

volatile uint32_t isrCount = 0;

volatile uint32_t isrCount0 = 0;
volatile uint32_t isrCount1 = 0;
volatile uint32_t isrCount2 = 0;
volatile uint32_t isrCount3 = 0;

volatile uint32_t isrCount0Offset = 0;
volatile uint32_t isrCount1Offset = 0;
volatile uint32_t isrCount2Offset = 0;
volatile uint32_t isrCount3Offset = 0;

volatile bool secondFlag = false;

void IRAM_ATTR onTimerFast();
void IRAM_ATTR ontimerChange();
void IRAM_ATTR displayMatrixColOpt();
void setLetterPattern(char letter);
volatile bool showingLetter = false;

void setLetterPattern(char letter) {
  // Clear pattern
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      pattern[r][c] = 0;
    }
  }
  
  static const uint8_t font[][8] = {
    // A
    {0b00111100, 0b01100110, 0b01100110, 0b01111110, 0b01100110, 0b01100110, 0b01100110, 0b00000000},
    // B
    {0b01111100, 0b01100110, 0b01111100, 0b01100110, 0b01100110, 0b01100110, 0b01111100, 0b00000000},
    // C
    {0b00111100, 0b01100110, 0b01100000, 0b01100000, 0b01100000, 0b01100110, 0b00111100, 0b00000000},
    // D
    {0b01111000, 0b01101100, 0b01100110, 0b01100110, 0b01100110, 0b01101100, 0b01111000, 0b00000000},
    // E
    {0b01111110, 0b01100000, 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b01111110, 0b00000000},
    // F
    {0b01111110, 0b01100000, 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b00000000},
    // G
    {0b00111100, 0b01100110, 0b01100000, 0b01101110, 0b01100110, 0b01100110, 0b00111100, 0b00000000},
    // H
    {0b01100110, 0b01100110, 0b01111110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00000000},
    // I
    {0b00111100, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00111100, 0b00000000},
    // J
    {0b00011110, 0b00001100, 0b00001100, 0b00001100, 0b01101100, 0b01101100, 0b00111000, 0b00000000},
    // K
    {0b01100110, 0b01101100, 0b01111000, 0b01110000, 0b01111000, 0b01101100, 0b01100110, 0b00000000},
    // L
    {0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01111110, 0b00000000},
    // M
    {0b01100011, 0b01110111, 0b01111111, 0b01101011, 0b01100011, 0b01100011, 0b01100011, 0b00000000},
    // N
    {0b01100011, 0b01110011, 0b01111011, 0b01101111, 0b01100111, 0b01100011, 0b01100011, 0b00000000},
    // O
    {0b00111100, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00000000},
    // P
    {0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b00000000},
    // Q
    {0b00111100, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00001110, 0b00000000},
    // R
    {0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b01111000, 0b01101100, 0b01100110, 0b00000000},
    // S
    {0b00111100, 0b01100110, 0b01100000, 0b00111100, 0b00000110, 0b01100110, 0b00111100, 0b00000000},
    // T
    {0b01111110, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00000000},
    // U
    {0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00000000},
    // V
    {0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00000000},
    // W
    {0b01100011, 0b01100011, 0b01100011, 0b01101011, 0b01111111, 0b01110111, 0b01100011, 0b00000000},
    // X
    {0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00111100, 0b01100110, 0b01100110, 0b00000000},
    // Y
    {0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00011000, 0b00011000, 0b00000000},
    // Z
    {0b01111110, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b01100000, 0b01111110, 0b00000000}
  };
  
  int letterIndex = letter - 'A';
  if (letterIndex < 0 || letterIndex >= 26) return;
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (font[letterIndex][row] & (1 << (7 - col))) {
        pattern[row][col] = 1;
      }
    }
  }
}

// Display pattern on LED matrix
void IRAM_ATTR displayMatrixColOpt() {
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

void IRAM_ATTR onTimerFast() {
  if (showingLetter) return;
  uint32_t count = ++isrCount;
  
  uint32_t muls0 = MULS[0];
  uint32_t muls1 = MULS[1];
  uint32_t muls2 = MULS[2];
  uint32_t muls3 = MULS[3];
  
  // Half interval
  uint32_t half0 = muls0 >> 1;
  uint32_t half1 = muls1 >> 1;
  uint32_t half2 = muls2 >> 1;
  uint32_t half3 = muls3 >> 1;
  
  // Deactivate previous
  pattern[0][isrCount0 & 0x7] = 0;
  pattern[1][isrCount0Offset & 0x7] = 0;
  pattern[2][isrCount1 & 0x7] = 0;
  pattern[3][isrCount1Offset & 0x7] = 0;
  pattern[4][isrCount2 & 0x7] = 0;
  pattern[5][isrCount2Offset & 0x7] = 0;
  pattern[6][isrCount3 & 0x7] = 0;
  pattern[7][isrCount3Offset & 0x7] = 0;

  // Get new counter values
  uint32_t cnt0 = count / muls0;
  uint32_t cnt1 = count / muls1;
  uint32_t cnt2 = count / muls2;
  uint32_t cnt3 = count / muls3;

  uint32_t off0 = (count + half0) / muls0;
  uint32_t off1 = (count + half1) / muls1;
  uint32_t off2 = (count + half2) / muls2;
  uint32_t off3 = (count + half3) / muls3;

  isrCount0 = cnt0;
  isrCount1 = cnt1;
  isrCount2 = cnt2;
  isrCount3 = cnt3;
  isrCount0Offset = off0;
  isrCount1Offset = off1;
  isrCount2Offset = off2;
  isrCount3Offset = off3;
  
  // Update pattern for new values
  pattern[0][cnt0 & 0x7] = 1;
  pattern[1][off0 & 0x7] = 1;
  pattern[2][cnt1 & 0x7] = 1;
  pattern[3][off1 & 0x7] = 1;
  pattern[4][cnt2 & 0x7] = 1;
  pattern[5][off2 & 0x7] = 1;
  pattern[6][cnt3 & 0x7] = 1;
  pattern[7][off3 & 0x7] = 1;
}

void IRAM_ATTR ontimerChange() {
  secondFlag = true;
}

void updateMuls(mode_id) {
    Serial.print("\nPattern "); Serial.println(mode_id / NUM_COUNTERS + 1);
    
    for (int i = 0; i < NUM_COUNTERS; i++) {
      int exponent = mode_id + i + 1;
      float period_calc = ((pow(BASE, exponent)) * CONSTANT) / FAST_INTERVAL;
      MULS[i] = uint32_t(period_calc);
      
      Serial.print("Counter "); Serial.print(i);
      Serial.print(", Period = "); Serial.print(MULS[i] * FAST_INTERVAL);
      Serial.print(" us ("); Serial.print((MULS[i] * FAST_INTERVAL) / 1000.0); Serial.print(" ms)");
      Serial.println();
    }
    
    Serial.print("-----------------------------------------------------\n");
}

void showLetter(mode_id) {
    uint8_t patternNum = mode_id / NUM_COUNTERS;
    char patternLetter = 'A' + (patternNum % 26);

    showingLetter = true;
    setLetterPattern(patternLetter);
    
    unsigned long letterStart = millis();
    while (millis() - letterStart < 200) {
      displayMatrixColOpt();
    }
    
    // Clear pattern and resume counters
    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 8; c++) {
        pattern[r][c] = 0;
      }
    }
    showingLetter = false;
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

  Serial.print("Base interval: ");
  Serial.print(FAST_INTERVAL);
  Serial.println(" us");
  Serial.print("BASE: ");
  Serial.println(BASE);
  Serial.print("Number of patterns: ");
  Serial.println(MAX_PATTERNS);

  updateMuls(0);
  showLetter(0);

  // Mode switch timer
  timerChange = timerBegin(1, 80, true);
  if (timerChange == NULL) {
    Serial.println("Timer Print failed to initialize!");
  }
  timerAttachInterrupt(timerChange, &ontimerChange, true);
  timerAlarmWrite(timerChange, 1000000, true);
  timerAlarmEnable(timerChange);

  // Fast timer
  timerFast = timerBegin(2, 80, true);
  if (timerFast == NULL) {
    Serial.println("Timer Fast failed to initialize!");
  }
  timerAttachInterrupt(timerFast, &onTimerFast, true);
  timerAlarmWrite(timerFast, FAST_INTERVAL, true);
  timerAlarmEnable(timerFast);

  Serial.println("Timer started - each counter has offset version at half period");
}

void loop() {
  displayMatrixColOpt();
  
  if (secondFlag) {
    secondFlag = false;
    uint32_t countCopy = isrCount;
    isrCount = 0;

    // Cycle through patterns
    mode_start_num = (mode_start_num < NUM_COUNTERS * (MAX_PATTERNS - 1)) ? mode_start_num + NUM_COUNTERS : 0;

    showLetter(mode_start_num);
    updateMuls(mode_start_num);
  }
}