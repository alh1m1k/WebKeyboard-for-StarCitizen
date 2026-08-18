#pragma once

#include "hid/composite.h"
#include "parser/command.h"



namespace interpreter {

	class mouse {

		public:

			typedef parser::command::token_type word_type;
			typedef parser::command::tokens_type sentence_type;

			bool executeOn(
				hid::composite::combination_writer_type& combination,
				const word_type& word
			);
	};

}