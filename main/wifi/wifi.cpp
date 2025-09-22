#include "wifi.h"

#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <atomic>
#include "config.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "generated.h"
#include "nvs.h"
#include "result.h"
#include <optional>
#include <iostream>
#include <string>
#include <sys/_stdint.h>
#include <esp_netif_sntp.h>
#include "esp_mac.h"
#include "esp_netif_ip_addr.h"
#include "nvsRes.h"

#include "util.h"

namespace wifi {
	
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

#if defined(WIFI_AP_DHCP_STATIC_IP) && defined(WIFI_AP_DHCP_STATIC_MASK)
	static esp_netif_ip_info_t static_ip = {};
#endif

	struct client_s {
		bool connected 	= false;
		status_e status = status_e::FREE;
		std::optional<std::promise<resBool>> nextPromise;
		std::mutex m 	= {};
		esp_netif_t* netif = nullptr;
		std::atomic<int> ap_sta_count = 0; //call from wifi evt loop probably synchronous but no way to check it
		std::shared_ptr<nvsRes> nvs;
	};
	
	client_s client = {};
	
	struct handlers {
		static void sta_connect_disconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
			client_s* client = static_cast<struct client_s*>(arg);
			bool chose = false;
			debugIf(LOG_WIFI, "wifi_event", event_id);
			switch (event_id) {
				case WIFI_EVENT_STA_DISCONNECTED:
				case WIFI_EVENT_STA_STOP:
					client->connected = false;
					chose = true;
					client->status = status_e::DISCONNECTED;
					break;
				case WIFI_EVENT_STA_CONNECTED:
					client->connected = true;
					chose = true;
					client->status = status_e::CONNECTED;
					break;
				default:
					std::cout << "handlers::sta_connect_disconnect rcv invalid eventId " << event_id << std::endl;
			}
			if (chose && client->nextPromise.has_value()) {
				client->m.lock();
				if (client->nextPromise.has_value()) {
					auto& promise = client->nextPromise.value();
					promise.set_value(client->connected ? (esp_err_t)ESP_OK : (esp_err_t)ESP_FAIL);
					client->nextPromise.reset();
				}
				client->m.unlock();
			}
		}
		static void sta_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
			//client_s* client = static_cast<struct client_s*>(arg);
			auto data= static_cast<ip_event_got_ip_t*>(event_data);
			info("GOT IP FROM AP", IP2STR(&data->ip_info.ip));
		}
		static void ap_connect_disconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
			client_s* client = static_cast<struct client_s*>(arg);
			debugIf(LOG_WIFI, "wifi_event", event_id);
			std::string macAddres = {};
			macAddres.resize(20);
			switch (event_id) {
				case WIFI_EVENT_AP_STACONNECTED:
					client->connected = true;
					client->status = status_e::CONNECTED;
					client->ap_sta_count++;
					std::snprintf(macAddres.data(), macAddres.capacity(), MACSTR, MAC2STR(static_cast<wifi_event_ap_stadisconnected_t*>(event_data)->mac));
					info("STA CONNECTED", macAddres);
					break;
				case WIFI_EVENT_AP_STOP:
					client->ap_sta_count = 0;
					client->connected = false;
					client->status = status_e::DISCONNECTED;
					break;
				case WIFI_EVENT_AP_STADISCONNECTED:
					client->ap_sta_count--;
					if (client->ap_sta_count == 0) {
						client->connected = false;
						client->status = status_e::DISCONNECTED;
					}
					std::snprintf(macAddres.data(), macAddres.capacity(), MACSTR, MAC2STR(static_cast<wifi_event_ap_stadisconnected_t*>(event_data)->mac));
					info("STA DISCONNECTED", macAddres);
					break;
				default:
					error("handlers::ap_connect_disconnect rcv invalid eventId", event_id);
			}
		}
		static void ap_assign_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
			//client_s* client = static_cast<struct client_s*>(arg);
			auto data= static_cast<ip_event_ap_staipassigned_t*>(event_data);
			
			{
				std::string macAddres = {};
				macAddres.resize(20);
				std::snprintf(macAddres.data(), macAddres.capacity(), MACSTR, MAC2STR(data->mac));
				info("ASSIGN IP FROM AP", macAddres, " -> ", IP2STR(&data->ip));
			}
		}
	};
	
	inline esp_err_t updateAP(const char* ap, const char* pwd, wifi_auth_mode_t auth) {
		
		if (size_t apLen = strlen(ap); apLen < 7 || apLen + 1 > 32) { //ssid less than 7 symbols start spot that client unable to connect
			error("wifi::updateAP SSID to short");
			return ESP_FAIL;
		}
		
		if (size_t pwdLen = strlen(pwd); pwdLen < 7 || pwdLen + 1 > 64) {
			error("wifi::updateAP password to short");
			return ESP_FAIL;
		}
		
		wifi_config_t conf;
		
		CHECK_CALL_RET(esp_wifi_get_config(WIFI_IF_AP, &conf));
		
		strcpy((char*)conf.ap.ssid, ap);
		strcpy((char*)conf.ap.password, pwd);
		conf.ap.authmode = auth;
		conf.ap.pmf_cfg.required = true;
	    if (strlen(pwd) == 0) {
       		conf.ap.authmode = WIFI_AUTH_OPEN;
    	}

#ifndef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
			if (conf.ap.authmode == WIFI_AUTH_WPA3_PSK) {
				throw unsupported("WIFI_AUTH_WPA3_PSK is unsupported or disabled");
			}
#endif

		infoIf(LOG_WIFI_AP_PWD, "updateAP", ap, " ", pwd, " ", auth);
		
		CHECK_CALL_RET(esp_wifi_set_config(WIFI_IF_AP, &conf));
		
		return ESP_OK;
	}
	
	inline esp_err_t updateSTA(const char* ap, const char* pwd) {
		
		if (size_t apLen = strlen(ap); apLen < 3 || apLen + 1 > 32) {
			return ESP_FAIL;
		}
		
		if (size_t pwdLen = strlen(pwd); pwdLen < 7 || pwdLen + 1 > 64) {
			return ESP_FAIL;
		}
		
		wifi_config_t conf;
		
		CHECK_CALL_RET(esp_wifi_get_config(WIFI_IF_STA, &conf));
		
		strcpy((char*)conf.sta.ssid, ap);
		strcpy((char*)conf.sta.password, pwd);
		
		infoIf(LOG_WIFI_AP_PWD, "updateSTA", ap, " ", pwd);
		
		CHECK_CALL_RET(esp_wifi_set_config(WIFI_IF_STA, &conf));
		
		return ESP_OK;
	}
	
	inline esp_err_t clearListeners() {
		
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_STOP, handlers::sta_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, handlers::sta_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, handlers::sta_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, IP_EVENT_STA_GOT_IP, handlers::sta_got_ip);
		
		
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STOP, handlers::ap_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, handlers::ap_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, handlers::ap_connect_disconnect);
		esp_event_handler_unregister(WIFI_EVENT, IP_EVENT_AP_STAIPASSIGNED, handlers::ap_assign_ip);
		
		return ESP_OK;
	}
	
	inline esp_err_t setupListeners() {
	
		if (esp_err_t code = UNTIL_FIRST_ERROR({
			if (auto m = mode(); !m) {
				return m.code(); 
			} else if (std::get<wifi_mode_t>(m) == wifi_mode_t::WIFI_MODE_STA) {
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, handlers::sta_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, handlers::sta_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, handlers::sta_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, handlers::sta_got_ip, (void*)&client));
			} else if (std::get<wifi_mode_t>(m) == wifi_mode_t::WIFI_MODE_AP) {
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, handlers::ap_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, handlers::ap_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, handlers::ap_connect_disconnect, (void*)&client));
				CHECK_CALL_RET(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, handlers::ap_assign_ip, (void*)&client));
			} else {
				error("wifi::setupListeners unsupport wifi mode");
				return ESP_FAIL;
			} 
		}); code != ESP_OK ) {
			clearListeners();
			return code;
		}
		return ESP_OK;
	}
		
	inline esp_err_t setupWatchdog() {
		return ESP_OK;
	}
	
	inline esp_err_t clearWathdog() {
		return ESP_OK;
	}
	
	resBool init() {
		
		client.m.lock();
		
		try {
			client.nvs = nvsRes::acquire();
		} catch (bad_api_call &e) {
			debugIf(LOG_WIFI, "wifi::init nvs error");
			client.m.unlock();
			return ResBoolFAIL;
		}
		
		
		if (esp_err_t code = UNTIL_FIRST_ERROR({
			CHECK_CALL_RET(esp_event_loop_create_default());
			CHECK_CALL_RET(esp_wifi_init(&config)); 
			CHECK_CALL_RET(esp_netif_init());
			if (client.netif = esp_netif_create_default_wifi_sta(); client.netif == nullptr) {
				return ESP_FAIL;
			}
			CHECK_CALL_RET(esp_wifi_set_storage(wifi_storage_t::WIFI_STORAGE_RAM)); 
			CHECK_CALL_RET(esp_wifi_set_mode(wifi_mode_t::WIFI_MODE_STA));
		}); code != ESP_OK ) {
			client.m.unlock();
			deinit();
			return ResBoolFAIL;
		}
		

		client.status = status_e::INITED;
		
		client.m.unlock();
					
		return ResBoolOK;
	
	}
	
	resBool deinit() {
		
		std::lock_guard<std::mutex> guardian {client.m};
		
		esp_wifi_deinit();
		esp_event_loop_delete_default();
		
		client.status = status_e::FREE;
		client.connected = false;
		client.nvs = nullptr;
		
		return ResBoolOK;
	}
	
	std::future<resBool> connectAsync(const char* ap, const char* pwd) {
		
		std::lock_guard<std::mutex> guardian {client.m};
		
		if (client.connected) {
			return resolve(ESP_OK);
		}
		
		if (client.status < status_e::INITED) {
			return resolve(ESP_ERR_WIFI_NOT_INIT);
		}
		
		info("wifi::connectAsync begin");
		
		if (!client.nextPromise.has_value()) {
			client.nextPromise.emplace();
		}
		
		auto& promise = client.nextPromise.value();
		
		if (esp_err_t code = UNTIL_FIRST_ERROR({
			CHECK_CALL_RET(esp_wifi_start());
			CHECK_CALL_RET(updateSTA(ap, pwd));
			CHECK_CALL_RET(setupListeners());
			CHECK_CALL_RET(esp_wifi_connect()); 
			CHECK_CALL_RET(setupWatchdog()); 
		}, ap, pwd); code != ESP_OK ) {
			error("wifi::connectAsync error", code);
			promise.set_value(code);
		}
			
		return promise.get_future();
	}
	
	resBool connect(const char* ap, const char* pwd) {
		
		auto result = connectAsync(ap, pwd);
		result.wait();
		
		return result.get(); 
	}
	
	resBool connect(const std::string& ap, const std::string& pwd) {
		
		auto result = connectAsync(ap.c_str(), pwd.c_str());
		result.wait();
		
		return result.get(); 
	}
	
	resBool hotspotBegin(const char* ap, const char* pwd, wifi_auth_mode_t auth) {
		
		std::lock_guard<std::mutex> guardian {client.m};
		
		if (client.connected) {
			return ESP_OK;
		}
		
		if (client.status < status_e::INITED) {
			return ESP_ERR_WIFI_NOT_INIT;
		}
		
		info("wifi::hotspotBegin");

		if (esp_err_t code = UNTIL_FIRST_ERROR({
			CHECK_CALL_RET(updateAP(ap, pwd, auth));
			CHECK_CALL_RET(setupListeners());
			CHECK_CALL_RET(esp_wifi_start());
		}, ap, pwd, auth); code != ESP_OK ) {
			error("wifi::hotspotBegin error", code);
			return code;
		}
			
		return ESP_OK;
	}
	
	resBool hotspotBegin(const std::string& ap, const std::string& pwd, wifi_auth_mode_t auth) {
		debugIf(LOG_WIFI_AP_PWD, "hotspotBegin", ap.c_str(), " ", pwd.c_str(), " ", auth);
		return hotspotBegin(ap.c_str(), pwd.c_str(), auth);
	}
	
	resBool hotspotEnd() {
		return ESP_OK;
	}
	
	std::future<resBool> disconnectAsync() {
		
		std::lock_guard<std::mutex> guardian {client.m};
		
		if (!client.connected) {
			return resolve(ESP_OK);
		}
		
		clearListeners();
		clearWathdog();
		
		if (esp_err_t ret = esp_wifi_stop(); ret != ESP_OK) {
			return resolve(ret);
		}
		
		client.connected = false;
		client.status = status_e::DISCONNECTED;
						
		return resolve(ESP_OK);
	}
		
	resBool disconnect() {
		
		std::lock_guard<std::mutex> guardian {client.m};
		
		if (!client.connected) {
			return ResBoolFAIL;
		}
		
		clearListeners();
		clearWathdog();
		
		CHECK_CALL_RET(esp_wifi_stop());
		
		client.connected = false;
		client.status = status_e::DISCONNECTED;
				
		return ResBoolOK;
	}
	
	status_e status() {
		return client.status;
	}
		
	resBool mode(wifi_mode_t mode) {
		esp_netif_ip_info_t ip_info = {};
		//esp_netif_dhcp_status_t s = {};
		std::lock_guard<std::mutex> guardian {client.m};
		if (client.netif != nullptr) {
			debugIf(LOG_WIFI, "destroy old netif");
			esp_netif_destroy_default_wifi(client.netif);
			client.netif = nullptr;
		}
		if (esp_err_t code = esp_wifi_set_mode(mode); code == ESP_OK) {
			switch (mode) {
				case WIFI_MODE_AP:
					client.netif = esp_netif_create_default_wifi_ap();
					
					
#ifdef WIFI_AP_DHCP_STATIC_IP
						CHECK_CALL_RET(esp_netif_dhcps_stop(client.netif));
											 	
						esp_netif_str_to_ip4(WIFI_AP_DHCP_STATIC_IP, &static_ip.ip);
						esp_netif_str_to_ip4(WIFI_AP_DHCP_STATIC_MASK, &static_ip.netmask);
						esp_netif_str_to_ip4(WIFI_AP_DHCP_STATIC_GATEWAY, &static_ip.gw);
						
						esp_netif_get_ip_info(client.netif, &ip_info);
						infoIf(LOG_WIFI, "default dhcpc ip", IP2STR(&ip_info.ip), " ", IP2STR(&ip_info.netmask), " ", IP2STR(&ip_info.gw));
						info("static ip", IP2STR(&static_ip.ip), " ", IP2STR(&static_ip.netmask), " ", IP2STR(&static_ip.gw));
	 					CHECK_CALL_RET(esp_netif_set_ip_info(client.netif, &static_ip));
						CHECK_CALL_RET(esp_netif_dhcps_start(client.netif));
#endif
					break;
				case WIFI_MODE_STA:
					client.netif = esp_netif_create_default_wifi_sta();
					break;
				default:
					client.netif = nullptr;
					error("wifi::mode no valid netif for selected wifi mode", mode);
			}
			return code;
		} else {
			return code;
		}
	}
	
	result<wifi_mode_t> mode() {
		wifi_mode_t mode;
		if (auto code = esp_wifi_get_mode(&mode); code != ESP_OK) {
			return (esp_err_t)code;
		}
		return mode; 
	}
	
	resBool chanel(uint8_t num) {
		std::lock_guard<std::mutex> guardian {client.m};
		return (esp_err_t)esp_wifi_set_channel(num, WIFI_SECOND_CHAN_ABOVE);
	}
	
	result<uint8_t>	chanel() {
		uint8_t chan;
		wifi_second_chan_t chan2;
		if (auto code = esp_wifi_get_channel(&chan, &chan2); code != ESP_OK) {
			return (esp_err_t)code;
		}
		return chan; 
	}
	
	resBool power(int8_t power) {
		std::lock_guard<std::mutex> guardian {client.m};
		return (esp_err_t)esp_wifi_set_max_tx_power(power); 
	}
	
	result<int8_t> power() {
		int8_t power = 0;
		if (auto code = esp_wifi_get_max_tx_power(&power); code != ESP_OK) {
			return (esp_err_t)code;
		}
		return power; 
	}
}


