/*Each software timer has an ID, which is a tag value that can be used by the application writer
for any purpose. The ID is stored in a void pointer (void *), so can store an integer value
directly, point to any other object, or be used as a function pointer.

An initial value is assigned to the ID when the software timer is created—after which the ID
can be updated using the vTimerSetTimerID() API function, and queried using the
pvTimerGetTimerID() API function.

***************************************************************************************************************************************************************

void vTimerSetTimerID( const TimerHandle_t xTimer, void *pvNewID );

xTimer - the handle of the software timer being updated with a new ID value.
		 The handle will have been returned from the call to xTimerCreate()
		 used to create the software timer.

pvNewID - the value t owhich the software timer's ID will be set.

***************************************************************************************************************************************************************

void *pvTimerGetTimerID( TimerHandle_t xTimer );

xTimer - the handle of the software timer being updated with a new ID value.
		 The handle will have been returned from the call to xTimerCreate()
		 used to create the software timer.

Returned value - the ID of the software timer being queried.


************************USING THE CALLBACK FUNCTION PARAMETER AND THE SOFTWARE TIMER ID***************************************************/

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

static void prvTimerCallback( TimerHandle_t xTimer );
TimerHandle_t xAutoReloadTimer, xOneShotTimer; /*These variables will receive the software timer handle upon the creation of the timer.*/

void app_main(void)
{

	BaseType_t xTimer1Started, xTimer2Started; /*These variables receive pdTRUE or pdFALSE depending if the timer could be started or nor*/

	/*Note now that the software timers will be associated with the same callback function prvTimerCallback.
	  prvTimerCallback() will execute when either timer expires. The implementation of
      prvTimerCallback() uses the function’s parameter to determine if it was called because the
      one-shot timer expired, or because the auto-reload timer expired.*/
	xOneShotTimer = xTimerCreate(
				    /* Text name for the software timer - not used by FreeRTOS. */
					"OneShot",
					/*The software timer's period in ticks*/
					mainONE_SHOT_TIMER_PERIOD,
					/*Setting uxAutoReload to pdFALSE creates a one-shot software timer. */
					pdFALSE,
	/*------------> The timer’s ID is initialized to 0. */
					0,
					/*The callback function t obe used by the software timer being created. */
					prvTimerCallback );



	xAutoReloadTimer = xTimerCreate(
				    /* Text name for the software timer - not used by FreeRTOS. */
					"AutoReload",
					/*The software timer's period in ticks*/
					mainAUTO_RELOAD_TIMER_PERIOD,
					/*Setting uxAutoReload to pdTRUE creates a auto-reload software timer. */
					pdTRUE,
					/* The timer’s ID is initialized to 0. */
					0,
					/*The callback function to be used by the software timer being created. */
					prvTimerCallback );

		/*Check the software timers were created*/
	if( (xOneShotTimer != NULL) && (xAutoReloadTimer != NULL ) )
	{
		printf("Software timers created\r\n");
		xTimer1Started = xTimerStart( xOneShotTimer, 0 );
		xTimer2Started = xTimerStart( xAutoReloadTimer, 0 );
	}

	else printf("Software timers not created\r\n");

}


static void prvTimerCallback( TimerHandle_t xTimer )
{
	TickType_t xTimeNow;
	uint32_t ulExecutionCount;
	/*A count of the number of times this software timer has expired is stored in the timer's
	ID. Obtain the ID, increment it, then save it as the new ID value. The ID is a void
	pointer, so is cast to a uint32_t. */
	ulExecutionCount = (uint32_t) pvTimerGetTimerID( xTimer );
	ulExecutionCount++;
	vTimerSetTimerID( xTimer, ( void *) ulExecutionCount );

	/* Obtain the current tick count. */
	xTimeNow = xTaskGetTickCount();

	/*The handle of the one-shot timer was stored in xOneShotTimer when the timer was created.
	Compare the handle passed into this function with xOneShotTimer to determine if it was the
	one-shot or auto-reload timer that expired, then output a string to show the time at which
	the callback was executed. */
	if( xTimer == xOneShotTimer ) printf("One-shot timer callback executing %d\n", xTimeNow );

	else printf("Auto-reload timer callback executing %d\n", xTimeNow );

	if( ulExecutionCount == 5)
	{
		/*Stop the auto-reload timer after it has executed 5 times. This callback function
		executes in the context of the RTOS daemon task so must not call any functions that
		might place the daemon task into the Blocked state. Therefore a block time of 0 is
		used.*/
		xTimerStop( xTimer, 0 );
	}

}