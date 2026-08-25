#include "my_EEPROM.h"

void writeFACE(uint16_t addr)
{
  EEPROM_Write_to_Empty_location(addr,     0xFA);
  EEPROM_Write_to_Empty_location(addr + 1, 0xCE);
  EEPROM_Write_to_Empty_location(addr + 2, 0x0F);
}

void eraseFACE(uint16_t addr)
{
  EEPROM_Erase_only(addr);
  EEPROM_Erase_only(addr + 1);
  EEPROM_Erase_only(addr + 2);
}

int findFACE()
{
  for (int i = 0; i <= 1021; i++)
  {
    if (EEPROM_read1byte(i)     == 0xFA &&
        EEPROM_read1byte(i + 1) == 0xCE &&
        EEPROM_read1byte(i + 2) == 0x0F)
    {
      return i;
    }
  }

  return -1;
}

void setup()
{
  Serial.begin(38400);

  int addr = findFACE();

  // ยังไม่มี FACE0F
  if (addr == -1)
  {
    // ล้างข้อมูลเก่าทั้งหมดก่อน
    for (int i = 0; i < 1024; i++)
    {
      if (EEPROM_read1byte(i) != 0xFF)
        EEPROM_Erase_only(i);
    }

    writeFACE(1021);
    addr = 1021;
  }

  // มี FACE0F อยู่แล้ว
  else if (addr >= 3)
  {
    eraseFACE(addr);
    addr -= 3;
    writeFACE(addr);
  }

  Serial.print("FACE0F at ");
  Serial.print(addr);
  Serial.print(" - ");
  Serial.println(addr + 2);

  display_all_data_in_EEPROM();
}

void loop()
{
}