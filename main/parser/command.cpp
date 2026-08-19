#include "command.h"


#include "util.h"

#include <cstring>
#include <iterator>

namespace parser {

	using namespace std::literals;

	template<typename T>
	void command::tokenizer(T begin, T end, char prefix) {

		_tokens = tokens_type(3);
		_errors.clear();

		if (*begin != prefix || std::distance(begin, end) == 1) { //case of "bla bla" and "+"
			_tokens.emplace_back(
				token::type_e::SYMBOL,
				DEFAULT_PRESS_TYPE,
				std::string_view(begin, end)
			);
			return;
		}

		//T beginOfToken;

		auto beginOfBlock = std::find(begin, end, prefix);
		while (beginOfBlock != end) {
			auto endingOfBlock = std::find(++beginOfBlock, end, prefix);

			auto endingOfBlockNext = endingOfBlock+1; //hungry mode impl
			while (endingOfBlockNext != end) {
				if (*endingOfBlockNext == prefix) {
					++endingOfBlockNext;
				} else {
					break;
				}
			}
			endingOfBlock = endingOfBlockNext-1;

			if (endingOfBlock == end) {
				//case when there are leading * but not ending *
				//so ++beginOfBlock will-be start of symbol and endingOfBlock will-be end IF distance > 0
				if (std::distance(beginOfBlock, endingOfBlock) > 0) { //there ending is out of block ie end
					_tokens.emplace_back(
						token::type_e::SYMBOL,
						DEFAULT_PRESS_TYPE,
						std::string_view(beginOfBlock, endingOfBlock)
					);
				}
			} else if (std::distance(beginOfBlock, endingOfBlock) > 0) {
				//if both starting and ending * exist and distance > 0 (distance dont include last) (value be "someValue*" )
				if (auto pressSeparatorPos = std::find(beginOfBlock, endingOfBlock, ':'); pressSeparatorPos == endingOfBlock) {
					_tokens.emplace_back(
						token::type_e::KEY,
						DEFAULT_PRESS_TYPE,
						std::string_view{beginOfBlock, endingOfBlock}
					);
				} else {
					if (std::find(pressSeparatorPos+1, endingOfBlock, ':') != endingOfBlock) {
						//only one of : allow to be in decl
						_errors.emplace_back(
							token::type_e::ERROR,
							DEFAULT_PRESS_TYPE,
							std::string_view(beginOfBlock, endingOfBlock)
						);
					} else {
						_tokens.emplace_back(
							token::type_e::KEY,
							std::string_view(pressSeparatorPos+1, endingOfBlock),
							std::string_view(beginOfBlock, pressSeparatorPos)
						);
					}
				}
			} else {
				error("parsers::command::tokenizer undefined op");
			}
			beginOfBlock = endingOfBlock;
		}
	}

	bool command::parse(const std::string& message) {
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}

	bool command::parse(const std::string_view& message) {
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}

	void command::cleanup() {
		_tokens = tokens_type();
		_errors = tokens_type();
	}

}