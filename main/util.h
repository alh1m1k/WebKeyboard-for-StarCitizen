#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include "type_traits"

#include "esp_err.h"
#include "result.h"


inline size_t align32(size_t size) {
	if (size % 4 == 0) {
		return size;
	} else {
		return ((size / 4) + 1) * 4;
	}
}


template <typename  ...additional>
inline void _log(additional&&... args) {
	(std::cout << ... << args);
}

#define logIf(enable, ...) if constexpr (enable) _log(__VA_ARGS__)

inline void panic(const char* msg, esp_err_t code) {
	_log(msg, " #", code, "\n");
}

template <typename  ...additional>
inline void debug(const char* msg, additional&&... args) {
	_log("[debug] ", msg, " -> ", args..., "\n");
}

#define debugIf(enable, msg, ...) if constexpr (enable) debug(msg __VA_OPT__(,) __VA_ARGS__)

template <typename  ...additional>
inline void info(const char* msg, additional&&... args) {
	_log("[info] ", msg, " -> ", args..., "\n");
}

#define infoIf(enable, msg, ...) if constexpr (enable) info(msg __VA_OPT__(,) __VA_ARGS__)

template <typename  ...additional>
inline void error(const char* msg, additional&&... args) {
	_log("[error] ", msg, " -> ", args..., "\n");
}

#define errorIf(enable, msg, ...) if constexpr (enable) error(msg __VA_OPT__(,) __VA_ARGS__)



template<typename ... Args>
std::string sprintf( const std::string& format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

template<typename ... Args>
std::string sprintf( const char* format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format, args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format, args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

template<typename vector>
inline void eraseByReplace(vector& v, typename vector::iterator eraseIt) {
	auto replaceWith =  v.end() - 1;
	if (replaceWith != eraseIt) {
		*eraseIt = *replaceWith; //remove by replace
	} 
	v.erase(replaceWith);
}

inline std::string genGandom(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}

// Function to trim leading whitespace
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    }));
}

// Function to trim trailing whitespace
static inline void rtrim(std::string &s) {
    s.erase(std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base(), s.end());
}

// Function to trim both leading and trailing whitespace
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

//does not ?guarantee? FIFO
template<bool threadSafe = false>
static uint32_t genId() {
	if constexpr (threadSafe) {
		static std::atomic<uint32_t> id{0};
		return id++;
	} else {
		static uint32_t id = 0;
		return id++;
	}
}

template<typename T, typename = void>
struct has_value_type : std::false_type {};

template<typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

template<typename valueType = uint32_t, bool threadSafe = false>
class generator {
	
	using conditionaType = typename std::conditional<threadSafe, std::atomic<valueType>, valueType>::type;
	
	conditionaType value;
	
	public:
	
		generator() {}
		
		generator(valueType initialValue): value(initialValue) {}
		
		generator(generator& copy) = delete;
		
		generator(generator&& move): value(move.value) {}
		
		virtual ~generator() {}
		
		valueType inline generate() {
			return value++;
		}
		
		void inline skip(int value) {
			value += value;
		}
		
		valueType operator()() {
			return generate();
		}
};
