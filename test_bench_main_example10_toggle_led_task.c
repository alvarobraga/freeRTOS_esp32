#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define LED 2
#define TOGGLE 18

void vTaskFunction1(void *pvParameters)
{
    TickType_t xLastWakeTime;
    // xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        
        if(gpio_get_level(TOGGLE)) gpio_set_level(LED, 1);

        else gpio_set_level(LED, 0);
        

        // printf("%d\n", gpio_get_level(TOGGLE));
        
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 100 ) ); 
        
    }
}

void app_main(void)
{
	
  gpio_pad_select_gpio(LED);
  gpio_pad_select_gpio(TOGGLE);

  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_direction(TOGGLE, GPIO_MODE_INPUT);

  // float array[] = {1.69, 9.72, 2.73, 4.58, 5.68, 2.40, 5.80, 4.67, 0.49, 3.93, 5.96,
  //      		   		 7.17, 3.79, 8.39, 6.79, 9.75, 8.43, 5.91, 1.46, 1.77, 4.13, 4.34,
  //              		 4.49, 9.47, 8.97, 9.71, 6.87, 0.39, 6.07, 0.53, 9.31, 1.69, 1.61,
  //              		 6.70, 1.62, 6.58, 8.63, 9.59, 1.08, 7.77, 7.86, 3.50, 6.54, 8.87,
  //              		 3.58, 5.91, 4.55, 3.30, 9.61, 2.80, 0.30, 5.40, 0.60, 6.28, 9.61,
  //              		 8.95, 4.08, 7.35, 1.95, 0.00, 8.42, 4.16, 5.87, 4.68, 8.27, 9.69,
  //              		 3.36, 9.61, 0.61, 9.25, 8.43, 9.67, 6.34, 9.04, 1.29, 4.51, 3.40,
  //              		 2.81, 5.03, 7.17, 9.61, 7.85, 3.27, 8.81, 1.09, 5.19, 7.12, 3.95,
  //              		 4.13, 4.21, 3.69, 4.77, 2.53, 7.79, 9.32, 3.07, 9.92, 2.29, 6.56,
  //              		 3.05};


  xTaskCreate(vTaskFunction1,
                "Task 1",
                10000,
                NULL,
                1,
                NULL);

  

}

