#include "nvs.h"

namespace Nvs {

	void setup() {
		esp_err_t ret = nvs_flash_init();
	    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	        ESP_ERROR_CHECK(nvs_flash_erase());
	        ret = nvs_flash_init();
	    }
	    ESP_ERROR_CHECK( ret );
	}

	bool load_bit(const char* key) {
		
		if (ESP_OK == nvs_open(page, NVS_READONLY, &handle)) {

	    	uint8_t bit = 0;
	    	if (nvs_get_u8(handle, key, &bit) == ESP_OK) {
	    		ESP_LOGI(TAG_NVS, "Bit '%s' restored = %d", key, bit);
	    		return bit;
	    	} else {
	    		ESP_LOGE(TAG_NVS, "Bit '%s' could not be restored. Defaulted to 0.", key);
	    	}

	    } else {
	    	ESP_LOGE(TAG_NVS, "Bit '%s' could not be restored: error loading nvs page. Defaulted to 0.", key);
	    }

	    return 0;
	}	

	void store_bit(const char* key, bool bit) {
	    if (ESP_OK == nvs_open(page, NVS_READWRITE, &handle)) {
	        ESP_ERROR_CHECK(nvs_set_u8(handle, key, (uint8_t) bit));
	        ESP_ERROR_CHECK(nvs_commit(handle));
	    } else {
	    	ESP_LOGE(TAG_NVS, "Bit '%s' could not be stored: Error loading NVS page.", key);
	    }
	}	

}