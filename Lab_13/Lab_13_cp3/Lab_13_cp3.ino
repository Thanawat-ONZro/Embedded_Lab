#include <Wire.h>
#include "I2CKeyPad.h"

#define I2CADDR 0x20

I2CKeyPad keyPad(I2CADDR);

char keymap[19] = "123A456B789C*0#DNF"; 

long operand1 = 0;
long operand2 = 0;
char operation = 0;
int state = 0;

void setup() {
  Serial.begin(38400);
  Wire.begin();
  
  if (keyPad.begin() == false) {
    Serial.println("\nERROR: Cannot communicate to keypad. Please check wiring.");
    while (1);
  }
  
  keyPad.loadKeyMap(keymap);
  
  Serial.println("--- Ready Checkpoint 13.3 (Afternoon) ---");
}

void loop() {
  if (keyPad.isPressed()) {
    char key = keyPad.getChar();
    
    if (key != 'N' && key != 'F') {
      
      if (key >= '0' && key <= '9') {
        if (state == 0) {
          operand1 = (operand1 * 10) + (key - '0');
        } else {
          operand2 = (operand2 * 10) + (key - '0');
        }
        Serial.print(key);
      }
      
      else if (key == '*' || key == 'D') {
        operation = key; 
        state = 1;      
        Serial.print(key);
      }
      
      else if (key == '#') {
        Serial.println();
        
        if (operation == '*') {
          Serial.print(operand1);
          Serial.print("*");
          Serial.print(operand2);
          Serial.print("=");
          Serial.println(operand1 * operand2);
        }
        else if (operation == 'D') {
          if (operand2 == 0) {
            Serial.println("Error: Divide by Zero");
          } else {
            long q = operand1 / operand2;
            long r = operand1 % operand2;
            
            Serial.print(operand1);
            Serial.print("/");
            Serial.print(operand2);
            Serial.print("=");
            Serial.print(q);
            
            Serial.print(" (ผลหารเท่ากับ ");
            Serial.print(q);
            Serial.print(" เศษ ");
            Serial.print(r);
            Serial.println(")");
          }
        }
        
        operand1 = 0;
        operand2 = 0;
        state = 0;
        operation = 0;
      }
      
      while (keyPad.isPressed()) {
        delay(10);
      }
    }
  }
}