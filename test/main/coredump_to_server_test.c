/**
 * @brief ESP32_coredump-to-server - Demonstrates the use of "coredump_to_server"
 *
 * Written in 2019 by Coert Vonk 
 * 
 * To the extent possible under law, the author(s) have dedicated all copyright and related and
 * neighboring rights to this software to the public domain worldwide. This software is
 * distributed without any warranty. You should have received a copy of the CC0 Public Domain
 * Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 * 
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <coredump_to_server.h>

#define RESET_PIN (0)
#define RESET_SECONDS (1)

static char const * const TAG = "coredump_to_server_test";

void IRAM_ATTR
_button_isr_handler(void * arg)
{
    static int64_t start = 0;
    if (gpio_get_level(RESET_PIN) == 0) {
        start = esp_timer_get_time();
    } else {
        if (esp_timer_get_time() - start > RESET_SECONDS * 1000L * 1000L) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR((TaskHandle_t)arg, &xHigherPriorityTaskWoken);
        }
    }
}

static esp_err_t
_coredump_to_server_begin_cb(void * priv)
{
    ets_printf("================= CORE DUMP START =================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_end_cb(void * priv)
{
    ets_printf("================= CORE DUMP END ===================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_write_cb(void * priv, char const * const str)
{
    ets_printf("%s\r\n", str);
    return ESP_OK;
}

void
app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    coredump_to_server_config_t coredump_cfg = {
        .start = _coredump_to_server_begin_cb,
        .end = _coredump_to_server_end_cb,
        .write = _coredump_to_server_write_cb,
        .priv = NULL,
    };
    coredump_to_server(&coredump_cfg);

    gpio_pad_select_gpio(RESET_PIN);
    gpio_set_direction(RESET_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(RESET_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RESET_PIN, _button_isr_handler, xTaskGetCurrentTaskHandle());

    ESP_LOGI(TAG, "waiting for BOOT/RESET button ..");
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        assert(0);
    }
}