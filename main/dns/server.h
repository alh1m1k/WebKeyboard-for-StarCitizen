#pragma once

#include "result.h"
#include "service.h"
#include <mdns.h>


namespace dns {
	
	class server {
		
		const char* hostname;
		const char* descr;
		
		public:
		
			server(const char* hostname, const char* descr = ""): hostname(hostname), descr(descr) {
				
			} 
			
			resBool begin() {
				
				CHECK_CALL_RET(mdns_init());
				CHECK_CALL_RET(mdns_hostname_set(hostname));
				CHECK_CALL_RET(mdns_instance_name_set(descr));
				
				return ESP_OK;
			}
			
			resBool end() {
				mdns_free();
				return ESP_OK;
			}
			
			resBool addService(service&& serv) {
				return mdns_service_add(NULL, serv.getType(), serv.getProtocol(), serv.getPort(), NULL, 0);
			}
			
			template<typename ...T>
			resBool addService(T... args) {
				return addService(service(args...));
			}
	};
	
}

