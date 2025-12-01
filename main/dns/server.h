#pragma once

#include "generated.h"
#include "result.h"
#include "dns_server.h"
#include "esp_netif.h"


namespace dns {

	class server {
		static inline dns_server_handle_t handle = nullptr; //valid?
		public:
			server() = delete;
			static resBool begin() {
				//dns_server_config_t config = { 1, {	{ APP_DNS_DOMAIN, nullptr, { esp_ip4addr_aton( WIFI_AP_DHCP_STATIC_IP ) } } } };
				dns_server_config_t config = { 1, {	{ APP_DNS_DOMAIN, "WIFI_AP_DEF", {} } } };
				if (handle = start_dns_server(&config); handle != nullptr) {
					return ResBoolOK;
				} else {
					return ResBoolFAIL;
				}
			};
			static resBool end()   {
				if (handle != nullptr) {
					stop_dns_server(handle);
					return ResBoolOK;
				}
				return ResBoolFAIL;
			};
	};
	
}

