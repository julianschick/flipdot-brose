#include "nvs.h"

#define TAG "nvs"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

namespace Nvs {

	void setup() {
		esp_err_t initErr = nvs_flash_init();
	    if (initErr == ESP_ERR_NVS_NO_FREE_PAGES || initErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	        ESP_LOGI(TAG, "Erasing NVS partition for reinitalization...");
            esp_err_t eraseErr = nvs_flash_erase();
	        if (eraseErr != ESP_OK) {
	            ESP_LOGE(TAG, "Error erasing NVS partition: %s", esp_err_to_name(eraseErr));
	            ESP_LOGE(TAG, "NVS not available");
	            initialized = false;
	            return;
	        } else {
                initErr = nvs_flash_init();
	        }
	    }

	    if (initErr == ESP_OK) {
	        ESP_LOGI(TAG, "NVS successfully initialized and available");
	        initialized = true;
	    } else {
	        if (initErr == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "NVS partition not found");
	        } else {
                ESP_LOGE(TAG, "Error initializing NVS: %s", esp_err_to_name(initErr));
	        }

            ESP_LOGE(TAG, "NVS not available");
	    }
	}

	namespace {

        bool checkInitState() {
            if (!initialized) {
                ESP_LOGE(TAG, "Accessing uninitialized NVS");
                return false;
            }

            return true;
        }

        bool ensurePage(string page) {
            if (openedPage != page) {
                if (openedPage != "") {
                    nvs_close(handle);
                    openedPage = "";
                }

                esp_err_t openErr = nvs_open(page.c_str(), NVS_READWRITE, &handle);
                if (openErr == ESP_OK) {
                    openedPage = page;
                    return true;
                } else {
                    ESP_LOGE(TAG, "NVS page %s could not be opened: %s", page.c_str(), esp_err_to_name(openErr));
                    return false;
                }
            }

            return true;
        }

        void handleGetError(esp_err_t err, string key) {
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGE(TAG, "Key %s on page %s could not be found.", key.c_str(), openedPage.c_str());
            } else if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error getting value: %s", esp_err_to_name(err));
            }
        }

        void handleSetError(esp_err_t err, string key) {
            if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
                ESP_LOGE(TAG, "Key %s on page %s could not be set, not enough space left.", key.c_str(),
                         openedPage.c_str());
            } else if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error setting value: %s", esp_err_to_name(err));
            }
        }
    }

    bool getBit(string page, string key, bool defaultValue) {
        if (!checkInitState()) return defaultValue;
        if (!ensurePage(page)) return defaultValue;

        uint8_t value = defaultValue;
        esp_err_t err = nvs_get_u8(handle, key.c_str(), &value);
        handleGetError(err, key);
        return value;
    }

    bool setBit(string page, string key, bool value) {
        if (!checkInitState()) return false;
        if (!ensurePage(page)) return false;

        esp_err_t err = nvs_set_u8(handle, key.c_str(), value);
        if (err == ESP_OK) {
            return true;
        } else {
            handleSetError(err, key);
            return false;
        }
    }

    uint8_t get8(string page, string key, uint8_t defaultValue) {
	    if (!checkInitState()) return defaultValue;
	    if (!ensurePage(page)) return defaultValue;

	    uint8_t value = defaultValue;
	    esp_err_t err = nvs_get_u8(handle, key.c_str(), &value);
	    handleGetError(err, key);
	    return value;
    }

    bool set8(string page, string key, uint8_t value) {
        if (!checkInitState()) return false;
        if (!ensurePage(page)) return false;

        esp_err_t err = nvs_set_u8(handle, key.c_str(), value);
        if (err == ESP_OK) {
            return true;
        } else {
            handleSetError(err, key);
            return false;
        }
    }

    void getBlob(string page, string key, void* defaultValue, size_t* length) {
        if (!checkInitState()) return;
        if (!ensurePage(page)) return;

        esp_err_t err = nvs_get_blob(handle, key.c_str(), defaultValue, length);
        handleGetError(err, key);
    }

    bool setBlob(string page, string key, void* value, size_t length) {
        if (!checkInitState()) return false;
        if (!ensurePage(page)) return false;

        esp_err_t err = nvs_set_blob(handle, key.c_str(), value, length);
        if (err == ESP_OK) {
            return true;
        } else {
            handleSetError(err, key);
            return false;
        }
    }

    bool commit() {
        if (!checkInitState()) return false;

        if (openedPage == "") {
            ESP_LOGE(TAG, "Comitting without opened page.");
            return false;
        } else {
            esp_err_t err = nvs_commit(handle);
            if (err == ESP_OK) {
                return true;
            } else {
                ESP_LOGE(TAG, "Error committing page: %s", openedPage.c_str());
                return false;
            }
        }
    }

}