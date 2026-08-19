#include "command.h"

namespace interpreter {

	using namespace std::literals;

	static hid::pressType conv(const std::string_view& candidate) noexcept {
		if (candidate == "press"sv) {
			return hid::pressType::PRESS;
		}
		if (candidate == "long"sv) {
			return hid::pressType::LONGPRESS;
		}
		if (candidate == "double"sv) {
			return hid::pressType::DOUBLETAP;
		}
		if (candidate == "short"sv) {
			return hid::pressType::SHORT;
		}
		if (candidate == "symbols-short"sv) {
			return hid::pressType::SHORT;
		}
		if (candidate == "down"sv) {
			return hid::pressType::DOWN;
		}
		if (candidate == "up"sv) {
			return hid::pressType::UP;
		}
		return hid::pressType::PRESS;
	}

	command::command() noexcept = default;

	command::exec_result_type command::executeOn(hid::composite& dev, const sentence_type& sentence) {
		auto sequence = dev.sequence();
		bool success = true, keyContainText = false;

		logIf(true, "[info] ", "performed tokens -> [");
		const auto total = sentence.size();
		for (auto i = 0; i < total && success; ++i) {
			if (sentence[i].type == word_type::type_e::KEY) {
				for (
					auto combination = sequence.combination(conv(sentence[i].modifierView));
					i < total && success; ++i
				) {
					if (sentence[i].type == word_type::type_e::SYMBOL) {
						/* case of +alt+f +ctrl+z whe last symbol at end of sentence
						 * was a part of combination
						 */
						if (i == total - 1 && sentence[i].dataView.size() == 1) {
							success = pushTo(combination, sentence[i]);
							logIf(true, "\"", sentence[i].dataView, ":", sentence[i].modifierView,
								"\"k", !success ? "<-error" : "", total == i + 1 ? "" : ", "
							);
						} else {
							break;
						}
					} else if (
						sentence[i].modifierView == "symbols"sv ||
						sentence[i].modifierView == "symbols-press"sv ||
						sentence[i].modifierView == "symbols-short"sv
					) {
						//this is text encoded as key ie:
						//+enter+someLongCommandOrMessage:symbols+enter+
						keyContainText = true;
						break;
					} else if (sentence[i].dataView == "$$" || sentence[i].dataView == ">>") {
						//separator symbol, combination is complete now
						break;
					} else {
						success = pushTo(combination, sentence[i]);
						logIf(true, "\"", sentence[i].dataView, ":", sentence[i].modifierView,
							"\"k", !success ? "<-error" : "", total == i + 1 ? "" : ", "
						);
					}
				}
			}
			if (i >= total) { break; }
			if (sentence[i].type == word_type::type_e::SYMBOL || keyContainText) {
				success = sequence.keyboardTyping(
					sentence[i].dataView,
					conv(sentence[i].modifierView) == hid::pressType::SHORT
				);
				keyContainText = false;
			}
			logIf(true, "\"", sentence[i].dataView, ":", sentence[i].modifierView,
				sentence[i].type == word_type::type_e::SYMBOL ? "\"s" : "\"k",
				!success ? "<-error" : "", total == i + 1 ? "" : ", "
			);
		}
		logIf(true, "]\n");

		if (success) {
			return sequence.lastWriteResult();
		} else {
			return ESP_FAIL;
		}
	}

	bool command::pushTo(hid::composite::combination_writer_type& combination, const parser::command::token_type& word) {
		if (isJoystickPrefix(word.dataView)) {
			return joystickInterpreter.executeOn(combination, word);
		}
		if (isMousePrefix(word.dataView)) {
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