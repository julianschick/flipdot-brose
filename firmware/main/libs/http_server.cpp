#include "http_server.h"

#include <regex>
#include <esp_log.h>
#include "comx.h"
#include "../util/bitarray.h"

namespace HttpServer {

    void start() {
        handle = NULL;
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = uri_matcher;

        ESP_LOGI(TAG_HTTP, "Starting HTTP server on port %d", config.server_port);
        if (httpd_start(&handle, &config) == ESP_OK) {
            httpd_register_uri_handler(handle, &whoareyou);
            httpd_register_uri_handler(handle, &bitset_post);
            httpd_register_uri_handler(handle, &bitset_put);
            httpd_register_uri_handler(handle, &message_post);
            httpd_register_uri_handler(handle, &message_put);
            httpd_register_uri_handler(handle, &led_post);
            httpd_register_uri_handler(handle, &reboot);
            httpd_register_uri_handler(handle, &test_get);
            //httpd_register_uri_handler(handle, &position_set);
            return;
        }

        ESP_LOGI(TAG_HTTP, "Error starting HTTP server");
    }

    void stop() {
        httpd_stop(handle);
    }

    void set_display(FlipdotDisplay* display_) {
        display = display_;
    }

    void set_led_driver(WS2812Driver* led_driver_) {
        led_driver = led_driver_;
    }

    PRIVATE_SYMBOLS

    inline string get_url(httpd_req_t* req) {
        string result = string(req->uri);
        size_t qm_index = result.find_first_of("?");
        if (qm_index != -1) {
            return result.substr(0, qm_index);
        } else {
            return result;
        }
    }

    esp_err_t read_content(httpd_req_t* req, char* buffer, size_t len, size_t* actual_length) {
        size_t recv_size = MIN(req->content_len, len);

        printf("content_len=%d\n", req->content_len);

        int ret = httpd_req_recv(req, buffer, recv_size);
        if (ret <= 0) {
            *actual_length = 0;
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            respond_500(req);
            return ESP_FAIL;
        } else {
            *actual_length = ret;
            return ESP_OK;
        }
    }

    vector<uri_param_t> get_params(httpd_req_t* req) {
        string uri = string(req->uri);
        ESP_LOGI(TAG_HTTP, "uri = %s", uri.c_str());

        size_t qm_index = uri.find_first_of("?");
        if (qm_index != -1) {
            string params = uri.substr(qm_index+1, uri.size() - qm_index);
            vector<uri_param_t> result;

            ESP_LOGI(TAG_HTTP, "params = %s", params.c_str());

            stringstream ss(params);
            string token;
            while (getline(ss, token, '&')) {
                size_t eq_index = token.find_first_of("=");

                if (eq_index != -1) {
                    uri_param_t param;
                    param.name = token.substr(0, eq_index);
                    param.value = token.substr(eq_index+1, token.size() - eq_index);
                    result.push_back(param);
                }
            }

            return result;

        } else {
            return vector<uri_param_t>();
        }
    }

    bool uri_matcher(const char *uri_template, const char *uri_to_match, size_t match_upto) {
        string uri(uri_to_match);
        uri = uri.substr(0, match_upto);

        return regex_match(uri, regex(uri_template));
    }

    inline void respond_200(httpd_req_t* req, string message) {
        const char* msg = message.c_str();
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, msg, strlen(msg));
    }

    inline void respond_400(httpd_req_t* req, string message) {
        const char* msg = message.c_str();
        httpd_resp_set_status(req, "400");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, msg, strlen(msg));
    }

    inline void respond_500(httpd_req_t* req) {
        const char* msg = "Internal server error\n";
        httpd_resp_set_status(req, "500");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, msg, strlen(msg));   
    }

    esp_err_t whoareyou_handler(httpd_req_t *req) {
        respond_200(req, Comx::interpret("whoareyou"));
        return ESP_OK;
    }

    esp_err_t receive_and_display_bitset(httpd_req_t *req) {
        char content[256];
        size_t content_size = 0;
        if (read_content(req, content, sizeof(content) - 1, &content_size) != ESP_OK) {
            return ESP_FAIL;
        }

        printf("bitset pushed:\n");

        for (int i = 0; i < content_size; i++) {
            printf("byte %d = %#02x\n", i, content[i]);
        }

        BitArray bits (display->get_number_of_pixels());
        bits.copy_from((uint8_t*) content, content_size);

        display->display(bits);
        respond_200(req, "OK\n");

        return ESP_OK;
    }

    esp_err_t bitset_post_handler(httpd_req_t *req) {
        return receive_and_display_bitset(req);
    }

    esp_err_t bitset_put_handler(httpd_req_t *req) {
        return receive_and_display_bitset(req);
    }

    esp_err_t receive_and_display_message(httpd_req_t *req) {
        char content[256];
        size_t content_size = 0;
        if (read_content(req, content, sizeof(content) - 1, &content_size) != ESP_OK) {
            return ESP_FAIL;
        }

        PixelString::TextAlignment alignment = PixelString::LEFT;
        vector<uri_param_t> params = get_params(req);
        for (int i = 0; i < params.size(); i++) {
            string& name = params[i].name;
            string& value = params[i].value;

            if (name == "alignment") {
                if (value == "right") {
                    alignment = PixelString::RIGHT;
                } else if (value == "centered") {
                    alignment = PixelString::CENTERED;
                }
            }
        }

        content[content_size] = 0x00;

        std::string str (content);
        ESP_LOGI(TAG_HTTP, "%s", content);
        respond_200(req, "OK\n");

        display->display_string(str, alignment);
        return ESP_OK;
    }

    esp_err_t message_post_handler(httpd_req_t *req) {
        return receive_and_display_message(req);
    }

    esp_err_t message_put_handler(httpd_req_t *req) {
        return receive_and_display_message(req);
    }

    esp_err_t led_post_handler(httpd_req_t *req) {
        uint8_t content[16];
        size_t recv_size = MIN(req->content_len, sizeof(content));

        int ret = httpd_req_recv(req, (char*) content, recv_size);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            respond_500(req);
            return ESP_FAIL;
        }

        if (recv_size < 3) {
            respond_400(req, "Too few bytes\n");
            return ESP_FAIL;
        }

        color_t color = {{content[2], content[0], content[1], 0x00}};
        led_driver->set_all_colors(color);
        led_driver->update();
        respond_200(req, "OK\n");
        return ESP_OK;
    }

    esp_err_t reboot_handler(httpd_req_t *req) {
        respond_200(req, "REBOOT");
        Comx::interpret("reboot");
        return ESP_OK;
    }

    esp_err_t cmd_get_handler(httpd_req_t *req) {
        string url = get_url(req);

        smatch match;
        std::regex_match(url, match, regex("/([0-9]+)/cmd/?"));
        if (match.size() > 1) {
            respond_200(req, Comx::interpret("getcmd " + match.str(1)));
        } else {
            respond_500(req);
        }
        return ESP_OK;
    }

    esp_err_t test_get_handler(httpd_req_t* req) {
        vector<uri_param_t> params = get_params(req);

        ESP_LOGD(TAG_HTTP, "params: %d", params.size());
        for (int i = 0; i < params.size(); i++) {
            ESP_LOGD(TAG_HTTP, "param extracted: %s = %s", params[i].name.c_str(), params[i].value.c_str());
        }

        respond_200(req, "GOOD");
        return ESP_OK;
    }

    esp_err_t position_set_handler(httpd_req_t *req) {
        string url = get_url(req);

        smatch match;
        std::regex_match(url, match, regex("/([0-9]+)/([0-9]+)/?"));
        if (match.size() > 2) {
            respond_200(req, Comx::interpret("setpos " + match.str(1) + " " + match.str(2)));
        } else {
            respond_500(req);
        }
        return ESP_OK;
    }

    PRIVATE_END

}

