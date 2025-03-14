/* UART asynchronous example
*/

/* Includes --------------------------------------------- */
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"

/* Defines ---------------------------------------------- */
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

/* Private variables ------------------------------------ */
static const int RX_BUF_SIZE = 1024;

/* Private function prototypes -------------------------- */
void uart_init(void);
int uart_sendData(const char* logName, const char* data);
static void Task_UART_Tx(void *arg);
static void Task_UART_Rx(void *arg);

/* Main-Function ======================================== */
void app_main(void)
{
	/* ******** Initializations ******** */
    uart_init();
    
    /* ******** RTOS Tasks ******** */
    xTaskCreate(Task_UART_Rx, "uart_Task_UART_Rx", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(Task_UART_Tx, "uart_Task_UART_Tx", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
}

// Functions ===============================================
void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int uart_sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void Task_UART_Tx(void *arg)
{
    static const char *Task_UART_Tx_TAG = "Task_UART_Tx";
    esp_log_level_set(Task_UART_Tx_TAG, ESP_LOG_INFO);
    while (1) {
        uart_sendData(Task_UART_Tx_TAG, "Hello from ESP32.\n\r");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void Task_UART_Rx(void *arg)
{
    static const char *Task_UART_Rx_TAG = "Task_UART_Rx";
    esp_log_level_set(Task_UART_Rx_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(Task_UART_Rx_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(Task_UART_Rx_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

/* ***** END OF FILE ************************************ */
