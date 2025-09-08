#pragma once

namespace dns {

	class service {
		
		const char* type;
		const char* proto;
		const uint16_t port;
		
		public:
			service(const char* type, const char* proto, const uint16_t port): type(type), proto(proto), port(port){};
			
			inline const char* getType() {
				return type;
			}
			
			inline const char* getProtocol() {
				return proto;
			}
			
			inline uint16_t getPort() {
				return port;
			}
	};

}