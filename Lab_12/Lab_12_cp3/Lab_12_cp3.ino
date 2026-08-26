#include <LiquidCrystal.h>
#include "DHT.h"

#define DHTPIN 7
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

void setup() {
  Serial.begin(9600);
  
  lcd.begin(16, 2);
  lcd.clear();
  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("  DHT11 Sensor  ");
  lcd.setCursor(0, 1);
  lcd.print(" Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Error reading   ");
    lcd.setCursor(0, 1);
    lcd.print("from DHT11!     ");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t, 1);
  lcd.print((char)223);
  lcd.print("C   ");

  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print(h, 1);
  lcd.print(" %   ");
}