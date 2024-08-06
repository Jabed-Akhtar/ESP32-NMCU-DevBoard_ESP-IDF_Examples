/*
 * ...
 * 
 * Out can be read using v_out = ( angle_rotated / 270° ) * 3.3V
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "driver/adc_types_legacy.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include <driver/adc.h>

// Pins ===============
// GND:
// 3V3:
// ADC0:

void app_main(void)
{
	// Initial hello-world message.
    printf("Hello world!\n");
    
    /* Variables */
    uint16_t read_adc_raw;	// Store ADC value (0-4095)
    
    /* Configurations */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);

    for (int i = 10; i >= 0; i--) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
        // Read ADC value
        read_adc_raw = adc1_get_raw(ADC1_CHANNEL_0);
        printf("ADC raw value: %d\n", read_adc_raw);
        
        // Print message with restart time remained
        if (i%5==0)
        {
			printf("Restarting in %d seconds...\n", i);
		}
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
