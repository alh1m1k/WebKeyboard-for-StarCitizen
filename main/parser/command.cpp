#include "command.h"


#include "util.h"
#include <iterator>

namespace parser {

	using namespace std::literals;

	command::token::press_e command::view2press(const std::string_view candidate) noexcept  {
		//debug("command::view2press", candidate);
		if (candidate == "press"sv) {
			return command::token::press_e::PRESS;
		}
		if (candidate == "long"sv) {
			return command::token::press_e::LONGPRESS;
		}
		if (candidate == "double"sv) {
			return command::token::press_e::DOUBLETAP;
		}
		if (candidate == "down"sv) {
			return command::token::press_e::DOWN;
		}
		if (candidate == "up"sv) {
			return command::token::press_e::UP;
		}
		if (candidate == "manual"sv) {
			return command::token::press_e::MANUAL;
		}
		return command::token::press_e::INVALID;
	}

	template<typename T>
	bool command::tokenizer(T begin, T end, char prefix) {

		if (*begin != prefix || std::distance(begin, end) == 1) { //case of "bla bla" and "+"
			_tokens.emplace_back(command::token::type_e::SYMBOL, command::DEFAULT_PRESS_TYPE,  std::string_view(begin, end));
			return true;
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
					_tokens.emplace_back(token::type_e::SYMBOL, DEFAULT_PRESS_TYPE, std::string_view(beginOfBlock, endingOfBlock));
				}
			} else if (std::distance(beginOfBlock, endingOfBlock) > 0) {
				//if both starting and ending * exist and distance > 0 (distance dont include last) (value be "someValue*" )
				if (auto pressSeparatorPos = std::find(beginOfBlock, endingOfBlock, ':'); pressSeparatorPos == endingOfBlock) {
					_tokens.emplace_back(token::type_e::KEY, DEFAULT_PRESS_TYPE, std::string_view{beginOfBlock, endingOfBlock});
				} else {
					if (std::find(pressSeparatorPos+1, endingOfBlock, ':') != endingOfBlock) {
						//only one of : allow to be in decl
						_errors.emplace_back(token::type_e::ERROR, DEFAULT_PRESS_TYPE, std::string_view(beginOfBlock, endingOfBlock));
					} else {
						auto pressTypeView = std::string_view(pressSeparatorPos+1, endingOfBlock);
						if (pressTypeView == "symbols"sv || pressTypeView == "symbols-press"sv) {
							_tokens.emplace_back(token::type_e::SYMBOL, DEFAULT_PRESS_TYPE, std::string_view(beginOfBlock, pressSeparatorPos));
						} else if (pressTypeView == "symbols-short"sv) {
							_tokens.emplace_back(token::type_e::SYMBOL, token::press_e::SHORT, std::string_view(beginOfBlock, pressSeparatorPos));
						} else if (auto press = view2press(pressTypeView); press == token::press_e::INVALID) {
							_errors.emplace_back(token::type_e::ERROR, token::press_e::INVALID, std::string_view(pressSeparatorPos, endingOfBlock));
						} else {
							_tokens.emplace_back(token::type_e::KEY, press, std::string_view(beginOfBlock, pressSeparatorPos));
						}
					}
				}
			} else {
				error("parsers::command::tokenizer undefined op");
			}
			beginOfBlock = endingOfBlock;
		}

		return true;
	}

	bool command::parse(const std::string& message) {
		_tokens.clear();
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}

	bool command::parse(const std::string_view& message) {
		_tokens.clear();
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}

}