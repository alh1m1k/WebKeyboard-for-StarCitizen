#pragma once

#include <variant>
#include <future>

#include "esp_err.h"
#include "util.h"

/*template<typename T, typename VARIANT_T>
struct isVariantMember;

template<typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>> 
  : public std::disjunction<std::is_same<T, ALL_T>...> {};*/


template<typename ...ARG>
class result: public std::variant<ARG..., esp_err_t> {
		
	constexpr static auto size = sizeof...(ARG);
	
	public: 
	
/*		template<typename T>
		result(T value): std::variant<ARG..., esp_err_t>(value) {};*/
		
		template<typename T>
		result(const T& value): std::variant<ARG..., esp_err_t>(std::move(value)) {};
		
		result(esp_err_t ecode): std::variant<ARG..., esp_err_t>(ecode) {};
				
		~result(){} 
				
		inline operator bool() const {
			/*
			*
			if constexpr (index() != size) {
				return true;
			} else {
				return std::get<size>(*this) == (esp_err_t)ESP_OK;
			}
			
			*/
			if (this->index() != size) {
				return true;
			} else {
				return std::get<size>(*this) == (esp_err_t)ESP_OK;
			}
		}
		
		//todo how to not declare this??
/*		inline constexpr size_t index() const {
			return std::variant<ARG..., esp_err_t>::index();
		}*/
		
		//there is member data() func because i don't now how effectively implement it for non trivial classes, except of just inline std::get
				
		inline esp_err_t code() const {
			if (!*this) {
				return std::get<size>(*this);
			} else {
				return ESP_OK;
			}
		}
};

typedef result<> 						resultBoolean;
typedef resultBoolean 					resBool;

const resBool ResBoolOK 	= (esp_err_t)ESP_OK;
const resBool ResBoolFAIL 	= (esp_err_t)ESP_FAIL;

inline std::future<resBool> resolve(esp_err_t status) {
	auto prm = std::promise<resBool>();
	prm.set_value(status);
	return prm.get_future();
}

#define CHECK_CALL(fn) (esp_err_t)(fn)
#define CHECK_CALL_RET(fn) { esp_err_t ret = (fn); if (ret != ESP_OK) return ret; } 
#define UNTIL_FIRST_ERROR(checkBlock, capture...) ( [capture]() -> esp_err_t {checkBlock; return ESP_OK;}() )


