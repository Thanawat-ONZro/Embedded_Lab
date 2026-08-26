#include <avr/io.h>
#include "my_twi.h"

void setup() {
  Serial.begin(38400); // ตั้งความเร็วรับส่งสัญญาณตามแลป
  init_twi_module();   // เริ่มต้นมอดูล TWI/I2C[cite: 1, 3, 7]

  Serial.println("I2C Scanner for PCF8574 / PCF8574A ");

  uint8_t found_count = 0;

  // วนลูปสแกนหา 7-bit Address ตั้งแต่ 1 ถึง 126
  for (uint8_t addr = 1; addr < 127; addr++) {
    
    uint8_t sla_w = (addr << 1); // แปลง 7-bit Address เป็น 8-bit SLA+W (Write bit = 0)[cite: 2]

    // 1. ส่งภาวะเริ่ม (START Condition)
    TWI_send_start_condition();
    TWI_wait_until_start_has_been_sent();

    // 2. ส่งหมายเลขที่อยู่ (SLA+W)
    TWDR = sla_w;
    TWCR = (1 << TWINT) | (1 << TWEN);
    TWI_wait_until_sla_transmitted();

    // 3. ตรวจสอบสถานะการตอบรับ (0x18 = ACK ได้รับจาก Slave)[cite: 2, 7]
    uint8_t status = TWSR & 0xF8;
    if (status == 0x18) {
      found_count++;
      
      Serial.print("Found Device at Address: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);

      // จำแนกประเภทไอซีตามช่วงแอดเดรส
      if (addr >= 0x20 && addr <= 0x27) {
        Serial.println(" -> IC Type: PCF8574");
      } 
      else if (addr >= 0x38 && addr <= 0x3F) {
        Serial.println(" -> IC Type: PCF8574A");
      } 
      else {
        Serial.println(" -> Other I2C Device");
      }
    }

    // 4. ส่งภาวะหยุด (STOP Condition) เพื่อคืนบัส
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  }

  if (found_count == 0) {
    Serial.println("No PCF8574 or PCF8574A found on the bus!");
  }
  Serial.println("Scan Finished.");
}

void loop() {
  // ทำการสแกนครั้งเดียวใน setup()
}