#ifndef WS2812DRIVER_H
#define WS2812DRIVER_H

#include "../globals.h"
#include "driver/rmt.h"

#define BIT_PER_LED 24

typedef struct {
    uint8_t blue;
    uint8_t red;
    uint8_t green;
    uint8_t padding;
} color_field_t;

union color_t {
    color_field_t brg;
    uint32_t bits;
};

typedef struct {
    uint32_t bit0_high_time;
    uint32_t bit0_low_time;
    uint32_t bit1_high_time;
    uint32_t bit1_low_time;
} ws2812_timing_config_t;

class WS2812Driver {

public:
    WS2812Driver(gpio_num_t pin, rmt_channel_t rmt_channel, ws2812_timing_config_t& timing_config, size_t led_count);
    virtual ~WS2812Driver();

    void set_color(size_t led, color_t& color);
    void set_all_colors(color_t& color);
    void clear();

    color_t get_color(size_t led);
    inline size_t get_led_count() { return led_count; };

    void update();
    color_t* get_state() { return state; }

private:
    gpio_num_t pin;
    rmt_channel_t rmt_channel;
    ws2812_timing_config_t timing_config;
    size_t led_count;

    color_t black;
    color_t* state;

    rmt_item32_t* tx_buf;

    void init_rmt();
    void setup_tx_buf();
    bool range_check(size_t led);
};


#endif //WS2812DRIVER_H