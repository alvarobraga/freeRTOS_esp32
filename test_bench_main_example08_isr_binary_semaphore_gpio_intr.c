// 1. The interrupt occurred.
// 2. The ISR executed and ‘gave’ the semaphore to unblock the task.
// 3. The task executed immediately after the ISR, and ‘took’ the semaphore.
// 4. The task processed the event, then attempted to ‘take’ the semaphore again—entering
//    the Blocked state because the semaphore was not yet available (another interrupt had
//    not yet occurred).

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "freertos/semphr.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define STACK_SIZE 2000
#define LED 2
#define TOGGLE 18
#define LED_BLUE 5

SemaphoreHandle_t xBinarySemaphore = NULL;
bool ledStatus = false, ledStatusBlue = false;

/*Interrupt service routine called when button is pressed*/
static void IRAM_ATTR vButtonISRhandler( void )
{
	BaseType_t xHigherPriorityTaskWoken;

	/* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
	   it will get set to pdTRUE inside the interrupt safe API function if a
       context switch is required. */
    xHigherPriorityTaskWoken = pdFALSE;


	/*BaseType_t xSemaphoreGiveFromISR( SemaphoreHandle_t xBinarySemaphore,
									  BaseType_t *pxHigherPriorityTaskWoken );

	xBinarySemaphore - the xBinarySemaphore being 'given'.
				 A xBinarySemaphore is referenced by a variable of BaseType_t
				 SemaphoreHandle_t, and must be explicity created before
				 being used.

	pxHigherPriorityTaskWoken - It is possible that a single semaphore will have one or more
								tasks blocked on it waiting for the semaphore to become
								available. Calling xSemaphoreGiveFromISR() can make the
								semaphore available, and so cause a task that was waiting
								for the semaphore to leave the Blocked state. If calling
								xSemaphoreGiveFromISR() causes a task to leave the
								Blocked state, and the unblocked task has a priority higher
								than the currently executing task (the task that was
								interrupted), then, internally, xSemaphoreGiveFromISR() will
								set *pxHigherPriorityTaskWoken to pdTRUE.

								If xSemaphoreGiveFromISR() sets this value to pdTRUE,
								then normally a context switch should be performed before
								the interrupt is exited. This will ensure that the interrupt
								returns directly to the highest priority Ready state task.

	Returned value - There are two possible return values:
						1. pdPASS
					 pdPASS will be returned only if the call to
					 xSemaphoreGiveFromISR() is successful.
						2. pdFAIL
					 If a semaphore is already available, it cannot be given,
					 and xSemaphoreGiveFromISR() will return pdFAIL.*/

	// xSemaphoreGiveFromISR(xBinarySemaphore, NULL);
	xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);


	/* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR(). If
	   xHigherPriorityTaskWoken was set to pdTRUE inside xSemaphoreGiveFromISR()
       then calling portYIELD_FROM_ISR() will request a context switch. If
       xHigherPriorityTaskWoken is still pdFALSE then calling
       portYIELD_FROM_ISR() will have no effect.*/

	if(xHigherPriorityTaskWoken)	portYIELD_FROM_ISR();

}

static void vButtonTask( void *pvParameters )
{
	for(;;)
	{
		/*BaseType_t xSemaphoreTake( SemaphoreHandle_t xBinarySemaphore, TickType_t xTicksToWait );

		xBinarySemaphore - The semaphore being ‘taken’.
					 A semaphore is referenced by a variable of type SemaphoreHandle_t. It
					 must be explicitly created before it can be used.

		xTicksToWait - The maximum amount of time the task should remain in the Blocked
					   state to wait for the semaphore if it is not already available.
					   If xTicksToWait is zero, then xSemaphoreTake() will return immediately if
                       the semaphore is not available.
                       Setting xTicksToWait to portMAX_DELAY will cause the task to wait
                       indefinitely (without a timeout) if INCLUDE_vTaskSuspend is set to 1 in
                       FreeRTOSConfig.h


        Returned value - There are two possible return values:
                            1. pdPASS
                         pdPASS is returned only if the call to xSemaphoreTake() was
                         successful in obtaining the semaphore.
                         If a block time was specified (xTicksToWait was not zero), then it is
                         possible that the calling task was placed into the Blocked state to wait
                         for the semaphore if it was not immediately available, but the
                            2. pdFALSE
                         The semaphore is not available.
                         If a block time was specified (xTicksToWait was not zero), then the
                         calling task will have been placed into the Blocked state to wait for the
                         semaphore to become available, but the block time expired before this
                         happened.*/

		if( xSemaphoreTake( xBinarySemaphore, portMAX_DELAY ) == pdTRUE )
		{
			printf("BUTTON PRESSED!!\r\n");
			ledStatus = !ledStatus;
			gpio_set_level(LED, ledStatus);

		}

	}
}


static void vPeriodicTask( void *pvParameters )
{
	const TickType_t xDelay300ms = pdMS_TO_TICKS( 300UL );

	for(;;)
	{
		/* Block until it is time to generate the software interrupt again. */
		ledStatus = !ledStatus;
		gpio_set_level(LED_BLUE, ledStatus);
		vTaskDelay( xDelay300ms );
	}
}



void app_main()
{
	/* Before a semaphore is used it must be explicitly created. In this example
       a semaphore is created. */
  xBinarySemaphore = xSemaphoreCreateBinary();

  gpio_pad_select_gpio(LED);
  gpio_pad_select_gpio(TOGGLE);
  gpio_pad_select_gpio(LED_BLUE);

  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_direction(TOGGLE, GPIO_MODE_INPUT);
  gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);

  /*Enable interruption on falling edge*/
  gpio_set_intr_type(TOGGLE, GPIO_INTR_NEGEDGE);

  xTaskCreate(vButtonTask,
                "Task that handles the pressing of the toggle",
                STACK_SIZE,
                NULL,
                2,
                NULL);

   xTaskCreate(vPeriodicTask,
                "Blinks blue LED periodically",
                STACK_SIZE,
                NULL,
                1,
                NULL);

  /*Install ISR service that will handle the toggle */
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

  /*Attach the interrupt service routine*/
  gpio_isr_handler_add(TOGGLE, vButtonISRhandler, NULL);

}