/* This is John and Michael. We are awesome! */

#define F_CPU 16000000UL //GO CLOCK GO!

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h> // needed for lcd_lib
#include "lcd_lib.h"
#include "uart.h"

#define t1 200  

int8_t lcd_buffer[17];	// LCD display buffer
const int8_t LCD_initialize[] PROGMEM = "LCD Initialized\0";
const int8_t LCD_number[] PROGMEM = "Cap=\0";
uint8_t anipos, dir, count;	// move a character around  
uint16_t LCD_capacitance;

// timeout counter for task1
volatile unsigned char time1; 

// timer 1 capture variables for computing sq wave period	
volatile unsigned int T1capture;

// Capacitance in nF
volatile unsigned int capacitance;

//Function Prototypes
void task1(void);
void init_lcd(void);
void initialize(void);
void blinkLED(void);

void task1(void)
{
	DDRB=(0<<DDB3);	// Port b.3 is an input
	DDRB=(1<<DDB2); // and port b.2 is an output
	
	if (T1capture > 100)
	{
		//Compute Capacitance
		capacitance = (int)T1capture/(11207); //amount of time for charging capacitance to comparator switch
										  // Assuming R3/R3 + R4 ratio is 0.6 and the capicitance is between 1 nF and 100 nF
		//Update LCD	
		sprintf(lcd_buffer,"%-u",capacitance);
		LCDGotoXY(4, 0);
	  	// display the count 
		LCDstring(lcd_buffer, strlen(lcd_buffer));	
	}
	DDRB=(0<<DDB2); // Set port b.2 input to charge cap
}

void init_lcd(void) 
{
	LCDinit();				//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
	CopyStringtoLCD(LCD_initialize, 0, 0);
}
void initialize(void) 
{
	TCCR0A = (1<<WGM01);	      // Set WGM bits to 010 to set desired mode (CTC mode, pg. 106)
	TCCR0B = (1<<CS01)|(1<<CS00); // Bits 0 and 1. Set timer 0 prescaler to 64
	TIMSK0 = (1<<OCIE0A); 		  // Bit 1: Enable compare match for timer0 interrupts
	OCR0A = 249;				  // Set the compare register to 250 time ticks
	
	// Set up timer1 for full speed
	TCCR1B = (1<<ICES1)|(1<<CS00);  // Set capture to positive edge, and set timer1 for full speed
	TIMSK1 = (1<<ICIE1);            // Turn on timer1 interrupt-on-capture


	ACSR = (1<<ACIC) ; 			// Set analog comp to connect to timer capture input
	TIFR1 = (1<<ICF1);
	DDRB = 0;          			// Comparator negative input is B.3

	//LCD init
	init_lcd();
	LCDclr();

	CopyStringtoLCD(LCD_number, 0, 0);
	T1capture = 0;

	//set up the ports
  	DDRD |= 0x04;  // and d.2 which runs another LED
	sei();
}

void blinkLED(void) 
{
	// blink the onboard LED
	PORTD ^= 0x04;
}

//Timer 0 overflow ISR
ISR (TIMER0_COMPA_vect) 
{
  if (time1>0)	--time1;
}
//Timer 1 capture ISR
ISR (TIMER1_CAPT_vect) 
{
    T1capture = ICR1;
	blinkLED();
} 
int main(void)
{
	initialize();
	while(1){if (time1==0){time1=t1; task1();}}		
}