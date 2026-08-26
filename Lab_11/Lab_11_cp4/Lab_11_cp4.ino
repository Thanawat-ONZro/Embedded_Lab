//Checkpoint 11.4
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

struct Counter {
  int value;       
  bool is_running; 
  bool count_up;   
};

volatile Counter c1 = {0, false, true};    
volatile Counter c2 = {0, false, true};    
volatile bool update_flag = true;

unsigned long s2_press_start = 0;
unsigned long s6_press_start = 0;
bool s2_reset_done = false;
bool s6_reset_done = false;

uint8_t last_btn_state = 0;

void sendCommand(uint8_t value) {
  digitalWrite(PIN_STROBE, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, value);
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

void formatDigits(int val, uint8_t* out_digits) {
  int min = (val / 600) % 10;
  int sec_tens = (val / 100) % 6;
  int sec_units = (val / 10) % 10;
  int ds = val % 10;

  out_digits[0] = digitMap[min] | 0x80;       
  out_digits[1] = digitMap[sec_tens];         
  out_digits[2] = digitMap[sec_units] | 0x80; 
  out_digits[3] = digitMap[ds];               
}

void updateDisplay() {
  uint8_t digits[8];

  formatDigits(c1.value, &digits[0]);
  formatDigits(c2.value, &digits[4]);

  sendCommand(0x44); 
  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_STROBE, LOW);
    shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, 0xC0 + (i * 2));
    shiftOut(PIN_DATA, PIN_CLOCK, LSBFIRST, digits[i]);
    digitalWrite(PIN_STROBE, HIGH);
  }

  setLED(0, c1.count_up ? 1 : 0);
  setLED(7, c2.count_up ? 1 : 0);
}

void setupTimer1() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 6249;                        
  TCCR1B |= (1 << WGM12);              
  TCCR1B |= (1 << CS12);               
  TIMSK1 |= (1 << OCIE1A);             
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  if (c1.is_running) {
    if (c1.count_up) {
      c1.value++;
      if (c1.value > 5999) c1.value = 0;
    } else {
      c1.value--;
      if (c1.value < 0) c1.value = 5999;
    }
  }

  if (c2.is_running) {
    if (c2.count_up) {
      c2.value++;
      if (c2.value > 5999) c2.value = 0;
    } else {
      c2.value--;
      if (c2.value < 0) c2.value = 5999;
    }
  }

  update_flag = true;
}

void processButtons() {
  uint8_t btn = readButtons();
  unsigned long current_time = millis();

  if (btn & 0x02) {
    if (s2_press_start == 0) {
      s2_press_start = current_time;
      s2_reset_done = false;
    } else if (!s2_reset_done && (current_time - s2_press_start >= 2000)) {
      c1.value = c1.count_up ? 0 : 5999;
      s2_reset_done = true;
      update_flag = true;
    }
  } else {
    s2_press_start = 0;
    s2_reset_done = false;
  }

  if (btn & 0x20) {
    if (s6_press_start == 0) {
      s6_press_start = current_time;
      s6_reset_done = false;
    } else if (!s6_reset_done && (current_time - s6_press_start >= 2000)) {
      c2.value = c2.count_up ? 0 : 5999;
      s6_reset_done = true;
      update_flag = true;
    }
  } else {
    s6_press_start = 0;
    s6_reset_done = false;
  }

  uint8_t pressed_btn = btn & (~last_btn_state);
  last_btn_state = btn; 

  if (pressed_btn == 0) return; 

  if (pressed_btn & 0x01) { 
    c1.is_running = !c1.is_running;
  }
  else if (pressed_btn & 0x04) { 
    c1.count_up = !c1.count_up;
  }
  else if (pressed_btn & 0x08) { 
    if (!c1.is_running) {
      if (c1.count_up) {
        c1.value++;
        if (c1.value > 5999) c1.value = 0;
      } else {
        c1.value--;
        if (c1.value < 0) c1.value = 5999;
      }
      update_flag = true;
    }
  }

  if (pressed_btn & 0x10) { 
    c2.is_running = !c2.is_running;
  }
  else if (pressed_btn & 0x40) { 
    c2.count_up = !c2.count_up;
  }
  else if (pressed_btn & 0x80) { 
    if (!c2.is_running) {
      if (c2.count_up) {
        c2.value++;
        if (c2.value > 5999) c2.value = 0;
      } else {
        c2.value--;
        if (c2.value < 0) c2.value = 5999;
      }
      update_flag = true;
    }
  }
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