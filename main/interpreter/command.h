#pragma once

#include "hid/composite.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "parser/command.h"

namespace interpreter {

	class command {

		composite keyboardInterpreter = {};
		joystick joystickInterpreter = {};
		mouse mouseInterpreter = {};

		static bool isJoystickPrefix(std::string_view word);

		static bool isMousePrefix(std::string_view word);

		bool pushTo(hid::composite::combination_writer_type& combination, const parser::command::token_type& word);

		public:

			typedef hid::composite::sequence_writer_type::write_result_type exec_result_type;
			typedef parser::command::token_type		word_type;
			typedef parser::command::tokens_type	sentence_type;

			explicit command() noexcept;

			exec_result_type executeOn(hid::composite& dev, const sentence_type& sentence);
	};

}


