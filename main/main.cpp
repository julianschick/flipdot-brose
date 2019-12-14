/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/spi_master.h"
#include "ws2812_control.h"

#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14

#define PIN_NUM_CLR GPIO_NUM_19
#define PIN_NUM_RCLK_SEL GPIO_NUM_18
#define PIN_NUM_RCLK_CONF GPIO_NUM_5
#define PIN_NUM_OE_SEL GPIO_NUM_17
#define PIN_NUM_OE_CONF GPIO_NUM_16

inline void safety_init() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (uint64_t) 1 << (uint64_t) PIN_NUM_OE_CONF;;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(PIN_NUM_OE_CONF, 1);
}

//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

spi_device_handle_t spi;

void select_device(uint8_t mask) {
    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = &mask;

    esp_err_t ret;

    ret = gpio_set_level(PIN_NUM_RCLK_SEL, 0);
    ESP_ERROR_CHECK(ret);

    ret = spi_device_transmit(spi, &tx);
    ESP_ERROR_CHECK(ret);

    gpio_set_level(PIN_NUM_RCLK_SEL, 1);
    ESP_ERROR_CHECK(ret);
}

void deselect_device() {
    select_device(0x00);
}

uint8_t encodeColumn(int col) {
    if (col < 1 || col > 28) return 0x00;
    if (col <= 7) return col;
    if (col <= 14) return col + 1;
    if (col <= 21) return col + 2;
    return col + 3;
}

uint8_t encodeStatus(int col, int dir) {
    uint8_t result = ~encodeColumn(col) << 3;
    if (dir == 0) {
        result |= BIT1;
    }
    return result;
}

uint16_t encodeRow(int row) {
    if (row < 1 || row > 16) {
        return 0x0000;
    }
    return 0x0001 << (row - 1);
}

void clear() {
    gpio_set_level(PIN_NUM_CLR, 0);

    gpio_set_level(PIN_NUM_RCLK_CONF, 0);
    gpio_set_level(PIN_NUM_RCLK_SEL, 0);

    ets_delay_us(1);

    gpio_set_level(PIN_NUM_RCLK_CONF, 1);
    gpio_set_level(PIN_NUM_RCLK_SEL, 1);

    gpio_set_level(PIN_NUM_CLR, 1);
}

void set_pixel(int row, int col) {
    if (row < 0 || row > 16) return;
    if (col < 0 || col > 28) return;

    uint16_t row_mask = encodeRow(row);
    uint8_t buffer[5];

    buffer[0] = (uint8_t) (row_mask >> 8);
    buffer[1] = (uint8_t) row_mask;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = encodeStatus(col, 0);

    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 5 * 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = buffer;

    esp_err_t ret;

    ret = gpio_set_level(PIN_NUM_RCLK_CONF, 0);
    ESP_ERROR_CHECK(ret);

    ret = spi_device_transmit(spi, &tx);
    ESP_ERROR_CHECK(ret);

    gpio_set_level(PIN_NUM_RCLK_CONF, 1);
    ESP_ERROR_CHECK(ret);

    ets_delay_us(1);

    gpio_set_level(PIN_NUM_OE_SEL, 0);
    ets_delay_us(500);
    gpio_set_level(PIN_NUM_OE_SEL, 1);
}

void reset_pixel(int row, int col) {
    if (row < 0 || row > 16) return;
    if (col < 0 || col > 28) return;

    uint16_t row_mask = encodeRow(row);
    uint8_t buffer[5];

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = (uint8_t) (row_mask >> 8);
    buffer[3] = (uint8_t) row_mask;
    buffer[4] = encodeStatus(col, 1);

    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 5 * 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = buffer;

    esp_err_t ret;

    ret = gpio_set_level(PIN_NUM_RCLK_CONF, 0);
    ESP_ERROR_CHECK(ret);

    ret = spi_device_transmit(spi, &tx);
    ESP_ERROR_CHECK(ret);

    gpio_set_level(PIN_NUM_RCLK_CONF, 1);
    ESP_ERROR_CHECK(ret);

    ets_delay_us(1);

    gpio_set_level(PIN_NUM_OE_SEL, 0);
    ets_delay_us(500);
    gpio_set_level(PIN_NUM_OE_SEL, 1);
}

void init_spi() {
    esp_err_t ret;

    spi_bus_config_t bus_config;
    bus_config.miso_io_num = -1;
    bus_config.mosi_io_num = PIN_NUM_MOSI;
    bus_config.sclk_io_num = PIN_NUM_CLK;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz = 0;
    bus_config.flags = SPICOMMON_BUSFLAG_MASTER;
    bus_config.intr_flags = 0;

    spi_device_interface_config_t device_config;
    device_config.command_bits = 0;
    device_config.address_bits = 0;
    device_config.dummy_bits = 0;
    device_config.mode=0;
    device_config.duty_cycle_pos = 128; // 50%/50%
    device_config.cs_ena_pretrans = 0;
    device_config.cs_ena_posttrans = 0;
    device_config.clock_speed_hz=APB_CLK_FREQ / 2; // 40 MHz (max Speed for 74HC595)
    device_config.input_delay_ns = 0;
    device_config.spics_io_num=-1;
    device_config.flags = 0;
    device_config.queue_size=1;
    device_config.pre_cb = 0;
    device_config.post_cb = 0;

    ret = spi_bus_initialize(HSPI_HOST, &bus_config, 1);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(HSPI_HOST, &device_config, &spi);
    ESP_ERROR_CHECK(ret);

}

extern "C" {
    void app_main(void);
}

void app_main() {
    gpio_set_level(PIN_NUM_OE_CONF, 1);
    safety_init();

    uint64_t mask = 0x00;
    //mask |= (uint64_t)0x01 << (uint64_t)PIN_NUM_MOSI;
    //mask |= (uint64_t)0x01 << (uint64_t)PIN_NUM_CLK;
    mask |= (uint64_t) 0x01 << (uint64_t) PIN_NUM_CLR;
    mask |= (uint64_t) 0x01 << (uint64_t) PIN_NUM_RCLK_SEL;
    mask |= (uint64_t) 0x01 << (uint64_t) PIN_NUM_RCLK_CONF;
    mask |= (uint64_t) 0x01 << (uint64_t) PIN_NUM_OE_SEL;
    mask |= (uint64_t) 0x01 << (uint64_t) PIN_NUM_OE_CONF;

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = mask;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(PIN_NUM_OE_CONF, 1);
    gpio_set_level(PIN_NUM_OE_SEL, 1);
    gpio_set_level(PIN_NUM_CLR, 1);
    gpio_set_level(PIN_NUM_RCLK_SEL, 1);
    gpio_set_level(PIN_NUM_RCLK_CONF, 1);

    clear();

    printf("Hello world!\n");

    init_spi();

    printf("Initialized!\n");

    ws2812_control_init();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    struct led_state new_state;
    for (int i = 0; i < NUM_LEDS; i++) {
        new_state.leds[i] = 0x101010;
    }
    ws2812_write_leds(new_state);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uint8_t devices[4] = {0x01, 0x02, 0x04, 0x08};
    gpio_set_level(PIN_NUM_OE_CONF, 0);

    while (true) {

        for (int i = 0; i < 4; i++) {

            select_device(devices[i]);

            for (int col = 1; col <= 28; col++) {
                for (int row = 1; row <= 16; row++) {

                    set_pixel(row, col);
                }
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        for (int i = 0; i < 4; i++) {

            select_device(devices[i]);
            for (int col = 1; col <= 28; col++) {
                for (int row = 1; row <= 16; row++) {

                    reset_pixel(row, col);
                }
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }

}
