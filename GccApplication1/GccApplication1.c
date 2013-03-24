/*------------------------------------------------*/
/* FFTEST : A test program for FFT module         */
#define	F_CPU		8000000
#define BAUD        9600

#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <avr/sfr_defs.h>
#include "suart.h"		/* Defs for using Software UART module (Debugging via AVRSP-COM) */
#include "ffft.h"		/* Defs for using Fixed-point FFT module */

int16_t capture[FFT_N];			/* Wave captureing buffer */
complex_t bfly_buff[FFT_N];		/* FFT buffer */
uint16_t spektrum[FFT_N/2];		/* Spectrum output buffer */

/*------------------------------------------------*/
/* UART data functions                          */

void uart_init()
{
	/*Set baud rate */
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 1 stop bit */
	UCSR0C = (3<<UCSZ00);
}


void uart_putchar(char c, FILE *stream) {
	/*if (c == '\n') {
		uart_putchar('\r', stream);
	}*/
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

char uart_getchar(FILE *stream) {
	loop_until_bit_is_set(UCSR0A, RXC0); /* Wait until data exists. */
	return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);
FILE uart_io = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
/*------------------------------------------------*/
/* Capture waveform                               */

void capture_wave (int16_t *buffer, uint16_t count)
{	
	/*
	//TEST: Generate cosine
	double incr = (double)( 10 * 2 * M_PI / count );
	double angle = 0;
	do {
		*buffer++ = (int)(16384 * cos(angle));
		angle += incr;
	} while(--count);*/
	
	ADMUX = _BV(REFS0)|_BV(ADLAR)|_BV(MUX2)|_BV(MUX0);	// channel 5

	do {
		ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADIF)|_BV(ADPS2)|_BV(ADPS1);
		
		loop_until_bit_is_set(ADCSRA, ADIF);
		*buffer++ = ADC - 32768;
	} while(--count);

	ADCSRA = 0;
}


/* This is an alternative function of capture_wave() and can omit captureing buffer.

void capture_wave_inplace (complex_t *buffer, uint16_t count)
{
	const prog_int16_t *window = tbl_window;
	int16_t v;

	ADMUX = _BV(REFS0)|_BV(ADLAR)|_BV(MUX2)|_BV(MUX1)|_BV(MUX0);	// channel

	do {
		ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADFR)|_BV(ADIF)|_BV(ADPS2)|_BV(ADPS1);
		while(bit_is_clear(ADCSRA, ADIF));
		v = fmuls_f(ADC - 32768, pgm_read_word_near(window));
		buffer->r = v;
		buffer->i = v;
		buffer++; window++;
	} while(--count);

	ADCSRA = 0;
}
*/

/*------------------------------------------------*/
/* Online Monitor via an ISP cable                */

int main (void)
{
	char *cp;
	uint16_t m, n, s;
	uint16_t t1,t2,t3;
	DDRD = 0x05;
	stdout = &uart_output;
	stdin  = &uart_input;
	
	
	//DDRE = 0b00000010;	/* PE1:<conout>, PE0:<conin> in N81 38.4kbps */
	uart_init();
		
		
	char UART_BUF[128];
	for(;;) {
		capture_wave(capture, FFT_N);		
		fft_input(capture, bfly_buff);
		fft_execute(bfly_buff);
		fft_output(bfly_buff, spektrum);
		printf("\n\r-----------------");
		
		for (n = 0; n < FFT_N / 2; n++) {
			s = spektrum[n];
			s /= 512;
			printf("\n\r:%3d ", n, s);
			for (m = 0; m < s; m++) printf("*");
			
		}
		
		//printf("\ninput=%u, execute=%u, output=%u", t1, t2, t3);
		_delay_ms(200);
		PORTD ^= 0x04;
		_delay_ms(200);
		
	}
	
}
