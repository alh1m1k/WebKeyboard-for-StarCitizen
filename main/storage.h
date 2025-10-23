#pragma once

#include "generated.h"

#include <functional>
#include <memory>
#include <nvs_handle.hpp>

#include "config.h"
#include "esp_err.h"
#include "exception/bad_api_call.h"
#include "result.h"
#include "wifi/nvsRes.h"

class storage {
	
	std::shared_ptr<wifi::nvsRes> 	nvsRes;
	std::unique_ptr<nvs::NVSHandle> nvsHandle;
	
	esp_err_t initErr = ESP_OK;
	esp_err_t lastErr = ESP_OK;
	
	public:
	
		storage(): storage("nvs") {

		}
		
		storage(const char* ns) {
			try {
				nvsRes	  = wifi::nvsRes::acquire();
			} catch(bad_api_call& e) {
				initErr = e.code();
				error("storage::storage fail open nvs storage", initErr, " ", ns);
				return;
			}
			nvsHandle = nvs::open_nvs_handle(ns, NVS_READWRITE, &initErr);
			if (initErr != ESP_OK) {
				error("storage::storage fail open nvs storage", initErr, " ", ns);
			}
		}
		
		virtual ~storage() {
/*			if (nvsHandle) {
				nvsHandle.commit();
			}*/
		}
		
		bool reset() {
			nvsHandle->erase_item("wifi-ssid");
			nvsHandle->erase_item("wifi-pwd");
			nvsHandle->erase_item("wifi-auth");
			nvsHandle->erase_item("storage-sign");
			return true;
		}
		
		std::string getWifiSSID(std::string defaultValue = WIFI_SSID) {
			
/*			size_t 	  size 	= 0;
			esp_err_t code 	= ESP_OK;
*//*
			get_item_size: Size of the item, if it exists.
            For strings, this size includes the zero terminator.
*//*
			if (code = nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi-ssid", size); code == ESP_OK && size >= 3+1 ) {
				std::string value = {};
				value.resize(size); //use string like buffer
*//*
			get_string(,,len) The length of the output buffer pointed to by out_str/out_blob.
		    Use \c get_item_size to query the size of the item beforehand.
*//*
				if (code = nvsHandle->get_string("wifi-ssid", value.data(), value.capacity()); code == ESP_OK) {
					lastErr = ESP_OK;
					value.resize(size-1); //resize string to actual size
					return value;
				}
			} 
			
			lastErr = code;*/
			return defaultValue;
		}
		
		resBool setWifiSSID(const std::string& value) {
			//lastErr is variable so it safe use it to construct resBool
			return lastErr = nvsHandle->set_string("wifi-ssid", value.c_str());
		}
		
		std::string getWifiPWD(std::string defaultValue = WIFI_PWD) {
			
/*			size_t 	  size 	= 0;
			esp_err_t code 	= ESP_OK;
			
			if (code = nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi-pwd", size); code == ESP_OK && size >= 3+1 ) {
				std::string value = {};
				value.resize(size); //@see getWifiSSID
				if (code = nvsHandle->get_string("wifi-pwd", value.data(), value.capacity()); code == ESP_OK) {
					value.resize(size-1);
					return value;
				}
			}
				
			lastErr = code;*/
			return defaultValue;
		}
		
		resBool setWifiPWD(const std::string& value) {
			return lastErr = nvsHandle->set_string("wifi-pwd", value.c_str());
		}
		
		wifi_auth_mode_t getWifiAuth(wifi_auth_mode_t defaultValue = WIFI_AP_AUTH) {
			
/*			size_t 	  size 	= 0;
			esp_err_t code 	= ESP_OK;
			
			if (code = nvsHandle->get_item_size(nvs::ItemType::I32, "wifi-auth", size); code == ESP_OK) {
				int32_t value;
				if (code = nvsHandle->get_item("wifi-auth", value); code == ESP_OK) {
					return (wifi_auth_mode_t)value;
				}
			}
			
			lastErr = code;*/
			return defaultValue;
		}
		
		resBool setWifiAuth(const wifi_auth_mode_t& value) {
			return lastErr = nvsHandle->set_item("wifi-auth", (int)value);
		}
		
		std::string getStorageSign() {
			
			size_t 	  size 	= 0;
			esp_err_t code 	= ESP_OK;
			
			if (auto code = nvsHandle->get_item_size(nvs::ItemType::SZ, "storage-sign", size); code == ESP_OK) {
				std::string value = {};
				value.resize(size); //@see getWifiSSID
				if (code = nvsHandle->get_string("storage-sign", value.data(), value.capacity()); code == ESP_OK) {
					value.resize(size-1);
					return value;
				}
			}
			
			lastErr = code;
			return {};
		}
		
		resBool setStorageSign(const std::string& value) {
			return lastErr = nvsHandle->set_string("storage-sign", value.c_str());
		}
		
		esp_err_t lastError() {
			return lastErr;
		}
		
		esp_err_t initError() {
			return initErr;
		}
		
};