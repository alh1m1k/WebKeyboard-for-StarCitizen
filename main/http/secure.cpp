#include "secure.h"

#include "esp_tls.h"

namespace http {

	httpd_ssl_config_t secure::config(
		uint16_t port,
		const std::span<const uint8_t>& certPem,
		const std::span<const uint8_t>& privKeyPem,
		const std::span<const uint8_t>& caCertPem
  	) noexcept {
		httpd_ssl_config_t config 				= HTTPD_SSL_CONFIG_DEFAULT();

		config.httpd.max_uri_handlers 			= 20;
		config.httpd.max_open_sockets 			= CONFIG_LWIP_MAX_SOCKETS - SYSTEM_SOCKET_RESERVED;
		//config.httpd.stack_size 		    	= HTTPD_TASK_STACK_SIZE; //https is bigger
		//config.httpd.open_fn  					= &socketOpen; //used for https
		config.httpd.close_fn 					= &socketClose;
		config.httpd.lru_purge_enable 			= SOCKET_RECYCLE_USE_LRU_COUNTER;
		config.httpd.global_user_ctx      		= this; //todo handle move
		config.httpd.global_user_ctx_free_fn 	= &globalUserCtxFree;
		config.httpd.server_port				= port;

		if (!certPem.empty()) {
			config.servercert 			= certPem.data();
			config.servercert_len 		= certPem.size();
		}
		if (!privKeyPem.empty()) {
			config.prvtkey_pem 			= privKeyPem.data();
			config.prvtkey_len 			= privKeyPem.size();
		}
		if (!caCertPem.empty()) {
			config.cacert_pem 			= caCertPem.data();
			config.cacert_len 			= caCertPem.size();
		}

		assert(
			(config.servercert == nullptr && config.servercert == config.prvtkey_pem) ||
			(config.servercert != config.prvtkey_pem)
		);

		if (config.servercert == nullptr || config.prvtkey_pem == nullptr) {
			config.transport_mode = HTTPD_SSL_TRANSPORT_INSECURE;
			assert(!ASSERT_IF_INSECURE_HTTPS);
		}

		return config;
	}

	resBool secure::begin(uint16_t port, const cert_data_type& certPem, const cert_data_type& privKeyPem, const cert_data_type& caCertPem) noexcept {
		static_assert(SYSTEM_SOCKET_RESERVED > 0, "SYSTEM_SOCKET_RESERVED > 0");
#ifdef ASSERT_IF_SOCKET_COUNT_LESS
		static_assert(CONFIG_LWIP_MAX_SOCKETS - SYSTEM_SOCKET_RESERVED >= ASSERT_IF_SOCKET_COUNT_LESS, "ASSERT_IF_MIN_SOCKET_COUNT_LESS");
#endif
		static_assert(CONFIG_HTTPD_WS_SUPPORT);
		static_assert(CONFIG_HTTPD_MAX_REQ_HDR_LEN >= 1024);
		static_assert(CONFIG_ESP_HTTPS_SERVER_ENABLE);
		static_assert((CONFIG_LWIP_MAX_SOCKETS - SYSTEM_SOCKET_RESERVED) <= 7, "ssl socket count, esp32 memory limit");
		httpd_ssl_config_t conf = config(port, certPem, privKeyPem, caCertPem);
		if (auto code = httpd_ssl_start(&handler, &conf); code == ESP_OK) {
			afterStart();
			return code;
		} else {
			return code;
		}
	}

	resBool secure::end() noexcept {
		beforeStop();
		return httpd_ssl_stop(handler);
	}

}
