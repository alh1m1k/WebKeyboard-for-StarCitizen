#pragma once

#include <variant>
#include <future>
#include <concepts>
#include <iostream>

#include "esp_err.h"

template <typename T, typename U>
concept TConstructibleFrom = requires(std::initializer_list<U> il) { T(il); };

template <typename T, typename... Args>
constexpr bool is_in_pack = (std::is_same_v<T, typename std::remove_const<Args>::type> || ...);

template<typename ...ARG>
class result final : public std::variant<ARG..., esp_err_t> {
		
	constexpr static auto size = sizeof...(ARG);

	using base_type = std::variant<ARG..., esp_err_t>;

	template <typename U>
	static base_type makeFromInitializerList(std::initializer_list<U> il) {
		base_type result;
		bool constructed = false;

		//unfold ARG
		([&]<typename T>() {
			if constexpr (TConstructibleFrom<T, U>) {
				if (!constructed) {
					result.template emplace<T>(il);
					constructed = true;
				}
			}
		}.template operator()<ARG>(), ...);

		return result;
	}

	public:

		template<typename T> requires std::constructible_from<std::variant<ARG..., std::monostate>, T>
		result(T&& value): base_type(std::forward<T>(value)) { };

		template <typename U> requires (TConstructibleFrom<ARG, U> || ...)
		result(std::initializer_list<U> il): base_type(makeFromInitializerList<U>(il)) {}

		result(esp_err_t code): base_type(code) {};

		~result() = default;
				
		inline operator bool() const {
			//std::cout << "result index " << (int)this->index() << " " << (int)size << "\n";
			return this->index() == size ? std::get<size>(*this) == (esp_err_t)ESP_OK : true;
		}

		/**
		 * trivial accessor awl if result variant only have one value and error
		 * types also work: std::enable_if_t<RESTRICT == 1, int> = 0
		 */
		template <int RESTRICT = size, typename = std::enable_if_t<RESTRICT == 1>>
		auto& data() const {
			return std::get<0>(*this);
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


