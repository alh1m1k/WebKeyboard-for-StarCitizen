#pragma once

#include "generated.h"

#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <string>

#include "result.h"
#include "generated.h"

namespace wifi {
	
	//wifi state
	
	enum class status_e {
		FREE,
		INITED,
		CONNECTING,
		CONNECTED,
		DISCONNECTED,
	};
	
	resBool 				init();
	
	resBool 				deinit();
	
	std::future<resBool>	connectAsync(const char* ap, const char* pwd);
	
	resBool 				connect(const char* ap, const char* pwd);
	
	resBool 				connect(const std::string& ap, const std::string& pwd);
	
	std::future<resBool>	disconnectAsync();
	
	resBool 				disconnect();
	
	resBool 				hotspotBegin(const char* ap, const char* pwd, wifi_auth_mode_t auth);
	
	resBool 				hotspotBegin(const std::string& ap, const std::string& pwd, wifi_auth_mode_t auth);
	
	resBool 				hotspotEnd();
	
	status_e                status();
	
	resBool 				mode(wifi_mode_t mode);
	
	result<wifi_mode_t> 	mode();
	
	resBool 				chanel(uint8_t num);
	
	result<uint8_t>			chanel();
	
	resBool 				power(int8_t power);
	
	result<int8_t> 			power();
}


