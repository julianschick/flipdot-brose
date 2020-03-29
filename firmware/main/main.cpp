#include "globals.h"

#include "drivers/flipdotdriver.h"
#include "drivers/ws2812driver.h"
#include "controllers/ws2812controller.h"

#include "libs/nvs.h"
#include "libs/blue.h"
#include "libs/mdns.h"
#include "libs/wifi.h"
#include "libs/http_server.h"
#include "libs/tcp_server.h"

#define PIN_NUM_MOSI GPIO_NUM_13
#define PIN_NUM_CLK  GPIO_NUM_14

#define PIN_NUM_CLR GPIO_NUM_19
#define PIN_NUM_RCLK_SEL GPIO_NUM_18
#define PIN_NUM_RCLK_CONF GPIO_NUM_21
#define PIN_NUM_OE_SEL GPIO_NUM_5
#define PIN_NUM_OE_CONF GPIO_NUM_15

FlipdotDisplay* dsp;
WS2812Driver* led_drv;
WS2812Controller* led_ctrl;

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

void led_task(void *pvParameters) {

    /*color_t c = {{0x00, 0x00, 0x00, 0x00}};

    uint32_t counter = 0;

    while(1) {
        uint8_t phase = (counter / 256) % 6;

        switch (phase) {
            case 0: c.brg.green = c.brg.green < 255 ? c.brg.green + 1 : 255; break;
            case 1: c.brg.red = c.brg.red > 0 ? c.brg.red - 1 : 0; break;
            case 2: c.brg.blue = c.brg.blue < 255 ? c.brg.blue + 1 : 255; break;
            case 3: c.brg.green = c.brg.green > 0 ? c.brg.green - 1 : 0; break;
            case 4: c.brg.red = c.brg.red < 255 ? c.brg.red + 1 : 255; break;
            case 5: c.brg.blue = c.brg.blue > 0 ? c.brg.blue - 1 : 0; break;
        }

        led_drv->set_all_colors(c);
        led_drv->update();
        vTaskDelay(20 / portTICK_PERIOD_MS);
        counter++;
    }*/

    /*vTaskDelay(5000 / portTICK_PERIOD_MS);
    color_t c = {{0x00, 0xFF, 0x00, 0xFF}};

    led_ctrl->setTransitionMode(WS2812Controller::LINEAR_SLOW);
    led_ctrl->setAllLedsToSameColor(c);*/
    led_ctrl->readState();

    while(1) {
        led_ctrl->cycle();

        if (!led_ctrl->isBusy()) {
            vTaskDelay(1);
        }
    }

}

extern "C" void app_main() {

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
    timing.set_usecs = 110;     //110
    timing.reset_usecs = 140;   //140

    FlipdotDriver *drv = new FlipdotDriver(28, 16, 4, &pins, &timing);

    uint32_t short_period = 28;
    uint32_t long_period = 72;
    ws2812_timing_config_t timing_config = {short_period, long_period, long_period, short_period};
    led_drv = new WS2812Driver(GPIO_NUM_32, RMT_CHANNEL_0, timing_config, 36);
    led_ctrl = new WS2812Controller(led_drv);
    led_drv->clear();

    printf("Driver initialized!\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);

    dsp = new FlipdotDisplay(drv);

    printf("Display initialized!\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);

    Nvs::setup();
    //ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // needed for mdns
    start_mdns_service();
    Wifi::setup();
    Blue::setup();

    HttpServer::set_display(dsp);
    HttpServer::set_led_driver(led_drv);
    HttpServer::start();

    xTaskCreatePinnedToCore(tcp_server_task, "tcp-server-task", 8500, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(led_task, "led-task", 5000, NULL, 4, NULL, 1);


    printf("Connections initialized!\n");

    //dsp->init_by_test();

    //color_t c = {{0x05, 0x10, 0x10, 0x00}};
    //led_drv->set_all_colors(c);
    //led_drv->update();

    //printf("Init done!\n");
}
