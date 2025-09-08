#pragma once

#include <sys/_stdint.h>

namespace hid {

	struct task {
		
		uint8_t storage[4];
		
		public:
		
			enum class pressType: uint8_t {
				PRESS = 0x01,
				LONGPRESS,
				DOUBLETAP,
				SHORT,
				DOWN,
				UP,		
				INVALID
			};
			
			enum class flag: uint8_t {
				COMBINATION_BEGIN =	(1UL << (7)),
				COMBINATION_END   =	(1UL << (6)),
				
				MASK 			  = COMBINATION_BEGIN|COMBINATION_END
			};
			
			enum class suffix: uint8_t {
				FORMAT_KB_KEY 				= 0x00,	//{flag, modifiers, key, press}
				FORMAT_KB_DEFAULT_PRESS,			//{flag, modifiers, key, key2}
				FORMAT_KB_ENDING_PRESS,				//{flag, key, key2, press}
				FORMAT_KB_KEYS,						//{flag, key, key2, key3}
				SPECIAL_MEANING				= 0xFF&(~(uint8_t)flag::MASK), //max avl flag
			};
									
		    template <typename... T> 
		    task(uint8_t flags, T... ts) noexcept : storage{flags, ts...} {  } 
		    
		    task(uint32_t serialized) noexcept : storage {
				(uint8_t)serialized, 
				(uint8_t)((uint32_t)serialized >> 8), 
				(uint8_t)((uint32_t)serialized >> 16), 
				(uint8_t)((uint32_t)serialized >> 24)
			} {}
			
									
			~task() noexcept {
				
			}
			
			inline uint8_t& operator[](uint8_t i) noexcept {
				return storage[i+1];
			}
			
			inline uint32_t serialize() const noexcept {
				return *(uint32_t*)&storage;
			}
			
			inline uint8_t& flags() noexcept {
				return storage[0];
			}
			
	};

}