#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define STACK_SIZE 2000

static void vSenderTask ( void *pvParameters );
static void vReceiverTask( void *pvParameters );

/*Declare a variable of type QueueHandle_t. This is used to store the handle
to the queue that is accessed by all three tasks.*/
QueueHandle_t xQueue;

/* Define an enumerated type used to identify the source of the data. */
typedef enum
{
	eSender1,
	eSender2
} DataSource_t;

/* Define the structure type that will be passed on the queue. */
typedef struct xExampleStructure
{
	uint8_t ucValue;
	DataSource_t eDataSource;
} Data_t;

/* Declare two variables of type Data_t that will be passed on the queue. */
static const Data_t xStructsToSend[ 2 ] = 
{
	{100, eSender1}, /* Used by Sender1. */
	{200, eSender2}  /* Used by Sender1. */
};

void app_main(void)
{
	/* The queue is created to hold a maximum of 3 structures of type Data_t. */
	xQueue = xQueueCreate( 3, sizeof( Data_t ));

	if( xQueue != NULL )
	{
		/* Create two instances of the task that will write to the queue. The
		   parameter is used to pass the structure that the task will write to the
           queue, so one task will continuously send xStructsToSend[ 0 ] to the queue
           while the other task will continuously send xStructsToSend[ 1 ]. Both
           tasks are created at priority 2, which is above the priority of the receiver. */
		xTaskCreate( vSenderTask, "Sender1", STACK_SIZE, &( xStructsToSend[0] ), 2, NULL );
		xTaskCreate( vSenderTask, "Sender2", STACK_SIZE, &( xStructsToSend[1] ), 2, NULL );

		/* Create the task that will read from the queue. The task is created with
		   priority 1, so below the priority of the sender tasks. */
		xTaskCreate( vReceiverTask, "Receiver", STACK_SIZE, NULL, 1, NULL);

	}
	else
	{
		/* The queue could not be created. */
		printf("Queue could not be created\r\n");		
	}

	/* If all is well then main() will never reach here as the scheduler will
	   now be running the tasks. If main() does reach here then it is likely that
       there was insufficient heap memory available for the idle task to be created.
       Chapter 2 provides more information on heap memory management. */
       for( ;; );
}

static void vSenderTask( void *pvParameters )
{
	BaseType_t xStatus;
	const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );

	for(;;)
	{
		
		/* Send to the queue.
		   The second parameter is the address of the structure being sent. The
		address is passed in as the task parameter so pvParameters is used
		directly.
		The third parameter is the Block time - the time the task should be kept
		in the Blocked state to wait for space to become available on the queue
		if the queue is already full. A block time is specified because the
		sending tasks have a higher priority than the receiving task so the queue
		is expected to become full. The receiving task will remove items from
		the queue when both sending tasks are in the Blocked state. */

		xStatus = xQueueSendToBack( xQueue, pvParameters, xTicksToWait );

		if( xStatus != pdPASS )
		{
			/* The send operation could not complete, even after waiting for 100ms.
				This must be an error as the receiving task should make space in the
				queue as soon as both sending tasks are in the Blocked state. */
			printf( "Could not send to the queue.\r\n");
		}
	}
}

static void vReceiverTask( void *pvParameters )
{
	Data_t xReceivedStructure;
	BaseType_t xStatus;

	for(;;)
	{
		/* Because it has the lowest priority this task will only run when the
			sending tasks are in the Blocked state. The sending tasks will only enter
			the Blocked state when the queue is full so this task always expects the
			number of items in the queue to be equal to the queue length, which is 3 in
		this case. */

		if( uxQueueMessagesWaiting( xQueue ) != 3)
		{
			printf("Queue should have been full!\r\n");
		}

		/* Receive from the queue.
		The second parameter is the buffer into which the received data will be
		placed. In this case the buffer is simply the address of a variable that
		has the required size to hold the received structure.
		The last parameter is the block time - the maximum amount of time that the
		task will remain in the Blocked state to wait for data to be available
		if the queue is already empty. In this case a block time is not necessary
		because this task will only run when the queue is full. */
		xStatus = xQueueReceive( xQueue, &xReceivedStructure, 0 );

		if( xStatus == pdPASS )
		{
			
			/* Data was successfully received from the queue, print out the received
			value and the source of the value. */
			if( xReceivedStructure.eDataSource == eSender1 )
			{
				printf("From Sender 1 = %d\n", xReceivedStructure.ucValue );
			}

			else
			{
				printf("From Sender 2 = %d\n", xReceivedStructure.ucValue );
			}

		}

		else
		{
			/* Nothing was received from the queue. This must be an error as this
               task should only run when the queue is full. */
			printf("Could not receive from the queue.\r\n");
		}

	}
}