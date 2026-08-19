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
				ERROR
			};
			const type_e type = {};
			const std::string_view modifierView = {};
			const std::string_view dataView = {};
		};

		std::vector<token> _tokens;
		std::vector<token> _errors;
		
		template<typename T>
		void tokenizer(T begin, T end, char prefix);

		public:

			using token_type = token;
			typedef std::vector<token> tokens_type;

			static constexpr auto DEFAULT_PRESS_TYPE = std::string_view("press");
								
			command() = default;
						
			bool parse(const std::string& message);
			
			bool parse(const std::string_view& message);

			void cleanup();

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