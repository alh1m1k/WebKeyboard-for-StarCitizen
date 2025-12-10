#pragma once

#include <ostream>
#include <span>

#include "codes.h"
#include "contentType.h"
#include "esp_err.h"

#include "cache/iControl.h"
#include "request.h"
#include "response.h"
#include "server.h"

namespace http::resource {
	
	enum class endings {
		BINARY,
		TEXT
	};

	/*
	 * We heroically fixed what shouldn't have been broken in the first place (c)
	 *
		 At some point, it seemed like this class should be constexpr,
		 since the data part of the class is determined at compile(assembly) time.
		 This change required removing the callback from the class,
		 since a functor can't be constexpr. However, implementation showed that,
		 moving the functor makes the code extremely bulky,
		 and also that obtaining the addresses and parameters of the embed file is
		 problematic in a constexpr compatible way.
		 These changes were reverted.
	*/
	class file {

		const int 	addressStart;
		const int 	addressEnd;
		const int 	ending;
		const char* const name;
		const char* const contentType;

		public:

			typedef result<codes> handler_res_type;
			typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;
			typedef std::function<result<codes>(request& req, response& resp, server& serv, const file& file)> callback_type;

			friend response& operator<<(response& stream, const file& result);

			const char* const checksum;

			std::function<result<codes> (request& req, response& resp, server& serv, const file& file)> callback;

			file(
				int addressStart,
				int addressEnd,
				endings end,
				const char* const name				= nullptr,
				const char* const contentType		= nullptr,
				const char* const checksum 			= nullptr,
				const callback_type& callback		= nullptr
			) noexcept;
			
			file(
				int addressStart,
				int addressEnd,
				endings end,
				const char * name					= nullptr,
				enum contentType ct					= contentType::UNDEFINED,
				const char* checksum 				= nullptr,
				const callback_type& callback		= nullptr
			) noexcept;

			inline bool operator==(const file& other) const noexcept {
				return other.name == this->name;
			}

			inline bool operator!=(const file& other) const noexcept {
				return other.name != this->name;
			}

			inline bool operator<(const file& other) const noexcept {
				return other.name < this->name;
			}

			inline operator std::span<const uint8_t>() const noexcept {
				return {(const uint8_t*)addressStart, (size_t)(addressEnd - addressStart)};
			}

			inline size_t size() const noexcept {
				return addressEnd - addressStart;
			}

			handler_res_type operator()(request& req, response& resp, server& serv) const noexcept;
	};
	
	response& operator<<(response& stream, const std::string_view& str);

	inline file nofile = {0, 0, endings::BINARY, nullptr, nullptr, nullptr};

}


