#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>

#define LCD_ADDR   0x27
#define PCF_ADDR   0x20

RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ตั้งค่าปุ่มและพินบน PCF8574
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

// สถานะการตั้งค่า
enum EditState { NORMAL, SET_HOUR, SET_MIN, SET_DAY, SET_MONTH, SET_YEAR };
EditState editMode = NORMAL;

// ตัวแปรพักค่าสำหรับการแก้ไข
int editHour = 0;
int editMin = 0;
int editDay = 1;
int editMonth = 1;
int editYear = 2026;
DateTime currentRTC;

// ฟังก์ชันหาจำนวนวันสูงสุดของเดือนนั้นๆ (รองรับปีอธิกสุรทิน / Leap Year)
int getMaxDays(int y, int m) {
  if (m == 2) {
    bool isLeap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    return isLeap ? 29 : 28;
  }
  if (m == 4 || m == 6 || m == 9 || m == 11) {
    return 30;
  }
  return 31;
}

void TaskClock(void *pvParameters);
void TaskKeypad(void *pvParameters);

void setup() {
  Wire.begin();
  
  i2cMutex = xSemaphoreCreateMutex();

  xTaskCreate(TaskClock,  "ClockTask",  140, NULL, 1, NULL);
  xTaskCreate(TaskKeypad, "KeypadTask", 140, NULL, 2, NULL);

  vTaskStartScheduler();
}

void loop() {}

// --- Task 1: แสดงผล LCD และอ่านค่า RTC ---
void TaskClock(void *pvParameters) {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    if (!rtc.begin()) {
      lcd.print("RTC Error!");
    }
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    xSemaphoreGive(i2cMutex);
  }

  for (;;) {
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      if (editMode == NORMAL) {
        currentRTC = rtc.now();
        editHour  = currentRTC.hour();
        editMin   = currentRTC.minute();
        editDay   = currentRTC.day();
        editMonth = currentRTC.month();
        editYear  = currentRTC.year();
      }

      // แถวที่ 1: แสดง D/M/Y
      lcd.setCursor(0, 0);
      lcd.print("D/M/Y=");
      if (editDay < 10) lcd.print('0');
      lcd.print(editDay);
      lcd.print('/');
      if (editMonth < 10) lcd.print('0');
      lcd.print(editMonth);
      lcd.print('/');
      lcd.print(editYear);

      // แถวที่ 2: แสดง Time และสถานะโหมดตั้งค่า
      lcd.setCursor(0, 1);
      lcd.print("Time=");
      if (editHour < 10) lcd.print('0');
      lcd.print(editHour);
      lcd.print(':');
      if (editMin < 10) lcd.print('0');
      lcd.print(editMin);

      if (editMode == NORMAL) {
        lcd.print(':');
        if (currentRTC.second() < 10) lcd.print('0');
        lcd.print(currentRTC.second());
        lcd.print("   ");
      } else if (editMode == SET_HOUR) {
        lcd.print(" SET H ");
      } else if (editMode == SET_MIN) {
        lcd.print(" SET M ");
      } else if (editMode == SET_DAY) {
        lcd.print(" SET D ");
      } else if (editMode == SET_MONTH) {
        lcd.print(" SET MO");
      } else if (editMode == SET_YEAR) {
        lcd.print(" SET Y ");
      }

      xSemaphoreGive(i2cMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// --- Task 2: สแกน Keypad และประมวลผลการกดปุ่ม ---
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
      // ปุ่ม A: โหมดตั้งค่า วัน -> เดือน -> ปี -> วัน
      if (key == 'A') {
        if (editMode == NORMAL || editMode == SET_HOUR || editMode == SET_MIN) {
          editMode = SET_DAY;
        } else if (editMode == SET_DAY) {
          editMode = SET_MONTH;
        } else if (editMode == SET_MONTH) {
          editMode = SET_YEAR;
        } else if (editMode == SET_YEAR) {
          editMode = SET_DAY;
        }
      }
      // ปุ่ม B: โหมดตั้งค่า ชั่วโมง <-> นาที
      else if (key == 'B') {
        if (editMode == NORMAL || editMode == SET_DAY || editMode == SET_MONTH || editMode == SET_YEAR) {
          editMode = SET_HOUR;
        } else if (editMode == SET_HOUR) {
          editMode = SET_MIN;
        } else if (editMode == SET_MIN) {
          editMode = SET_HOUR;
        }
      }
      // ปุ่ม C: เพิ่มค่า (+1)
      else if (key == 'C' && editMode != NORMAL) {
        if (editMode == SET_HOUR) {
          editHour = (editHour + 1) % 24;
        } 
        else if (editMode == SET_MIN) {
          editMin = (editMin + 1) % 60;
        } 
        else if (editMode == SET_DAY) {
          int maxD = getMaxDays(editYear, editMonth);
          editDay = (editDay >= maxD) ? 1 : editDay + 1;
        } 
        else if (editMode == SET_MONTH) {
          editMonth = (editMonth % 12) + 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD; // ปรับวันลงหากเดือนใหม่มีวันน้อยกว่า
        } 
        else if (editMode == SET_YEAR) {
          editYear = (editYear >= 2099) ? 2000 : editYear + 1;
          int maxD = getMaxDays(editYear, editMonth);
          if (editDay > maxD) editDay = maxD; // ปรับวันลงหากเปลี่ยนปีแล้วไม่ใช่ Leap year (เช่น 29 ก.พ.)
        }
      }
      // ปุ่ม D: ลดค่า (-1)
      else if (key == 'D' && editMode != NORMAL) {
        if (editMode == SET_HOUR) {
          editHour = (editHour == 0) ? 23 : editHour - 1;
        } 
        else if (editMode == SET_MIN) {
          editMin = (editMin == 0) ? 59 : editMin - 1;
        } 
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
      // ปุ่ม #: บันทึกค่าลง DS1307
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