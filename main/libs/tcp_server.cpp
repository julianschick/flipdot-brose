#include "tcp_server.h"

#include <string.h>
#include <lwip/err.h>
#include <lwip/sockets.h>

#define TAG "tcp-server"
#include <esp_log.h>

#include "../util/bitarray.h"
#include "../flipdotdisplay.h"

#define TCP_IPV4
#define TCP_PORT 3000

void tcp_handle_data(uint8_t* data, size_t len, uint8_t* reply_data, size_t* reply_len) {
    size_t max_reply_len = *reply_len;
    reply_data[0] = 0x78;
    *reply_len = 1;

    // who are you?
    if (data[0] == 0x01 && max_reply_len >= 2) {

        reply_data[0] = 0x7a;
        reply_data[1] = 0x28;
        *reply_len = 2;

    // show bitset
    } else if (data[0] == 0xA0 && len >= 3) {
        FlipdotDisplay::DisplayMode mode = data[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;

        BitArray bitset (dsp->get_height()*dsp->get_width());
        bitset.copy_from((uint8_t*) data + 2, len - 2);
        dsp->display(bitset, mode);

    // clear display
    } else if (data[0] == 0xA1) {
        dsp->clear();

    // fill display
    } else if (data[0] == 0xA2) {
        dsp->fill();

    // show text message
    } else if (data[0] == 0xA3 && len >= 5) {
        FlipdotDisplay::DisplayMode mode = data[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;
        PixelString::TextAlignment alignment = PixelString::LEFT;
        switch (data[2]) {
            case 0x00:
                alignment = PixelString::LEFT;
                break;
            case 0x01:
                alignment = PixelString::RIGHT;
                break;
            case 0x02:
                alignment = PixelString::CENTERED;
                break;
        }

        std::string str = std::string(data[4], len - 4);
        dsp->display_string(str, alignment, mode);

    // set specified pixels
    } else if (data[0] == 0xA4 && len >= 5) {
        FlipdotDisplay::DisplayMode mode = data[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;

        for (int i = 0; 2 + i*3 + 2 < len; i++) {
            uint8_t x = data[2 + i*3 + 0];
            uint8_t y = data[2 + i*3 + 1];
            bool show = data[2 + i*3 + 2];

            dsp->flip_single_pixel(x, y, show, mode);
        }

    // set all leds
    } else if (data[0] == 0xB0 && len >= 4) {
        color_t c = {{
             (uint8_t) data[3],
             (uint8_t) data[1],
             (uint8_t) data[2],
             0x00
         }};

        led_drv->set_all_colors(c);
        led_drv->update();

    // set specified leds
    } else if (data[0] == 0xB1 && len >= 5) {

        for (int i = 0; 1 + i*4 + 3 < len; i++) {
            color_t c = {{
                 (uint8_t) data[1 + i*4 + 3],
                 (uint8_t) data[1 + i*4 + 1],
                 (uint8_t) data[1 + i*4 + 2],
                 0x00
             }};

            led_drv->set_color(data[1 + i*4], c);
        }
        led_drv->update();

    // get bitset
    } else if (data[0] == 0xC0) {
        if (!dsp->is_state_known()) {
            reply_data[0] = 0x00;
            *reply_len = 1;
        } else {
            size_t len = dsp->copy_state(reply_data, max_reply_len);
            *reply_len = len;
        }

    // get specified pixels
    } else if (data[0] == 0xC1 && len >= 3) {
        for (int i = 0; 1 + i*2 + 1 < len; i++) {
            uint8_t x = data[1 + i*2 + 0];
            uint8_t y = data[1 + i*2 + 1];

            if (!dsp->is_state_known()) {
                reply_data[i] = 0xA1;
            } else if (!dsp->is_valid_index(x, y)) {
                reply_data[i] = 0xA0;
            } else {
                reply_data[i] = dsp->get_pixel(x, y);
            }

            *reply_len = i + 1;
        }

    // get all leds
    } else if (data[0] == 0xD0) {
        for (size_t i = 0; i < led_drv->get_led_count(); i++) {
            color_t color = led_drv->get_color(i);
            reply_data[i*3 + 0] = color.brg.red;
            reply_data[i*3 + 1] = color.brg.green;
            reply_data[i*3 + 2] = color.brg.blue;
        }

        *reply_len = led_drv->get_led_count() * 3;

    // invalid command
    } else {
        reply_data[0] = 0x78;
        reply_data[0] = 0x71;
        *reply_len = 2;
    }
}

void tcp_server_task(void *pvParameters) {
    char rx_buffer[2048];
    char tx_buffer[2048];
    size_t tx_len;
    int addr_family;
    int ip_protocol;

    while (1) {

#ifdef TCP_IPV4
        struct sockaddr_in bound_addr;
        bound_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bound_addr.sin_family = AF_INET;
        bound_addr.sin_port = htons(TCP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        //inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(3000);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

#ifdef TCP_IPV4
        ESP_LOGI(TAG, "Socket created in IPv4 mode");
#else
        ESP_LOGI(TAG, "Socket created in IPv6 mode");
#endif

        int err = bind(listen_sock, (struct sockaddr*) &bound_addr, sizeof(bound_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Unable to bind socket: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", TCP_PORT);

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred trying to listen: errno %d", errno);
            close(listen_sock);
            return;
        }

        while (1) {

            ESP_LOGI(TAG, "Waiting for incoming connection...");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);

            int connection_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (connection_sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                continue;
            }

            int len = recv(connection_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "Data reception failed: errno %d", errno);
                break;
            }

            // Data received
            else {
                ESP_LOGI(TAG, "%d bytes of data received", len);

                if (len > 0) {
                    tx_len = 2048;
                    tcp_handle_data((uint8_t*) rx_buffer, len, (uint8_t*) tx_buffer, &tx_len);
                }

                if (tx_len > 0) {
                    ESP_LOGI(TAG, "Replying with %d bytes of data", tx_len);

                    int to_write = tx_len;
                    while (to_write > 0) {
                        int written = send(connection_sock, tx_buffer + (tx_len - to_write), to_write, 0);
                        if (written < 0) {
                            ESP_LOGE(TAG, "Error occurred sending reply: errno %d", errno);
                        }
                        to_write -= written;
                    }
                }

                shutdown(connection_sock, 0);
                close(connection_sock);
            }
        }

        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(listen_sock, 0);
        close(listen_sock);
    }
    
    vTaskDelete(NULL);
}

