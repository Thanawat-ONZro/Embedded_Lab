  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #include <TimeLib.h>
  #include <DS1307RTC.h>

  LiquidCrystal_I2C lcd(0x27, 16, 2);
  tmElements_t tm;

  void print2digits(int num)
  {
    if (num < 10) lcd.print('0');
    lcd.print(num);
  }

  void setup()
  {
    Wire.begin();

    lcd.init();
    lcd.backlight();
    lcd.clear();
  }

  void loop()
  {
    if (RTC.read(tm))
    {
      lcd.setCursor(0, 0);
      lcd.print("Time: ");

      print2digits(tm.Hour);
      lcd.print(":");
      print2digits(tm.Minute);
      lcd.print(":");
      print2digits(tm.Second);
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("RTC Error       ");
    }

    delay(200);
  }