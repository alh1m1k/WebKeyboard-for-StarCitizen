#pragma once

#include <ostream>
#include <span>

#include "codes.h"
#include "contentType.h"
#include "esp_err.h"

#include "resource.h"


namespace http::resource::memory {
	
	enum class endings {
		BINARY,
		TEXT
	};
	
	class file: public resource {

		const int 	addressStart;
		const int 	addressEnd;
		const int 	ending;
		const char* name;
		const char* contentType;

		const std::function<result<codes> (const file& file, request& req, response& resp, server& serv)> cacheControl;

		protected:
		
			result<codes> handle(request& req, response& resp, server& serv) const noexcept override;
		
		public:

			friend response& operator<<(response& stream, const file& result);

			typedef std::function<result<codes> (const file& file, request& req, response& resp, server& serv)> cache_control_type;

			const static cache_control_type noControl;

			const char* checksum;

			file(int addressStart, int addressEnd, endings end, const char * name, const char * contentType, const char* checksum = nullptr, const cache_control_type& cache = noControl) noexcept;
			
			file(int addressStart, int addressEnd, endings end, const char * name, enum contentType ct, const char* checksum = nullptr, const cache_control_type& cache = noControl) noexcept;

            //is override needed, for now its for unification with other virtual function?
            ~file() override = default;
			
			bool operator==(const file& other) const noexcept;
			
			bool operator!=(const file& other) const noexcept;
			
			bool operator<(const file& other) const noexcept;

			inline operator std::span<const uint8_t>() const noexcept {
				return {(const uint8_t*)addressStart, (size_t)(addressEnd - addressStart)};
			}

			resource::handler_res_type operator()(request& req, response& resp, server& serv) const noexcept;

	};
	
	response& operator<<(response& stream, const std::string_view& str);

	inline file nofile = {0, 0, endings::BINARY, nullptr, nullptr};

}


