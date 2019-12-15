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
#include "util/bitarray.h"
#include "lowlevel/flipdotdriver.h"
#include "flipdotdisplay.h"

#define PIN_NUM_MOSI GPIO_NUM_13
#define PIN_NUM_CLK  GPIO_NUM_14

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

extern "C" {
    void app_main(void);
}

void app_main_2() {
    BitArray* bits1 = new BitArray(10);
    BitArray* bits2 = new BitArray(10);

    bits1->set(2, true);
    bits1->set(3, true);
    bits1->set(8, true);
    bits2->set(0, true);

    int8_t* diff = new int8_t[10];
    bits1->transition_vector_to(*bits2, diff);

    for (int i = 0; i < 10; i++) {
        printf("%d\n", diff[i]);
    }

    bits2->transition_vector_to(*bits1, diff);

    for (int i = 0; i < 10; i++) {
        printf("%d\n", diff[i]);
    }

    while(true) {
        vTaskDelay(1);
    }
}

void app_main() {
    gpio_set_level(PIN_NUM_OE_CONF, 1);
    safety_init();

    flipdot_driver_pins_t pins;
    pins.ser = PIN_NUM_MOSI;
    pins.serclk = PIN_NUM_CLK;
    pins.clr = PIN_NUM_CLR;
    pins.rclk_conf = PIN_NUM_RCLK_CONF;
    pins.rclk_sel = PIN_NUM_RCLK_SEL;
    pins.oe_conf = PIN_NUM_OE_CONF;
    pins.oe_sel = PIN_NUM_OE_SEL;

    flipdot_driver_timing_config_t timing;
    timing.set_usecs = 400;
    timing.reset_usecs = 400;

    FlipdotDriver *drv = new FlipdotDriver(28, 16, 4, &pins, &timing);

    printf("Driver initialized!\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);

    FlipdotDisplay *dsp = new FlipdotDisplay(drv);

    printf("Display initialized!\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);

    BitArray* allOff = new BitArray(dsp->get_number_of_pixels());
    BitArray* firstOn = new BitArray(dsp->get_number_of_pixels());
    BitArray* secondOn = new BitArray(dsp->get_number_of_pixels());
    BitArray* thirdOn = new BitArray(dsp->get_number_of_pixels());
    BitArray* fourthOn = new BitArray(dsp->get_number_of_pixels());
    BitArray* allOn = new BitArray(dsp->get_number_of_pixels());
    allOn->setAll(true);

    for (int i = 0; i < 16*28; i++) {
        firstOn->set(i, true);
    }
    for (int i = 16*28; i < 2*16*28; i++) {
        secondOn->set(i, true);
    }
    for (int i = 2*16*28; i < 3*16*28; i++) {
        thirdOn->set(i, true);
    }
    for (int i = 3*16*28; i < 4*16*28; i++) {
        fourthOn->set(i, true);
    }

    while (true) {
        dsp->display_overriding(*allOn);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        dsp->display_incrementally(*allOff);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        dsp->display_incrementally(*firstOn);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        dsp->display_incrementally(*secondOn);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        dsp->display_incrementally(*thirdOn);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        dsp->display_incrementally(*fourthOn);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        dsp->display_string("Hallo Welt");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        dsp->display_string("Ratzupaltuff!");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        dsp->display_string("Wolkenthermik");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

    }

    /*ws2812_control_init();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    struct led_state new_state;
    for (int i = 0; i < NUM_LEDS; i++) {
        new_state.leds[i] = 0x202020;
    }
    ws2812_write_leds(new_state);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ws2812_write_leds(new_state);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    int counter = 0;
    while (true) {

        for (int x = 0; x < 28 * 4; x++) {
            for (int y = 0; y < 16; y++) {
                drv->flip(x, y, counter % 2);
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        counter++;
        ws2812_write_leds(new_state);
    }*/

}
