// Checkpoint 11.1
#include <Arduino.h>

const int PIN_STROBE = 7;
const int PIN_DATA   = 8;
const int PIN_CLOCK  = 9; 

volatile int hours   = 23;
volatile int minutes = 59;
volatile int seconds = 55;
volatile bool update_flag = true; 

const uint8_t digitMap[] = {
  0b00111111,  
  0b00000110,  
  0b01011011,  
  0b01001111,  
  0b01100110,  
  0b01101101,  
  0b01111101,  
  0b00000111,  
  0b01111111,  
  0b01101111   
};
const uint8_t DASH = 0b01000000;

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

void updateDisplay() {
  uint8_t digits[8]; 
  
  digits[0] = digitMap[hours / 10];
  digits[1] = digitMap[hours % 10];
  digits[2] = DASH;
  digits[3] = digitMap[minutes / 10];
  digits[4] = digitMap[minutes % 10];
  digits[5] = DASH;
  digits[6] = digitMap[seconds / 10];
  digits[7] = digitMap[seconds % 10]; 

  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_STROBE, LOW); 
    shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0xC0 + (i * 2)); 
    shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, digits[i]); 
    digitalWrite(PIN_STROBE, HIGH); 
  }
}

void setupTimer1() {
  noInterrupts(); 
  TCCR1A = 0;     
  TCCR1B = 0;     
  TCNT1  = 0;     

  OCR1A = 15624;                       
  TCCR1B |= (1 << WGM12);              
  TCCR1B |= (1 << CS12) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A);             
  interrupts(); 
}

ISR(TIMER1_COMPA_vect) {
  seconds++;
  if (seconds >= 60) {
    seconds = 0;
    minutes++;
    if (minutes >= 60) {
      minutes = 0;
      hours++;
      if (hours >= 24) {
        hours = 0;
      }
    }
  }
  update_flag = true; 
}

void setup() {
  pinMode(PIN_STROBE, OUTPUT); 
  pinMode(PIN_CLOCK, OUTPUT);  
  pinMode(PIN_DATA, OUTPUT);   

  sendCommand(0x8F); 
  resetTM1638();     
  setupTimer1();     
}

void loop() {
  if (update_flag) {
    update_flag = false; 
    updateDisplay();     
  }
}