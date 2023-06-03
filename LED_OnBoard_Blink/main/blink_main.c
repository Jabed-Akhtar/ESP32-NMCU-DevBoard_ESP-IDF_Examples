/*
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @author         : Jabed-Akhtar (Github)
 * @date           : 06.06.2023
 ******************************************************************************
 * - MCU: ESP32-NodeMCU Dev Board
 * - evidences/pics can be found at location: '<git-repo-root-folder>/zz_Evidences/...'
 *
 ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

/* Private variables ---------------------------------------------------------*/
static const char *TAG = "LED_Blink";

#define BLINK_GPIO 2 //CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

/* Private function prototypes -----------------------------------------------*/
static void blink_led(void);
static void configure_led(void);

/*
 * main app
 */
void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

/* ***** END OF FILE ******************************************************** */
