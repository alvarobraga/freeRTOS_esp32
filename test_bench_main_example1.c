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

#define BLINK_GPIO   5
#define BLINK_GPIO_2 2

void vTaskFunction1(void *pvParameters);
void vTaskFunction2(void *pvParameters);


void app_main(void)
{

    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_pad_select_gpio(BLINK_GPIO_2);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_2, GPIO_MODE_OUTPUT);

    xTaskCreate(vTaskFunction1,
                "Task 1",
                10000,
                NULL,
                1,
                NULL);

    xTaskCreate( vTaskFunction2, "Task 2", 10000, NULL, 2, NULL );


}

void vTaskFunction1(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        gpio_set_level(BLINK_GPIO, 1);
        gpio_set_level(BLINK_GPIO_2, 1);

        printf("**** TASK 1 IS RUNNING ****\n");
        
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 1000 ) ); 
        
    }
}

void vTaskFunction2(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();


    for(;;)
    {
        gpio_set_level(BLINK_GPIO, 0);
        gpio_set_level(BLINK_GPIO_2, 0);

        printf("**** TASK 2 IS RUNNING ****\n");        
                
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 4500 ) );
        
    }
}


