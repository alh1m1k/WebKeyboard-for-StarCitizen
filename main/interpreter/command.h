#pragma once

#include "parser/command.h"
#include "context.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"


namespace interpreter {

	class command {

		keyboard keyboardInterpreter = {};
		joystick joystickInterpreter = {};
		mouse mouseInterpreter = {};


		static bool isJoystickPrefix(std::string_view word);

		static bool isMousePrefix(std::string_view word);

		bool pushTo(
			hid::composite::combination_writer_type& combination,
			const parser::command::token_type& word,
			context_type& ctx
		);

		bool typingTo(
			hid::composite::sequence_writer_type& sequence,
			const parser::command::token_type& word,
			context_type& ctx
		);


		public:

			using result_type = hid::composite::sequence_writer_type::write_result_type;
			using word_type = parser::command::token_type;
			using sentence_type = parser::command::tokens_type;

			explicit command() noexcept;

			result_type executeOn(hid::composite& dev, const sentence_type& sentence);
	};

}


