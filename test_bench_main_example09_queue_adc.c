#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define STACK_SIZE 2000
#define DEFAULT_VREF    3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling
#define LED_BLUE   5
#define LED_RED   2
#define THRESHOLD 3260.00

typedef float Voltage_t;
typedef int8_t AlarmCode_t;

QueueHandle_t xQueue;
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;
Voltage_t voltage;
bool ledRedStatus = 0;
bool ledBlueStatus = 1;
AlarmCode_t alarmCode = 0x00;

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

	gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
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
        //Multisampling
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
        //Convert adc_reading to voltage in mV
        // uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        // printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

        voltage = 3.3/4096.0 * adc_reading * 1000;
        printf("Raw: %d\tVoltage: %.2fmV\n", adc_reading, voltage);

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
			if(fReceivedVoltage >= THRESHOLD )
			{
				printf("Abnormal Temperature!!\r\n");
				gpio_set_level(LED_RED, ledRedStatus);
				ledRedStatus = !ledRedStatus;
				alarmCode = 0x02;
			} 

			else 
			{
				alarmCode = 0x00;
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

static void vPeriodicTask( void *pvParameters )
{
	
	TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
	AlarmCode_t alarmCode;

	for(;;)
	{
		alarmCode = (AlarmCode_t) pvParameters;

		if( alarmCode & 0x02)
		{
			gpio_set_level(LED_BLUE, ledBlueStatus);
			ledBlueStatus = !ledBlueStatus;

			vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 100 ) );
		}

		else
		{
			gpio_set_level(LED_BLUE, !ledBlueStatus);
			ledBlueStatus = !ledBlueStatus;

			vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 800 ) );
		}
	}
}
/**************************************************************************/

void app_main(void)
{
	vConfigADC();
	vConfigIO();

	xQueue = xQueueCreate( 3, sizeof( Voltage_t ));

	if( xQueue != NULL )
	{

		xTaskCreate( vReadSensor, "Read ADC1", STACK_SIZE, NULL, 2, NULL );
		xTaskCreate( vCheckThreshold, "Raise alarm if above threshold", STACK_SIZE, NULL, 3, NULL );
		xTaskCreate( vPeriodicTask, "Blink blue LED", STACK_SIZE, (void*) alarmCode, 1, NULL);

	}
	else
	{
		/* The queue could not be created. */
		printf("Queue could not be created\r\n");		
	}
}