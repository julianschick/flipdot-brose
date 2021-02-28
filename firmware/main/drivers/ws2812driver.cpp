#include "ws2812driver.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG "ws2812-driver"
#include "esp_log.h"

WS2812Driver::WS2812Driver(gpio_num_t pin, rmt_channel_t rmt_channel, ws2812_timing_config_t& timing_config, size_t led_count)
    : pin(pin), rmt_channel(rmt_channel), timing_config(timing_config), led_count(led_count)
{
    init_rmt();
    ESP_LOGD(TAG, "RMT subdevice for LED driver initialized, RMT channel %d, GPIO pin %d", rmt_channel, pin);
    ESP_LOGD(TAG, "Timing for binary 0: %d tq HIGH, %d tq LOW", timing_config.bit0_high_time, timing_config.bit0_low_time);
    ESP_LOGD(TAG, "Timing for binary 1: %d tq HIGH, %d tq LOW", timing_config.bit1_high_time, timing_config.bit1_low_time);

    black = {{0x00, 0x00, 0x00, 0x00}};
    state = new color_t[led_count];
    tx_buf = new rmt_item32_t[led_count * BIT_PER_LED];

    clear();
    ESP_LOGI(TAG, "Driver initialized for %d LEDs", led_count);
}

void WS2812Driver::init_rmt() {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(pin, rmt_channel);
    config.mem_block_num = 8;
    config.clk_div = 1;

    float freq = 80 / config.clk_div;
    float tq = 1000000000 / (freq * 1000000);

    ESP_LOGD(TAG, "RMT frequency %.2f MHz giving a time quantum (tq) of %.2f ns", freq, tq);

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

WS2812Driver::~WS2812Driver() {
    ESP_ERROR_CHECK(rmt_driver_uninstall(rmt_channel));

    delete[] state;
    delete[] tx_buf;
}

void WS2812Driver::set_color(size_t led, color_t& color) {
    if (!range_check(led)) {
        ESP_LOGW(TAG, "LED index out of range: %d.", led);
        return;
    }

    ESP_LOGD(TAG, "Setting LED %d to color rgb(%#02x, %#02x, %#02x)", led, color.brg.red, color.brg.green, color.brg.blue);
    state[led] = color;
}

void WS2812Driver::set_all_colors(color_t& color) {
    ESP_LOGD(TAG, "Setting all LEDs to color rgb(%#02x, %#02x, %#02x)", color.brg.red, color.brg.green, color.brg.blue);

    for (size_t i = 0; i < led_count; i++) {
        state[i] = color;
    }
}

color_t WS2812Driver::get_color(size_t led) {
    if (!range_check(led)) {
        ESP_LOGW(TAG, "LED index out of range: %d.", led);
        return black;
    }

    return state[led];
}

void WS2812Driver::clear() {
    ESP_LOGD(TAG, "Clearing all LEDs");
    for (size_t i = 0; i < led_count; i++) {
        state[i] = black;
    }
    update();
}

bool WS2812Driver::range_check(size_t led) {
    return led < led_count;
}

void WS2812Driver::update() {
    ESP_LOGV(TAG, "update()");

    setup_tx_buf();
    ESP_ERROR_CHECK(rmt_write_items(rmt_channel, tx_buf, led_count/2 * BIT_PER_LED, false));
    ESP_ERROR_CHECK(rmt_wait_tx_done(rmt_channel, portMAX_DELAY));

    ESP_ERROR_CHECK(rmt_write_items(rmt_channel, tx_buf + (led_count/2 * BIT_PER_LED), led_count/2 * BIT_PER_LED, false));
    ESP_ERROR_CHECK(rmt_wait_tx_done(rmt_channel, portMAX_DELAY));

    ESP_LOGV(TAG, "update() done");
}

void WS2812Driver::setup_tx_buf()
{
    for (size_t i = 0; i < led_count; i++) {

        ESP_LOGV(TAG, "LED[%d] = rgb(%#02x, %#02x, %#02x)", i, state[i].brg.red, state[i].brg.green, state[i].brg.blue);

        uint32_t mask = 1 << (BIT_PER_LED - 1);
        for (uint32_t bit = 0; bit < BIT_PER_LED; bit++) {
            uint32_t bit_is_set = state[i].bits & mask;
            tx_buf[i * BIT_PER_LED + bit] = bit_is_set ?
                    (rmt_item32_t){{{timing_config.bit1_high_time, 1, timing_config.bit1_low_time, 0}}} :
                    (rmt_item32_t){{{timing_config.bit0_high_time, 1, timing_config.bit0_low_time, 0}}};
            mask >>= 1;
        }
    }
}