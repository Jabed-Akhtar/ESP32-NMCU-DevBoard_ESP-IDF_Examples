/* DHT11 Sensor
 * MCU: NodeMCU ESP32, ESP32-DevkitC, nodemcu-32s, ESP-WROOM-32
 *
 * Sources of some codes / functions used:
 * - https://github.com/abdellah2288/esp32-dht11
 */

/* Includes --------------------------------------------- */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Defines ---------------------------------------------- */
#define DHT11_PIN GPIO_NUM_4
#define DHT11_CONNECTION_TIMEOUT 5

/* Private variables ------------------------------------ */
typedef struct
{
    int dht11_pin;
    float temperature;
    float humidity;
} dht11_t;

/* Private function prototypes -------------------------- */
int wait_for_state(dht11_t dht11,int state,int timeout);
void hold_low(dht11_t dht11,int hold_time_us);
int dht11_read(dht11_t *dht11,int connection_timeout);
void Task_Publish_DHT11_Data(void* param);

/* Main-Function ======================================== */
void app_main(void)
{
	/* ******** FreeRTOS - Tasks ******** */
    xTaskCreate(
		&Task_Publish_DHT11_Data,
		"Demo Task", 2*2048,
		NULL,
		5,
		NULL
		);
	
    while (true)
	{
		// Keep this empty -> Use FreeRTOS!
    }
}

// Functions ===============================================
int wait_for_state(dht11_t dht11,int state,int timeout)
{
    gpio_set_direction(dht11.dht11_pin, GPIO_MODE_INPUT);
    int count = 0;
    
    while(gpio_get_level(dht11.dht11_pin) != state)
    {
        if(count >= timeout) return -1;
        count += 2;
        ets_delay_us(2);
        
    }

    return  count;
}

void hold_low(dht11_t dht11,int hold_time_us)
{
    gpio_set_direction(dht11.dht11_pin,GPIO_MODE_OUTPUT);
    gpio_set_level(dht11.dht11_pin,0);
    ets_delay_us(hold_time_us);
    gpio_set_level(dht11.dht11_pin,1);
}

int dht11_read(dht11_t *dht11,int connection_timeout)
{
    int waited = 0;
    int one_duration = 0;
    int zero_duration = 0;
    int timeout_counter = 0;

    uint8_t received_data[5] =
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };

    while(timeout_counter < connection_timeout)
    {
        timeout_counter++;
        gpio_set_direction(dht11->dht11_pin,GPIO_MODE_INPUT);
        hold_low(*dht11, 18000);
        
        waited = wait_for_state(*dht11,0,40);

        if(waited == -1)
        {
            ESP_LOGE("DHT11:","Failed at phase 1");
            ets_delay_us(20000);
            continue;
        } 


        waited = wait_for_state(*dht11,1,90);
        if(waited == -1)
        {
            ESP_LOGE("DHT11:","Failed at phase 2");
            ets_delay_us(20000);
            continue;
        } 
        
        waited = wait_for_state(*dht11,0,90);
        if(waited == -1)
        {
            ESP_LOGE("DHT11:","Failed at phase 3");
            ets_delay_us(20000);
            continue;
        } 
        break;
        
    }
    
    if(timeout_counter == connection_timeout) return -1;

    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            zero_duration = wait_for_state(*dht11,1,58);
            one_duration = wait_for_state(*dht11,0,74);
            received_data[i] |= (one_duration > zero_duration) << (7 - j);
        }
    }
    int crc = received_data[0]+received_data[1]+received_data[2]+received_data[3];
    crc = crc & 0xff;
    if(crc == received_data[4]) {
      dht11->humidity = received_data[0] + received_data[1] / 10.0;
      dht11->temperature = received_data[2] + received_data[3] / 10.0;
      return 0;
    }
    else {
        ESP_LOGE("DHT11:", "Wrong checksum");
        return -1;
    }
}

/* RTOS Task to publish DHT11 data every 2 sec
*/
void Task_Publish_DHT11_Data(void* param)
{
	dht11_t dht11_sensor;
    dht11_sensor.dht11_pin = DHT11_PIN;
    
	while (true) {
		if(!dht11_read(&dht11_sensor, DHT11_CONNECTION_TIMEOUT))
	    {  
	        printf("[Temperature]> %.2f \n",dht11_sensor.temperature);
	        printf("[Humidity]> %.2f \n",dht11_sensor.humidity);
	    }
		
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

/* ***** END OF FILE ************************************ */
