#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL // 16 MHz
#include <util/delay.h>

unsigned int sniffed_bit_index = 0;

unsigned char pind_value = 0x08;
unsigned char new_pind_value = 0x08;

#define NB_RESPONSE 98
unsigned char response[] = {0xFF, 0xFF, 0xFF, 0xFF,
							0x75, 0x02, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x50,
							0xFF, 0xFF, 0xFF, 0xFF,
							0x71, 0x93,
							0xFF, 0xFF,
							0x77, 0x01, 0x25, 0x01, 0x00, 0xAC, 0xF5, 0x57, 0x29, 0xF6, 0x8A, 0xAF,
							0xFF, 0xFF,
							0x79, 0x44, 0x57, 0x48, 0x33, 0x32, 0x36, 0x34, 0x34, 0x37, 0x35, 0x41, 0x46, 0x39, 0x34, 0x4E, 0x31, 0x4A, 0x00, 0x2E, 0x88, 0xAF,
							0xFF, 0xFF,
							0x7B, 0x44, 0x57, 0x48, 0x33, 0x32, 0x36, 0x34, 0x34, 0x37, 0x35, 0x41, 0x46, 0x39, 0x34, 0x4E, 0x31, 0x4A, 0x00, 0x16, 0x08, 0xB5,
							0xFF, 0xFF,
							0x73, 0x00, 0x00, 0xC0, 0x00, 0x5E, 0x84, 0x00, 0x00, 0x05, 0x46, 0x46, 0x34, 0x33, 0x32, 0xE8
							};
unsigned int response_index = 0;
unsigned char response_byte = 0;
unsigned int response_byte_i = 0;
unsigned int first_response = 1;

inline void send_zero()
{
	DDRD |= 0x08;
}

inline void send_one()
{
	DDRD &= 0xF7;
}

inline void listen()
{
	DDRD &= 0xF7;
}

int main()
{
	cli();
	
	DDRD &= 0xF7; // set PD3 as input
	PORTD &= 0xF7; // remove pull-up resistor to not alterate the bus
	
	TCNT0 = 0;
	TIMSK0 = 0;
	TCCR0B = 0x00;
	
	response_index = 0;
	
	pind_value = PIND;
	
	sei();
	
	TCNT1 = 0;
	TCCR1B = 0x02;
	
	while(1)
	{
		//detect falling edge
		new_pind_value = (PIND & 0x08);
		
		if( pind_value != 0x00 && new_pind_value == 0x00 )
		{
			if( TCNT1 > 24 )
			{
				sniffed_bit_index = 0;
			}
			
			++sniffed_bit_index;
			
			if( sniffed_bit_index >= 8 )
			{
				sniffed_bit_index = 0;
				
				// byte found, check if we have to respond
				response_index++;
				if( response_index < NB_RESPONSE )
					response_byte = response[response_index];
				else
					response_byte = 0xFF;
				
				while( response_byte != 0xFF )
				{
					
					if( first_response )
					{
						_delay_us(53);
						first_response = 0;
					}
					else
					{
						_delay_us(10);
					}
					
					for(response_byte_i = 0; response_byte_i < 8; ++response_byte_i)
					{
						if( response_byte & 0x01 )
						{
							send_zero();
							_delay_us(1.6);
							listen();
							_delay_us(8.6);
						}
						else
						{
							send_zero();
							_delay_us(6.9);
							listen();
							_delay_us(3.3);
						}
						response_byte >>= 1;
					}
					
					response_index++;
					if( response_index < NB_RESPONSE )
						response_byte = response[response_index];
					else
						response_byte = 0xFF;
				}
				first_response = 1;
			}
			
			TCNT1 = 0;
		}
		
		pind_value = new_pind_value;
	}
	
	return 0;
}
