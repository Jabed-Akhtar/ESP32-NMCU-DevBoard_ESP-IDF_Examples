/* LCDDisplay1602_via_IIC
 * MCU: NodeMCU ESP32, ESP32-DevkitC, nodemcu-32s, ESP-WROOM-32
 *
 * Sources of some codes / functions used:
 * - https://controllerstech.com/i2c-in-esp32-esp-idf-lcd-1602/
 */

/* Includes --------------------------------------------- */
#include <stdio.h>
#include <stdint.h>
#include "unistd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

/* Defines ---------------------------------------------- */
#define BLINK_GPIO 2

/* -------- I2C defines -------- */
#define I2C_SLAVE_LCD_ADDRESS 		0x27			// I2C slave address, address of I2C-Module for LCD-Display
#define I2C_NUM 					I2C_NUM_0		// I2C number in ESP32
#define I2C_MASTER_SCL_IO           GPIO_NUM_22		// I2C master clock
#define I2C_MASTER_SDA_IO           GPIO_NUM_21     // I2C master data
#define I2C_MASTER_NUM              0               // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          400000          // I2C master clock frequency
#define I2C_MASTER_TX_BUF_DISABLE   0               // I2C master Tx doesn't need buffer_msg_lcd
#define I2C_MASTER_RX_BUF_DISABLE   0               // I2C master Rx doesn't need buffer_msg_lcd
#define I2C_MASTER_TIMEOUT_MS       1000

/* Private variables ------------------------------------ */
static const char *TAG = "LCD Display - with I2C";
esp_err_t err;

static uint8_t led_state = 0;

char buffer_msg_lcd[16];
uint8_t lcd_heartbeat = 0;

/* Private function prototypes -------------------------- */
static void configure_led(gpio_num_t gpio_num);
static void blink_led(gpio_num_t gpio_num, uint32_t led_state);
static esp_err_t i2c_master_init(void);
void lcd_init (void);
void lcd_clear (void);
void lcd_put_cur(int row, int col);
void lcd_send_cmd (char cmd);
void lcd_send_data (char data);
void lcd_send_string (char *str);
void Task_LCD_Write(void* param);

/* Main-Function ======================================== */
void app_main(void)
{
	/* ******** Initializations ******** */
    configure_led(BLINK_GPIO);
    
    ESP_LOGI(TAG, "Starting up application");
    
    ESP_ERROR_CHECK(i2c_master_init());
    
    lcd_init();
    lcd_clear();

    lcd_put_cur(0, 0);
    lcd_send_string("Program started!");
    
    xTaskCreate(&Task_LCD_Write, "Demo Task", 2048, NULL, 5, NULL);

    while (1) {
        // Keep this Empty. Use FreeRTOS.
    }
}

// Functions ===============================================
/* Configure LED ===============
 * @brief Configure LED for status / heartbeat of this Node
 */
static void configure_led(gpio_num_t gpio_num)
{
    gpio_reset_pin(gpio_num);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "LED configured!");
}

/* Blink LED ===============
 * @brief Toggle the LED
 */
static void blink_led(gpio_num_t gpio_num, uint32_t led_state)
{
    gpio_set_level(gpio_num, led_state);
}

/* Initialize I2C master ===============
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(
		i2c_master_port,
		conf.mode,
		I2C_MASTER_RX_BUF_DISABLE,
		I2C_MASTER_TX_BUF_DISABLE,
		0
	);
}

/* Initialize LCD ===============
 * @brief LCD initialization - I2C slave
 */
void lcd_init (void)
{
	// 4 bit initialization
	usleep(50000);  	// wait for >40ms
	lcd_send_cmd (0x30);
	usleep(5000);  	// wait for >4.1ms
	lcd_send_cmd (0x30);
	usleep(200);  	// wait for >100us
	lcd_send_cmd (0x30);
	usleep(10000);
	lcd_send_cmd (0x20);  	// 4bit mode
	usleep(10000);

  	// Display initialization
	lcd_send_cmd (0x28); 	// Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	usleep(1000);
	lcd_send_cmd (0x08); 	// Display on/off control --> D=0,C=0, B=0  ---> display off
	usleep(1000);
	lcd_send_cmd (0x01);  	// clear display
	usleep(1000);
	usleep(1000);
	lcd_send_cmd (0x06); 	// Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	usleep(1000);
	lcd_send_cmd (0x0C); 	// Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	usleep(1000);
}

/* Clear LCD ===============
 * @brief Clear all characters on the LCD
 */
void lcd_clear (void)
{
	lcd_send_cmd (0x01);
	usleep(5000);
}

/* Put cursor LCD ===============
 * @brief Put cursor on the LCD to write the next character
 */
void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }

    lcd_send_cmd (col);
}

/* Send command to LCD ===============
 */
void lcd_send_cmd (char cmd)
{
  char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  //en=1, rs=0
	data_t[1] = data_u|0x08;  //en=0, rs=0
	data_t[2] = data_l|0x0C;  //en=1, rs=0
	data_t[3] = data_l|0x08;  //en=0, rs=0
	err = i2c_master_write_to_device(
		I2C_NUM,
		I2C_SLAVE_LCD_ADDRESS,
		data_t,
		4,
		1000
	);
	if (err!=0) ESP_LOGI(TAG, "Error in sending command");
}

/* Send data to LCD ===============
 * @brief Send one character to the LCD
 */
void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	data_t[0] = data_u|0x0D;  //en=1, rs=0
	data_t[1] = data_u|0x09;  //en=0, rs=0
	data_t[2] = data_l|0x0D;  //en=1, rs=0
	data_t[3] = data_l|0x09;  //en=0, rs=0
	err = i2c_master_write_to_device(
		I2C_NUM,
		I2C_SLAVE_LCD_ADDRESS,
		data_t,
		4,
		1000
	);
	if (err!=0) ESP_LOGI(TAG, "Error in sending data");
}

/* Send string to LCD ===============
 * @brief Send whole string to the LCD
 *	- this function depends upon the lcd_send_data() function
 */
void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

/* Task to write string to LCD ===============
 * @brief This is a FreeRTOS task which keeps sending counter value to the LCD every one second (periodically)
 */
void Task_LCD_Write(void* param)
{
    // char num[16];
    while (true) {
		ESP_LOGI(TAG, "Turning the LED %s!", led_state == true ? "ON" : "OFF");
		blink_led(BLINK_GPIO, led_state);
		led_state = !led_state;
		
        sprintf(buffer_msg_lcd, "Counter: %d", lcd_heartbeat);
    	lcd_put_cur(1, 0);
    	lcd_send_string(buffer_msg_lcd);
    	
    	lcd_heartbeat++;
    	if (lcd_heartbeat > 255)
    	{
			lcd_heartbeat = 0;
		}
		
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/* ***** END OF FILE ************************************ */
