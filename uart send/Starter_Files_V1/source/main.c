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
#define BIT_0	( 1 << 0 )
#define BIT_4	( 1 << 4 )
#define STRING_LENGTH     19
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
TaskHandle_t lite_send_handler = NULL;
TaskHandle_t heavy_send_handler = NULL;
SemaphoreHandle_t xSemaphore = NULL;
const char send_string[STRING_LENGTH] =  "Hello From Task 1\n";
const char send_string2[STRING_LENGTH] = "Hello From Task 2\n";
char send_string3[STRING_LENGTH] ;
static uint8_t iteration =0 ;

/*-----------------------------------------------------------*/
void vTaskLiteUartSender(void * pvParameters)
{
	/* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
	
	
    for( ;; )
    {
		/* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        uint8_t index ;
		if( xSemaphoreTake( xSemaphore,0) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
			for(index =0 ; index < 10 ; index++)
			{
				vSerialPutString(send_string,strlen(send_string));
				vTaskDelay(5);
			}
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
            xSemaphoreGive( xSemaphore );
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */
        }
		vTaskDelay(50);
	}
}

void vTaskHeavyUartSender(void * pvParameters)
{
	/* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
    for( ;; )
    {
		/* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
		uint8_t index ;
		uint32_t count ;
        if( xSemaphoreTake( xSemaphore,0) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
			for(index =0 ; index < 10 ; index++ )
			{	
				vSerialPutString(send_string2,strlen(send_string2));
				for(count = 0 ; count < 100000 ; count++);
            }

            /* We have finished accessing the shared resource.  Release the
            semaphore. */
            xSemaphoreGive( xSemaphore );
			
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */
        }
		vTaskDelay(500);
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
    
	/* Create a mutex type semaphore. */
	xSemaphore = xSemaphoreCreateMutex();


	 xTaskCreate(
                    vTaskHeavyUartSender,       /* Function that implements the task. */
                    "heavy send",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    3,/* Priority at which the task is created. */
                    &heavy_send_handler );      /* Used to pass out the created task's handle. */
    /* Create Tasks here */
	 xTaskCreate(
                    vTaskLiteUartSender,       /* Function that implements the task. */
                    "lite send",          /* Text name for the task. */
                    configMINIMAL_STACK_SIZE,      /* Stack size in words, not bytes. */
                    ( void * )NULL,    /* Parameter passed into the task. */
                    2,/* Priority at which the task is created. */
                    &lite_send_handler );      /* Used to pass out the created task's handle. */
					    /* Create Tasks here */
	
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


