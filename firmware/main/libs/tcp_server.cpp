#include "tcp_server.h"

#include <string.h>
#include <lwip/err.h>
#include <lwip/sockets.h>

#define TAG "tcp-server"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>

#include "../commandinterpreter.h"

#define TCP_IPV4
#define TCP_PORT 3000
#define TCP_RECV_TIMEOUT_SEC 4

CommandInterpreter* cmx = new CommandInterpreter(512, &send_response, &close_tcp_connection);
int listen_sock = 0;
int connection_sock = 0;

void close_tcp_connection() {
    ESP_LOGD(TAG, "Closing connection");
    shutdown(connection_sock, 0);
    close(connection_sock);
    connection_sock = 0;
}

void send_response(const uint8_t* data, size_t len) {
    if (len > 0) {
        if (connection_sock > 0) {
            ESP_LOGD(TAG, "Replying with %d bytes of data", len);

            int to_write = len;
            while (to_write > 0) {
                int written = send(connection_sock, data + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred sending reply: errno %d", errno);
                    close_tcp_connection();
                    return;
                }
                to_write -= written;
            }
        } else {
            ESP_LOGD(TAG, "Trying to reply, but connection has already been closed");
        }
    }
}



void tcp_server_task(void *pvParameters) {
    char rx_buffer[64];
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
#endif

        listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            break;
        }

        // Make it possible to reuse bound address immediately
        int flag = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

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
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            break;
        }

        while (1) {

            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);

            if (connection_sock == 0) {
                ESP_LOGD(TAG, "Waiting for incoming connection...");
                connection_sock = accept(listen_sock, (struct sockaddr *) &source_addr, &addr_len);
                if (connection_sock < 0) {
                    ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                    connection_sock = 0;
                    continue;
                } else {
                    // Set recv timeout
                    timeval tv = {.tv_sec = TCP_RECV_TIMEOUT_SEC, .tv_usec = 0};
                    setsockopt(connection_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                }
            } else {
                ESP_LOGD(TAG, "Waiting for more data...");
            }

            int len = recv(connection_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

            // Error occured during receiving
            if (len < 0) {
                if (errno == EAGAIN) {
                    ESP_LOGD(TAG, "Data reception timeout");
                } else if (errno == ECONNRESET) {
                    ESP_LOGD(TAG, "Connection has been closed by peer");
                } else {
                    ESP_LOGE(TAG, "Data reception failed: errno %d", errno);
                }

                close_tcp_connection();
                continue;

            // Data received
            } else if (len > 0) {
                ESP_LOGD(TAG, "%d bytes of data received", len);

                cmx->queue((uint8_t*)rx_buffer, len);
                while(cmx->process()) { }
                // while processing, close_tcp_connection can be called, as it is the close handler

            // Connection has been shut down by peer
            } else {
                ESP_LOGD(TAG, "Connection has been closed by peer");
                close_tcp_connection();
            }
        }

        if (connection_sock > 0) {
            close_tcp_connection();
        }

        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(listen_sock, 0);
        close(listen_sock);
    }

    vTaskDelete(NULL);
}