#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_BUFFER_SIZE 512
#define FRAME_BUFFER_SIZE 256

uint16_t frame_nb = 0x0100;
uint8_t session_id = 0x00;

unsigned char ipad_uart_rcv_buffer[UART_BUFFER_SIZE];
unsigned char ipad_uart_send_buffer[UART_BUFFER_SIZE];
unsigned int i_rcv_pointer;
unsigned int i_rcv_end;
unsigned int i_send_pointer;
unsigned int i_send_end;

unsigned char accessory_uart_rcv_buffer[UART_BUFFER_SIZE];
unsigned char accessory_uart_send_buffer[UART_BUFFER_SIZE];
unsigned int a_rcv_pointer;
unsigned int a_rcv_end;
unsigned int a_send_pointer;
unsigned int a_send_end;

unsigned char usb_uart_rcv_buffer[UART_BUFFER_SIZE];
unsigned char usb_uart_send_buffer[UART_BUFFER_SIZE];
unsigned int u_rcv_pointer;
unsigned int u_rcv_end;
unsigned int u_send_pointer;
unsigned int u_send_end;

unsigned char accessory_frame_buffer[FRAME_BUFFER_SIZE];
unsigned int accessory_frame_size;
int accessory_frame_expected_length;

unsigned char ipad_frame_buffer[FRAME_BUFFER_SIZE];
unsigned int ipad_frame_size;
int ipad_frame_expected_length;

unsigned char usb_frame_buffer[FRAME_BUFFER_SIZE];
unsigned int usb_frame_size;

void init_uart()
{
	accessory_frame_size = 0;
	accessory_frame_expected_length = -1;
	ipad_frame_size = 0;
	ipad_frame_expected_length = -1;
	usb_frame_size = 0;
	
	// uart from ipad is uart1
	UCSR1C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR1 = 16; // 57.6k
	UCSR1B = 0x18; // enable transmission/reception
	i_rcv_pointer = 0;
	i_rcv_end = 0;
	i_send_pointer = 0;
	i_send_end = 0;
	UCSR1B |= 0x80; // enable rx complete interrupt
	
	// uart from reader is uart2
	UCSR2C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR2 = 16; // 57.6k
	UCSR2B = 0x18; // enable transmission/reception
	a_rcv_pointer = 0;
	a_rcv_end = 0;
	a_send_pointer = 0;
	a_send_end = 0;
	UCSR2B |= 0x80; // enable rx complete interrupt
	
	// uart for third party is uart0
	UCSR0C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR0 = 16; // 57.6k
	UCSR0B = 0x18; // enable transmission/reception
	u_rcv_pointer = 0;
	u_rcv_end = 0;
	u_send_pointer = 0;
	u_send_end = 0;
	UCSR0B |= 0x80; // enable rx complete interrupt
}

inline void send_to_ipad(unsigned char byte)
{
	ipad_uart_send_buffer[i_send_end] = byte;
	i_send_end = (i_send_end + 1) % UART_BUFFER_SIZE;
	
	UCSR1B |= 0x20; // enable interrupts
}

inline void send_to_accessory(unsigned char byte)
{
	accessory_uart_send_buffer[a_send_end] = byte;
	a_send_end = (a_send_end + 1) % UART_BUFFER_SIZE;
	
	UCSR2B |= 0x20; // enable interrupts
}

inline void send_to_usb(unsigned char byte)
{
	usb_uart_send_buffer[u_send_end] = byte;
	u_send_end = (u_send_end + 1) % UART_BUFFER_SIZE;
	
	UCSR0B |= 0x20; // enable interrupts
}

inline int read_from_ipad(unsigned char* c)
{
	if( i_rcv_pointer != i_rcv_end )
	{
		*c = ipad_uart_rcv_buffer[i_rcv_pointer];
		i_rcv_pointer = (i_rcv_pointer + 1) % UART_BUFFER_SIZE;
		return 1;
	}
	else
	{
		return 0;
	}
}

inline int read_from_accessory(unsigned char* c)
{
	if( a_rcv_pointer != a_rcv_end )
	{
		*c = accessory_uart_rcv_buffer[a_rcv_pointer];
		a_rcv_pointer = (a_rcv_pointer + 1) % UART_BUFFER_SIZE;
		return 1;
	}
	else
	{
		return 0;
	}
}

inline int read_from_usb(unsigned char* c)
{
	if( u_rcv_pointer != u_rcv_end )
	{
		*c = usb_uart_rcv_buffer[u_rcv_pointer];
		u_rcv_pointer = (u_rcv_pointer + 1) % UART_BUFFER_SIZE;
		return 1;
	}
	else
	{
		return 0;
	}
}

inline void parse_new_byte_from_ipad(unsigned char c)
{
	static unsigned int i;
	static uint8_t ack_crc = 0;
	
	if( ipad_frame_size == 0 && c != 0xFF )
	{
		ipad_uart_rcv_buffer[i_rcv_end] = c;
		i_rcv_end = (i_rcv_end + 1) % UART_BUFFER_SIZE;
		return;
	}
	
	if( ipad_frame_size == 1 && c != 0x55 )
	{
		ipad_uart_rcv_buffer[i_rcv_end] = c;
		i_rcv_end = (i_rcv_end + 1) % UART_BUFFER_SIZE;
		ipad_frame_size = 0;
		return;
	}
	
	ipad_frame_buffer[ipad_frame_size++] = c;
	
	if( ipad_frame_size == 3 )
	{
		ipad_frame_expected_length = (unsigned int)c + 4;
	}
	else if( ipad_frame_size == ipad_frame_expected_length )
	{
		ipad_frame_expected_length = -1;
		
		// update session id (if not an ack message cause it does not contain session id)
		if( ipad_frame_buffer[4] != 0x02 )
			session_id = ipad_frame_buffer[8];
		
		if( ipad_frame_buffer[4] == 0x43 ) // data from app
		{
			// send ack
			ack_crc = (0xFF + 0x55 + 0x06 + 0x00 + 0x41 + ipad_frame_buffer[5] + ipad_frame_buffer[6] + 0x43) & 0xFF;
			ack_crc = (0x154 - ack_crc) & 0xFF;
			send_to_ipad(0xFF);
			send_to_ipad(0x55);
			send_to_ipad(0x06);
			send_to_ipad(0x00);
			send_to_ipad(0x41);
			send_to_ipad(ipad_frame_buffer[5]);
			send_to_ipad(ipad_frame_buffer[6]);
			send_to_ipad(0x00);
			send_to_ipad(0x43);
			send_to_ipad(ack_crc);
			
			for(i=9; i<(ipad_frame_size-1); ++i)
			{
				send_to_usb(ipad_frame_buffer[i]);
			}
		}
		else if( ipad_frame_buffer[4] == 0x02 && ipad_frame_buffer[8] == 0x42 )
		{
			// ignore this ack
		}
		else
		{
			for(i=0; i<ipad_frame_size; ++i)
			{
				ipad_uart_rcv_buffer[i_rcv_end] = ipad_frame_buffer[i];
				i_rcv_end = (i_rcv_end + 1) % UART_BUFFER_SIZE;
			}
		}
		
		ipad_frame_size = 0;
	}
}

inline void parse_new_byte_from_usb(unsigned char c)
{
	static uint8_t crc = 0;
	static unsigned int i;
	
	usb_frame_buffer[usb_frame_size++] = c;
	
	if( c == '\n' )
	{
		crc = (0xFF + 0x55 + ((usb_frame_size+6)&0xFF) + 0x42 + ((frame_nb>>8)&0xFF) + (frame_nb&0xFF) + session_id) & 0xFF;
		
		send_to_ipad(0xFF);
		send_to_ipad(0x55);
		send_to_ipad((usb_frame_size+6)&0xFF);
		send_to_ipad(0x00);
		send_to_ipad(0x42);
		send_to_ipad((frame_nb>>8)&0xFF);
		send_to_ipad(frame_nb&0xFF);
		send_to_ipad(0x00);
		send_to_ipad(session_id);
		
		for(i=0; i<usb_frame_size; ++i)
		{
			send_to_ipad(usb_frame_buffer[i]);
			crc = (crc + usb_frame_buffer[i]) & 0xFF;
		}
		
		crc = (0x154 - crc) & 0xFF;
		send_to_ipad(crc);
		
		frame_nb++;
		usb_frame_size = 0;
	}
}

ISR(USART1_UDRE_vect) // ipad transmit buffer is empty, send next byte
{
	if( i_send_pointer != i_send_end )
	{
		UDR1 = ipad_uart_send_buffer[i_send_pointer];
		i_send_pointer = (i_send_pointer + 1) % UART_BUFFER_SIZE;
	}
	else
	{
		// nothing to send, disable interrupts
		UCSR1B &= 0xDF;
	}
}

ISR(USART2_UDRE_vect) // accessory transmit buffer is empty, send next byte
{
	if( a_send_pointer != a_send_end )
	{
		UDR2 = accessory_uart_send_buffer[a_send_pointer];
		a_send_pointer = (a_send_pointer + 1) % UART_BUFFER_SIZE;
	}
	else
	{
		// nothing to send, disable interrupts
		UCSR2B &= 0xDF;
	}
}

ISR(USART0_UDRE_vect) // usb transmit buffer is empty, send next byte
{
	if( u_send_pointer != u_send_end )
	{
		UDR0 = usb_uart_send_buffer[u_send_pointer];
		u_send_pointer = (u_send_pointer + 1) % UART_BUFFER_SIZE;
	}
	else
	{
		// nothing to send, disable interrupts
		UCSR0B &= 0xDF;
	}
}

ISR(USART1_RX_vect) // received one byte from ipad
{
	parse_new_byte_from_ipad(UDR1);
}

ISR(USART2_RX_vect) // received one byte from accessory
{
	accessory_uart_rcv_buffer[a_rcv_end] = UDR2;
	a_rcv_end = (a_rcv_end + 1) % UART_BUFFER_SIZE;
}

ISR(USART0_RX_vect) // received one byte from third party
{
	parse_new_byte_from_usb(UDR0);
}

int main()
{
	unsigned char c;
	
	cli();
	init_uart();
	sei();
	
	while( 1 )
	{
		while( read_from_ipad(&c) )
		{
			send_to_accessory(c);
		}
		
		while( read_from_accessory(&c) )
		{
			send_to_ipad(c);
		}
	}
	
	return 0;
}
