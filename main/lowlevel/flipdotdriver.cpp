#include "flipdotdriver.h"

FlipdotDriver::FlipdotDriver(int module_width_, int module_height_, int device_count_,
        flipdot_driver_pins_t* pins_,
        flipdot_driver_timing_config_t* timing_) :
    module_width(module_width_), module_height(module_height_), device_count(device_count_)
{
    pins = *pins_;
    timing = *timing_;

    total_width = module_width * device_count;
    total_height = module_height;

    init_gpio();
    init_spi();

    // Activate config register output, can now operate
    gpio_set_level(pins.oe_conf, 0);
}

void FlipdotDriver::init_spi() {
    esp_err_t ret;

    spi_bus_config_t bus_config;
    bus_config.miso_io_num = -1;
    bus_config.mosi_io_num = pins.ser;
    bus_config.sclk_io_num = pins.serclk;
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

void FlipdotDriver::init_gpio() {
    uint64_t mask = 0x00;
    mask |= (uint64_t) 0x01 << (uint64_t) pins.clr;
    mask |= (uint64_t) 0x01 << (uint64_t) pins.rclk_sel;
    mask |= (uint64_t) 0x01 << (uint64_t) pins.rclk_conf;
    mask |= (uint64_t) 0x01 << (uint64_t) pins.oe_sel;
    mask |= (uint64_t) 0x01 << (uint64_t) pins.oe_conf;

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = mask;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Disable config and select outputs
    gpio_set_level(pins.oe_conf, 1);
    gpio_set_level(pins.oe_sel, 1);

    // Disable clear flag
    gpio_set_level(pins.clr, 1);

    // RCLK lines on HIGH by default
    gpio_set_level(pins.rclk_sel, 1);
    gpio_set_level(pins.rclk_conf, 1);

    // Zero all shift-registers
    clear_registers();

    // ... which means no device is selected
    selected_device = 0;
}

void FlipdotDriver::set_pixel(int x, int y) {
    flip(x, y, true);
}

void FlipdotDriver::reset_pixel(int x, int y) {
    flip(x, y, false);
}

void FlipdotDriver::set_pixel_and_block(int x, int y) {
    flip_and_block(x, y, true);
}

void FlipdotDriver::reset_pixel_and_block(int x, int y) {
    flip_and_block(x, y, false);
}

void FlipdotDriver::flip(int x, int y, bool show) {
    if (x < 0 || x >= total_width) return;
    if (y < 0 || y >= total_height) return;

    int device = (x / module_width) + 1;
    int device_x = x % module_width;

    if (selected_device != device) {
        select_device(device);
    }

    uint16_t row_mask = encode_row(y);
    uint8_t buffer[5];

    // set
    if (show) {
        buffer[0] = (uint8_t) (row_mask >> 8);
        buffer[1] = (uint8_t) row_mask;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = encode_status(device_x, 0);

    // reset
    } else {
        buffer[0] = 0x00;
        buffer[1] = 0x00;
        buffer[2] = (uint8_t) (row_mask >> 8);
        buffer[3] = (uint8_t) row_mask;
        buffer[4] = encode_status(device_x, 1);
    }

    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 5 * 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = buffer;

    gpio_set_level(pins.rclk_conf, 0);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &tx));
    gpio_set_level(pins.rclk_conf, 1);

    ets_delay_us(1);
    gpio_set_level(pins.oe_sel, 0);
    ets_delay_us(show ? timing.set_usecs : timing.reset_usecs);
    gpio_set_level(pins.oe_sel, 1);
}

void FlipdotDriver::flip_and_block(int x, int y, bool show) {
    if (x < 0 || x >= total_width) return;
    if (y < 0 || y >= total_height) return;

    int device = (x / module_width) + 1;
    int device_x = x % module_width;

    if (selected_device != device) {
        select_device(device);
    }

    uint16_t row_mask = encode_row(y);
    uint8_t buffer[5];

    // set
    if (show) {
        buffer[0] = (uint8_t) (row_mask >> 8);
        buffer[1] = (uint8_t) row_mask;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = encode_status(device_x, 0);

        // reset
    } else {
        buffer[0] = 0x00;
        buffer[1] = 0x00;
        buffer[2] = (uint8_t) (row_mask >> 8);
        buffer[3] = (uint8_t) row_mask;
        buffer[4] = encode_status(device_x, 1);
    }

    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 5 * 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = buffer;

    gpio_set_level(pins.rclk_conf, 0);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &tx));
    gpio_set_level(pins.rclk_conf, 1);

    ets_delay_us(1);
    gpio_set_level(pins.oe_sel, 0);
    //ets_delay_us(show ? timing.set_usecs : timing.reset_usecs);
    //gpio_set_level(pins.oe_sel, 1);
}

void FlipdotDriver::select_device(int device) {
    uint8_t mask = 0x01;
    mask <<= (device - 1);

    spi_transaction_t tx;
    tx.flags = 0;
    tx.cmd = 0;
    tx.addr = 0;
    tx.length = 8;
    tx.rxlength = 0;
    tx.rx_buffer = 0;
    tx.tx_buffer = &mask;

    esp_err_t ret;

    ret = gpio_set_level(pins.rclk_sel, 0);
    ESP_ERROR_CHECK(ret);

    ret = spi_device_transmit(spi, &tx);
    ESP_ERROR_CHECK(ret);

    gpio_set_level(pins.rclk_sel, 1);
    ESP_ERROR_CHECK(ret);

    selected_device = device;
}

void FlipdotDriver::deselect_device() {
    select_device(0x00);
}

uint8_t FlipdotDriver::encode_column(int x) {
    if (x < 0 || x >= module_width) return 0x00;

    int col = x + 1;
    if (col <= 7) return col;
    if (col <= 14) return col + 1;
    if (col <= 21) return col + 2;
    return col + 3;
}

uint8_t FlipdotDriver::encode_status(int x, int dir) {
    uint8_t result = ~encode_column(x) << 3;
    if (dir == 0) {
        result |= BIT1;
    }
    return result;
}

uint16_t FlipdotDriver::encode_row(int y) {
    if (y < 0 || y >= module_height) {
        return 0x0000;
    }
    return 0x0001 << y;
}

void FlipdotDriver::clear_registers() {
    gpio_set_level(pins.clr, 0);

    gpio_set_level(pins.rclk_conf, 0);
    gpio_set_level(pins.rclk_sel, 0);

    ets_delay_us(1);

    gpio_set_level(pins.rclk_conf, 1);
    gpio_set_level(pins.rclk_sel, 1);

    gpio_set_level(pins.clr, 1);
}


