#pragma once

#include <variant>
#include <future>

#include "esp_err.h"
#include "util.h"

template<typename ...ARG>
class result final : public std::variant<ARG..., esp_err_t> {
		
	constexpr static auto size = sizeof...(ARG);
	
	public: 

		template<typename T>
		result(T&& value): std::variant<ARG..., esp_err_t>(std::forward<std::conditional_t<std::is_scalar_v<T>, T, T&&>>(value)) {};

		result(esp_err_t ecode): std::variant<ARG..., esp_err_t>(ecode) {};

		template<typename T>
		explicit result(T& value): std::variant<ARG..., esp_err_t>(value) {};
				
		~result() = default;
				
		inline operator bool() const {
			return this->index() == size ? std::get<size>(*this) == (esp_err_t)ESP_OK : true;
		}

		/**
		 * trivial accessor awl if result variant only have one value and error types
		 * also work: std::enable_if_t<RESTRICT == 1, int> = 0
		 */
		template <int RESTRICT = size, typename = std::enable_if_t<RESTRICT == 1>>
		auto& data() const {
			return !!*this ? std::get<0>(*this) : throw std::bad_variant_access();
		}

		//note there is no non-trivial accessor, for more than one *data type
		//use std::get* instead
				
		inline esp_err_t code() const {
			return !*this ? std::get<size>(*this) : ESP_OK;
		}
};

typedef result<>  resBool;

const resBool ResBoolOK   = (esp_err_t)ESP_OK;
const resBool ResBoolFAIL = (esp_err_t)ESP_FAIL;

inline std::future<resBool> resolve(const esp_err_t status) {
	auto prm = std::promise<resBool>();
	prm.set_value(status);
	return prm.get_future();
}

#define CHECK_CALL(fn) (esp_err_t)(fn)
#define CHECK_CALL_RET(fn) { esp_err_t ret = (fn); if (ret != ESP_OK) return ret; } 
#define UNTIL_FIRST_ERROR(checkBlock, capture...) ( [capture]() -> esp_err_t {checkBlock; return ESP_OK;}() )


