#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL // 16 MHz
#include <util/delay.h>


#define USART_BUFFER_SIZE 2048

unsigned char usart_buffer[USART_BUFFER_SIZE];
unsigned char sniffed_byte = 0;
unsigned int sniffed_bit_index = 0;
unsigned int usart_pointer = 0;
unsigned int usart_end = 0;
unsigned char pind_value = 0x08;
volatile unsigned char sending_usart = 0;

unsigned char hex_symbols[16] = {	'0', '1', '2', '3',
									'4', '5', '6', '7',
									'8', '9', 'A', 'B',
									'C', 'D', 'E', 'F'
								};

void init_usart()
{
	UCSR0C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR0H = 0;
	UBRR0L = 103;
	UCSR0B = 0x08; // enable transmission
}

inline void send_byte_to_usart(unsigned char byte)
{
	usart_buffer[usart_end] = byte;
	usart_end = (usart_end + 1) % USART_BUFFER_SIZE;
}

inline void send_byte_in_hex_format(unsigned char byte)
{
	send_byte_to_usart(hex_symbols[(byte>>4)&0x0F]);
	send_byte_to_usart(hex_symbols[byte & 0x0F]);
	send_byte_to_usart('\n');
}

ISR(USART0_UDRE_vect) // transmit buffer is empty, send next byte
{
	if( usart_pointer != usart_end )
	{
		UDR0 = usart_buffer[usart_pointer];
		usart_pointer = (usart_pointer + 1) % USART_BUFFER_SIZE;
	}
	else
	{
		// nothing to send, disable interrupts
		UCSR0B &= 0xCF;
		sending_usart = 0;
	}
}

int main()
{
	cli();
	
	init_usart();
	
	DDRD &= 0xF7; // set PD3 as input
	PORTD &= 0xF7; // remove pull-up resistor to not alterate the bus
	
	TCNT0 = 0;
	TIMSK0 = 0;
	TCCR0B = 0x00;
	
	sei();
	
	unsigned char new_pind_value = 0x08;
	
	TCCR1B = 0x02;
	
	while(1)
	{
		TCNT1 = 0;
		while(1)
		{
			//detect falling edge
			new_pind_value = (PIND & 0x08);
			
			if( pind_value != 0x00 && new_pind_value == 0x00 )
			{
				// wait 2 us
				_delay_us(2);
				
				if( TCNT1 > 24 )
				{
					sniffed_bit_index = 0;
					sniffed_byte = 0;
				}
				
				if( PIND & 0x08 ) // line is high
				{
					sniffed_byte |= (1 << sniffed_bit_index);
				}
				else
				{
					sniffed_byte &= ~(1 << sniffed_bit_index);
				}
				
				sniffed_bit_index++;
				
				if( sniffed_bit_index >= 8 )
				{
					send_byte_in_hex_format(sniffed_byte);
					sniffed_byte = 0;
					sniffed_bit_index = 0;
				}
				
				TCNT1 = 0;
			}
			
			if( TCNT1 > 65000 )
				break; // send bytes to uart when communication is over
			
			pind_value = new_pind_value;
		}
		
		if( usart_pointer != usart_end )
		{
			sending_usart = 1;
			UCSR0B |= 0x20; // enable interrupts
			
			while(sending_usart) ;
		}
	}
	
	return 0;
}
