#ifndef FLIPDOT_TCP_SERVER_H
#define FLIPDOT_TCP_SERVER_H

#include "../classes.h"

void tcp_server_task(void* pvParameters);
void response_handler(const uint8_t* data, size_t len);
void close_handler();

#endif //FLIPDOT_TCP_SERVER_H
