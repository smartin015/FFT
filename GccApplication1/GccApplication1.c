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

//#define NUM_LEDS 5
//uint16_t ledbins[NUM_LEDS];

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
		ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADIF)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0); //x128 prescale... 64kHz yields 
		
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

void display_spektrum() {
	uint16_t m, n, s;
	printf("\n\r-----------------");
	for (n = 0; n < FFT_N / 2; n++) {
		s = spektrum[n];
		s /= 512;
		printf("\n\r:%3d ", n, s);
		for (m = 0; m < s; m++) printf("*");
		
	}
}

int main (void)
{
	stdout = &uart_output;
	stdin  = &uart_input;
	DDRD = 0x05;
	//DDRC = 0x1F;
	uart_init();
	
	PORTC = 0x1F;
	_delay_ms(500);
	
	uint16_t max = 0;
	uint16_t m, n, s;
	for(;;) {
		
		capture_wave(capture, FFT_N);		
		fft_input(capture, bfly_buff);
		fft_execute(bfly_buff);
		fft_output(bfly_buff, spektrum);
		
		printf("\n\r--------------------");
		max = 0;
		//Note: skip first few b/c low frequency sucks.
		for (n = 4; n < FFT_N / 2; n++) {
			if (spektrum[n] > spektrum[max])
				max = n;
			//printf("\n\r%d", n*NUM_LEDS / FFT_N);
			//ledbins[(n*NUM_LEDS) / FFT_N] += spektrum[n];
		}
		
		/*
		printf("\n\r-----------------");
		for (n = 0; n < NUM_LEDS; n++) {
			s = ledbins[n];
			s /= 512;
			printf("\n\r:%3d ", n);
			for (m = 0; m < s; m++) printf("*");
			
		}
		
		char z;
		if (max > (FFT_N/2) - (FFT_N/10))
			z = 4;
		else if (max > (FFT_N/2) - 2*(FFT_N/10))
			z = 3;
		else if (max > (FFT_N/2) - 3*(FFT_N/10))
			z = 2;
		else if (max > (FFT_N/2) - 4*(FFT_N/10))
			z = 1;
		else z = 0;
		
		PORTC = (1 << z); 
	    */	
		//printf("\ninput=%u, execute=%u, output=%u", t1, t2, t3);
		_delay_ms(200);
		PORTD ^= 0x04;	
	}
	
}
