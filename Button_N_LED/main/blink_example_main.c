/*
 ******************************************************************************
 * @file           : button_n_led_main.c
 * @brief          : Main program body
 ******************************************************************************
 * @author         : Jabed-Akhtar (Github)
 * @date           : 2024-08-07
 ******************************************************************************
 * - MCU: ESP32-NodeMCU Dev Board
 * - evidences/pics can be found at location: '<git-repo-root-folder>/zz_Evidences/...'
 *
 * Descriptions: On pressing the push button, LED turns OFF and on leaving the Button, the LED remains turned ON.
 *
 ******************************************************************************
*/

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"		// For GPIOs
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Private define ----------------------------------------------------------- */
/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 21	// ESP32 pin GPIO21 connected to LED
#define BUTTON_PIN 18	// ESP32 pin GPIO18 connected to Button

static uint8_t s_led_state = 0;

/* Private function prototypes ---------------------------------------------- */
static void configure_led(void);
static void configure_button(void);
static void blink_led(void);

/**
  * @brief  The application entry point.
  * @retval void
  */
void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();
    configure_button();
    
	// main-while-loop
    while (1) {
        // Read Button state
        if (gpio_get_level(BUTTON_PIN)==1)
        {
			ESP_LOGI(TAG, "Button pressed: %s!", "ON");
			
			s_led_state = 1;
			blink_led();
		}
		else
		{
			ESP_LOGI(TAG, "Button pressed: %s!", "OFF");
			
			s_led_state = 0;
			blink_led();
		}
        
        //vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}

/* Configuring the LED */
static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

/* Configuring the Button */
static void configure_button(void)
{
    ESP_LOGI(TAG, "Configuring Button!");
    gpio_reset_pin(BUTTON_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
}

/* Helper function - Blink-LED */
static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

/* ##### END OF FILE ############### */
