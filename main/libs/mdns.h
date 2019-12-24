#ifndef MDNS_H_
#define MDNS_H_

#include <mdns.h>

void start_mdns_service()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));

    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    mdns_service_instance_name_set("_http", "_tcp", MDNS_SERVICE_NAME);
}

#endif //MDNS_H_