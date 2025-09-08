#pragma once

#include "keyboard.h"

#include "result.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include <sys/_stdint.h>
#include <sys/types.h>

#include "task.h"
#include "util.h"



namespace hid {
	
	template<class BACK_PUSHER>
	class writer {
		
		public: typedef result<uint32_t> write_result;

		protected:
		
			BACK_PUSHER& backend;
			
			write_result lastResult = ESP_OK;
			
			uint8_t lastModifier = 0; 
						
			bool writeSymbol(const char symbol, uint8_t modifier, const uint8_t type) {
												
				if (dictionay[(uint8_t)symbol][0]) {
					modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
				}
				
				uint8_t charCode = dictionay[(uint8_t)symbol][1];
				
				return write(charCode, modifier, type);
			}
					
			bool write(const uint8_t charCode, uint8_t modifier, const uint8_t type) {
								
				task kbKey = {(uint8_t)task::suffix::FORMAT_KB_KEY, modifier, charCode, type};

				if (charCode == 0) {
					error("shortcutWriter::write check flow value is zero");
				}

				if (lastResult = backend->push_back(kbKey); lastResult) {
					return true;
				}
				
				return false;
			}
			
		public:
		
			static constexpr char dictionay[128][2] = {HID_ASCII_TO_KEYCODE}; 
				
			writer(BACK_PUSHER& bp, uint8_t modifier): backend(bp), lastModifier(modifier) {}
			
			inline auto& modify(const int code) {
				lastModifier |= (int)code;
				return *this;
			}
			
			uint8_t& modifyer() {
				return lastModifier;
			}
					
			bool symbol(const char symbol, task::pressType press = task::pressType::PRESS) {
				bool ret = writeSymbol(symbol, lastModifier, (uint8_t)press);
				lastModifier = 0;
				return ret;
			}
			
			bool special(const uint8_t symbol, task::pressType press = task::pressType::PRESS) {
				bool ret = write(symbol, lastModifier, (uint8_t)press);	
				lastModifier = 0;
				return ret;
			}
			
			write_result lastWriteResult() {
				//todo fix me
				return lastResult;  //what if now walid write happend
			}
			
	};
}