// SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount,
// 											UBaseType_t uxInitialCount );

// uxMaxCount The maximum value to which the semaphore will count. To continue the
// 		   queue analogy, the uxMaxCount value is effectively the length of the
//            queue.
//            When the semaphore is to be used to count or latch events, uxMaxCount
//            When the semaphore is to be used to manage access to a collection of
//            resources, uxMaxCount should be set to the total number of resources
//            that are available.

// uxInitialCount The initial count value of the semaphore after it has been created.
//                When the semaphore is to be used to count or latch events,
//                uxInitialCount should be set to zero—as, presumably, when the
//                semaphore is created, no events have yet occurred.
//                When the semaphore is to be used to manage access to a collection of
//                resources, uxInitialCount should be set to equal uxMaxCount—as,
//                presumably, when the semaphore is created, all the resources are
//                available

// Returned value If NULL is returned, the semaphore cannot be created because there is
//                insufficient heap memory available for FreeRTOS to allocate the
//                semaphore data structures. Chapter 2 provides more information on
//                heap memory management.
//                A non-NULL value being returned indicates that the semaphore has been
//                created successfully. The returned value should be stored as the handle
//                to the created semaphore.

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_types.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "freertos/semphr.h"

/*DEFINES RELATED TO THE TIMERS*/

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (0.50)//(3.4179) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (1.0)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload
#define FROM_TIMER_0		  1
#define FROM_TIMER_1          2

/*DEFINES RELATED TO THE TASKS*/

#define STACK_SIZE_1          2048
#define STACK_SIZE_2		  5000

/*DEFINES RELATED TO THE ADC*/

#define DEFAULT_VREF          3300        
#define NO_OF_SAMPLES         64          //Multisampling

/*DEFINES RELATED TO DIGITAL INPUT AND OUTPUT*/

#define BUTTON                18
#define LED_BLUE              5
#define LED_RED               2

/*DEFINES RELATED TO THE WARNINGS*/

#define WARNING_1             500.0
#define WARNING_2			  1500.0
#define WARNING_3			  2000.0
#define WARNING_4			  2500.0
#define WARNING_5			  3200.0

/*DEFINES RELATED TO THE ISR*/

#define ESP_INTR_FLAG_DEFAULT 0
#define FROM_GPIO			  3

/*TYDEF DECLARATIONS*/

typedef float  Voltage_t;
typedef int8_t WarningCode_t;
typedef int8_t SourceIntr_t;

typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

/*ADC CONFIGURATION VARIABLES*/

static            esp_adc_cal_characteristics_t *adc_chars;
static const      adc_channel_t channel   =      ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const      adc_atten_t atten       =      ADC_ATTEN_DB_0;
static const      adc_unit_t unit         =      ADC_UNIT_1;

/*ISR VARIABLES*/

SemaphoreHandle_t xCountingSemaphore = NULL;

/*QUEUE VARIABLES*/

QueueHandle_t     xQueue;

/*GLOBAL VARIABLES*/

Voltage_t         voltage;
SourceIntr_t      sourceIntr    = 0;
bool              ledRedStatus  = 0;
bool              ledBlueStatus = 1;
WarningCode_t     warningCode;

/**************************************************************************/

static void vConfigADC(void)
{
	//Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } 
    else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

/**************************************************************************/

static void vConfigIO(void)
{
	gpio_pad_select_gpio(LED_BLUE);
	gpio_pad_select_gpio(LED_RED);
	gpio_pad_select_gpio(BUTTON);

	gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED,  GPIO_MODE_OUTPUT);
    gpio_set_direction(BUTTON,   GPIO_MODE_INPUT);

    gpio_set_intr_type(BUTTON, GPIO_INTR_NEGEDGE);
}

/**************************************************************************/

static void IRAM_ATTR vButtonISRhandler( void )
{
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(xCountingSemaphore, &xHigherPriorityTaskWoken);
	sourceIntr = FROM_GPIO;

	if(xHigherPriorityTaskWoken)	portYIELD_FROM_ISR();
}

/**************************************************************************/

static void IRAM_ATTR timer_group0_isr(void *para)
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    int timer_idx = (int) para;
    timer_intr_t timer_intr = timer_group_intr_get_in_isr(TIMER_GROUP_0);

    if (timer_intr & TIMER_INTR_T0) {
        timer_group_intr_clr_in_isr(TIMER_GROUP_0, TIMER_0);
        sourceIntr = FROM_TIMER_0;
    }

    else if (timer_intr & TIMER_INTR_T1) {
        timer_group_intr_clr_in_isr(TIMER_GROUP_0, TIMER_1);
        sourceIntr = FROM_TIMER_1;
    } 

    else printf("Event not supported"); // not supported even type

    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, timer_idx);

    xSemaphoreGiveFromISR(xCountingSemaphore, &xHigherPriorityTaskWoken);
    // xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken)	portYIELD_FROM_ISR();
}

/**************************************************************************/

static void example_tg0_timer_init(int timer_idx,
    bool auto_reload, double timer_interval_sec)
{

    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                       (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

/**************************************************************************/

static void example_evt_task(void *pvParameters)
{

    while (1) {

        if( xSemaphoreTake( xCountingSemaphore, portMAX_DELAY ) == pdTRUE )
        {
            switch(sourceIntr)
            {

            	case FROM_TIMER_0:
            		printf("INTERRUPTION FROM TIMER 0\r\n");
            		break;

            	case FROM_TIMER_1:
            		printf("INTERRUPTION FROM TIMER 1\r\n");
            		break;

            	case FROM_GPIO:
            		printf("INTERRUPTION FROM GPIO\r\n");
            		break;

            	default:
            		break;
            
            }

        }

    }
}

/**************************************************************************/

static void vPeriodicTask( void *pvParameters )
{
	TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{

		switch(warningCode)
		{
			case 0x00:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x00\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 600 ) );
				break;

			case 0x01:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x01\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 500 ) );
				break;

			case 0x02:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x02\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 400 ) );
				break;

			case 0x03:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x03\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 300 ) );
				break;

			case 0x04:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x04\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 200 ) );
				break;

			case 0x05:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("case 0x05\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 100 ) );
				break;

			default:
				gpio_set_level(LED_BLUE, !ledBlueStatus);
				ledBlueStatus = !ledBlueStatus;
				printf("default\r\n");
				vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 1000 ) );
				break;
		}

	}
}

/**************************************************************************/

static void vReadSensor( void *pvParameters )
{
	TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    BaseType_t xStatus;

	for(;;)
	{
		uint32_t adc_reading = 0;
        for (int i = 0; i < NO_OF_SAMPLES; i++)
         {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            } else {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;

        voltage = 3.3/4096.0 * adc_reading * 1000;
        printf("Raw: %d\tVoltage: %.2fmV\r\n", adc_reading, voltage);

        xStatus = xQueueSendToBack( xQueue, &voltage, 0 );

        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 1000 ) );
	}
}

/**************************************************************************/

static void vCheckThreshold( void *pvParameters )
{
	const TickType_t xTicksToWait = pdMS_TO_TICKS( 3100 );
	Voltage_t fReceivedVoltage;
	BaseType_t xStatus;

	for(;;)
	{

		xStatus = xQueueReceive( xQueue, &fReceivedVoltage, xTicksToWait );
		
		if( xStatus == pdPASS)
		{
			
			if(fReceivedVoltage >= WARNING_1)
			{
				if(fReceivedVoltage < WARNING_2)	    
				{
					printf("WARNING 1\r\n");
					warningCode = 0x01;
				}

				else if(fReceivedVoltage < WARNING_3)	
				{
					printf("WARNING 2\r\n");
					warningCode = 0x02;
				}

				else if(fReceivedVoltage < WARNING_4)
				{
					printf("WARNING 3\r\n");
					warningCode = 0x03;
				}

				else if(fReceivedVoltage < WARNING_5)	
				{
					printf("WARNING 4\r\n");
					warningCode = 0x04;
				}

				else                                    
				{
					printf("WARNING 5\r\n");
					warningCode = 0x05;
					gpio_set_level(LED_RED, ledRedStatus);
					ledRedStatus = !ledRedStatus;
				}		

			} 

			else  
			{
				printf("NO WARNINGS\r\n");
				warningCode = 0x00;
				gpio_set_level(LED_RED, 0);
			}

		}

		else
		{
			/* Data was not received from the queue even after waiting for 100ms.
			   This must be an error as the sending tasks are free running and will be
               continuously writing to the queue. */
			printf( "Could not receive from the queue.\r\n" );
		}

    }
	
}


/**************************************************************************/

void app_main()
{
	
	vConfigADC();
	vConfigIO();

	xCountingSemaphore     =    xSemaphoreCreateCounting( 10, 0 );
	// xBinarySemaphore = xSemaphoreCreateBinary();

   	xQueue   =    xQueueCreate( 3, sizeof( Voltage_t ));

	example_tg0_timer_init(TIMER_0, 
		                   TEST_WITH_RELOAD, 
		                   TIMER_INTERVAL0_SEC);

	example_tg0_timer_init(TIMER_1, 
		                   TEST_WITH_RELOAD, 
		                   TIMER_INTERVAL1_SEC);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(BUTTON, 
    	                 vButtonISRhandler, 
    	                 NULL);


    xTaskCreate(vPeriodicTask,
                "Blinks blue LED periodically",
                STACK_SIZE_1,
                NULL,
                1,
                NULL);

    xTaskCreate(example_evt_task, 
   	           "timer_evt_task", 
   	           STACK_SIZE_1, 
   	           NULL, 
   	           4, 
   	           NULL);



	if( xQueue != NULL )
	{

		xTaskCreate( vReadSensor, 
			         "Read ADC1", 
			         STACK_SIZE_1, 
			         NULL, 
			         5, 
			         NULL );

		xTaskCreate( vCheckThreshold, 
			         "Raise warnings", 
			         STACK_SIZE_1, 
			         NULL, 
			         3, 
			         NULL );
		printf("Queue created\r\n");

	}
	else
	{
		/* The queue could not be created. */
		printf("Queue could not be created\r\n");		
	}


}