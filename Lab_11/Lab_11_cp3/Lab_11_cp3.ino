//Checkpoint 11.3
#include <Arduino.h>

const int PIN_STROBE = 7;
const int PIN_DATA   = 8;
const int PIN_CLOCK  = 9;

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

volatile int seconds = 0;    
volatile int milliseconds = 0; 
volatile bool is_running = false;
volatile bool count_up = true; 
volatile bool update_flag = true;

uint8_t last_btn_state = 0;
unsigned long last_btn_time = 0;

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

void updateDisplay() {
  uint8_t digits[8];

  digits[0] = digitMap[(seconds / 1000) % 10]; 
  digits[1] = digitMap[(seconds / 100) % 10];  
  digits[2] = digitMap[(seconds / 10) % 10];   
  digits[3] = digitMap[seconds % 10];          

  digits[4] = digitMap[(milliseconds / 100) % 10]; 
  digits[5] = digitMap[(milliseconds / 10) % 10];  
  digits[6] = digitMap[milliseconds % 10];         
  digits[7] = digitMap[0];                         

  sendCommand(0x44); 

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

  OCR1A = 249;                         
  TCCR1B |= (1 << WGM12);              
  TCCR1B |= (1 << CS11) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A);             
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  if (!is_running) return;

  if (count_up) {
    milliseconds++;
    if (milliseconds >= 1000) {
      milliseconds = 0;
      seconds++;
      if (seconds > 3599) seconds = 0;
    }
  } else {
    milliseconds--;
    if (milliseconds < 0) {
      milliseconds = 999;
      seconds--;
      if (seconds < 0) seconds = 3599;
    }
  }
  update_flag = true;
}

void processButtons() {
  uint8_t current_btn = readButtons();

  uint8_t pressed_btn = current_btn & (~last_btn_state);
  last_btn_state = current_btn; 

  if (pressed_btn == 0) return; 

  if (millis() - last_btn_time < 50) return;
  last_btn_time = millis();

  int th = (seconds / 1000) % 10;
  int hu = (seconds / 100) % 10;
  int te = (seconds / 10) % 10;
  int un = seconds % 10;

  if (pressed_btn & 0x01) {
    th = (th + 1) % 4;
    milliseconds = 0; 
  }
  else if (pressed_btn & 0x02) {
    hu = (hu + 1) % 6;
    milliseconds = 0; 
  }
  else if (pressed_btn & 0x04) {
    te = (te + 1) % 10;
    milliseconds = 0; 
  }
  else if (pressed_btn & 0x08) {
    un = (un + 1) % 4;
    milliseconds = 0; 
  }
  else if (pressed_btn & 0x10) {
    count_up = !count_up;
  }
  else if (pressed_btn & 0x80) {
    is_running = !is_running;
  }

  seconds = (th * 1000) + (hu * 100) + (te * 10) + un;
  if (seconds > 3599) seconds = 3599;

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
  processButtons();

  if (update_flag) {
    update_flag = false;
    updateDisplay();
  }
}