#include "command.h"

namespace interpreter {

	inline hid::pressType conv(parser::command::token_type::press_e in) noexcept {
		if (in >= parser::command::token_type::press_e::INVALID) {
			return hid::pressType::PRESS;
		}
		return static_cast<hid::pressType>(in);
	}

	command::command() noexcept = default;

	command::exec_result_type command::executeOn(hid::composite& dev, const sentence_type& sentence) {
		auto sequence = dev.sequence();
		bool success = true;

		logIf(true, "[info] ", "performed tokens -> [");
		const auto total = sentence.size();
		for (auto i = 0; i < total && success; ++i) {
			if (sentence[i].type == word_type::type_e::KEY) {
				for (
					auto combination = sequence.combination(conv(sentence[i].press));
					i < total; ++i
				) {
					/* case of +alt+f  +ctrl+z whe last symbol at end of sentence
					*  was part of combination
					*/
					if (sentence[i].type == word_type::type_e::SYMBOL) {
						if (i == total - 1 && sentence[i].view.size() == 1) {
							success = delegate(combination, sentence[i]);
						} else {
							break;
						}
					} else if (sentence[i].view == "$$" || sentence[i].view == ">>") {
						logIf(true, "\"", sentence[i].view, ":", (int)sentence[i].press,
							"\"k<-separator", total == i + 1 ? "" : ", "
						);
						break;
					} else {
						success = delegate(combination, sentence[i]);
					}
					logIf(true, "\"", sentence[i].view, ":", (int)sentence[i].press,
						"\"k", !success ? "<-error" : "", total == i + 1 ? "" : ", "
					);
				}
			}
			if (sentence[i].type == word_type::type_e::SYMBOL) {
				success = sequence.keyboardTyping(sentence[i].view, sentence[i].press == word_type::press_e::SHORT);
				logIf(true, "\"", sentence[i].view, ":", (int)sentence[i].press,
					"\"s", !success ? "<-error" : "", total == i + 1 ? "" : ", "
				);
			}
		}
		logIf(true, "]\n");

		if (success) {
			return sequence.lastWriteResult();
		} else {
			return ESP_FAIL;
		}
	}

	bool command::delegate(hid::composite::combination_writer_type& combination, const parser::command::token_type& word) {
		if (isJoystickPrefix(word.view)) {
			return joystickInterpreter.executeOn(combination, word);
		}
		if (isMousePrefix(word.view)) {
			return mouseInterpreter.executeOn(combination, word);
		}
		return keyboardInterpreter.executeOn(combination, word);
	}

	bool command::isJoystickPrefix(const std::string_view word) {
		if (word.starts_with("js")) {
			if (word[2] == '-') {
				return true;
			} else if ((int)'0' <= word[2] && (int)'9' >= word[2]) {
				return word[3] == '-';
			}
		}
		return false;
	}

	bool command::isMousePrefix(const std::string_view word) {
		return word.starts_with("mouse-");
	}

}