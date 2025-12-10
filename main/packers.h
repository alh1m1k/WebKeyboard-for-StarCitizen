#pragma once

#include "generated.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <charconv>

#include <esp_wifi_types.h>

#include "util.h"
#include "result.h"
#include "http/socket/socket.h"


using namespace std::literals;

std::string_view trim(std::string_view s) {
	while (!s.empty() && s.front() == ' ') {
		s.remove_prefix(1);
	}
	while (!s.empty() &&  s.back() == ' ') {
		s.remove_suffix(1);
	}
	return s;
} 

wifi_auth_mode_t checkWifi(wifi_auth_mode_t mode) {
	switch (mode) {          
	    case WIFI_AUTH_WPA2_PSK:        
	    case WIFI_AUTH_WPA_WPA2_PSK:
	    case WIFI_AUTH_WPA3_PSK:         
	    case WIFI_AUTH_WPA2_WPA3_PSK:
	    	return mode;    
		default: 
			return WIFI_AUTH_MAX;
	}
}

struct pack {
	std::string_view taskId;
	std::string_view body;
	bool success = false;
};

pack unpackMsg(const http::socket::socket::message& rawMessage, const char separator = ':') {
	
	auto idItStart= std::find(rawMessage.begin(), rawMessage.end(), separator);
	if (idItStart == rawMessage.end()) {
		debugIf(LOG_MESSAGES, "malformed message (no taskId)", rawMessage);
		return {.taskId = {}, .body = {}, .success = false};
	}
	
	auto idItEnd = std::find(++idItStart, rawMessage.end(), separator);
	if (idItEnd == rawMessage.end()) {
		debugIf(LOG_MESSAGES, "malformed message (no taskId)", rawMessage);
		return {.taskId = {}, .body = {}, .success = false};
	}

	auto taskId = std::string_view(idItStart, idItEnd);
	taskId = trim(taskId);
	
	if (++idItEnd == rawMessage.end()) {
		debugIf(LOG_MESSAGES, "message (no body)", rawMessage);
		return {.taskId = taskId, .body = {}, .success = true};
	}

	return {.taskId = taskId, .body = {idItEnd, rawMessage.end()}, .success = true};
}

struct kbPack {
	std::string_view input 		= {};    //key or bunch keys
	std::string_view actionId 	= {};  	//aka ctrlId
	std::string_view actionType = {}; 	//aka click dbclick other
	bool hasAction 				= false;
	bool hasInput 				= false;
};

//todo refactor remove inverse
kbPack unpackKb(const std::string_view rawMessage, const char separator = ':') {
		
    auto idItStart= std::find(rawMessage.rbegin(), rawMessage.rend(), separator);
	if (idItStart == rawMessage.rend()) {
		return {.input = rawMessage, .actionId = {}, .actionType = {}, .hasAction = false, .hasInput = true};
	}
	
	auto actionType = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(idItStart)), 
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(idItStart), 
			std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rbegin())
		)
    );
	
	auto idItEnd = std::find(++idItStart, rawMessage.rend(), separator);
	if (idItEnd == rawMessage.rend()) {
		return {.input = rawMessage, .actionId = {}, .actionType = {}, .hasAction = false, .hasInput = true};
	}
	
	auto actionId = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd)),
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd), 
			std::reverse_iterator<std::string_view::reverse_iterator>(idItStart)
		)
   	);
	                                
	if (++idItEnd == rawMessage.rend()) {
		return {.input = {}, .actionId = actionId, .actionType = actionType, .hasAction = true, .hasInput = false};
	}
	                                
	auto input = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rend())),
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rend()),
			std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd)
		)
    );
	
	return {.input = input, .actionId = actionId, .actionType = actionType, .hasAction = true, .hasInput = true};
}

struct kbRepeatPack {
	struct kbPack   pack               = {};
	uint32_t 		intervalMS 		   =  0;
	int32_t 		repeat 	   		   = -1;
	bool 			success	   		   = false;
	bool 			valid	   		   = false;
	bool 			validIntervalMS	   = false;
	bool 			validRepeat		   = false;
	bool 			validActionType	   = false;
	bool 			validActionId	   = false;
	bool 			validInput	   	   = false;
};

kbRepeatPack unpackKbRepeatPack(const std::string_view rawMessage, const char separator = ':') {
		
    auto repeatItStart= std::find(rawMessage.rbegin(), rawMessage.rend(), separator);
	if (repeatItStart == rawMessage.rend()) {
		debugIf(LOG_MESSAGES, "failt1");
		return {};
	}
	
	auto repeatView = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(repeatItStart)), 
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(repeatItStart), 
			std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rbegin())
		)
    );

	auto intervalMSItEnd = std::find(++repeatItStart, rawMessage.rend(), separator);
	if (intervalMSItEnd == rawMessage.rend()) {
		debugIf(LOG_MESSAGES, "failt2");
		return {};
	}
	
	auto intervalMSView = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(intervalMSItEnd)),
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(intervalMSItEnd), 
			std::reverse_iterator<std::string_view::reverse_iterator>(repeatItStart)
		)
   	);
	                                
	if (++intervalMSItEnd == rawMessage.rend()) {
		debugIf(LOG_MESSAGES, "failt3");
		return {};
	}
		                                
	auto KbPackView = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rend())),
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(rawMessage.rend()),
			std::reverse_iterator<std::string_view::reverse_iterator>(intervalMSItEnd)
		)
    );
    
    bool success 			= true;
    bool valid 				= true;
    bool validRepeat 		= true;
    bool validIntervalMS 	= true;
    bool validActionType 	= true;
    bool validActionId 		= true;
    bool validIntput		= true;
    bool hasInput			= true;

	int32_t 	repeat 		= -1;
	uint32_t 	intervalMS 	=  0;
			
    auto idItStart= std::find(KbPackView.rbegin(), KbPackView.rend(), separator);
	if (idItStart == KbPackView.rend()) {
		debugIf(LOG_MESSAGES, "failt4");
		return {};
	}
	
	auto actionTypeView = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(idItStart)), 
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(idItStart), 
			std::reverse_iterator<std::string_view::reverse_iterator>(KbPackView.rbegin())
		)
    );
	
	auto idItEnd = std::find(++idItStart, KbPackView.rend(), separator);
	if (idItEnd == KbPackView.rend()) {
		debugIf(LOG_MESSAGES, "failt5");
		return {};
	}
	
	auto actionIdView = std::string_view(
		&(*std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd)),
		std::distance(
			std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd), 
			std::reverse_iterator<std::string_view::reverse_iterator>(idItStart)
		)
   	);
   	
   	std::string_view inputView = {};
	                                
	if (++idItEnd != KbPackView.rend()) {
		inputView = std::string_view(
			&(*std::reverse_iterator<std::string_view::reverse_iterator>(KbPackView.rend())),
			std::distance(
				std::reverse_iterator<std::string_view::reverse_iterator>(KbPackView.rend()),
				std::reverse_iterator<std::string_view::reverse_iterator>(idItEnd)
			)
    	);  
	} else {
		hasInput = false;
	}
	                                    
    if (!(actionTypeView == "switched-off"sv || actionTypeView == "switched-on"sv)) {
		valid = false;
		validActionType = false;
	}
	
	if (actionTypeView == "switched-on"sv) {
		std::from_chars_result result = std::from_chars(repeatView.begin(), repeatView.end(), repeat);   
		if (result.ec != std::errc{} || (repeat > 1000)) {
			repeat = -1;
			valid = false;
			validRepeat = false;
		}      
		result = std::from_chars(intervalMSView.begin(), intervalMSView.end(), intervalMS);   
		if (result.ec != std::errc{} || (intervalMS < 10 || intervalMS > 3600000)) {
			intervalMS = 0;
			valid = false;
			validIntervalMS = false;
		} 
		if (inputView.empty()) {
			valid = false;
			validActionId = false;
		}
	}
	
	if (actionIdView.empty()) {
		valid = false;
		validActionId = false;
	}
	
	return {
		.pack = { .input = inputView, .actionId = actionIdView, .actionType = actionTypeView, .hasAction = true, .hasInput = hasInput },
		.intervalMS 		= intervalMS, .repeat = repeat, .success = success, 
		.valid 				= valid, 
		.validIntervalMS 	= validIntervalMS, 
		.validRepeat 		= validRepeat, 
		.validActionType 	= validActionType, 
		.validActionId 		= validActionId,
		.validInput    		= validIntput,
	};
}

//todo add support for ns without ":" ie "SSID" instead of "SSID:"
result<std::string_view> unpackNs(const std::string_view rawMessage, const char* ns, const char separator = ':') {
	if (auto index = rawMessage.find(ns); index == std::string_view::npos) {
		debugIf(LOG_MESSAGES, "unpackSettings no ns", ns, " ", rawMessage);
		return ESP_FAIL;
	} else {
		auto it = rawMessage.begin() + (index + strlen(ns));
		if (auto endIt = rawMessage.end(), endOfSizeIt = std::find(it, endIt, ':'); endOfSizeIt == endIt) {
			debugIf(LOG_MESSAGES, "unpackSettings no size", ns, " ", rawMessage);
			return ESP_FAIL+1;
		} else {
			size_t size;
			const std::from_chars_result result = std::from_chars(it, endOfSizeIt, size);
		    if (result.ec != std::errc{}) {
				debugIf(LOG_MESSAGES, "unpackSettings invalid size", ns, " ", rawMessage, " ", std::string_view(it, endOfSizeIt));
		        return ESP_FAIL+2;
		    }
		    if (std::distance(rawMessage.begin(), endOfSizeIt+1+size) <= rawMessage.size()){
				return std::string_view(endOfSizeIt+1, endOfSizeIt+1+size);
			} else {
				debugIf(LOG_MESSAGES, "unpackSettings range error", ns, " ", rawMessage, " ", *(endOfSizeIt+1), size);
				return ESP_FAIL+3;
			}
		}
	}
}

struct settignsPack {
	std::string_view 		ssid;         
	wifi_auth_mode_t 		auth;  		
	std::string_view 		password;
	bool success 		  = false;
	bool hasPassword 	  = false;
	bool valid            = false;
	bool validSSID 		  = false;
	bool validAuth  	  = false;
	bool validPWD  		  = false;
};

settignsPack unpackSettings(const std::string_view rawMessage, const char separator = ':') {
	
	if (auto resSSID = unpackNs(rawMessage, "SSID:", separator); !resSSID) {
		return { };
	} else {
		if (auto resAUTH = unpackNs(rawMessage, "AUTH:", separator); !resAUTH) {
			return { };
		} else {
			
			bool valid 			= true;
			bool validSSID 		= true;
			bool validAuth  	= true;
			bool validPWD  		= false;
			bool hasPassword 	= false;
			
			auto resSSIDView = std::get<std::string_view>(resSSID);
			
			int auth;
			auto resAuthView = std::get<std::string_view>(resAUTH);
			if (const auto result = std::from_chars(resAuthView.begin(), resAuthView.end(), auth); result.ec != std::errc{}) {
				debugIf(LOG_MESSAGES, "unable extract wifi_auth_mode_t from text value", resAuthView);
				return { };
			}
			
			if (resSSIDView.size() < WIFI_MIN_SSID_LEN || resSSIDView.size() > WIFI_MAX_SSID_LEN) {
				valid 		= false;
				validSSID 	= false;
				error("::unpackSettings msg has invalid ssid len", resSSIDView);
			}
			
			if (auth == 100) {
				auth = (int)wifi_auth_mode_t::WIFI_AUTH_MAX; //keep same
			} else if (auth < 0 || auth >= (int)wifi_auth_mode_t::WIFI_AUTH_MAX) {
				valid 		= false;
				validAuth 	= false;
				error("::unpackSettings invalid value of wifi_auth_mode_t", auth);
			} else if (auth = (int)checkWifi((wifi_auth_mode_t)auth); auth == (int)wifi_auth_mode_t::WIFI_AUTH_MAX) {
				valid 		= false;
				validAuth 	= false;
				error("::unpackSettings banned value of wifi_auth_mode_t", auth);
			}
			
			std::string_view resPWDView;
			if (auto resPWD = unpackNs(rawMessage, "PASSWORD:", separator); resPWD.code() == ESP_FAIL) {
				//no password;
				hasPassword = false;
				validPWD    = false;
				resPWDView = {};
			} else if (resPWD.code() == ESP_OK) {
				hasPassword = true;
				validPWD    = true;
				resPWDView  = std::get<std::string_view>(resPWD);
				if (resPWDView.size() < WIFI_MIN_PWD_LEN || resPWDView.size() > WIFI_MAX_PWD_LEN) {
					valid 		= false;
					validPWD 	= false;
					error("::unpackSettings msg has invalid pwd len", resPWDView);
				}
			} else {
				return { };
			}
			
			return { 
				.ssid = resSSIDView, .auth = (wifi_auth_mode_t)auth, .password = resPWDView, 
				.success = true, 
				.hasPassword = hasPassword,  .valid = valid, .validSSID = validSSID, .validAuth = validAuth, .validPWD = validPWD
			};
		}
	}
	
}

std::string resultMsg(const char* prefix, std::string_view id, bool status) {
	constexpr auto ok_size 	= strlen(MSG_OK);
	constexpr auto fail_size 	= strlen(MSG_FAIL);
	 
	std::string buffer = prefix;
	buffer.reserve(strlen(prefix) + 1 + id.size() + 1 + (status ? ok_size : fail_size));
	buffer += ":";
	buffer += id;
	buffer += ":";
	buffer += (status ? MSG_OK : MSG_FAIL);
	return buffer;
}


std::string settingsGetMsg(std::string_view id, const std::string& ssid, wifi_auth_mode_t authMode) {
	
	authMode = checkWifi(authMode);
	
	auto auth = std::to_string(authMode == wifi_auth_mode_t::WIFI_AUTH_MAX ? 100 : (int)authMode);

	auto buffer = resultMsg("settings-get", id, true);
	
	buffer += ":SSID:" + std::to_string(ssid.size()) + ":" + ssid;
	buffer += ":AUTH:" + std::to_string(auth.size()) + ":" + auth;
	buffer += ":AUTHDATA:73:WPA3 PSK:6:WPA2 PSK:3:WPA2 WPA3 PSK:7:WPA WPA2 PSK:4:Other(Keep Same):100";
	
	return buffer;
}

//reason 1 - connected
//reason 2 - reconnected
//reason 3 - disconnect
std::string connectedNotify(uint32_t packetId, std::string& clientId, int reason) {

	constexpr auto templateConnectedSize 		= strlen(MSG_CLIENT_CONNECTED);
	constexpr auto templateReConnectedSize 		= strlen(MSG_CLIENT_RECONNECTED);
	constexpr auto templateDisconnectedSize 	= strlen(MSG_CLIENT_DISCONNECTED);
	constexpr auto maxPacketIdSize 				= 10; //std::snprintf(nullptr, 0, "%d", 1);

	std::string buffer = "";
	int wr;
	if (reason == 1) {
		buffer.resize(templateConnectedSize + clientId.size() + maxPacketIdSize);
		wr = std::snprintf(buffer.data(), buffer.capacity(), MSG_CLIENT_CONNECTED, packetId, clientId.c_str());
	} else if (reason == 2) {
		buffer.resize(templateReConnectedSize + clientId.size() + maxPacketIdSize);
		wr = std::snprintf(buffer.data(), buffer.capacity(), MSG_CLIENT_RECONNECTED, packetId, clientId.c_str());
	} else if (reason == 3) {
		buffer.resize(templateDisconnectedSize + clientId.size() + maxPacketIdSize);
		wr = std::snprintf(buffer.data(), buffer.capacity(), MSG_CLIENT_DISCONNECTED, packetId, clientId.c_str());
	} else {
		throw std::invalid_argument("reason invalid");
	}

	if (wr == buffer.capacity()) {
		throw std::runtime_error("buffer capacity safeguard");
	}
	
	buffer.resize(wr);

	return buffer;
}

std::string signNotify(uint32_t packetId, const std::string& ns, const std::string& sign) {
	
	constexpr auto maxPacketIdSize 				= 10; //std::snprintf(nullptr, 0, "%d", 1);
		
	std::string buffer = "";
	buffer.resize(ns.size() + sign.size() + maxPacketIdSize + 8);

	auto wr = std::snprintf(buffer.data(), buffer.capacity(), "%s:%lu:%s" , ns.c_str(), packetId, sign.c_str());
	
	if (wr == buffer.capacity()) {
		throw std::runtime_error("buffer capacity safeguard");
	}
	
	buffer.resize(wr);

	return buffer;
}
			
std::string kbNotify(uint32_t packetId, std::string_view actionId, std::string_view actionType) {
	
	constexpr auto _template = "kba:%lu:%.*s:%.*s";
	constexpr auto maxPacketIdSize= 10; ///std::snprintf(nullptr, 0, "%lu", std::numeric_limits<uint32_t>::max);
			
	std::string buffer = "";
	buffer.resize(10 + maxPacketIdSize + actionId.size() + actionType.size());
	
	if (actionId.size() > std::numeric_limits<int>::max() || actionType.size() > std::numeric_limits<int>::max()) {
		throw unsupported();
	}

	auto wr = std::snprintf(buffer.data(), buffer.capacity(), _template, packetId, (int)actionId.size(), actionId.data(), (int)actionType.size(), actionType.data());
	
	if (wr == buffer.capacity()) {
		throw std::runtime_error("buffer capacity safeguard");
	}
	
	buffer.resize(wr);

	return buffer; //copy easing
}


std::string ctrNotify(uint32_t packetId, std::string_view byteStream) {
	
	constexpr auto maxPacketIdSize= 10; ///std::snprintf(nullptr, 0, "%lu", std::numeric_limits<uint32_t>::max);
			
	std::string buffer = "";
	buffer.reserve(10 + maxPacketIdSize + byteStream.size());
	
	buffer  = "ctr:";
	buffer += std::to_string(packetId);
	buffer += ":";
	buffer += byteStream;
	
	debugIf(LOG_MESSAGES, "ctrNotify", buffer.size());

	return buffer; //copy easing
}


std::string settingsSetNotify(uint32_t packetId) {
	
	constexpr auto _template = "nt:%lu:Settings updated";
	constexpr auto maxPacketIdSize= 10; 
	constexpr auto bufferSize= maxPacketIdSize + strlen(_template);
			
	std::string buffer = "";
	buffer.resize(bufferSize);
	
	auto wr = std::snprintf(buffer.data(), buffer.capacity(), _template, packetId);
	
	buffer.resize(wr);
	
	return buffer; //copy easing
}

result<uint32_t> packetIdFromView(std::string_view view) {
	uint32_t packetId = 0;
	std::from_chars_result result = std::from_chars(view.begin(), view.end(), packetId);   
	if (result.ec != std::errc{}) {
		return ESP_FAIL;
	} 
	return packetId;
}


