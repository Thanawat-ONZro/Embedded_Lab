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

// Mutex สำหรับป้องกันการแย่งใช้บัส I2C พร้อมกัน
SemaphoreHandle_t i2cMutex;

// สถานะการตั้งค่าเวลา
enum EditState { NORMAL, SET_HOUR, SET_MIN };
EditState editMode = NORMAL;

int editHour = 0;
int editMin = 0;
DateTime currentRTC;

void TaskClock(void *pvParameters);
void TaskKeypad(void *pvParameters);

void setup() {
  Wire.begin();
  
  // สร้าง Mutex จัดการคิวบัส I2C
  i2cMutex = xSemaphoreCreateMutex();

  // สร้าง Task แสดงผลและ Task รับค่าปุ่มกด
  xTaskCreate(TaskClock,  "ClockTask",  140, NULL, 1, NULL);
  xTaskCreate(TaskKeypad, "KeypadTask", 140, NULL, 2, NULL);

  vTaskStartScheduler();
}

void loop() {}

// Task จัดการ LCD และอ่านค่า RTC
void TaskClock(void *pvParameters) {
  // เริ่มต้น Hardware หลัง RTOS Scheduler เริ่มทำงาน
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
      // ดึงเวลาปัจจุบันเฉพาะตอนไม่ได้ตั้งค่า
      if (editMode == NORMAL) {
        currentRTC = rtc.now();
        editHour = currentRTC.hour();
        editMin  = currentRTC.minute();
      }

      // แถวที่ 1: แสดง D/M/Y
      lcd.setCursor(0, 0);
      lcd.print("D/M/Y=");
      if (currentRTC.day() < 10) lcd.print('0');
      lcd.print(currentRTC.day());
      lcd.print('/');
      if (currentRTC.month() < 10) lcd.print('0');
      lcd.print(currentRTC.month());
      lcd.print('/');
      lcd.print(currentRTC.year());

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
        lcd.print(" SET H");
      } else if (editMode == SET_MIN) {
        lcd.print(" SET M");
      }

      xSemaphoreGive(i2cMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// Task สแกน Keypad และคำนวณการตั้งเวลา
void TaskKeypad(void *pvParameters) {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
    customKeypad.begin();
    xSemaphoreGive(i2cMutex);
  }

  for (;;) {
    char key = NO_KEY;
    
    // อ่านค่าปุ่มกดผ่านบัส I2C
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      key = customKeypad.getKey();
      xSemaphoreGive(i2cMutex);
    }

    if (key != NO_KEY) {
      // ปุ่ม B: สลับโหมดตั้งค่า (ชั่วโมง <-> นาที)
      if (key == 'B') {
        if (editMode == NORMAL) {
          editMode = SET_HOUR;
        } else if (editMode == SET_HOUR) {
          editMode = SET_MIN;
        } else if (editMode == SET_MIN) {
          editMode = SET_HOUR;
        }
      } 
      // ปุ่ม C: เพิ่มค่าเวลา (+1)
      else if (key == 'C' && editMode != NORMAL) {
        if (editMode == SET_HOUR) {
          editHour = (editHour + 1) % 24;
        } else if (editMode == SET_MIN) {
          editMin = (editMin + 1) % 60;
        }
      } 
      // ปุ่ม D: ลดค่าเวลา (-1)
      else if (key == 'D' && editMode != NORMAL) {
        if (editMode == SET_HOUR) {
          editHour = (editHour == 0) ? 23 : editHour - 1;
        } else if (editMode == SET_MIN) {
          editMin = (editMin == 0) ? 59 : editMin - 1;
        }
      } 
      // ปุ่ม #: บันทึกเวลาลง RTC DS1307 และกลับสู่โหมดปกติ
      else if (key == '#') {
        if (editMode != NORMAL) {
          if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            rtc.adjust(DateTime(currentRTC.year(), currentRTC.month(), currentRTC.day(), editHour, editMin, 0));
            xSemaphoreGive(i2cMutex);
          }
          editMode = NORMAL;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}