#pragma once 

#include "generated.h"

#include <memory>
#include <algorithm>
#include <string>
#include <optional>
#include <sys/_stdint.h>
#include <vector>
		
#include "../exception/not_implemented.h"
#include "keyboardTask.h"
#include "result.h"

//#define __TEST_ONLY_PURE_PARESER

#ifndef __TEST_ONLY_PURE_PARESER
#include "../hid/keyboard.h"
#endif

namespace parsers {
		
	class keyboard {
		
		public: struct token {
			enum class type_e: uint8_t {
				SYMBOL = 0x01,
				KEY,
				SEQUENCE,
				SEQUENCE_END,
				ERROR
			};
			enum class press_e: uint8_t {
				PRESS = 0x01,
				LONGPRESS,
				DOUBLETAP,
				SHORT,
				DOWN,
				UP,
				MANUAL,
				DELAY,
				OTHER,
				INVALID
			};
			const type_e			type; //0 symbol 1 key
			const press_e           press;
			const std::string_view 	view;
		};
				
		private:
						
		std::vector<token> _tokens;
		std::vector<token> _errors;
		
		template<typename T>
		bool tokenizer(T begin, T end, char prefix);
		
		token::press_e view2press(std::string_view candidate);
		
#ifndef __TEST_ONLY_PURE_PARESER	
		bool specialKey(hid::keyboard::sequence_writer_type& kb, const std::string_view& str, const token& tock);
#endif
		
		public:
		
			typedef hid::keyboard::sequence_writer_type::write_result write_result;
		
			const static auto DEFAULT_PRESS_TYPE = keyboard::token::press_e::PRESS;
								
			keyboard();
						
			bool parse(const std::string& message);
			
			bool parse(const std::string_view& message);
			
			inline const std::vector<token>& tokens() const {
				return _tokens;
			}
			
			inline bool hasErrors() const {
				return _errors.size();
			}
			
			inline const std::vector<token>& errors() const {
				return _errors;
			}
					
#ifndef __TEST_ONLY_PURE_PARESER	
			write_result writeTo(hid::keyboard& kb);
#endif
	};

}