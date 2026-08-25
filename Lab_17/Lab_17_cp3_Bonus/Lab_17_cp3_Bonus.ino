#include <Wire.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <TM1638.h>
#include "my_EEPROM.h"

// ---------- TM1638 ----------
#define CLK 3
#define DIO 2
#define STB 4
TM1638 tm(CLK, DIO, STB);

// ---------- Keypad ----------
const byte ROWS = 4, COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {7,6,5,4};
byte colPins[COLS] = {3,2,1,0};

Keypad_I2C keypad(
  makeKeymap(keys),
  rowPins, colPins,
  ROWS, COLS,
  0x20
);

// ---------- State ----------
#define ST1 1
#define ST2 2
#define ST3 3
#define ST4 4
#define ST5 5

byte state = ST1;
byte phase = 0;

uint16_t addr = 0;
uint8_t data = 0;

byte addrCount = 0;
byte dataCount = 0;


// =====================================================
// DISPLAY
// =====================================================

void showAddr()
{
  tm.displayVal(7, (addr / 1000) % 10);
  tm.displayVal(6, (addr / 100) % 10);
  tm.displayVal(5, (addr / 10) % 10);
  tm.displayVal(4, addr % 10);
}

void showData()
{
  tm.displayVal(1, data >> 4);
  tm.displayVal(0, data & 0x0F);
}


// =====================================================
// KEYPAD
// =====================================================

// แปลง 0-9,A-D,* ,# เป็น 0-F
int hexValue(char k)
{
  if (k >= '0' && k <= '9') return k - '0';
  if (k >= 'A' && k <= 'D') return k - 'A' + 10;
  if (k == '*') return 14;   // E
  if (k == '#') return 15;   // F

  return -1;
}


// รับ Address ฐานสิบ
void readAddr(char k)
{
  if (k >= '0' && k <= '9' && addrCount < 4)
  {
    uint16_t temp = addr * 10 + (k - '0');

    if (temp <= 1023)
    {
      addr = temp;
      addrCount++;
      showAddr();
    }
  }
}


// รับ HEX 2 หลัก
bool readData(char k)
{
  int v = hexValue(k);

  if (v >= 0 && dataCount < 2)
  {
    data = (data << 4) | v;
    dataCount++;

    showAddr();
    showData();
  }

  return dataCount == 2;
}


// =====================================================
// RESET STATE
// =====================================================

void goST1()
{
  state = ST1;
  phase = 0;

  addr = 0;
  data = 0;

  addrCount = 0;
  dataCount = 0;

  tm.displayClear();
}


// =====================================================
// เริ่ม State
// =====================================================

void startState(byte newState)
{
  state = newState;
  phase = 0;

  addr = 0;
  data = 0;

  addrCount = 0;
  dataCount = 0;

  tm.displayClear();
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(38400);

  Wire.begin();
  keypad.begin();

  tm.reset();
  tm.displayClear();

  Serial.println("LAB17 CHECKPOINT 3");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  char key = keypad.getKey();


  // ===================================================
  // ST1 : เลือกงาน
  // ===================================================

  if (state == ST1)
  {
    tm.displayClear();

    if (tm.getButton(S1))
    {
      startState(ST2);      // เขียน 1 byte
      delay(200);
    }

    else if (tm.getButton(S2))
    {
      startState(ST3);      // เขียน 16 byte
      delay(200);
    }

    else if (tm.getButton(S3))
    {
      startState(ST4);      // เขียนต่อเนื่อง
      delay(200);
    }

    else if (tm.getButton(S4))
    {
      startState(ST5);      // ลบหลาย byte
      delay(200);
    }

    else if (tm.getButton(S8))
    {
      display_all_data_in_EEPROM();
      delay(200);
    }
  }


  // ===================================================
  // ST2 : เขียน 1 byte
  // ===================================================

  else if (state == ST2)
  {
    if (tm.getButton(S6))
    {
      goST1();
      delay(200);
      return;
    }

    // รับ Address
    if (phase == 0)
    {
      readAddr(key);

      if (tm.getButton(S8) && addrCount > 0)
      {
        phase = 1;
        data = 0;
        dataCount = 0;

        delay(200);
      }
    }

    // รับ Data
    else
    {
      readData(key);

      if (tm.getButton(S1) && dataCount == 2)
      {
        update_if_data_changed(addr, data);

        Serial.print("Write [");
        Serial.print(addr);
        Serial.print("] = ");
        Serial.println(data, HEX);

        goST1();
        delay(200);
      }
    }
  }


  // ===================================================
  // ST3 : เขียนค่าเดียว 16 byte
  // ===================================================

  else if (state == ST3)
  {
    if (tm.getButton(S6))
    {
      goST1();
      delay(200);
      return;
    }

    if (phase == 0)
    {
      readAddr(key);

      if (tm.getButton(S8) && addrCount > 0)
      {
        phase = 1;
        data = 0;
        dataCount = 0;

        delay(200);
      }
    }

    else
    {
      readData(key);

      if (tm.getButton(S8) && dataCount == 2)
      {
        for (byte i = 0; i < 16; i++)
        {
          if (addr + i <= 1023)
            update_if_data_changed(addr + i, data);
        }

        Serial.println("Write 16 bytes complete");

        goST1();
        delay(200);
      }
    }
  }


  // ===================================================
  // ST4 : เขียนข้อมูลต่อเนื่อง
  // ===================================================

  else if (state == ST4)
  {
    if (tm.getButton(S6))
    {
      goST1();
      delay(200);
      return;
    }

    if (phase == 0)
    {
      readAddr(key);

      if (tm.getButton(S8) && addrCount > 0)
      {
        phase = 1;
        data = 0;
        dataCount = 0;

        delay(200);
      }
    }

    else
    {
      if (readData(key))
      {
        update_if_data_changed(addr, data);

        Serial.print("Write [");
        Serial.print(addr);
        Serial.print("] = ");
        Serial.println(data, HEX);

        addr++;

        data = 0;
        dataCount = 0;

        showAddr();
      }
    }
  }


  // ===================================================
  // ST5 : ลบหลายตำแหน่ง
  // ===================================================

  else if (state == ST5)
  {
    static byte count = 0;
    static byte countDigit = 0;

    if (tm.getButton(S6))
    {
      count = 0;
      countDigit = 0;

      goST1();
      delay(200);
      return;
    }

    // รับ Address
    if (phase == 0)
    {
      readAddr(key);

      if (tm.getButton(S8) && addrCount > 0)
      {
        phase = 1;

        count = 0;
        countDigit = 0;

        delay(200);
      }
    }

    // รับจำนวนที่จะลบ
    else
    {
      if (key >= '0' && key <= '9' && countDigit < 2)
      {
        count = count * 10 + (key - '0');
        countDigit++;

        tm.displayVal(1, count / 10);
        tm.displayVal(0, count % 10);
      }

      // S1 ยืนยันลบ
      if (tm.getButton(S1) && countDigit > 0)
      {
        for (byte i = 0; i < count; i++)
        {
          if (addr + i <= 1023)
            EEPROM_Erase_only(addr + i);
        }

        Serial.println("Erase complete");

        count = 0;
        countDigit = 0;

        goST1();
        delay(200);
      }
    }
  }
}