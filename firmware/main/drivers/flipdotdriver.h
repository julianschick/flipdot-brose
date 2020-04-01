#ifndef FLIPDOTDRIVER_H
#define FLIPDOTDRIVER_H

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "../util/pixelmap.h"

typedef struct {
    gpio_num_t ser;
    gpio_num_t serclk;
    //
    gpio_num_t clr;
    //
    gpio_num_t rclk_sel;
    gpio_num_t rclk_conf;
    //
    gpio_num_t oe_sel;
    gpio_num_t oe_conf;
    //
}  flipdot_driver_pins_t;

typedef struct {
    int set_usecs = 200;
    int reset_usecs = 200;
} flipdot_driver_timing_config_t;

class FlipdotDriver {

public:
    FlipdotDriver(int module_width_, int module_height_, int device_count_, flipdot_driver_pins_t* pins_, flipdot_driver_timing_config_t* timing_);

    void flip(PixelCoord& coord, bool show);
    void flip(int x, int y, bool show);

    inline int get_width() { return total_width; };
    inline int get_height() { return total_height; };
    inline int get_number_of_pixels() { return total_width * total_height; };

    void set_timing(int usecs);

private:
    flipdot_driver_pins_t pins;
    flipdot_driver_timing_config_t timing;
    spi_device_handle_t spi;

    int module_width, module_height, device_count;
    int total_width, total_height;

    int selected_device;

    void init_spi();
    void init_gpio();

    void select_device(int device);
    inline uint8_t encode_column(int col);
    inline uint8_t encode_status(int col, int dir);
    inline uint16_t encode_row(int row);
    void clear_registers();

    void bound_timing();
};


#endif //FLIPDOTDRIVER_H
