/*
 * pcf8574.h
 *
 * Created: 10/4/2018 1:56:41 PM
 *  Author: panyayot
 */ 


#ifndef MY_PCF8574_H_
#define MY_PCF8574_H_


#include "my_twi.h"
#define PCF8574_SLA_W	0x40
#define PCF8574_SLA_R	0x41
#define PCF8574A_SLA_W	0x70
#define PCF8574A_SLA_R	0x71


//-------------------------------------------------------------------------------
//-----Function list in the library my_pcf8574.h
//-------------------------------------------------------------------------------
uint8_t PCF8574_read(uint8_t addr);
void PCF8574_write(uint8_t addr, uint8_t data);
//-------------------------------------------------------------------------------


uint8_t PCF8574_read(uint8_t addr)
{
	uint8_t tmp;
	tmp = PCF8574_SLA_R;
	tmp |= (addr<<1);
	TWI_send_start_condition();
	TWI_wait_until_start_has_been_sent( );
	TWI_send_slave_address(tmp);
	TWI_wait_until_sla_transmitted();
	
	tmp = TWI_read_data();
	return tmp;
}
void PCF8574_write(uint8_t addr, uint8_t data)
{
	uint8_t tmp;
	tmp = PCF8574_SLA_W;
	tmp |= (addr<<1);
	TWI_send_start_condition();
	TWI_wait_until_start_has_been_sent( );
	TWI_send_slave_address(tmp);	
	TWI_wait_until_sla_transmitted();

	TWI_send_first_data(data);
	TWI_wait_until_data_transmitted();
	TWI_send_stop_condition();
}


#endif /* MY_PCF8574_H_ */
