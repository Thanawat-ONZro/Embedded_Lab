const int soundPin = 2; // Sound module D0 -> D2 (INT0)
const int relayPin = 5; // Relay IN -> D5

const unsigned long DEBOUNCE_TIME = 150; // Debounce delay (ms)
const unsigned long FAST_GAP_MAX  = 500;  // Max gap for fast clap
const unsigned long SLOW_GAP_MIN  = 700;  // Min gap for slow clap
const unsigned long SLOW_GAP_MAX  = 2000; // Max gap for slow clap
const unsigned long RESET_TIMEOUT = 2500; // Pattern reset timeout

volatile unsigned long lastISRTime = 0;
volatile bool clapDetected = false;

int clapStep = 0;             
int state = 0;                // Lamp state: 0 = OFF (Relay HIGH), 1 = ON (Relay LOW)
unsigned long lastClapTime = 0;

void soundISR() {
  unsigned long now = millis();
  if ((now - lastISRTime) > DEBOUNCE_TIME) {
    lastISRTime = now;
    clapDetected = true;
  }
}

void toggleLamp() {
  if (state == 0) {
    state = 1;
    Serial.println("Pattern matched! Lamp turned ON");
  } else {
    state = 0;
    Serial.println("Pattern matched! Lamp turned OFF");
  }
}

void setup() {

  digitalWrite(relayPin, HIGH); 
  pinMode(relayPin, OUTPUT);
  pinMode(soundPin, INPUT);

  state = 0; 
  clapStep = 0;

  Serial.begin(9600);
  delay(1000);

  digitalWrite(relayPin, HIGH); 
  EIFR |= (1 << INTF0); 

  Serial.println("\n=== System Ready (Afternoon Session - NC Terminal) ===");
  Serial.println("Lamp is OFF (initial state)");
  Serial.println("Waiting for pattern: [Fast x2] -> [Slow x1]");

  attachInterrupt(digitalPinToInterrupt(soundPin), soundISR, FALLING);
}

void loop() {
  unsigned long now = millis();

  // Pattern timeout check
  if (clapStep != 0 && (now - lastClapTime) > RESET_TIMEOUT) {
    clapStep = 0;
    Serial.println("Pattern timeout -> Reset");
  }

  if (clapDetected) {
    noInterrupts();
    clapDetected = false;
    unsigned long currentClap = lastISRTime;
    interrupts();

    unsigned long gap = currentClap - lastClapTime;

    switch (clapStep) {
      case 0:
        // Clap #1
        clapStep = 1;
        lastClapTime = currentClap;
        Serial.println("Clap #1 detected");
        break;

      case 1:
        // Clap #2 (Must be fast)
        if (gap <= FAST_GAP_MAX) {
          clapStep = 2;
          lastClapTime = currentClap;
          Serial.println("Clap #2 detected (fast) -> Waiting for slow clap #3");
        } else {
          clapStep = 0;
          Serial.println("Gap too slow for clap #2 -> Reset pattern");
        }
        break;

      case 2:
        // Clap #3 (Must be slow)
        if (gap >= SLOW_GAP_MIN && gap <= SLOW_GAP_MAX) {
          Serial.println("Clap #3 detected (slow) -> Pattern complete!");
          toggleLamp();
          clapStep = 0;
        } else if (gap < SLOW_GAP_MIN) {
          clapStep = 0;
          Serial.println("Gap too fast for clap #3 -> Reset pattern");
        } else {
          clapStep = 0;
          Serial.println("Gap too slow for clap #3 -> Reset pattern");
        }
        break;
    }
  }

  // Hardware logic for NC terminal
  if (state == 1) {
    digitalWrite(relayPin, LOW);  
  } else {
    digitalWrite(relayPin, HIGH); 
  }
}