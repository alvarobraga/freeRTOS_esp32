/*Software timers are used to schedule the execution of a function at a set time in the future, or
periodically with a fixed frequency. The function executed by the software timer is called the
software timer’s callback function.

The only thing special about software timer’s callback function is their prototype, which must return void, and take a handle to a software timer as
its only parameter. void ATimerCallback( TimerHandle_t xTimer );

They do not require hardware support, and are not related to hardware timers or hardware
counters.

All software timer callback functions execute in the context of the same RTOS daemon (or
‘timer service’) task1.

The daemon task is a standard FreeRTOS task that is created automatically when the
scheduler is started.

Software timer API functions send commands from the calling task to the daemon task on a
queue called the ‘timer command queue’. Examples of commands include ‘start a timer’, ‘stop a timer’ and ‘reset a timer’.

The daemon task is scheduled like any other FreeRTOS task; it will only process commands,
or execute timer callback functions, when it is the highest priority task that is able to run.

Commands sent to the timer command queue contain a time stamp. The time stamp is used
to account for any time that passes between a command being sent by an application task,
and the same command being processed by the daemon task. For example, if a ‘start a timer’
command is sent to start a timer that has a period of 10 ticks, the time stamp is used to ensure
the timer being started expires 10 ticks after the command was sent, not 10 ticks after the
command was processed by the daemon task.

***************************************************************************************************************************************************************

TimerHandle_t xTimerCreate( const char * const pcTimerName,
							TickType_t xTimerPeriodInTicks,
							UBaseType_t uxAutoReload,
							void * pvTimerID,
							TimerCallbackFunction_t pxCallbackFunction );

pcTimerName - A descriptive name for the timer. This is not used by FreeRTOS in
			  any way. It is included purely as a debugging aid. Identifying a timer
              by a human readable name is much simpler than attempting to identify
              it by its handle.

xTimerPeriodInTicks - The timer’s period specified in ticks. The pdMS_TO_TICKS() macro
					  can be used to convert a time specified in milliseconds into a time
					  specified in ticks.

uxAutoReload - Set uxAutoReload to pdTRUE to create an auto-reload timer. Set
			   uxAutoReload to pdFALSE to create a one-shot timer.

pvTimerID - Each software timer has an ID value. The ID is a void pointer, and can
		    be used by the application writer for any purpose. The ID is
		    particularly useful when the same callback function is used by more
			than one software timer, as it can be used to provide timer specific
			storage. pvTimerID sets an initial value for the ID of the task being created.

pxCallbackFunction - Software timer callback functions are simply C functions that conform
                     to the prototype shown in Listing 72. The pxCallbackFunction
                     parameter is a pointer to the function (in effect, just the function name)
                     to use as the callback function for the software timer being created.

Returned value - If NULL is returned, then the software timer cannot be created
				 because there is insufficient heap memory available for FreeRTOS to
                 allocate the necessary data structure.
                 A non-NULL value being returned indicates that the software timer has
                 been created successfully. The returned value is the handle of the
                 created timer.

***************************************************************************************************************************************************************
xTimerStart() is used to start a software timer that is in the Dormant state, or reset (re-start) a
software timer that is in the Running state. xTimerStop() is used to stop a software timer that
is in the Running state. Stopping a software timer is the same as transitioning the timer into
the Dormant state.

BaseType_t xTimerStart( TimerHandle_t xTimer, TickType_t xTicksToWait );

xTimer - The handle of the software timer being started or reset. The handle
		 will have been returned from the call to xTimerCreate() used to create
         the software timer.

xTicksToWait - xTimerStart() uses the timer command queue to send the ‘start a
			   timer’ command to the daemon task. xTicksToWait specifies the
               maximum amount of time the calling task should remain in the Blocked
               state to wait for space to become available on the timer command
               queue, should the queue already be full.



******************CREATING ONE-SHOT AND AUTO-RELOAD TIMERS***********************************************************
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* The periods assigned to the one-shot and auto-reload timers are 3.333 second and half a
second respectively. */
#define mainONE_SHOT_TIMER_PERIOD pdMS_TO_TICKS( 3333 )
#define mainAUTO_RELOAD_TIMER_PERIOD pdMS_TO_TICKS( 500 )

static void prvOneShotTimerCallback( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallback( TimerHandle_t xTimer );
// static void performTest(uint8_t testCode);

uint32_t ulCallCount = 0;

void app_main(void)
{

	/*One-shot timers: a one-shot timer will execute its callback function once only. A one-shot timer can be restarted manually, but will not restart itself.
      
      Auto-reload timers: an auto-reload timer will re-start itself each time it expires, resulting in periodic execution of its callback function.
	*/
	// performTest(1);  
	TimerHandle_t xAutoReloadTimer, xOneShotTimer; /*These variables will receive the software timer handle upon the creation of the timer.*/
	BaseType_t xTimer1Started, xTimer2Started; /*These variables receive pdTRUE or pdFALSE depending if the timer could be started or nor*/

	TickType_t xTimeNow;

	/* Create the one shot timer, storing the handle to the created timer in xOneShotTimer. */
	xOneShotTimer = xTimerCreate(
				    /* Text name for the software timer - not used by FreeRTOS. */
					"OneShot",
					/*The software timer's period in ticks*/
					mainONE_SHOT_TIMER_PERIOD,
					/*Setting uxAutoReload to pdFALSE creates a one-shot software timer. */
					pdFALSE,
					/*This example does not use the timer ID. */
					0,
					/*The callback function t obe used by the software timer being created. */
					prvOneShotTimerCallback	);


	/*Create the auto-reload timer, storing the handle to the created timer in xAutoReloadTimer*/
	xAutoReloadTimer = xTimerCreate(
				    /* Text name for the software timer - not used by FreeRTOS. */
					"AutoReload",
					/*The software timer's period in ticks*/
					mainAUTO_RELOAD_TIMER_PERIOD,
					/*Setting uxAutoReload to pdTRUE creates a auto-reload software timer. */
					pdTRUE,
					/*This example does not use the timer ID. */
					0,
					/*The callback function to be used by the software timer being created. */
					prvAutoReloadTimerCallback );

	/*Check the software timers were created*/
	if( (xOneShotTimer != NULL) && (xAutoReloadTimer != NULL ) )
	{
		printf("Software timers created\r\n");
		xTimer1Started = xTimerStart( xOneShotTimer, 0 );
		xTimer2Started = xTimerStart( xAutoReloadTimer, 0 );

		printf("xTimer1Started: %d\n", xTimer1Started);
	}

	else
	{
		printf("Software timers not created\r\n");
	}

}

static void prvOneShotTimerCallback( TimerHandle_t xTimer )
{
	TickType_t xTimeNow;

	/*Obtain the current tick count*/
	xTimeNow = xTaskGetTickCount();
	/* Output a string to show the time at which the callback was executed. */
	printf("One-shot timer callback executing %d\n", xTimeNow );

	/*File scope variable*/
	ulCallCount++;
}

static void prvAutoReloadTimerCallback( TimerHandle_t xTimerStart )
{
	TickType_t xTimeNow;
	/* Obtain the current tick count. */
    xTimeNow = xTaskGetTickCount();
    printf("Auto-reload timer callback executing %d\n", xTimeNow );

    ulCallCount++;

}


// static void performTest(uint8_t testCode)
// {
// 	BaseType_t xSchedulerTest;

// 	switch(testCode)
// 	{
// 		case 1:				
// 				xSchedulerTest = xTaskGetSchedulerState();
// 				if( xSchedulerTest ) printf("Scheduler running\n");
// 				else printf("Scheduler not running\n");
// 				break;

// 		default:
// 				printf("testCode not yet defined\r\n");
// 				break;
// 	}
// }