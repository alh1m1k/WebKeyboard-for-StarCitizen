#pragma once

#include <sys/_stdint.h>
#include <sys/types.h>
#include <optional>

#include "generated.h"

#include "class/hid/hid.h"
#include "esp_err.h"
#include "task.h"
#include "util.h"
#include "writer.h"
#include "result.h"



namespace hid {

	template<class BACK_PUSHER>
	class shortcutWriter {
		
		public: typedef typename writer<BACK_PUSHER>::write_result write_result;
		
		protected:
		
			BACK_PUSHER& 						backend;
			write_result						flushResult = (esp_err_t)errors::INCOMPLETE;
			write_result* const					optOutResult; //const pointer to write_result
			uint8_t 							modifier;
			task::pressType 					pt;
			
		
			std::vector<uint8_t> buffer;
		
			bool write(const uint8_t charCode) {
				
				if (buffer.size() >= 6) {
					flushResult = (esp_err_t)errors::OVERFLOW;
					return false;
				}
				
				if (uint8_t mdf = extractModifier(charCode); mdf != 0) {
					modifier |= mdf;
					return true;
				}
				
/*				if (charCode == 0) {
					error("shortcutWriter::write check flow value is zero");
				}*/

				buffer.push_back(charCode);
				
				return true;
			}
									
			uint8_t extractModifier(const uint8_t charCode) {
				switch (charCode) {
					case HID_KEY_SHIFT_LEFT:    
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTSHIFT;              
					case HID_KEY_ALT_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTALT;                  
					case HID_KEY_GUI_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTGUI;    
					case HID_KEY_CONTROL_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTCTRL;                 
					case HID_KEY_CONTROL_RIGHT:   
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTCTRL;            
					case HID_KEY_SHIFT_RIGHT:  
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTSHIFT;                
					case HID_KEY_ALT_RIGHT: 
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTALT;                 
					case HID_KEY_GUI_RIGHT: 
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTGUI;  
				}
				return 0;
			}
			
			//todo try deque
			void flush() {
				debugIf(LOG_KEYBOARD, "begin flush", buffer.size(), " ", (uint)modifier);
				
				if (buffer.size() == 0 || flushResult.code() != (esp_err_t)errors::INCOMPLETE) {
					return;
				}
					
				if (buffer.size() == 1) {
					debugIf(LOG_KEYBOARD, "flush pack1", (uint)buffer[0]);
					flushResult = backend->push_back({(uint8_t)task::suffix::FORMAT_KB_KEY, modifier, buffer[0], (uint8_t)pt});
				} else if (buffer.size() == 2 && pt == task::pressType::PRESS) {
					debugIf(LOG_KEYBOARD, "flush pack2", (uint)buffer[0], " ", (uint)buffer[1]);
					flushResult = backend->push_back({(uint8_t)task::suffix::FORMAT_KB_DEFAULT_PRESS|(uint8_t)task::flag::COMBINATION_BEGIN|(uint8_t)task::flag::COMBINATION_END, modifier, buffer[0], buffer[1]});
				} else {
					debugIf(LOG_KEYBOARD, "flush mul", (uint)buffer[0], " ", (uint)buffer[1], " ", (uint)buffer[2], " ", (uint)buffer[3], " ", (uint)buffer[4], " ", (uint)buffer[5]);
					if (flushResult = backend->push_back({(uint8_t)task::suffix::FORMAT_KB_DEFAULT_PRESS|(uint8_t)task::flag::COMBINATION_BEGIN, modifier, buffer[0], buffer[1]}); flushResult) {
						task kbKey = {(uint32_t)0};
						int sended = 2;
						uint8_t index = 0;
						while(sended < buffer.size() && index < 3) {
							kbKey[index++] = buffer[sended++];
						}
						if (index == 3) {
							kbKey.flags() = (uint8_t)task::suffix::FORMAT_KB_KEYS;
							if (flushResult = backend->push_back(kbKey); flushResult) {
								auto last = buffer.size() > 5 ? buffer[5] : (uint8_t)0;
								flushResult = backend->push_back(
									{(uint8_t)task::suffix::FORMAT_KB_ENDING_PRESS|(uint8_t)task::flag::COMBINATION_END, last, (uint8_t)0, (uint8_t)pt}
								);
							}
						} else {
							kbKey.flags() = (uint8_t)task::suffix::FORMAT_KB_ENDING_PRESS|(uint8_t)task::flag::COMBINATION_END;
							kbKey[2] = (uint8_t)pt;
							flushResult = backend->push_back(kbKey);
						}
					}
				}
				buffer.clear();
			}
		
		public:
		
			enum class errors: esp_err_t {	
				INCOMPLETE = -100000,
				OVERFLOW,
			};
		
			typedef result<uint32_t> task_id_type;
					
			shortcutWriter(BACK_PUSHER& backend, task::pressType press = task::pressType::PRESS, uint8_t modifier = 0, write_result* const outResult = nullptr) 
				: backend(backend), optOutResult(outResult), modifier(modifier), pt(press)
			{
				buffer.reserve(6);
			}
			
			~shortcutWriter() {
				if (buffer.size()) {
					flush();
					if (flushResult.code() != (esp_err_t)errors::INCOMPLETE && optOutResult != nullptr) {
						*optOutResult = flushResult;
					}
				}
			}
			
			auto& modify(const uint8_t mdf) {
				modifier |= mdf;
				return *this;
			}
						
			auto& symbol(const char symbol) {
				uint8_t charCode = writer<BACK_PUSHER>::dictionay[(uint8_t)symbol][1];
				write(charCode);
				return *this;
			}
		
			auto& special(const uint8_t charCode) {
				write(charCode);
				return *this;
			}
			
			auto& done() {
				flush();
				return *this;
			}
			
			operator bool() {
				return !!backend;
			}
			
			write_result lastWriteResult() {
				return flushResult;
			}
		
	};

}