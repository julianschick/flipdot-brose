#include "http_server.h"

#include <regex>
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
            //httpd_register_uri_handler(handle, &cmd_get);
            //httpd_register_uri_handler(handle, &position_set);
            httpd_register_uri_handler(handle, &reboot);
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

    esp_err_t receive_and_display_bitset(httpd_req_t *req, bool incremental) {
        uint8_t content[256];
        size_t recv_size = MIN(req->content_len, sizeof(content));

        printf("content_len=%d\n", req->content_len);

        int ret = httpd_req_recv(req, (char*) content, recv_size);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            respond_500(req);
            return ESP_FAIL;
        }

        printf("bitset pushed:\n");

        for (int i = 0; i < recv_size; i++) {
            printf("byte %d = %#02x\n", i, content[i]);
        }

        BitArray bits (display->get_number_of_pixels());
        bits.copy_from(content, recv_size);

        if (incremental) {
            display->display_incrementally(bits);
        } else {
            display->display_overriding(bits);
        }

        respond_200(req, "OK\n");

        return ESP_OK;
    }

    esp_err_t bitset_post_handler(httpd_req_t *req) {
        return receive_and_display_bitset(req, false);
    }

    esp_err_t bitset_put_handler(httpd_req_t *req) {
        return receive_and_display_bitset(req, true);
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

    esp_err_t reboot_handler(httpd_req_t *req) {
        respond_200(req, "REBOOT");
        Comx::interpret("reboot");
        return ESP_OK;
    }

    PRIVATE_END

}

