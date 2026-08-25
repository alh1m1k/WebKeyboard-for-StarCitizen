#pragma once

#include "parser/command.h"
#include "hid/pressType.h"

namespace interpreter {

	struct context_type {
		const parser::command::tokens_type& sentence;
		const int& wordIndex;
		hid::pressType pt = hid::pressType::UNSPECIFIED;
		std::string_view details = {};
	};

}