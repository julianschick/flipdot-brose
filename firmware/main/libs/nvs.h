#ifndef NVS_H_
#define NVS_H_

#include <string>
#include <nvs.h>
#include <nvs_flash.h>
#include "../names.h"

namespace Nvs {

	using namespace std;

	namespace {
        nvs_handle handle;

	    bool initialized = false;
        string openedPage = "";

        bool checkInitState();
        bool ensurePage();

        void handleGetError(esp_err_t err, string key);
        void handleSetError(esp_err_t err, string key);
    }

	void setup();

    bool getBit(string page, string key, bool defaultValue);
    bool setBit(string page, string key, bool value);

	uint8_t get8(string page, string key, uint8_t defaultValue);
    bool set8(string page, string key, uint8_t value);

    void getBlob(string page, string key, void* defaultValue, size_t* length);
    bool setBlob(string page, string key, void* value, size_t length);

    bool commit();

}

#endif //NVS_H_