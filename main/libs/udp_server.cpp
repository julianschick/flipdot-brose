#include "udp_server.h"

#include <string.h>
#include <lwip/err.h>
#include <lwip/sockets.h>

#define TAG "udp-server"
#include <esp_log.h>

#include "../util/bitarray.h"
#include "../flipdotdisplay.h"

#define UDP_IPV4

void udp_handle_data(uint8_t* data, size_t len, uint8_t* reply_data, size_t* reply_len) {
    size_t max_reply_len = *reply_len;
    *reply_len = 0;

    if (data[0] == 0x01 && max_reply_len >= 2) {

        reply_data[0] = 0x7a;
        reply_data[1] = 0x28;
        *reply_len = 2;

    } else if (data[0] == 0xA0 && len >= 3) {
        FlipdotDisplay::DisplayMode mode = data[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;

        BitArray bitset (dsp->get_height()*dsp->get_width());
        bitset.copy_from((uint8_t*) data + 2, len - 2);
        dsp->display(bitset, mode);

    } else if (data[0] == 0xA1) {
        dsp->clear();
    } else if (data[0] == 0xA2) {
        dsp->light();
    } else if (data[0] == 0xA3 && len >= 5) {
        FlipdotDisplay::DisplayMode mode = data[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;
        PixelString::TextAlignment alignment = PixelString::LEFT;
        switch (data[2]) {
            case 0x00: alignment = PixelString::LEFT; break;
            case 0x01: alignment = PixelString::RIGHT; break;
            case 0x02: alignment = PixelString::CENTERED; break;
        }

        std::string str = std::string(data[4], len - 4);
        dsp->display_string(str, alignment, mode);

    } else if (data[0] == 0xB0 && len >= 4) {
        color_t c = {{
             (uint8_t) data[3],
             (uint8_t) data[1],
             (uint8_t) data[2],
             0x00
         }};

        led_drv->set_all_colors(c);
        led_drv->update();
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
    }

}

void udp_server_task(void *pvParameters) {
    char rx_buffer[256];
    char tx_buffer[256];
    size_t tx_len;
    //char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

#ifdef UDP_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(3000);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        //inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(5876);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

#ifdef UDP_IPV4
        ESP_LOGI(TAG, "Socket created in IPv4 mode");
#else
        ESP_LOGI(TAG, "Socket created in IPv6 mode");
#endif

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Unable to bind socket: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound");

        err = listen(sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            close(sock);
            return;
        }

        while (1) {

            ESP_LOGI(TAG, "Waiting for data...");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);

            int recv_sock = accept(sock, (struct sockaddr *)&source_addr, &addr_len);

            int len = recv(recv_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            //int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "Data reception failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                ESP_LOGI(TAG, "%d bytes of data received", len);

                if (len > 0) {
                    tx_len = 256;
                    udp_handle_data((uint8_t*) rx_buffer, len, (uint8_t*) tx_buffer, &tx_len);
                }

                // Get the sender's ip address as string
                /*if (sourceAddr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (sourceAddr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }*/

                //rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                //ESP_LOGI(TAG_UDP, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG_UDP, "%s", rx_buffer);

                if (tx_len > 0) {
                    ESP_LOGI(TAG, "Replying with %d bytes of data", tx_len);
                    int err = send(recv_sock, tx_buffer, tx_len, 0);

                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred sending reply: errno %d", errno);
                        break;
                    }
                }

                shutdown(recv_sock, 0);
                close(recv_sock);
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

