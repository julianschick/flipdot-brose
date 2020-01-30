#include "udp_server.h"

#include <string.h>
#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sockets.h>

#include "../util/bitarray.h"
#include "../flipdotdisplay.h"

#define UDP_IPV4

/*
 * command table
 *
 * offset   content
 * 0x00     command identifier
 *          0x01 = who are you? (returns 0x5b 0xe7)
 *          0xA0 = show bitset
 *          0xA1 = clear display
 *          0xA2 = full display
 *          0xA3 = show message
 *          0xB0 = set all leds
 *          0xB1 = set one led
 *
 *
 * command 'show bitset'
 * 0x01     if nonzero, display is updated incrementally
 * 0x02...  bitset data
 *
 * commands 'clear display' and 'full display'
 * 0x01     if nonzero, display is updated incrementally
 *
 * command 'show message'
 * 0x01     if nonzero, display is updated incrementally
 * 0x02     alignment: 0x00 left, 0x01 right, 0x02 centered
 * 0x03     reserved
 * 0x04...  message data
 *
 * command 'set all leds'
 * 0x01     red component
 * 0x02     green component
 * 0x03     blue component
 *
 * commands 'set one led'
 * 0x01     led address
 * 0x02     red component
 * 0x03     green component
 * 0x04     blue component
 *
 *
 */

void udp_server_task(void *pvParameters) {
    char rx_buffer[256];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

#ifdef UDP_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(5876);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(5876);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG_UDP, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG_UDP, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG_UDP, "Socket binded");

        while (1) {

            ESP_LOGI(TAG_UDP, "Waiting for data");
            struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG_UDP, "recvfrom failed: errno %d", errno);
                break;
            }
                // Data received
            else {

                if (len > 0) {
                    if (rx_buffer[0] == 0x0A && len >= 3) {
                        FlipdotDisplay::DisplayMode mode = rx_buffer[1] ? FlipdotDisplay::INCREMENTAL : FlipdotDisplay::OVERRIDE;

                        BitArray bitset (dsp->get_height()*dsp->get_width());
                        bitset.copy_from((uint8_t*) rx_buffer + 2, len - 2);
                        dsp->display(bitset, mode);

                    } else if (rx_buffer[0] == 0xB0 && len >= 4) {
                        color_t c = {{
                            (uint8_t) rx_buffer[3],
                            (uint8_t) rx_buffer[1],
                            (uint8_t) rx_buffer[2],
                            0x00
                        }};

                        led_drv->set_all_colors(c);
                        led_drv->update();
                    }

                }


                // Get the sender's ip address as string
                if (sourceAddr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (sourceAddr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG_UDP, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG_UDP, "%s", rx_buffer);

                int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                if (err < 0) {
                    ESP_LOGE(TAG_UDP, "Error occured during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG_UDP, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}
