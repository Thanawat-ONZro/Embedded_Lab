//Checkpoint 11.2
#include <Arduino.h>

const int PIN_STROBE = 7;
const int PIN_DATA   = 8;
const int PIN_CLOCK  = 9;

uint8_t activeLed = 255;  
uint8_t lastBtnState = 0; 

uint8_t currentMode = 0; 

void sendCommand(uint8_t value) {
  digitalWrite(PIN_STROBE, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, value); 
  digitalWrite(PIN_STROBE, HIGH);
}

void resetTM1638() {
  sendCommand(0x40);
  digitalWrite(PIN_STROBE, LOW); 
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0xC0);
  for (int i = 0; i < 16; i++) {
    shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0x00);
  }
  digitalWrite(PIN_STROBE, HIGH); 
}

void setLED(uint8_t position, uint8_t value) {
  pinMode(PIN_DATA, OUTPUT);
  sendCommand(0x44);
  
  digitalWrite(PIN_STROBE, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0xC1 + (position * 2));
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, value);
  digitalWrite(PIN_STROBE, HIGH);
}

void clearAllLEDs() {
  for (uint8_t i = 0; i < 8; i++) {
    setLED(i, 0);
  }
}

uint8_t readButtons() {
  uint8_t buttons = 0;
  digitalWrite(PIN_STROBE, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0x42);
  
  pinMode(PIN_DATA, INPUT);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t v = shiftIn(PIN_DATA, PIN_CLOCK, LSBFIRST) << i;
    buttons |= v;
  }
  
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_STROBE, HIGH);
  return buttons;
}

void stepPatternRight() {
  static uint8_t step = 0;
  static unsigned long lastStepTime = 0;

  if (millis() - lastStepTime >= 100) {
    lastStepTime = millis();
    
    if (step == 0) clearAllLEDs();
    setLED(step, 1);
    
    step++;
    if (step >= 8) {
      step = 0; 
    }
  }
}

void stepPatternLeft() {
  static int step = 7;
  static unsigned long lastStepTime = 0;

  if (millis() - lastStepTime >= 100) {
    lastStepTime = millis();
    
    if (step == 7) clearAllLEDs();
    setLED(step, 1);
    
    step--;
    if (step < 0) {
      step = 7; 
    }
  }
}

void setup() {
  pinMode(PIN_STROBE, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT); 
  pinMode(PIN_DATA, OUTPUT);  

  resetTM1638();   
  sendCommand(0x8F); 
}

void loop() {
  uint8_t btn = readButtons();
  uint8_t pressedBtn = btn & (~lastBtnState);

  if (pressedBtn > 0) {
    if (pressedBtn & 0b00000001) { 
      currentMode = 1; 
      clearAllLEDs();
    } 
    else if (pressedBtn & 0b10000000) { 
      currentMode = 2; 
      clearAllLEDs();
    } 
    else {
      if (currentMode != 0) {
        currentMode = 0;
        clearAllLEDs();
      }

      for (uint8_t i = 1; i <= 6; i++) {
        uint8_t mask = (1 << i);
        if (pressedBtn & mask) {
          if (activeLed == i) {
            activeLed = 255;
          } else {
            activeLed = i;
          }
        }
      }
    }
  }

  if (currentMode == 1) {
    stepPatternRight(); 
  } 
  else if (currentMode == 2) {
    stepPatternLeft(); 
  } 
  else {
    for (uint8_t i = 1; i <= 6; i++) {
      if (i == activeLed) {
        setLED(i, 1); 
      } else {
        setLED(i, 0); 
      }
    }
  }

  lastBtnState = btn;
  delay(10); 
}