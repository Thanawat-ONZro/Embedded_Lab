  #include <Arduino_FreeRTOS.h>

  #define PUSH_SW 2
  #define R_LED 3
  #define G_LED 7
  #define Y_LED 5
  #define O_LED 6
  #define POTEN A0

  volatile byte mode = 1;
  volatile uint16_t delayTime = 500;

  const byte mode1[] = {
    0b0001,
    0b0010,
    0b0100,
    0b1000,
    0b0000
  };

  const byte mode2[] = {
    0b1000,
    0b0100,
    0b0010,
    0b0001,
    0b0000
  };

  const byte mode3[] = {
    0b0000,
    0b0001,
    0b0011,
    0b0111,
    0b1111,
    0b0111,
    0b0011,
    0b0001
  };

  const byte mode4[] = {
    0b0000,
    0b1000,
    0b1100,
    0b1110,
    0b1111,
    0b1110,
    0b1100,
    0b1000
  };

  void Input_Task(void *pvParameters);
  void LED_Task(void *pvParameters);
  void setLED(byte data);

  void setup()
  {
    Serial.begin(9600);

    pinMode(PUSH_SW, INPUT);

    pinMode(R_LED, OUTPUT);
    pinMode(G_LED, OUTPUT);
    pinMode(Y_LED, OUTPUT);
    pinMode(O_LED, OUTPUT);

    digitalWrite(R_LED, LOW);
    digitalWrite(G_LED, LOW);
    digitalWrite(Y_LED, LOW);
    digitalWrite(O_LED, LOW);

    xTaskCreate(
      Input_Task,
      "Input",
      192,
      NULL,
      1,
      NULL
    );

    xTaskCreate(
      LED_Task,
      "LED",
      192,
      NULL,
      1,
      NULL
    );

    vTaskStartScheduler();
  }

  void setLED(byte data)
  {
    digitalWrite(R_LED, (data & 0b0001) ? HIGH : LOW);
    digitalWrite(G_LED, (data & 0b0010) ? HIGH : LOW);
    digitalWrite(Y_LED, (data & 0b0100) ? HIGH : LOW);
    digitalWrite(O_LED, (data & 0b1000) ? HIGH : LOW);
  }

  void Input_Task(void *pvParameters)
  {
    int lastSwitch = HIGH;
    unsigned long lastPress = 0;

    while (1)
    {
      int sw = digitalRead(PUSH_SW);

      if (lastSwitch == HIGH && sw == LOW)
      {
        if (millis() - lastPress > 150)
        {
          lastPress = millis();

          mode++;

          if (mode > 4)
          {
            mode = 1;
          }

          Serial.print("MODE = ");
          Serial.println(mode);
        }
      }

      lastSwitch = sw;

      int poten = analogRead(POTEN);

      int level = map(poten, 0, 1023, 0, 7);
      level = constrain(level, 0, 7);

      uint16_t newDelay = map(
        level,
        0,
        7,
        1500,
        50
      );

      delayTime = newDelay;

      Serial.print("POTEN = ");
      Serial.print(poten);

      Serial.print(" | LEVEL = ");
      Serial.print(level + 1);

      Serial.print(" | DELAY = ");
      Serial.print(delayTime);

      Serial.print(" ms | MODE = ");
      Serial.println(mode);

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  void LED_Task(void *pvParameters)
  {
    byte stepIndex = 0;
    byte oldMode = mode;

    while (1)
    {
      byte currentMode = mode;

      if (currentMode != oldMode)
      {
        stepIndex = 0;
        oldMode = currentMode;
      }

      byte data;
      byte length;

      if (currentMode == 1)
      {
        length = 5;
        data = mode1[stepIndex];
      }
      else if (currentMode == 2)
      {
        length = 5;
        data = mode2[stepIndex];
      }
      else if (currentMode == 3)
      {
        length = 8;
        data = mode3[stepIndex];
      }
      else
      {
        length = 8;
        data = mode4[stepIndex];
      }

      setLED(data);

      stepIndex++;

      if (stepIndex >= length)
      {
        stepIndex = 0;
      }

      uint16_t waitTime = delayTime;

      vTaskDelay(pdMS_TO_TICKS(waitTime));
    }
  }

  void loop()
  {
  }