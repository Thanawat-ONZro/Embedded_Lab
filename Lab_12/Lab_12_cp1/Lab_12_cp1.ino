int soundPin = 2; // D0 ของมอดูลรับเสียง ต่อกับขา D2
int relayPin = 5; // Signal(IN) ของมอดูลรีเลย์ ต่อกับขา D5

unsigned long previous_time = 0;
const unsigned long DEBOUNCE_TIME = 25; // ระยะเวลาป้องกันการสั่นของสัญญาณ (ms)

int state = 0; // สถานะหลอดไฟ: 0 = ดับ, 1 = ติด

bool check_clap() {
  static unsigned long current_time;
  char sw;
  sw = digitalRead(soundPin);

  if (sw) {
    return false; // สภาวะปกติไม่มีเสียง (HIGH)
  } else {
    current_time = millis();
    if ((current_time - previous_time) > DEBOUNCE_TIME) {
      previous_time = millis();
      return true; // ตรวจพบเสียง (LOW) และผ่าน debounce
    } else {
      previous_time = millis();
      return false;
    }
  }
}

void toggleLamp() {
  if (state == 0) {
    state = 1;
    Serial.println("Clap detected! Lamp turned ON");
  } else {
    state = 0;
    Serial.println("Clap detected! Lamp turned OFF");
  }
}

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(soundPin, INPUT);
  Serial.begin(9600);

  state = 0;                    // เริ่มต้นให้หลอดไฟดับ
  digitalWrite(relayPin, LOW);   // รีเลย์หยุดทำงาน (หลอดดับ)
  Serial.println("Lamp is OFF (initial state)");
  Serial.println("Waiting for single clap...");
}

void loop() {
  bool clapped = check_clap();

  if (clapped) {
    toggleLamp(); // ปรบมือ 1 ครั้ง สลับสถานะทันที
  }

  // ---- สั่งงานรีเลย์ตามสถานะหลอดไฟ ----
  if (state == 1) {
    digitalWrite(relayPin, HIGH); // สั่งติดไฟ
  } else {
    digitalWrite(relayPin, LOW);  // สั่งดับไฟ
  }
}