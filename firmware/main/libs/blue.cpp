#include "blue.h"

#include "comx.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

uint32_t Blue::client = 0;
string Blue::buffer = "";

void Blue::setup() {

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_err_t ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(security_callback)) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(callback)) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(ESP_SPP_MODE_CB)) != ESP_OK) {
        ESP_LOGE(TAG_BT, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /* Set default parameters for Secure Simple Pairing */
    /*esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));*/

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
}

void Blue::parse_buffer() {
    size_t first_newline = buffer.find_first_of('\n');

    while (first_newline != -1) {
        string line = buffer.substr(0, first_newline + 1);
        buffer.erase(0, first_newline + 1);        

        string response = Comx::interpret(line);

        if (!response.empty()) {
            esp_spp_write(client, response.length(), (uint8_t*) response.c_str());
        }

        first_newline = buffer.find_first_of('\n');
    }
}

void Blue::callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    uint8_t* input;
    uint16_t len;
    esp_err_t ret;

    switch (event) {

        case ESP_SPP_INIT_EVT:

            ESP_LOGI(TAG_BT, "ESP_SPP_INIT_EVT");

            esp_bt_dev_set_device_name(BT_DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

            ret = esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG_BT, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
            }
            break;

        case ESP_SPP_DATA_IND_EVT:

            ESP_LOGI(TAG_BT, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                 param->data_ind.len, param->data_ind.handle);
            esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);

            input = param->data_ind.data;
            len = param->data_ind.len;

            for (int i = 0; i < len; i++) {
                buffer += (char) *input;
                input++;
            }

            parse_buffer();

            break;

        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;
        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_OPEN_EVT");
            break;
        case ESP_SPP_CLOSE_EVT:
            client = 0;
            ESP_LOGI(TAG_BT, "ESP_SPP_CLOSE_EVT");
            break;
        case ESP_SPP_START_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_START_EVT");
            break;
        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_CL_INIT_EVT");
            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_CONG_EVT");
            break;
        case ESP_SPP_WRITE_EVT:
            ESP_LOGI(TAG_BT, "ESP_SPP_WRITE_EVT");
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            client = param->open.handle;
            ESP_LOGI(TAG_BT, "ESP_SPP_SRV_OPEN_EVT");
            break;
        default:
            break;
    }
}

void Blue::security_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    /*case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG_BT, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG_BT, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(TAG_BT, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }*/
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(TAG_BT, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG_BT, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG_BT, "Input pin code: 1337");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '3';
            pin_code[2] = '3';
            pin_code[3] = '7';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }
    /*case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG_BT, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG_BT, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG_BT, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;*/
    default: {
        ESP_LOGI(TAG_BT, "event: %d", event);
        break;
    }
    }
    return;
}