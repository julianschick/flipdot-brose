#ifndef MDNS_H_
#define MDNS_H_

#include <mdns.h>

void start_mdns_service()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set(MDNS_INSTANCE_NAME));

    mdns_service_add(NULL, MDNS_SERVICE_TYPE, "_tcp", 3000, NULL, 0);
    mdns_service_instance_name_set(MDNS_SERVICE_TYPE, "_tcp", MDNS_SERVICE_NAME);
}

#endif //MDNS_H_