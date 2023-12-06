/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "semphr.h"

#include "task.h"
#include "event_groups.h"
#include "lpc21xx.h"


/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"
#define STRING_LENGTH     15
/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */

static void prvSetupHardware( void );
static uint8_t btn0_lastState    = 1;
static uint8_t btn0_currentState = 1;
static uint8_t btn1_currentState = 1;
static uint8_t btn1_lastState    = 1;
TaskHandle_t button0_handler = NULL;
TaskHandle_t button1_handler = NULL;
TaskHandle_t hello_send_handler = NULL;
TaskHandle_t consumer_handler = NULL;
QueueHandle_t xQueue = NULL;
const char str_btnZeroRisingEdge[STRING_LENGTH] 	=  "BTN_0_RISING\n";
const char str_btnZeroFallingEdge[STRING_LENGTH] 	=  "BTN_0_FALLING\n";
const char str_btnOneRisingEdge[STRING_LENGTH] 		=  "BTN_1_RISING\n";
const char str_btnOneFallingEdge[STRING_LENGTH] 	=  "BTN_1_FALLING\n";
const char str_hello[STRING_LENGTH] 				=  "Hello\n";
char send_string[STRING_LENGTH];
/*-----------------------------------------------------------*/
void vTaskButtonOneStatus( void * pvParameters )
{
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
	
	
    for( ;; )
    {
        /* Task code goes here. */
		btn0_currentState = GPIO_read(PORT_0,PIN0);
		if( btn0_currentState != btn0_lastState)
		{
			if(btn0_currentState == PIN_IS_LOW)
			{
				/* Set bit 0 and bit 4 in xEventGroup. */
				if( xQueue != 0 )
				{
					/* Send a pointer to a struct AMessage object.  Don't block if the
					queue is already full. */
					xQueueSend( xQueue, ( void * ) str_btnZeroFallingEdge, ( TickType_t ) 0 );
				}
				
			}
			else
			{
				if( xQueue != 0 )
				{
					/* Send a pointer to a struct AMessage object.  Don't block if the
					queue is already full. */
					
					xQueueSend( xQueue, ( void * ) str_btnZeroRisingEdge, ( TickType_t ) 0 );
				}
			}
		}
		btn0_lastState = btn0_currentState;
		vTaskDelay(100);
		
    }
}
void vTaskButtonTwoStatus( void * pvParameters )
{
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
	
	
    for( ;; )
    {
        /* Task code goes here. */
		btn1_currentState = GPIO_read(PORT_0,PIN2);
		if( btn1_currentState != btn1_lastState)
		{
			if(btn1_currentState == PIN_IS_LOW)
			{
				/* Set bit 0 and bit 4 in xEventGroup. */
				if( xQueue != 0 )
				{
					/* Send a pointer to a struct AMessage object.  Don't block if the
					queue is already full. */
					
					xQueueSend( xQueue, ( void * )str_btnOneFallingEdge, ( TickType_t ) 0 );
				}
				
			}
			else
			{
				if( xQueue != 0 )
				{
					/* Send a pointer to a struct AMessage object.  Don't block if the
					queue is already full. */
					
					xQueueSend( xQueue, ( void * ) str_btnOneRisingEdge, ( TickType_t ) 0 );
				}
			}
		}
		btn1_lastState = btn1_currentState;
		vTaskDelay(100);
    }
}

void vTaskHelloUartSender(void * pvParameters)
{
	/* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
	
	
    for( ;; )
    {
		/* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
		if( xQueue != 0 )
		{
			/* Send a pointer to a struct AMessage object.  Don't block if the
			queue is already full. */
			
			xQueueSend( xQueue, ( void * ) str_hello, ( TickType_t ) 0 );
		}
				
		vTaskDelay(100);
	}
}



void vTaskConsumer(void * pvParameters)
{
	/* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
	
	
    for( ;; )
    {
		/* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
	   if( xQueue != NULL )
	   {
		  /* Receive a message from the created queue to hold complex struct AMessage
		  structure.  Block for 10 ticks if a message is not immediately available.
		  The value is read into a struct AMessage variable, so after calling
		  xQueueReceive() xRxedStructure will hold a copy of xMessage. */
		  if( xQueueReceive( xQueue,
							 send_string,
							 ( TickType_t ) 0 ) == pdPASS )
		  {
			 /* xRxedStructure now contains a copy of xMessage. */
			  vSerialPutString(send_string,strlen(send_string));
		  }
	   }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */
        }
		vTaskDelay(50);
	}
}

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	

    /* Attempt to create the event group. */
    xQueue = xQueueCreate( 10, STRING_LENGTH );
    

/* Create Tasks here */
	  xTaskCreate(
                    vTaskButtonOneStatus,       /* Function that implements the task. */
                    "button one",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &button0_handler );      /* Used to pass out the created task's handle. */
    
	  xTaskCreate(
                    vTaskButtonTwoStatus,       /* Function that implements the task. */
                    "button two",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    3,/* Priority at which the task is created. */
                    &button1_handler );      /* Used to pass out the created task's handle. */
					
	  xTaskCreate(
                    vTaskHelloUartSender,       /* Function that implements the task. */
                    "hello send",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &hello_send_handler );      /* Used to pass out the created task's handle. */
    
	 xTaskCreate(
                    vTaskConsumer,       /* Function that implements the task. */
                    "consumer",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    3,/* Priority at which the task is created. */
                    &consumer_handler );      /* Used to pass out the created task's handle. */
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


