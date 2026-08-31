#define BLYNK_TEMPLATE_ID "TMPL6zGwx5BJI"
#define BLYNK_TEMPLATE_NAME "Checkpoint 3"
#define BLYNK_AUTH_TOKEN "nPF9l3NefJiujOgMf1dbGtStgHwC_Vtq"
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define Y_LED 0   // GPIO0
#define R_LED 15  // GPIO15
#define G_LED 2   // GPIO2

#define SW1_1 4   // GPIO4 (DIP Sw1 Bit 1 - ประตูหน้าบ้าน)
#define SW1_2 14  // GPIO14 (DIP Sw1 Bit 2 - ประตูหลังบ้าน)
#define SW1_5 12  // GPIO12 (DIP Sw1 Bit 5)
#define SW1_6 13  // GPIO13 (DIP Sw1 Bit 6)
#define SW1_7 5   // GPIO5 (DIP Sw1 Bit 7)
#define SW1_8 16  // GPIO16 (DIP Sw1 Bit 8)

char ssid[] = "IPHONE_14T";
char pass[] = "password";

BlynkTimer timer;

int btnR = 0, btnG = 0, btnY = 0;
unsigned long lastBlink = 0;
bool blinkState = false;

BLYNK_WRITE(V0) { btnR = param.asInt(); }
BLYNK_WRITE(V1) { btnG = param.asInt(); }
BLYNK_WRITE(V2) { btnY = param.asInt(); }

void ReadSensors() {
  uint8_t doorFront = digitalRead(SW1_1);
  uint8_t doorBack  = digitalRead(SW1_2);
  
  Blynk.virtualWrite(V4, doorFront);
  Blynk.virtualWrite(V5, doorBack);
}

void setup() {
  pinMode(Y_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(R_LED, OUTPUT);

  pinMode(SW1_1, INPUT);
  pinMode(SW1_2, INPUT);
  pinMode(SW1_5, INPUT);
  pinMode(SW1_6, INPUT);
  pinMode(SW1_7, INPUT);
  pinMode(SW1_8, INPUT);

  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(100L, ReadSensors);
}

void loop() {
  Blynk.run();
  timer.run();

  uint8_t b8 = digitalRead(SW1_8);
  uint8_t b7 = digitalRead(SW1_7);
  uint8_t b6 = digitalRead(SW1_6);
  uint8_t b5 = digitalRead(SW1_5);

  uint8_t mode = (b8 << 3) | (b7 << 2) | (b6 << 1) | b5;

  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    blinkState = !blinkState;
  }

  switch (mode) {
    case 0b1000: // 10002
      digitalWrite(R_LED, blinkState);
      digitalWrite(Y_LED, blinkState);
      digitalWrite(G_LED, blinkState);
      break;

    case 0b1001: // 10012
      digitalWrite(R_LED, btnR);
      digitalWrite(Y_LED, LOW);
      digitalWrite(G_LED, LOW);
      break;

    case 0b1010: // 10102
      digitalWrite(R_LED, LOW);
      digitalWrite(Y_LED, LOW);
      digitalWrite(G_LED, btnG);
      break;

    case 0b1011: // 10112
      digitalWrite(R_LED, LOW);
      digitalWrite(Y_LED, btnY);
      digitalWrite(G_LED, LOW);
      break;

    case 0b0100: // 01002
      digitalWrite(R_LED, btnR);
      digitalWrite(G_LED, btnG);
      digitalWrite(Y_LED, btnY);
      break;

    case 0b0101: // 01012 (20%)
      digitalWrite(Y_LED, LOW);
      digitalWrite(G_LED, LOW);
      analogWrite(R_LED, blinkState ? 205 : 0);
      break;

    case 0b0110: // 01102 (50%)
      digitalWrite(R_LED, LOW);
      digitalWrite(Y_LED, LOW);
      analogWrite(G_LED, blinkState ? 512 : 0);
      break;

    case 0b0111: // 01112 (70%)
      digitalWrite(R_LED, LOW);
      digitalWrite(G_LED, LOW);
      analogWrite(Y_LED, blinkState ? 716 : 0);
      break;

    default:
      digitalWrite(R_LED, LOW);
      digitalWrite(Y_LED, LOW);
      digitalWrite(G_LED, LOW);
      break;
  }
}