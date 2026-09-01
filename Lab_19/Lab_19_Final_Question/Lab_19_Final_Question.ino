#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <RTClib.h>

// ประกาศ Mutex 2 ตัวสำหรับจัดการทรัพยากรที่ต่างกัน
SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t rtcMutex;

RTC_DS1307 rtc;

void TaskA_ReadRTC(void *pvParameters);
void TaskB_Keypad(void *pvParameters);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  // สร้าง Mutex ทั้ง 2 ตัว
  i2cMutex = xSemaphoreCreateMutex();
  rtcMutex = xSemaphoreCreateMutex();

  // สร้าง Task ทั้ง 2 ตัวโดยให้มี Priority เท่ากัน
  xTaskCreate(TaskA_ReadRTC, "TaskA", 140, NULL, 1, NULL);
  xTaskCreate(TaskB_Keypad,  "TaskB", 140, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop() {}

// Task A: ถือ rtcMutex ก่อน แล้วพยายามขอ i2cMutex
void TaskA_ReadRTC(void *pvParameters) {
  for (;;) {
    Serial.println(F("[Task A] Attempting to lock rtcMutex..."));
    if (xSemaphoreTake(rtcMutex, portMAX_DELAY) == pdTRUE) {
      Serial.println(F("[Task A] ---> LOCKED rtcMutex"));

      // ดีเลย์หน่วงเวลาสั้นๆ เพื่อเปิดโอกาสให้ Context Switch ไปยัง Task B
      vTaskDelay(pdMS_TO_TICKS(50));

      Serial.println(F("[Task A] Attempting to lock i2cMutex (Waiting)..."));
      // รอขอ i2cMutex ซึ่งถูก Task B ถือครองอยู่
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        DateTime now = rtc.now();
        xSemaphoreGive(i2cMutex);
      }

      xSemaphoreGive(rtcMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Task B: ถือ i2cMutex ก่อน แล้วพยายามขอ rtcMutex (ลำดับตรงกันข้ามกับ Task A)
void TaskB_Keypad(void *pvParameters) {
  for (;;) {
    Serial.println(F("[Task B] Attempting to lock i2cMutex..."));
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      Serial.println(F("[Task B] ---> LOCKED i2cMutex"));

      // ดีเลย์เพื่อรอให้ Task A เข้ามาล็อค rtcMutex ให้สำเร็จก่อน
      vTaskDelay(pdMS_TO_TICKS(50));

      Serial.println(F("[Task B] Attempting to lock rtcMutex (Waiting)..."));
      // รอขอ rtcMutex ซึ่งถูก Task A ถือครองอยู่
      if (xSemaphoreTake(rtcMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println(F("[Task B] Accessed RTC Structure"));
        xSemaphoreGive(rtcMutex);
      }

      xSemaphoreGive(i2cMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}