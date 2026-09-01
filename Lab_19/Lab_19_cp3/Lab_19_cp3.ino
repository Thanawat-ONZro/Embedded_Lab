#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>

#define LCD_ADDR     0x27
#define PCF_ADDR     0x20
#define EEPROM_ADDR  0x50

#define NUM_SLOTS    32

RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {4, 5, 6, 7};

Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, PCF_ADDR);

SemaphoreHandle_t i2cMutex;

enum EditState { NORMAL, SET_HOUR, SET_MIN, SET_DAY, SET_MONTH, SET_YEAR };
EditState editMode = NORMAL;

int displayMode = 1;
int currentSlot = 0;
uint8_t currentSeq = 0;

int editHour = 0;
int editMin = 0;
int editDay = 1;
int editMonth = 1;
int editYear = 2026;
DateTime currentRTC;

int getMaxDays(int y, int m) {
  if (m == 2) {
    bool isLeap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    return isLeap ? 29 : 28;
  }
  if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
  return 31;
}

void writeEEPROM_24C32(uint16_t memAddr, uint8_t data) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((uint8_t)(memAddr >> 8));
  Wire.write((uint8_t)(memAddr & 0xFF));
  Wire.write(data);
  Wire.endTransmission();
  delay(10);
}

uint8_t readEEPROM_24C32(uint16_t memAddr) {
  uint8_t rData = 0xFF;
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((uint8_t)(memAddr >> 8));
  Wire.write((uint8_t)(memAddr & 0xFF));
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)EEPROM_ADDR, (uint8_t)1);
  if (Wire.available()) {
    rData = Wire.read();
  }
  return rData;
}

void display_all_data_in_24C32() {
  Serial.println(F("\n--- Data in 24C32 EEPROM (Wear Leveling Slots) ---"));
  for (int i = 0; i < NUM_SLOTS * 2; i++) {
    uint8_t c = readEEPROM_24C32(i);
    if (i > 0 && i % 16 == 0) {
      Serial.println();
    }
    if (c < 0x10) Serial.print('0');
    Serial.print(c, HEX);
    Serial.print(' ');
  }
  Serial.println(F("\n-------------------------------------------------"));
}

void loadModeWearLeveling() {
  int bestSlot = -1;
  uint8_t maxSeq = 0;
  bool found = false;

  for (int i = 0; i < NUM_SLOTS; i++) {
    uint8_t seq = readEEPROM_24C32(i * 2);
    uint8_t mode = readEEPROM_24C32(i * 2 + 1);

    if (mode == 1 || mode == 2) {
      if (!found) {
        maxSeq = seq;
        bestSlot = i;
        found = true;
      } else {
        if ((uint8_t)(seq - maxSeq) < 128 && seq != maxSeq) {
          maxSeq = seq;
          bestSlot = i;
        }
      }
    }
  }

  if (found) {
    displayMode = readEEPROM_24C32(bestSlot * 2 + 1);
    currentSeq = maxSeq;
    currentSlot = (bestSlot + 1) % NUM_SLOTS;
  } else {
    displayMode = 1;
    currentSeq = 1;
    currentSlot = 0;
    writeEEPROM_24C32(0, currentSeq);
    writeEEPROM_24C32(1, displayMode);
    currentSlot = 1;
  }
}

void saveModeWearLeveling(uint8_t newMode) {
  currentSeq++;
  writeEEPROM_24C32(currentSlot * 2, currentSeq);
  writeEEPROM_24C32(currentSlot * 2 + 1, newMode);
  currentSlot = (currentSlot + 1) % NUM_SLOTS;
}

void TaskClock(void *pvParameters);
void TaskKeypad(void *pvParameters);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  i2cMutex = xSemaphoreCreateMutex();

  xTaskCreate(TaskClock,  "ClockTask",  150, NULL, 1, NULL);
  xTaskCreate(TaskKeypad, "KeypadTask", 140, NULL, 2, NULL);

  vTaskStartScheduler();
}

void loop() {}

void printDateString() {
  lcd.print("D/M/Y=");
  int d = (editMode == NORMAL) ? currentRTC.day() : editDay;
  int m = (editMode == NORMAL) ? currentRTC.month() : editMonth;
  int y = (editMode == NORMAL) ? currentRTC.year() : editYear;

  if (d < 10) lcd.print('0');
  lcd.print(d);
  lcd.print('/');
  if (m < 10) lcd.print('0');
  lcd.print(m);
  lcd.print('/');
  lcd.print(y);
}

void printTimeString() {
  lcd.print("Time=");
  int h = (editMode == NORMAL) ? currentRTC.hour() : editHour;
  int mi = (editMode == NORMAL) ? currentRTC.minute() : editMin;

  if (h < 10) lcd.print('0');
  lcd.print(h);
  lcd.print(':');
  if (mi < 10) lcd.print('0');
  lcd.print(mi);

  if (editMode == NORMAL) {
    lcd.print(':');
    if (currentRTC.second() < 10) lcd.print('0');
    lcd.print(currentRTC.second());
    lcd.print("   ");
  } else if (editMode == SET_HOUR)  lcd.print(" SET H ");
  else if (editMode == SET_MIN)   lcd.print(" SET M ");
  else if (editMode == SET_DAY)   lcd.print(" SET D ");
  else if (editMode == SET_MONTH) lcd.print(" SET MO");
  else if (editMode == SET_YEAR)  lcd.print(" SET Y ");
}

void TaskClock(void *pvParameters) {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    if (!rtc.begin()) {
      lcd.print("RTC Fail!");
    }
    
    // สั่งให้ Oscillator ทำงานถ้ายังไม่เริ่มรัน
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    loadModeWearLeveling();
    
    xSemaphoreGive(i2cMutex);
  }

  for (;;) {
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      // ดึงเวลาจริงจาก RTC ตลอดเวลาเมื่ออยู่ในโหมดปกติ
      if (editMode == NORMAL) {
        currentRTC = rtc.now();
      }

      if (displayMode == 1) {
        lcd.setCursor(0, 0);
        printDateString();
        lcd.setCursor(0, 1);
        printTimeString();
      } else {
        lcd.setCursor(0, 0);
        printTimeString();
        lcd.setCursor(0, 1);
        printDateString();
      }

      xSemaphoreGive(i2cMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void TaskKeypad(void *pvParameters) {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    customKeypad.begin();
    xSemaphoreGive(i2cMutex);
  }

  for (;;) {
    char key = NO_KEY;
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      key = customKeypad.getKey();
      xSemaphoreGive(i2cMutex);
    }

    if (key != NO_KEY) {
      // ปุ่ม 0: Toggle Mode 1 <-> Mode 2
      if (key == '0' && editMode == NORMAL) {
        displayMode = (displayMode == 1) ? 2 : 1;
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
          saveModeWearLeveling(displayMode);
          lcd.clear();
          display_all_data_in_24C32();
          xSemaphoreGive(i2cMutex);
        }
      }
      // ปุ่ม A: เข้าโหมดตั้ง วัน -> เดือน -> ปี -> วัน
      else if (key == 'A') {
        if (editMode == NORMAL) {
          editHour  = currentRTC.hour();
          editMin   = currentRTC.minute();
          editDay   = currentRTC.day();
          editMonth = currentRTC.month();
          editYear  = currentRTC.year();
          editMode  = SET_DAY;
        } else if (editMode == SET_DAY) {
          editMode = SET_MONTH;
        } else if (editMode == SET_MONTH) {
          editMode = SET_YEAR;
        } else if (editMode == SET_YEAR) {
          editMode = SET_DAY;
        } else if (editMode == SET_HOUR || editMode == SET_MIN) {
          editMode = SET_DAY;
        }
      }
      // ปุ่ม B: เข้าโหมดตั้ง ชั่วโมง <-> นาที
      else if (key == 'B') {
        if (editMode == NORMAL) {
          editHour  = currentRTC.hour();
          editMin   = currentRTC.minute();
          editDay   = currentRTC.day();
          editMonth = currentRTC.month();
          editYear  = currentRTC.year();
          editMode  = SET_HOUR;
        } else if (editMode == SET_HOUR) {
          editMode = SET_MIN;
        } else if (editMode == SET_MIN) {
          editMode = SET_HOUR;
        } else if (editMode == SET_DAY || editMode == SET_MONTH || editMode == SET_YEAR) {
          editMode = SET_HOUR;
        }
      }
      // ปุ่ม C: เพิ่มค่า (+1)
      else if (key == 'C' && editMode != NORMAL) {
        if (editMode == SET_HOUR)       editHour = (editHour + 1) % 24;
        else if (editMode == SET_MIN)   editMin = (editMin + 1) % 60;
        else if (editMode == SET_DAY) {
          int maxD = getMaxDays(editYear, editMonth);
          editDay = (editDay >= maxD) ? 1 : editDay + 1;
        } 
        else if (editMode == SET_MONTH) {
          editMonth = (editMonth % 12) + 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD;
        } 
        else if (editMode == SET_YEAR) {
          editYear = (editYear >= 2099) ? 2000 : editYear + 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD;
        }
      }
      // ปุ่ม D: ลดค่า (-1)
      else if (key == 'D' && editMode != NORMAL) {
        if (editMode == SET_HOUR)       editHour = (editHour == 0) ? 23 : editHour - 1;
        else if (editMode == SET_MIN)   editMin = (editMin == 0) ? 59 : editMin - 1;
        else if (editMode == SET_DAY) {
          int maxD = getMaxDays(editYear, editMonth);
          editDay = (editDay <= 1) ? maxD : editDay - 1;
        } 
        else if (editMode == SET_MONTH) {
          editMonth = (editMonth <= 1) ? 12 : editMonth - 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD;
        } 
        else if (editMode == SET_YEAR) {
          editYear = (editYear <= 2000) ? 2099 : editYear - 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD;
        }
      }
      // ปุ่ม #: บันทึกเวลาลง RTC DS1307
      else if (key == '#') {
        if (editMode != NORMAL) {
          if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            rtc.adjust(DateTime(editYear, editMonth, editDay, editHour, editMin, 0));
            xSemaphoreGive(i2cMutex);
          }
          editMode = NORMAL;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}