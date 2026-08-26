#include <avr/io.h>
#include "my_twi.h"
#include "my_pcf8574.h"

// =========================================================
// 1. ตัวแปรควบคุมระบบและสวิตช์
// =========================================================
uint8_t mode = 0;                  // 0: X1, 1: X2, 2: X3, 3: X4
unsigned long press_start_time = 0; // บันทึกเวลาที่เริ่มกดสวิตช์
bool prev_pressed = false;         // สถานะสวิตช์ในรอบก่อนหน้า
bool long_press_handled = false;   // ธงเช็กว่าประมวลผลการกดค้าง 2 วินาทีไปแล้วหรือยัง
unsigned long last_btn_sample = 0; // เวลาสุ่มอ่านค่าสวิตช์

unsigned long last_led_time = 0;   // ตัวจับเวลาอัปเดต LED

// X1: นับลงแบบไบนารี (127 -> 0)
int16_t x1_count = 127;

// X2: ดับหมดก่อน แล้วค่อยๆ สว่างเพิ่มทีละดวงจาก D6 (ซ้าย) -> D0 (ขวา) วนลูปอัตโนมัติ
uint8_t x2_step = 0;
const uint8_t x2_patterns[8] = {
    0x00, // Step 0: ดับทั้งหมด
    0x40, // Step 1: D6
    0x60, // Step 2: D6, D5
    0x70, // Step 3: D6, D5, D4
    0x78, // Step 4: D6..D3
    0x7C, // Step 5: D6..D2
    0x7E, // Step 6: D6..D1
    0x7F  // Step 7: D6..D0 (ติดครบ 7 ดวง)
};

// X3: สว่าง 2 ดวง เลื่อนไปทางซ้ายทุกครั้งที่กดสั้น
uint8_t x3_pos = 0;

// X4: รูปแบบกระพริบซับซ้อนตามลำดับ
struct X4Step {
    uint8_t pattern;   // รูปแบบ LED (0x7F = ติดหมด, 0x00 = ดับหมด)
    uint16_t duration; // เวลาหน่วง (ms)
};

const X4Step x4_sequence[] = {
    {0x00, 1000}, // เริ่มต้น ดับ 1 วินาที
    
    // กระพริบ 1 ครั้ง (ติด 0.5s / ดับ 0.5s) -> ดับ 1s
    {0x7F, 500}, {0x00, 500},
    {0x00, 1000},
    
    // กระพริบ 2 ครั้ง -> ดับ 1s
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x00, 1000},
    
    // กระพริบ 3 ครั้ง -> ดับ 1s
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x00, 1000},
    
    // กระพริบ 4 ครั้ง -> ดับ 1s
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x7F, 500}, {0x00, 500},
    {0x00, 1000}
};
const uint8_t X4_TOTAL_STEPS = sizeof(x4_sequence) / sizeof(x4_sequence[0]);
uint8_t x4_step = 0;

// =========================================================
// 2. ฟังก์ชันช่วยเตรียมข้อมูลขับ LED
// =========================================================
uint8_t prepare_data(uint8_t d)
{
    uint8_t tmp = ~d; // กลับบิตสำหรับ Common Anode (0 = ติด, 1 = ดับ)
    tmp |= 0x80;      // บังคับ P7 = 1 รักษาสภาพพอร์ต Input
    return tmp;
}

// รีเซ็ตและอัปเดตสถานะ LED เมื่อเปลี่ยนโหมด
void reset_mode_state()
{
    last_led_time = millis();
    
    if (mode == 0) { // X1
        x1_count = 127;
        PCF8574_write(0, prepare_data((uint8_t)x1_count));
    }
    else if (mode == 1) { // X2: เริ่มที่สถานะดับหมด
        x2_step = 0;
        PCF8574_write(0, prepare_data(x2_patterns[x2_step]));
    }
    else if (mode == 2) { // X3: เริ่ม 2 ดวงขวาสุด (D0, D1)
        x3_pos = 0;
        uint8_t pattern = (0x03 << x3_pos) & 0x7F;
        PCF8574_write(0, prepare_data(pattern));
    }
    else if (mode == 3) { // X4
        x4_step = 0;
        PCF8574_write(0, prepare_data(x4_sequence[0].pattern));
    }
}

// ฟังก์ชันทำงานเมื่อมีการ "กดสั้น" (> 50ms และ < 2 วินาที)
void handle_short_press()
{
    // มีเฉพาะโหมด X3 เท่านั้นที่ใช้การกดสั้นเปลี่ยนตำแหน่งไฟ
    if (mode == 2) { 
        x3_pos = (x3_pos + 1) % 8; // เลื่อนตำแหน่ง 0 -> 7 แล้ววนกลับ
        uint8_t pattern = (0x03 << x3_pos) & 0x7F;
        PCF8574_write(0, prepare_data(pattern));
    }
}

// =========================================================
// 3. Setup & Loop
// =========================================================
void setup()
{
    Serial.begin(38400);
    init_twi_module();
    
    Serial.println("--- Started Checkpoint 13.2 (Afternoon Section) ---");
    Serial.println("Current Mode: X1");

    reset_mode_state();
}

void loop()
{
    // ---------------------------------------------------------
    // A. ระบบตรวจจับการกดสวิตช์ (สุ่มอ่านทุกๆ 20 ms)
    // ---------------------------------------------------------
    if (millis() - last_btn_sample >= 20)
    {
        last_btn_sample = millis();
        
        uint8_t sw_tmp = PCF8574_read(0);
        sw_tmp &= 0x80;
        bool is_pressed = (!sw_tmp); // Active Low: 0 คือกดสวิตช์

        if (is_pressed && !prev_pressed) {
            // เริ่มกด
            press_start_time = millis();
            long_press_handled = false;
            prev_pressed = true;
        }
        else if (is_pressed && prev_pressed) {
            // กดค้างครบ 2 วินาที -> เปลี่ยนโหมด
            if (!long_press_handled && (millis() - press_start_time >= 2000)) {
                long_press_handled = true;
                
                mode = (mode + 1) % 4; // สลับโหมด X1 -> X2 -> X3 -> X4 -> X1
                
                Serial.print("Switched to Mode: X");
                Serial.println(mode + 1);

                reset_mode_state();
            }
        }
        else if (!is_pressed && prev_pressed) {
            // ปล่อยสวิตช์
            unsigned long press_duration = millis() - press_start_time;
            prev_pressed = false;

            // กดสั้น
            if (!long_press_handled && press_duration >= 50) {
                handle_short_press();
            }
        }
    }

    // ---------------------------------------------------------
    // B. การทำงานอัตโนมัติตามโหมด
    // ---------------------------------------------------------
    switch (mode)
    {
        case 0: // โหมด X1: นับลงแบบไบนารี 127 -> 0 วินาทีละ 1 ค่า
            if (millis() - last_led_time >= 1000)
            {
                last_led_time = millis();
                PCF8574_write(0, prepare_data((uint8_t)x1_count));
                
                x1_count--;
                if (x1_count < 0) x1_count = 127;
            }
            break;

        case 1: // โหมด X2: อัตโนมัติ วนลูปติดเพิ่มทีละดวงทุกๆ 1 วินาที
            if (millis() - last_led_time >= 1000)
            {
                last_led_time = millis();
                
                x2_step = (x2_step + 1) % 8; // วนลูปขั้นตอน 0 -> 7
                PCF8574_write(0, prepare_data(x2_patterns[x2_step]));
            }
            break;

        case 2: // โหมด X3: ไม่ต้องทำอะไรใน Loop (เปลี่ยนตำแหน่งเมื่อกดสั้นเท่านั้น)
            break;

        case 3: // โหมด X4: กระพริบตามคาบเวลาแบบอัตโนมัติ
            if (millis() - last_led_time >= x4_sequence[x4_step].duration)
            {
                last_led_time = millis();
                PCF8574_write(0, prepare_data(x4_sequence[x4_step].pattern));
                
                x4_step = (x4_step + 1) % X4_TOTAL_STEPS;
            }
            break;
    }
}