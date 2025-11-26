#pragma once


#include "esp_wifi_types.h"
#include "make.h"


static constexpr const char* MSG_OK 	= "ok";
static constexpr const char* MSG_FAIL 	= "fail";

static constexpr const char* MSG_CLIENT_CONNECTED 		= "nt:%lu:Client %s connected";
static constexpr const char* MSG_CLIENT_RECONNECTED		= "nt:%lu:Client %s reconnected";
static constexpr const char* MSG_CLIENT_DISCONNECTED 	= "nt:%lu:Client %s disconnected";

#define LED_GREEN  (gpio_num_t)15  // the light is lit when set high level.
#define LED_YELLOW (gpio_num_t)16  // the light is lit when set high level.

#define NO_LED	   gpio_num_t::GPIO_NUM_MAX

#define BOARD_LEAD NO_LED
#define BOARD_BOOT GPIO_NUM_0

#define RESPONSE_MAX_UNCHUNKED_SIZE (1024 * 10)

#define HTTPD_TASK_STACK_SIZE (4096+1024)
