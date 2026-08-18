#pragma once

#include "generated.h"

#include "hid/composite.h"
#include "parser/command.h"

namespace interpreter {

	class composite {

		bool specialKey(hid::composite::combination_writer_type& kb, const std::string_view& str, const parser::command::token_type& tock);

		public:

			typedef parser::command::token_type word_type;
			typedef parser::command::tokens_type sentence_type;

			bool executeOn(hid::composite::combination_writer_type& combination, const word_type& word);
	};

}

