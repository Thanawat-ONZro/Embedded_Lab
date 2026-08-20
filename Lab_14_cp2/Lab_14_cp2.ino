#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {7, 6, 5, 4};
byte colPins[COLS] = {3, 2, 1, 0};

Keypad_I2C keypad(
  makeKeymap(hexaKeys),
  rowPins,
  colPins,
  ROWS,
  COLS,
  0x20
);

LiquidCrystal_I2C lcd(0x27, 16, 2);

tmElements_t tm;

byte mode = 0;

unsigned long blinkTime = 0;
bool blinkState = true;

void print2digits(int num)
{
  if (num < 10)
    lcd.print('0');

  lcd.print(num);
}

void showClock()
{
  if (millis() - blinkTime >= 1000)
  {
    blinkTime = millis();
    blinkState = !blinkState;
  }

  lcd.setCursor(0, 0);

  lcd.print("D/M/Y=");
  print2digits(tm.Day);
  lcd.print("/");
  print2digits(tm.Month);
  lcd.print("/");
  lcd.print(tmYearToCalendar(tm.Year));

  lcd.setCursor(0, 1);

  lcd.print("Time = ");

  // mode 1 = Hour กระพริบ
  if (mode == 1 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Hour);

  lcd.print(":");

  // mode 2 = Minute กระพริบ
  if (mode == 2 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Minute);
}

void setup()
{
  Serial.begin(38400);

  Wire.begin();
  keypad.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  if (!RTC.read(tm))
  {
    lcd.setCursor(0, 0);
    lcd.print("RTC Error");
  }
}

void loop()
{
  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    Serial.print("Key: ");
    Serial.println(key);
  }

  if (mode == 0)
  {
    RTC.read(tm);

    if (key == 'B')
    {
      mode = 1;
      blinkState = true;
      blinkTime = millis();
    }
  }
  else
  {
    if (key == 'B')
    {
      if (mode == 1)
        mode = 2;
      else
        mode = 1;

      blinkState = true;
      blinkTime = millis();
    }

    else if (key == 'C')
    {
      if (mode == 1)
      {
        tm.Hour++;

        if (tm.Hour > 23)
          tm.Hour = 0;
      }

      else if (mode == 2)
      {
        tm.Minute++;

        if (tm.Minute > 59)
          tm.Minute = 0;
      }
    }

    else if (key == 'D')
    {
      if (mode == 1)
      {
        if (tm.Hour == 0)
          tm.Hour = 23;
        else
          tm.Hour--;
      }

      else if (mode == 2)
      {
        if (tm.Minute == 0)
          tm.Minute = 59;
        else
          tm.Minute--;
      }
    }

    else if (key == '#')
    {
      tm.Second = 0;
      RTC.write(tm);
      mode = 0;
    }
  }

  showClock();

  delay(50);
}