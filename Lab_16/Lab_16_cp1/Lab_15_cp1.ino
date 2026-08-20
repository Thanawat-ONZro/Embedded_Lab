#include <Arduino_FreeRTOS.h>

#define R_LED 3
#define G_LED 7
#define Y_LED 5
#define PUSH_SW 2

int display = 0;

void setup()
{
  xTaskCreate(Read_Switch, "Read Switch", 128, NULL, 1, NULL);  
  xTaskCreate(Display_R_LED, "Red LED", 128, NULL, 1, NULL);
  xTaskCreate(Display_G_LED, "Green LED", 128, NULL, 1, NULL);
  xTaskCreate(Display_Y_LED, "Yellow LED", 128, NULL, 1, NULL);

  vTaskStartScheduler();
}

void Display_R_LED(void *pvParameters)
{
  pinMode(R_LED, OUTPUT);

  while (1)
  {
    if (display == 1)
      digitalWrite(R_LED, HIGH);
    else
      digitalWrite(R_LED, LOW);
  }
}

void Display_G_LED(void *pvParameters)
{
  pinMode(G_LED, OUTPUT);

  while (1)
  {
    if (display == 2)
      digitalWrite(G_LED, HIGH);
    else
      digitalWrite(G_LED, LOW);
  }
}

void Display_Y_LED(void *pvParameters)
{
  pinMode(Y_LED, OUTPUT);

  while (1)
  {
    if (display == 3)
      digitalWrite(Y_LED, HIGH);
    else
      digitalWrite(Y_LED, LOW);
  }
}

void Read_Switch(void *pvParameters)
{
  pinMode(PUSH_SW, INPUT);

  while (1)
  {
    int sw_status = digitalRead(PUSH_SW);

    if (sw_status == LOW)
    {
      vTaskDelay(pdMS_TO_TICKS(10));

      sw_status = digitalRead(PUSH_SW);

      if (sw_status == LOW)
      {
        while (sw_status == LOW)
        {
          sw_status = digitalRead(PUSH_SW);
        }

        display++;

        if (display > 3)
          display = 0;
      }
    }
  }
}

void loop()
{

}