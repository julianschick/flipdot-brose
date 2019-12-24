#ifndef NVS_H_
#define NVS_H_

#include "globals.h"

#include "nvs.h"
#include "nvs_flash.h"

namespace Nvs {

	using namespace std;

	PRIVATE_SYMBOLS

		const char*  page = "user_data";
		nvs_handle handle;

	PRIVATE_END

	void setup();
	bool load_bit(const char* key);
	void store_bit(const char* key, bool bit);

}


#endif //NVS_H_