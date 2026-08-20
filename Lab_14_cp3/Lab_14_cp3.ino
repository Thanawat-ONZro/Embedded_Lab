#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

unsigned long blinkTime = 0;
bool blinkState = true;

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

// 0 = Normal
// 1 = Hour
// 2 = Minute
// 3 = Day
// 4 = Month
// 5 = Year


bool isLeapYear(int year)
{
  if (year % 400 == 0)
    return true;

  if (year % 100 == 0)
    return false;

  if (year % 4 == 0)
    return true;

  return false;
}


byte daysInMonth(byte month, int year)
{
  if (month == 2)
  {
    if (isLeapYear(year))
      return 29;
    else
      return 28;
  }

  if (month == 4 ||
      month == 6 ||
      month == 9 ||
      month == 11)
  {
    return 30;
  }

  return 31;
}


void checkDay()
{
  int year = tmYearToCalendar(tm.Year);
  byte maxDay = daysInMonth(tm.Month, year);

  if (tm.Day > maxDay)
    tm.Day = maxDay;

  if (tm.Day < 1)
    tm.Day = 1;
}


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

  if (mode == 3 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Day);

  lcd.print("/");

  if (mode == 4 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Month);

  lcd.print("/");

  if (mode == 5 && !blinkState)
    lcd.print("    ");
  else
    lcd.print(tmYearToCalendar(tm.Year));


  lcd.setCursor(0, 1);

  lcd.print("Time = ");

  if (mode == 1 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Hour);

  lcd.print(":");

  if (mode == 2 && !blinkState)
    lcd.print("  ");
  else
    print2digits(tm.Minute);

  lcd.print("   ");
}


void setup()
{
  Serial.begin(38400);

  Wire.begin();
  keypad.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  RTC.read(tm);
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

    else if (key == 'A')
    {
      mode = 3;

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


    else if (key == 'A')
    {
      if (mode == 3)
        mode = 4;

      else if (mode == 4)
        mode = 5;

      else
        mode = 3;

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


      else if (mode == 3)
      {
        int year = tmYearToCalendar(tm.Year);
        byte maxDay = daysInMonth(tm.Month, year);

        tm.Day++;

        if (tm.Day > maxDay)
          tm.Day = 1;
      }


      else if (mode == 4)
      {
        tm.Month++;

        if (tm.Month > 12)
          tm.Month = 1;

        checkDay();
      }


      else if (mode == 5)
      {
        int year = tmYearToCalendar(tm.Year);

        year++;

        if (year > 2099)
          year = 2000;

        tm.Year = CalendarYrToTm(year);

        checkDay();
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


      else if (mode == 3)
      {
        int year = tmYearToCalendar(tm.Year);
        byte maxDay = daysInMonth(tm.Month, year);

        if (tm.Day <= 1)
          tm.Day = maxDay;
        else
          tm.Day--;
      }


      else if (mode == 4)
      {
        if (tm.Month <= 1)
          tm.Month = 12;
        else
          tm.Month--;

        checkDay();
      }


      else if (mode == 5)
      {
        int year = tmYearToCalendar(tm.Year);

        year--;

        if (year < 2000)
          year = 2099;

        tm.Year = CalendarYrToTm(year);

        checkDay();
      }
    }


    else if (key == '#')
    {
      RTC.write(tm);

      mode = 0;

      blinkState = true;
      blinkTime = millis();
    }
  }


  showClock();

  delay(100);
}