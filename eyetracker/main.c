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

// Function Prototypes
void initialize(void);
void gyro(void);
void i2cSetBitrate(unsigned short bitrateKHz);

void initialize(void) {
	//1 = output, 0 = input
	DDRC = 0b00000001; //Port C1 is Output
	PORTC = 0b00000011; // Port C1 and C2 is pulled high
	i2cSetBitrate(40);
	TWCR |= (1<<TWEN);
}

//**********************************************************
// --- external interrupt ISR ------------------------
// Blinking indicator LEDs
void gyro(void)
{
	char data;
	int i;
	unsigned char address = 0x00; //
	unsigned char ITG3200R = 0xD3; //Possibly 0x68 => D1
	unsigned char ITG3200W = 0xD2; //Possibly 0x68 => D0
	while (1)
	{
		printf("Starting\n");
		//Disable, Enable 
		TWCR &= ~(1<<TWEN);
		TWCR |= (1<<TWEN);
		
		//Send Start
		DDRC = DDRC | 0b00000010; // Port C2 is Output when writing
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

		printf("start sent\n");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000))
		{i++;}
		i = 0;

		//Send gyro write
		TWDR = ITG3200W;
		TWCR = (1<<TWINT)|(1<<TWEN);

		printf("sent gyro write");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000)){ i++; }
		i = 0;

		//Send gyro address
		TWDR = address;
		TWCR = (1<<TWINT)|(1<<TWEN);

		printf("sent gyro address");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000)){ i++; }
		i = 0;

		//Send Start
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

		//Send gyro read
		TWDR = ITG3200R;
		TWCR = (1<<TWINT)|(1<<TWEN);
		printf("start sent and gyro read sent");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000)){ i++; }
		i = 0;

		//Receive data, Send ack/nack
		TWCR = TWCR & 0x0F | (1<<TWINT) | (1<<TWEA);

		printf("sent ack/nack");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000)){ i++; }
		i = 0;

		//Get Data
		data = TWDR;
		printf("waiting for data");
		//Wait
		while (!(TWCR & (1<<TWINT)) && (i < 1000)){ i++; }
		i = 0;

		// STOP
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);

		printf("\nWHO_AM_I (0x00): 0x%x\n", data);

		//Disable, Enable 
		TWCR &= ~(1<<TWEN);
		TWCR |= (1<<TWEN);
	}
}

void i2cSetBitrate(unsigned short bitrateKHz)
{
	unsigned char bitrate_div;
	// Prescalar = 1
	// SCL freq = F_CPU/(16+2*TWBR))
	TWSR &= ~(1 << TWPS0); 
	TWSR &= ~(1 << TWPS1);
	
	//calculate bitrate division	
	bitrate_div = ((F_CPU/1000l)/bitrateKHz);
	if(bitrate_div >= 16)
		bitrate_div = (bitrate_div-16)/2;
	TWBR = bitrate_div; // 1st and 3rd bit on
}
//**********************************************************
// Displays the current sensor readings

int main(void)                                                              
{
  //init the UART -- trt_uart_init()	 is in trtUart.c
  trt_uart_init();
  stdout = stdin = stderr = &uart_str;
  fprintf(stdout,"\n\r TRT \n\r\n\r");

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
  trtCreateTask(gyro, 1000, SECONDS2TICKS(0.05), SECONDS2TICKS(0.05), &(args[0]));
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
