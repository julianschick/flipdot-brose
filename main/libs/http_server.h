#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include "globals.h"

//#include <esp_wifi.h>
//#include <esp_event_loop.h>
//#include <esp_log.h>
//#include <esp_system.h>
//#include <nvs_flash.h>
//#include <sys/param.h>
#include <esp_http_server.h>
#include <vector>

#include "../flipdotdisplay.h";
#include "../lowlevel/ws2812driver.h"

namespace HttpServer {

    using namespace std;

	PRIVATE_SYMBOLS

        struct uri_param_t {
            string name;
            string value;
        };

        httpd_handle_t handle;

        esp_err_t whoareyou_handler(httpd_req_t *req);
        httpd_uri_t whoareyou = {
            .uri       = "/whoareyou/?",
            .method    = HTTP_GET,
            .handler   = whoareyou_handler,
            .user_ctx  = NULL
        };

        esp_err_t bitset_post_handler(httpd_req_t *req);
        httpd_uri_t bitset_post = {
            .uri       = "/bitset/?",
            .method    = HTTP_POST,
            .handler   = bitset_post_handler,
            .user_ctx  = NULL
        };

        esp_err_t bitset_put_handler(httpd_req_t *req);
        httpd_uri_t bitset_put = {
                .uri       = "/bitset/?",
                .method    = HTTP_PUT,
                .handler   = bitset_put_handler,
                .user_ctx  = NULL
        };

        esp_err_t led_post_handler(httpd_req_t *req);
        httpd_uri_t led_post = {
                .uri       = "/led/?",
                .method    = HTTP_POST,
                .handler   = led_post_handler,
                .user_ctx  = NULL
        };

        esp_err_t test_get_handler(httpd_req_t *req);
        httpd_uri_t test_get = {
                .uri       = "/test",
                .method    = HTTP_GET,
                .handler   = test_get_handler,
                .user_ctx  = NULL
        };

        esp_err_t position_set_handler(httpd_req_t *req);
        httpd_uri_t position_set = {
            .uri       = "/[0-9]+/[0-9]+/?",
            .method    = HTTP_GET,
            .handler   = position_set_handler,
            .user_ctx  = NULL
        };

        esp_err_t cmd_get_handler(httpd_req_t *req);
        httpd_uri_t cmd_get = {
                .uri       = "/[0-9]+/cmd/?",
                .method    = HTTP_GET,
                .handler   = cmd_get_handler,
                .user_ctx  = NULL
        };

        esp_err_t reboot_handler(httpd_req_t *req);
        httpd_uri_t reboot = {
            .uri       = "/reboot/?",
            .method    = HTTP_GET,
            .handler   = reboot_handler,
            .user_ctx  = NULL
        };

        bool uri_matcher(const char *uri_template, const char *uri_to_match, size_t match_upto);
        inline string get_url(httpd_req_t* req);
        vector<uri_param_t> get_params(httpd_req_t* req);

        inline void respond_200(httpd_req_t* req, string message);
        inline void respond_500(httpd_req_t* req);

        FlipdotDisplay* display;
        WS2812Driver* led_driver;

	PRIVATE_END

	void start();
    void stop();    
    void set_display(FlipdotDisplay* display_);
    void set_led_driver(WS2812Driver* led_driver_);
}

#endif //HTTP_SERVER_H_