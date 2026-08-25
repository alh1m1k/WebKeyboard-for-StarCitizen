#pragma once

#include "generated.h"


#include <string>
#include <string_view>

#include <esp_wifi_types.h>


#include "result.h"
#include "http/socket/socket.h"

wifi_auth_mode_t checkWifi(wifi_auth_mode_t mode) noexcept;

struct pack {
	std::string_view taskId;
	std::string_view body;
	bool success = false;
};

pack unpackMsg(const http::socket::socket::message& rawMessage, char separator = ':');

struct kbPack {
	std::string_view input 		= {};   //key or bunch keys
	std::string_view actionId 	= {};  	//aka ctrlId
	std::string_view actionType = {}; 	//aka click dbclick other
	bool hasAction 				= false;
	bool hasInput 				= false;
};

//todo refactor remove inverse
kbPack unpackKb(std::string_view rawMessage, char separator = ':');

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

kbRepeatPack unpackKbRepeatPack(std::string_view rawMessage, char separator = ':');

//todo add support for ns without ":" ie "SSID" instead of "SSID:"
result<std::string_view> unpackNs(std::string_view rawMessage, const char* ns, char separator = ':');

struct settingsPack {
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

settingsPack unpackSettings(std::string_view rawMessage, char separator = ':');

std::string resultMsg(const char* prefix, std::string_view id, bool status);

std::string settingsGetMsg(std::string_view id, const std::string& ssid, wifi_auth_mode_t authMode);

//reason 1 - connected
//reason 2 - reconnected
//reason 3 - disconnect
std::string connectedNotify(uint32_t packetId, const std::string& clientId, int reason);

std::string renameNotify(uint32_t packetId, const std::string& clientName, const std::string& oldClientName);

std::string signNotify(uint32_t packetId, const std::string& ns, const std::string& sign);

std::string deviceNotify(uint32_t packetId, const std::string& ns, int deviceList);
			
std::string kbNotify(uint32_t packetId, std::string_view actionId, std::string_view actionType);

std::string ctrNotify(uint32_t packetId, std::string_view byteStream, uint8_t joystickId = 0);

std::string settingsSetNotify(uint32_t packetId);

result<uint32_t> packetIdFromView(std::string_view view);


