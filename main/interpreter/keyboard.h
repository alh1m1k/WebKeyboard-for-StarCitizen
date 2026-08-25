#pragma once

#include "context.h"
#include "generated.h"

#include "hid/composite.h"
#include "parser/command.h"

namespace interpreter {

	class keyboard {

		bool specialKey(
			hid::composite::combination_writer_type& combination,
			const std::string_view& str,
			const parser::command::token_type& tock
		);

		public:

			typedef parser::command::token_type word_type;
			typedef parser::command::tokens_type sentence_type;

			bool executeOn(
				hid::composite::combination_writer_type& combination,
				const word_type& word,
				context_type& ctx
			);
	};

}

