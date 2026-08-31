#define Y_LED 0   // GPIO0
#define R_LED 15  // GPIO15
#define G_LED 2   // GPIO2

#define SW1_4 16   // GPIO4
#define SW1_3 5  // GPIO14
#define SW1_2 13  // GPIO12
#define SW1_1 12  // GPIO13

void setup() {
  pinMode(Y_LED, OUTPUT);
  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);

  pinMode(SW1_1, INPUT);
  pinMode(SW1_2, INPUT);
  pinMode(SW1_3, INPUT);
  pinMode(SW1_4, INPUT);
}

void loop() {
  bool s1 = digitalRead(SW1_1);
  bool s2 = digitalRead(SW1_2);
  bool s3 = digitalRead(SW1_3);
  bool s4 = digitalRead(SW1_4);

  if (s4 == HIGH) { // 1xxx 
    digitalWrite(R_LED, HIGH);
    delay(1000);
    digitalWrite(R_LED, LOW);
    delay(1000);
  } 
  else if (!s1 && !s2 && !s3) { // 0000
    digitalWrite(R_LED, LOW);
    digitalWrite(Y_LED, LOW);
    digitalWrite(G_LED, LOW);
  } 
  else if (s1 && !s2 && !s3) { // 0001
    digitalWrite(R_LED, HIGH);
    digitalWrite(Y_LED, HIGH);
    digitalWrite(G_LED, LOW);
  } 

  else if (!s1 && s2 && !s3) { // 0010
    digitalWrite(R_LED, HIGH);
    digitalWrite(Y_LED, LOW);
    digitalWrite(G_LED, HIGH);
  } 
  else if (s1 && s2 && !s3) { // 0011
    digitalWrite(R_LED, LOW);
    digitalWrite(Y_LED, HIGH);
    digitalWrite(G_LED, HIGH);
  } 

  else if (!s1 && !s2 && s3) { // 0100
    digitalWrite(R_LED, HIGH);
    digitalWrite(G_LED, HIGH);
    digitalWrite(Y_LED, HIGH);
    delay(1000);
    digitalWrite(R_LED, LOW);
    digitalWrite(G_LED, LOW);
    digitalWrite(Y_LED, LOW);
    delay(1000);
  }
}