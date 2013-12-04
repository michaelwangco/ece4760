// EYETRACKER - John and Michael ECE 4760

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "trtSettings.h"
#include "trtkernel_1284.c"
#include <stdio.h>
#include <avr/sleep.h>

// serial communication library
// Don't mess with the semaphores
#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>
#include "trtUart.h"
#include "trtUart.c"
// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// semaphore to protect shared variable
#define SEM_SHARED 3
int args[3] ;



// ADC Variables and Defs
volatile int sensor_out;
volatile int ADC_out;
volatile int ADC_channel;
uint8_t ADC_result0;
uint8_t ADC_result1;

#define CHANNEL0 0
#define CHANNEL1 1
#define CHANNEL2 2
#define CHANNEL3 3
#define CHANNEL4 4
#define CHANNEL5 5
#define CHANNEL6 6
#define CHANNEL7 7

//typedef enum {CHANNEL0 = 0, CHANNEL1} channel;

// Function Prototypes
void initialize(void);
void init_ADC(void);
void read_ADC(void);
void sensor_task(void);
void switch_to_ADC(uint8_t ADC_channel);

void initialize(void) {
	DDRD = (0<<PIND2)|(0<<PIND3) ;
	DDRB = (0<<PINB2) ; //PINB and D receive eye IR through ext int
	DDRC = (1<<PINC0)|(1<<PINC1)|(1<<PINC2)| //PINC0-2 controls
		   (1<<PINC3)|(1<<PINC4)|(1<<PINC5); //PINC3-5 outputs blinks, and displays readings	

	PORTC = 0x04; //turn on both for testing
   
//   TCCR0B = (1<<CS01)  | (1<<CS00);
//   // turn on fast PWM and OC0A & OC0B output
//   // at full clock rate, toggle OC0A (pin B3) and OC0B (pin B4) 
//   // 16 microsec per PWM cycle sample time
//   TCCR0A = (0<<COM0A0) | (1<<COM0A1) | (0<<COM0B0) | (1<<COM0B1) | (1<<WGM00) | (1<<WGM01); 
//   OCR0A = 0 ; // set PWM to half full scale
//   OCR0B = 0 ;

	EIMSK = (1<<INT0)|(1<<INT1)|(1<<INT2) ; 	// turn on int0, int1, int2
	EICRA = (1<<ISC20)|(1<<ISC10)|(1<<ISC00) ;  // Any edge

	//TCCR2A = (1<<WGM21);
	//TCCR2B = 7 ; // divide by 1024
	// turn on timer 2 overflow ISR for double precision time
	//TIMSK2 = 1 ;
	//OCR2A = 255; //8-bit compare value
	init_ADC();
}

void init_ADC(void) {
  // ADC
  // Vref = 5V
  // ADC clock = 16 MHz / 128 = 125 kHz

  // set Vref to AVCC, left-adjust result into ADCH
  ADMUX = ( 1 << ADLAR ) | ( 1 << REFS0 );

  // enable ADC and set ADC division factor to 128
  ADCSRA = (1 << ADATE) | ( 1 << ADEN ) | ( 1 << ADIE ) | ( 1 << ADPS2 ) | ( 1 << ADPS1 ) | ( 1 << ADPS0 );
  ADC_out = 0;
  ADC_channel = CHANNEL0;
}

//**********************************************************
// --- external interrupt ISR ------------------------
// Blinking indicator LEDs
ISR (INT0_vect) { //PIND2 
	PORTC ^= 0x08;
    //sensor_out ^= 0x01;
}

ISR (INT1_vect) {  //PIND3
	PORTC ^= 0x10;
	//sensor_out ^= 0x02;
}

ISR (INT2_vect) {  //PINB2
	PORTC ^= 0x20;
	//sensor_out ^= 0x04;
}

ISR (TIMER2_OVF_vect) {  //FLIPS IR LEDs
	if (PORTC & 0x01)
	{
    	PORTC ^= 0x01;
		PORTC ^= 0x02;
	}
	else if (PORTC & 0x02)
	{
		PORTC ^= 0x02;
		PORTC ^= 0x04;
	}
	else if (PORTC & 0x04)
	{
		PORTC ^= 0x04;
    	PORTC ^= 0x01;
	}
	TCNT2 = 0;
}

ISR (ADC_vect) {
   ADC_out = ADCH;
   
   ADMUX &= 0b11110000;
   if (ADC_channel==1)
   {
	  ADMUX &= ~(1<<MUX0);
   }
   else if (ADC_channel ==0)
   {
   	  ADMUX |= (1<<MUX0);
   }
   //ADMUX = (15 << 4) & ADC_channel;
   ADCSRA |= (1 << ADSC); // start another conversion
}

//**********************************************************
// Displays the current sensor readings
void sensor_task(void) 
{
	uint32_t rel;
	uint32_t dead;
  	while(1)
  	{		
		//PORTA = sensor_out;
		fprintf(stdout,"ADC_out: %d\n", ADC_out);
	    rel = trtCurrentTime() + SECONDS2TICKS(0.1);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.1);
	  	trtSleepUntil(rel, dead);
	}
}

// Reads the ADC, prints, and switches to next channel
void read_ADC(void) {
	uint32_t rel;
	uint32_t dead;
	while(1) {
		if (ADC_channel == 0){
			ADC_result0 = ADC_out;
			fprintf(stdout,"Channel %d: MUX %d: %d\t", ADC_channel, ADMUX, ADC_result0);
		}
		else if (ADC_channel == 1){
			ADC_result1 = ADC_out;
			fprintf(stdout,"Channel %d: MUX %d: %d\t\n", ADC_channel, ADMUX, ADC_result1);
		}
		ADC_channel++;
		if (ADC_channel == 2) {ADC_channel = 0;}
	    rel = trtCurrentTime() + SECONDS2TICKS(0.1);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.1);
	  	trtSleepUntil(rel, dead);
	}	
}

int main(void)                                                              
{
  //init the UART -- trt_uart_init() is in trtUart.c
  trt_uart_init();
  stdout = stdin = stderr = &uart_str;
  fprintf(stdout,"\n\r TRT 9feb2009\n\r\n\r");

  initialize();
  // start TRT
  trtInitKernel(1000); // 80 bytes for the idle task stack
  // --- create semaphores ----------
  // You must creat the first two semaphores if you use the uart
  trtCreateSemaphore(SEM_RX_ISR_SIGNAL, 0) ; // uart receive ISR semaphore
  trtCreateSemaphore(SEM_STRING_DONE,0) ;  // user typed <enter>
  
  // variable protection
  trtCreateSemaphore(SEM_SHARED, 1) ; // protect shared variables

 // --- creat tasks  ----------------
  //trtCreateTask(sensor_task, 1000, SECONDS2TICKS(0.05), SECONDS2TICKS(0.05),  &(args[0]));
  trtCreateTask(read_ADC, 1000, SECONDS2TICKS(0.05), SECONDS2TICKS(0.05), &(args[0]));
  // --- Idle task --------------------------------------
  // just sleeps the cpu to save power 
  // every time it executes
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  while (1) 
  {
  	sleep_cpu();
  }
 } 
