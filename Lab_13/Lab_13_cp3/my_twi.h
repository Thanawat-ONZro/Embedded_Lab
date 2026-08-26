/*
 * my_twi.h
 *
 * Created: 9/28/2018 11:24:12 PM
 *  Author: panyayot
 */ 


#ifndef MY_TWI_H_
#define MY_TWI_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#define TWI_BITRATE 100000
#define TWBR_VALUE (((F_CPU/TWI_BITRATE)-16)/2   )
#define START_TRANSMITTED			0x08
#define REPEATED_START_TRANSMITTED	0x10

#include <util/twi.h>

//-------------------------------------------------------------------------------
//-----Function list in the library my_twi.h
//-------------------------------------------------------------------------------
void init_twi_module(void);
void Status_TWI_no_MR_received_then_ack(void);
void Status_TWI_incomplete_start(void);
void Status_TWI_no_sla_ack(void);
void Status_TWI_no_data_ack(void);
void TWI_send_start_condition (void);
void TWI_wait_until_start_has_been_sent (void);
void TWI_send_slave_address (unsigned char addr);
void TWI_wait_until_sla_transmitted (void);
void TWI_send_device_begin_address(unsigned char addr);
void TWI_send_data (unsigned char DATA);
void TWI_wait_until_data_transmitted (void);
void TWI_send_stop_condition (void);
uint8_t TWI_read_data(void);
//-------------------------------------------------------------------------------
void init_twi_module(void)
{
	TWBR = TWBR_VALUE;
}

void Status_TWI_no_MR_received_then_ack(void)		//error_twi_handler(void)
{
	Serial.println("TWI>no data received from slave and master has responded\n\r");
}

void Status_TWI_incomplete_start(void)		//error_twi_handler(void)
{
	Serial.println("TWI>Start condition fail\n\r");
}

void Status_TWI_no_sla_ack(void)			//error_twi_handler2(void)
{
	Serial.println("TWI>no SLA acknowledge\n\r");
}

void Status_TWI_no_data_ack(void)			//error_twi_handler3(void)
{
	Serial.println("TWI>no data acknowledge");
}

void TWI_send_start_condition (void) {
	TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWSTA);
}

void TWI_wait_until_start_has_been_sent (void) {
	while (  !(TWCR & (1<<TWINT) ) )
		;
}

void TWI_send_slave_address (unsigned char addr) {
	unsigned char tmp;
	tmp = TWSR & 0xF8;
	if ((tmp != START_TRANSMITTED) &&  (tmp != REPEATED_START_TRANSMITTED))
		Status_TWI_incomplete_start();		//error_twi_handler( );
	TWDR = addr;
	TWCR = (1<<TWINT) | (1<<TWEN);
}

void TWI_wait_until_sla_transmitted (void) {
	while ( !(TWCR & (1<<TWINT)) )
		;
}



void TWI_send_device_begin_address(unsigned char addr)
{
	uint8_t tmp;
	tmp = TWSR & 0xF8;
	if ( (tmp != TW_MT_SLA_ACK))
		Status_TWI_no_sla_ack();				//error_twi_handler2( );
	
	TWDR = addr;
	TWCR = (1<<TWINT) |  (1<<TWEN);
}


void TWI_send_data (unsigned char DATA) {
	uint8_t tmp;
	tmp = TWSR & 0xF8;
	if  (tmp != TW_MT_DATA_ACK)
		Status_TWI_no_data_ack();				//error_twi_handler2( );
	
	TWDR = DATA;
	TWCR = (1<<TWINT) |  (1<<TWEN);
}

void TWI_send_first_data (unsigned char DATA) {
	uint8_t tmp;
	tmp = TWSR & 0xF8;
	if ( tmp != TW_MT_SLA_ACK)
		Status_TWI_no_sla_ack();				//error_twi_handler2( );
	
	TWDR = DATA;
	TWCR = (1<<TWINT) |  (1<<TWEN);
}


void TWI_wait_until_data_transmitted (void) {
	while ( !(TWCR & (1<<TWINT)) )
	;
	if ( (TWSR & 0xF8) != TW_MT_DATA_ACK )
		Status_TWI_no_data_ack();							//error_twi_handler3( );
}

uint8_t TWI_read_data(void)
{
	uint8_t tmp;
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while ( !(TWCR & (1<<TWINT)) )
		;
	
	tmp = TWSR & 0xF8;
	if ( tmp != TW_MR_DATA_ACK )
		Status_TWI_no_MR_received_then_ack();
	return TWDR;
}


void TWI_send_stop_condition (void) {
	uint8_t tmp;
	tmp = TWSR & 0xF8;
	if (( tmp != TW_MT_DATA_ACK) && (tmp != TW_MR_DATA_ACK))
		Status_TWI_no_data_ack( );
	TWCR = (1<<TWINT)|(1<<TWEN)| (1<<TWSTO);
}

#endif /* MY_TWI_H_ */
