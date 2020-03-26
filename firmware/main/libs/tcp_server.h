#ifndef FLIPDOT_TCP_SERVER_H
#define FLIPDOT_TCP_SERVER_H

#include "../classes.h"

void tcp_server_task(void* pvParameters);
void send_response(const uint8_t* data, size_t len);
void close_tcp_connection();

#endif //FLIPDOT_TCP_SERVER_H
