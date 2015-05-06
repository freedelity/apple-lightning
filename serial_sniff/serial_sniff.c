#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_BUFFER_SIZE 512

unsigned char hex_symbols[16] = {	'0', '1', '2', '3',
									'4', '5', '6', '7',
									'8', '9', 'A', 'B',
									'C', 'D', 'E', 'F'
								};

unsigned char ipad_uart_rcv_buffer[UART_BUFFER_SIZE];
unsigned int i_rcv_pointer;
unsigned int i_rcv_end;

unsigned char usb_uart_send_buffer[UART_BUFFER_SIZE];
unsigned int usb_send_pointer;
unsigned int usb_send_end;

void init_uart()
{
	// uart from ipad is uart1
	UCSR1C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR1 = 16; // 57.6k
	UCSR1B = 0x10; // enable reception
	i_rcv_pointer = 0;
	i_rcv_end = 0;
	UCSR1B |= 0x80; // enable rx complete interrupt
	
	// uart for USB is uart0
	UCSR0C = 0x06; // 8 bit, no parity, 1 stop bit
	UBRR0 = 16; // 57.6k
	UCSR0B = 0x08; // enable transmission
	usb_send_pointer = 0;
	usb_send_end = 0;
}

inline void send_to_usb(unsigned char byte)
{
	usb_uart_send_buffer[usb_send_end] = byte;
	usb_send_end = (usb_send_end + 1) % UART_BUFFER_SIZE;
	
	UCSR0B |= 0x20; // enable interrupts
}

inline void send_hex_to_usb(unsigned char byte)
{
	send_to_usb(hex_symbols[(byte>>4)&0x0F]);
	send_to_usb(hex_symbols[(byte&0x0F)]);
	send_to_usb('\r');
	send_to_usb('\n');
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

ISR(USART0_UDRE_vect) // usb uart transmit buffer is empty, send next byte
{
	if( usb_send_pointer != usb_send_end )
	{
		UDR0 = usb_uart_send_buffer[usb_send_pointer];
		usb_send_pointer = (usb_send_pointer + 1) % UART_BUFFER_SIZE;
	}
	else
	{
		// nothing to send, disable interrupts
		UCSR0B &= 0xDF;
	}
}

ISR(USART1_RX_vect)
{
	// received one byte from ipad
	// put it in receive buffer
	ipad_uart_rcv_buffer[i_rcv_end] = UDR1;
	i_rcv_end = (i_rcv_end + 1) % UART_BUFFER_SIZE;
}

int main()
{
	unsigned char c;
	
	cli();
	init_uart();
	sei();
	
	while( 1 )
	{
		// while there are bytes in receive buffer from ipad
		// send them to USB UART in hex format
		while( read_from_ipad(&c) )
		{
			send_hex_to_usb(c);
		}
	}
	
	return 0;
}
