#define BLYNK_TEMPLATE_ID "TMPL6ZPgT6G1r"
#define BLYNK_TEMPLATE_NAME "Checkpoint 2"
#define BLYNK_AUTH_TOKEN "-mgza0KkyiHzTmoleF5XuVYOl-_SS_F7"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define Y_LED 0   // GPIO0
#define R_LED 15  // GPIO15
#define G_LED 2   // GPIO2
#define SW1   4   // GPIO4 (DIP Sw1 Bit 1)

char ssid[] = "IPHONE_14T";
char pass[] = "password";

BlynkTimer timer;

// V0: ควบคุมการเปิด-ปิด LED สีแดง ผ่าน Button Widget
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1) {
    digitalWrite(R_LED, HIGH);
  } else {
    digitalWrite(R_LED, LOW);
  }
}

// V1: ควบคุมความสว่าง LED สีเขียว ผ่าน Slider Widget (PWM 0-1023)
BLYNK_WRITE(V1) {
  int value = param.asInt();
  analogWrite(G_LED, value);
}

// อ่านค่าจาก DIP Switch Bit 1 ส่งไปแสดงผลที่ V3 (LED Widget บนแอป)
void ReadSW() {
  uint8_t d = digitalRead(SW1);
  Blynk.virtualWrite(V3, d);
}

void setup() {
  pinMode(Y_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(R_LED, OUTPUT);
  pinMode(SW1, INPUT);

  Serial.begin(115200);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // ตั้งเวลาให้อ่านค่าจากสวิตช์ทุกๆ 100 ms
  timer.setInterval(100L, ReadSW);
}

void loop() {
  Blynk.run();
  timer.run();
}