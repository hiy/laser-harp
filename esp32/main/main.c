#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "mrubyc.h"

#include "models/phototransistor.h"
#include "models/osc.h"
#include "models/wifi.h"
#include "models/laser.h"
#include "loops/master.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define DEFAULT_VREF 1100
#define NO_OF_SAMPLES 64
#define MEMORY_SIZE (1024 * 40)

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel_1 = ADC_CHANNEL_6; //GPIO34
static const adc_channel_t channel_2 = ADC_CHANNEL_7; //GPIO35
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define HOST_IP_ADDRESS "192.168.0.Y"
#define HOST_OSC_PORT 4559

static const char *TAG = "laser_harp";
static struct sockaddr_in dest_addr;
int sock;

static uint8_t memory_pool[MEMORY_SIZE];

static void c_gpio_init_output(mrb_vm *vm, mrb_value *v, int argc)
{
  int pin = GET_INT_ARG(1);
  console_printf("init pin %d\n", pin);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

static void c_gpio_set_level(mrb_vm *vm, mrb_value *v, int argc)
{
  int pin = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  gpio_set_level(pin, level);
}

static void c_init_adc(mrb_vm *vm, mrb_value *v, int argc)
{
  int pin = GET_INT_ARG(1);
  adc1_config_width(ADC_WIDTH_BIT_12);
  if (pin == 34)
  {
    adc1_config_channel_atten(channel_1, atten);
  }
  else if (pin == 35)
  {
    adc1_config_channel_atten(channel_2, atten);
  };
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

static void c_read_adc(mrb_vm *vm, mrb_value *v, int argc)
{
  int pin = GET_INT_ARG(1);
  uint32_t adc_reading = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++)
  {
    if (pin == 34)
    {
      adc_reading += adc1_get_raw((adc1_channel_t)channel_1);
    }
    else if (pin == 35)
    {
      adc_reading += adc1_get_raw((adc1_channel_t)channel_2);
    }
  }
  adc_reading /= NO_OF_SAMPLES;
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
  SET_INT_RETURN(millivolts);
}

static void c_setup_wifi(mrb_vm *vm, mrb_value *v, int argc)
{
  char addr_str[128];
  int addr_family;
  int ip_protocol;

  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(ESP_OK, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  wifi_config_t sta_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASSWORD,
          .bssid_set = false}};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  while (1)
  {
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDRESS);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(HOST_OSC_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0)
    {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    }
    else
    {
      ESP_LOGI(TAG, "Socket created, sending to %s:%d:%d", HOST_IP_ADDRESS, HOST_OSC_PORT, sock);
      break;
    }

    if (sock != -1)
    {
      ESP_LOGE(TAG, "Shutting down socket and restarting...");
      shutdown(sock, 0);
      close(sock);
    }
  }
}

static void c_send_osc_message(mrb_vm *vm, mrb_value *v, int argc)
{
  unsigned char *payload = GET_STRING_ARG(1);
  size_t size = GET_INT_ARG(2);

  while (1)
  {
    int err = sendto(sock, payload, size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0)
    {
      ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
    else
    {
      ESP_LOGI(TAG, "Message sent");
      break;
    }
  }
}

void app_main(void)
{
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_define_method(0, mrbc_class_object, "gpio_init_output", c_gpio_init_output);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_level", c_gpio_set_level);
  mrbc_define_method(0, mrbc_class_object, "init_adc", c_init_adc);
  mrbc_define_method(0, mrbc_class_object, "read_adc", c_read_adc);
  mrbc_define_method(0, mrbc_class_object, "setup_wifi", c_setup_wifi);
  mrbc_define_method(0, mrbc_class_object, "send_osc_message", c_send_osc_message);

  mrbc_create_task(wifi, 0);
  mrbc_create_task(phototransistor, 0);
  mrbc_create_task(osc, 0);
  mrbc_create_task(laser, 0);
  mrbc_create_task(master, 0);
  mrbc_run();
}
