/*
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body (Created with WiFi softAP Example)
 ******************************************************************************
 * @author         : Jabed-Akhtar (Github)
 * @date           : 06.06.2023
 * ******************************************************************************
 * script (this) related infos:
 *  - a source within this script: https://youtube.com/playlist?list=PLfIJKC1ud8ghS_i2Yky2actXWbQoqrscN
 * 	- evicences/pics can be found at location: '<git-repo-root-folder>/zz_Evidences/WebServer_AP_***'
 * 	- Related doc/file can be found at location '<git-repo-root-folder>/zz_docs/...'
 ******************************************************************************
 * Description:
 * - MCU: ESP32-NodeMCU Dev Board
 * - DHCP server assigned IP: 192.168.4.2
 * - Web-Server access IP-Address: 	192.168.4.1 		-> root page
 * 									192.168.4.1/ledon	-> LED-ON page
 * 									192.168.4.1/ledoff	-> LED_OFF page
 * - Steps:
 * 		1. Choose SoftAP Example
 * 		2. Build project
 * 		3. Open sdkconfig
 * 			|- Go to Example Configuration
 * 			|- Set WiFi SSID and WiFi Password
 * 		4. Add HTTPD part
 ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_tls.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/* Private define ------------------------------------------------------------*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */
#define LED_ONBOARD GPIO_NUM_2


/* Private function prototypes -----------------------------------------------*/
static void init_led(void);
void wifi_init_softap(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
static httpd_handle_t start_webserver(void);
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t ledon_handler(httpd_req_t *req);
static esp_err_t ledoff_handler(httpd_req_t *req);
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);


/* Private variables ---------------------------------------------------------*/
static const char *TAG = "webserver";

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = ledoff_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #4CAF50;} /* Green */\
.button2 {background-color: #008CBA;} /* Blue */\
</style>\
</head>\
<body>\
\
<h1>ESP32 Dashboard</h1>\
<p>Toggle the onboard LED (GPIO2)</p>\
<h3>LED STATE: OFF</h3>\
\
<button class=\"button button1\" onclick=\"window.location.href='/ledon'\">LED ON</button>\
\
</body>\
</html>"
};

static const httpd_uri_t ledon = {
    .uri       = "/ledon",
    .method    = HTTP_GET,
    .handler   = ledon_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #000000;} /* Black */\
.button2 {background-color: #008CBA;} /* Blue */\
</style>\
</head>\
<body>\
\
<h1>ESP32 Dashboard</h1>\
<p>Toggle the onboard LED (GPIO2)</p>\
<h3>LED STATE: ON</h3>\
\
<button class=\"button button1\" onclick=\"window.location.href='/ledoff'\">LED OFF</button>\
\
</body>\
</html>"
};

static const httpd_uri_t ledoff = {
    .uri       = "/ledoff",
    .method    = HTTP_GET,
    .handler   = ledoff_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
.button {\
  border: none;\
  color: white;\
  padding: 15px 32px;\
  text-align: center;\
  text-decoration: none;\
  display: inline-block;\
  font-size: 16px;\
  margin: 4px 2px;\
  cursor: pointer;\
}\
\
.button1 {background-color: #4CAF50;} /* Green */\
.button2 {background-color: #008CBA;} /* Blue */\
</style>\
</head>\
<body>\
\
<h1>ESP32 Dashboard</h1>\
<p>Toggle the onboard LED (GPIO2)</p>\
<h3>LED STATE: OFF</h3>\
\
<button class=\"button button1\" onclick=\"window.location.href='/ledon'\">LED ON</button>\
\
</body>\
</html>"
};


/**
  * @brief  The application entry point.
  * @retval int
  */
void app_main(void)
{
	static httpd_handle_t server = NULL;

	init_led();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
	// ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
}

static void init_led(void)
{
	gpio_reset_pin(LED_ONBOARD);

	gpio_set_direction(LED_ONBOARD, GPIO_MODE_OUTPUT);
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ledoff);
        httpd_register_uri_handler(server, &ledon);
        httpd_register_uri_handler(server, &root);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

/* An HTTP GET handler */
static esp_err_t ledon_handler(httpd_req_t *req)
{
	esp_err_t error;
	ESP_LOGI(TAG, "LED turned On.");
	gpio_set_level(LED_ONBOARD, 1);
	const char *response = (const char *) req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if (error!=ESP_OK)
	{
		ESP_LOGI(TAG, "Error %d while sending response.", error);
	}
	else ESP_LOGI(TAG, "Response sent successfully.");
	return error;
}

/* An HTTP GET handler */
static esp_err_t ledoff_handler(httpd_req_t *req)
{
	esp_err_t error;
	ESP_LOGI(TAG, "LED turned Off.");
	gpio_set_level(LED_ONBOARD, 0);
	const char *response = (const char *) req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if (error!=ESP_OK)
	{
		ESP_LOGI(TAG, "Error %d while sending response.", error);
	}
	else ESP_LOGI(TAG, "Response sent successfully.");
	return error;
}

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* ***** END OF FILE ******************************************************** */
