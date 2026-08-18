#pragma once 

#include "generated.h"

#include <string>
#include <vector>

namespace parser {
		
	class command {

		struct token {
			enum class type_e: uint8_t {
				SYMBOL = 0x01,
				KEY,
				JOYSTICK_BTN,
				MOUSE_BTN,
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
			const type_e type;
			const press_e press;
			const std::string_view view;
		};

		/**
		 *
		 */
		std::vector<token> _tokens;
		std::vector<token> _errors;
		
		template<typename T>
		bool tokenizer(T begin, T end, char prefix);
		
		static token::press_e view2press(std::string_view candidate) noexcept;

		public:

			using token_type = token;
			typedef std::vector<token> tokens_type;

			static constexpr auto DEFAULT_PRESS_TYPE = token::press_e::PRESS;
								
			command() = default;
						
			bool parse(const std::string& message);
			
			bool parse(const std::string_view& message);
			
			[[nodiscard]] inline const tokens_type& tokens() const noexcept {
				return _tokens;
			}
			
			[[nodiscard]] inline bool hasErrors() const {
				return !_errors.empty();
			}

			[[nodiscard]] inline const tokens_type& errors() const {
				return _errors;
			}

	};

}