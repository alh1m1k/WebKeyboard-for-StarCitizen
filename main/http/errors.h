#pragma once

#include "esp_err.h"

namespace http {
	#define HTTP_ERR_BASE = ESP_ERR_HTTPD_BASE + 100;
	
	#define HTTP_ERR_HEADERS_ARE_SENDED ESP_ERR_HTTPD_BASE + 1
	#define HTTP_ERR_FD_INVALID 		ESP_ERR_HTTPD_BASE + 2

}


