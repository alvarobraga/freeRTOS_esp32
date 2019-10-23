/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_spi_flash.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

// static const char *pcTextForTask1 = "blue";//"Task 1 is running\r\n";
// static const char *pcTextForTask2 = "red";//"Task 2 is running\r\n";

#define LED_BLUE  5
#define LED_RED 2
#define ON 1
#define OFF 0

TaskHandle_t xLedRedHandle = NULL;

void vLedBlueOn(void *pvParameters);
void vLedBlueOff(void *pvParameters);
void vLedRed(void *pvParameters);

void app_main(void)
{

    /* Configure the IOMUX register for pad LED_BLUE (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(LED_BLUE);
    gpio_pad_select_gpio(LED_RED);

    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    

    xTaskCreate(vLedBlueOn,
                "LED BLUE ON",
                10000,
                NULL,
                1,
                NULL);

    xTaskCreate(vLedBlueOff,
                "LED BLUE OFF",
                10000,
                NULL,
                1,
                NULL);

    xTaskCreate(vLedRed, 
                "LED RED", 
                10000, 
                NULL, 
                2, 
                &xLedRedHandle );    


}


void vLedBlueOn(void *pvParameters)
{
    
    for(;;)
    {
        gpio_set_level(LED_BLUE, !ON);      
        
    }
}

void vLedBlueOff(void *pvParameters)
{
    
    for(;;)
    {
        gpio_set_level(LED_BLUE, !OFF);      
        
    }
}

void vLedRed(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();


    for(;;)
    {
        gpio_set_level(LED_RED, ON);        
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 3000 ) );
        gpio_set_level(LED_RED, OFF);
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 3000 ) );

        /*FIRST EXAMPLE - vLedRed deletes itself*/
        // vTaskDelete(NULL);

        /*vLedRed deletes itself via the Handle*/
        vTaskDelete(xLedRedHandle);


    }
}
