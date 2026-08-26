#include <avr/io.h>
#include <avr/interrupt.h>
#include "my_twi.h"
#include "my_pcf8574.h"

uint8_t count;
unsigned long last_time;

uint8_t prepare_data(uint8_t d)
{
    uint8_t tmp;
    tmp = ~d;
    tmp |= 0x80;
    return tmp;
}

void setup()
{
    last_time = 0;
    Serial.begin(38400);

    init_twi_module();

    count = 0;
    uint8_t tmp = prepare_data(count);
    PCF8574_write(0, tmp);
}

void loop() 
{
    uint8_t tmp;

    tmp = PCF8574_read(0);
    tmp &= 0x80;

    if (!tmp) 
    {
        if (millis() - last_time > 20)
        {
            last_time = millis();
            tmp = PCF8574_read(0);
            tmp &= 0x80;

            if (!tmp)
            {
                count++;
                
                if (count > 127) 
                {
                    count = 0;
                }

                tmp = prepare_data(count);
                PCF8574_write(0, tmp);
            }
        }

        do
        {
            tmp = PCF8574_read(0);
            tmp &= 0x80;
        } while (!tmp);
    }
}