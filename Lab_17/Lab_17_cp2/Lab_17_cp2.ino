#include <Wire.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <TM1638.h>
#include "my_EEPROM.h"

#define CLK 3
#define DIO 2
#define STB 4

TM1638 tm(CLK, DIO, STB);

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] =
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {7, 6, 5, 4};
byte colPins[COLS] = {3, 2, 1, 0};

Keypad_I2C keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS,
  0x20
);

#define ST1 1
#define ST2 2
#define ST3 3

byte state = ST1;

uint16_t address = 0;
uint8_t data = 0;

byte addressCount = 0;
byte dataCount = 0;

unsigned long blinkTime = 0;
bool blinkState = true;

void showAddress()
{
  tm.displayVal(7, (address / 1000) % 10);
  tm.displayVal(6, (address / 100) % 10);
  tm.displayVal(5, (address / 10) % 10);
  tm.displayVal(4, address % 10);
}


void showData()
{
  tm.displayVal(1, (data >> 4) & 0x0F);
  tm.displayVal(0, data & 0x0F);
}


// =====================================================
// แปลง keypad เป็นค่า HEX
// =====================================================

int hexValue(char key)
{
  if (key >= '0' && key <= '9')
    return key - '0';

  if (key >= 'A' && key <= 'D')
    return key - 'A' + 10;

  if (key == '*')
    return 14;    // E

  if (key == '#')
    return 15;    // F

  return -1;
}


// =====================================================
// จัดการกระพริบ
// =====================================================

void updateBlink()
{
  if (millis() - blinkTime >= 1000)
  {
    blinkTime = millis();
    blinkState = !blinkState;
  }
}


// =====================================================
// setup
// =====================================================

void setup()
{
  Serial.begin(38400);

  Wire.begin();
  keypad.begin();

  tm.reset();
  tm.displayClear();

  Serial.println("==============================");
  Serial.println("LAB17 - Checkpoint 2");
  Serial.println("==============================");

  Serial.println("ST1");
}


// =====================================================
// loop
// =====================================================

void loop()
{
  char key = keypad.getKey();

  updateBlink();


  // ===================================================
  // ST1
  // ===================================================

  if (state == ST1)
  {
    tm.displayClear();


    // S8 -> เริ่มรับ Address
    if (tm.getButton(S8))
    {
      address = 0;
      addressCount = 0;

      blinkState = true;
      blinkTime = millis();

      state = ST2;

      Serial.println();
      Serial.println("ST2 : Enter Address");

      delay(200);
    }


    // S1 -> แสดง EEPROM ทั้งหมด
    if (tm.getButton(S1))
    {
      Serial.println();
      Serial.println("EEPROM DATA:");

      display_all_data_in_EEPROM();

      delay(200);
    }
  }


  // ===================================================
  // ST2 : รับ Address
  // ===================================================

  else if (state == ST2)
  {
    if (blinkState)
      showAddress();
    else
      tm.displayClear();


    // รับเฉพาะเลข 0-9
    if (key >= '0' && key <= '9')
    {
      if (addressCount < 4)
      {
        uint16_t newAddress =
          address * 10 + (key - '0');

        if (newAddress <= 1023)
        {
          address = newAddress;
          addressCount++;

          Serial.print("Address = ");
          Serial.println(address);
        }
      }
    }


    // S8 -> จบ Address แล้วไป ST3
    if (tm.getButton(S8) && addressCount > 0)
    {
      data = 0;
      dataCount = 0;

      blinkState = true;
      blinkTime = millis();

      tm.displayClear();

      state = ST3;

      Serial.println();
      Serial.println("ST3 : Enter HEX Data");

      delay(200);
    }
  }


  // ===================================================
  // ST3 : รับ Data
  // ===================================================

  else if (state == ST3)
  {
    if (blinkState)
      showData();
    else
      tm.displayClear();


    int value = hexValue(key);

    if (value >= 0 && dataCount < 2)
    {
      data = (data << 4) | value;
      dataCount++;

      Serial.print("Data = 0x");

      if (data < 0x10)
        Serial.print("0");

      Serial.println(data, HEX);
    }


    // S1 -> เขียน EEPROM
    if (tm.getButton(S1) && dataCount == 2)
    {
      update_if_data_changed(address, data);

      Serial.println();
      Serial.print("EEPROM[");
      Serial.print(address);
      Serial.print("] = 0x");

      if (data < 0x10)
        Serial.print("0");

      Serial.println(data, HEX);

      tm.displayClear();

      state = ST1;

      Serial.println("Back to ST1");

      delay(200);
    }
  }
}