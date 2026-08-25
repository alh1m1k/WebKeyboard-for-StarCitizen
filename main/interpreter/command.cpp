#include "command.h"

namespace interpreter {

	using namespace std::literals;

	static hid::pressType conv(const std::string_view& candidate) noexcept {
		if (candidate.empty()) { return hid::pressType::UNSPECIFIED; }
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
		if (candidate == "down"sv) {
			return hid::pressType::DOWN;
		}
		if (candidate == "up"sv) {
			return hid::pressType::UP;
		}
		if (candidate == "symbols"sv || candidate == "symbols-press"sv) {
			return hid::pressType::PRESS;
		}
		if (candidate == "symbols-short"sv) {
			return hid::pressType::SHORT;
		}
		return hid::pressType::INVALID;
	}

	inline static hid::pressType restrict(const hid::pressType candidate, const hid::pressType defaultValue = hid::pressType::PRESS) noexcept {
		if (candidate == hid::pressType::UNSPECIFIED || candidate == hid::pressType::INVALID) {
			return defaultValue;
		}
		return candidate;
	}

	inline static void logWord(
		const bool status,
		const parser::command::token_type& word,
		const context_type& ctx
	) noexcept {
		_log(
			"\"", word.dataView, ":", word.modifierView.empty() ? "unspecified" : word.modifierView,
			word.type == command::word_type::type_e::SYMBOL ? "\"s" : "\"k"
		);
		if (!status) {
			_log("<-error");
			if (!ctx.details.empty()) { _log('(', ctx.details, ')'); }
		}
		_log(ctx.sentence.size() == ctx.wordIndex + 1 ? "" : ", ");
	}

#define logWordIf(enable, status, word, ctx) ({ if constexpr (enable) { logWord(status, word, ctx); }})

	command::command() noexcept = default;

	command::exec_result_type command::executeOn(hid::composite& dev, const sentence_type& sentence) {
		auto sequence = dev.sequence();
		bool success = true, keyContainText = false;

		logIf(true, "[info] ", "performed tokens -> [");
		const auto total = sentence.size();
		for (auto i = 0; i < total && success; ++i) {
			if (sentence[i].type == word_type::type_e::KEY) {
				//after first key word in sentence start new combination
				//and consume any following key word
				context_type ctx = { .sentence = sentence, .wordIndex = i };
				for (
					auto combination = sequence.combination();
					i < total && success; ++i
				) {
					if (sentence[i].type == word_type::type_e::SYMBOL) {
						/* case of +alt+f +ctrl+z whe last symbol at end of sentence
						 * was a part of combination this is a symbol with key semantic
						 */
						if (i == total - 1 && sentence[i].dataView.size() == 1) {
							success = pushTo(combination, sentence[i], ctx);
							logWordIf(true, success, sentence[i], ctx);
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
						//combination is complete now
						keyContainText = true;
						break;
					} else if (sentence[i].dataView == "$$" || sentence[i].dataView == ">>") {
						//separator symbol, combination is complete now
						break;
					} else {
						//just generic key
						success = pushTo(combination, sentence[i], ctx);
						logWordIf(true, success, sentence[i], ctx);
					}
				}
			}
			//at this point combination cursor was ended ant combination flushed to sequence
			if (i >= total || !success) { break; } // boundary guardian todo: try remove it
			context_type ctx = { .sentence = sentence, .wordIndex = i,
				.pt = hid::pressType::UNSPECIFIED
			};
			if (sentence[i].type == word_type::type_e::SYMBOL || keyContainText) {
				//this is a symbol or symbols or key that containing symbols
				success = typingTo(sequence, sentence[i], ctx);
				keyContainText = false;
			}
			logWordIf(true, success, sentence[i], ctx);
		}
		logIf(true, success ? "]\n" : " ...]\n");

		if (success) {
			return sequence.lastWriteResult();
		} else {
			return ESP_FAIL;
		}
	}

	bool command::pushTo(
		hid::composite::combination_writer_type& combination,
		const parser::command::token_type& word,
		context_type& ctx
	) {
		if (ctx.pt == hid::pressType::UNSPECIFIED) {
			if (
				const auto currPt = conv(word.modifierView);
				currPt != hid::pressType::UNSPECIFIED
			) {
				if (currPt == hid::pressType::INVALID) {
					ctx.details = "invalid modifier";
					return false;
				}
				combination.changePressType(ctx.pt = currPt);
			}
		} else if (conv(word.modifierView) != hid::pressType::UNSPECIFIED) {
			ctx.details = "multiple modifier";
			return false;
		}
		if (isJoystickPrefix(word.dataView)) {
			if constexpr (
				hid::composite::joystick0_included_type::value ||
				hid::composite::joystick1_included_type::value
			) {
				return joystickInterpreter.executeOn(combination, word, ctx);
			} else {
				return false;
			}
		}
		if (isMousePrefix(word.dataView)) {
			if constexpr (hid::composite::mouse_included_type::value) {
				return mouseInterpreter.executeOn(combination, word, ctx);
			}
			return false;
		}
		if constexpr (hid::composite::keyboard_included_type::value) {
			return keyboardInterpreter.executeOn(combination, word, ctx);
		}
		ctx.details = "device disabled";
		return false;
	}

	bool command::typingTo(
		hid::composite::sequence_writer_type& sequence,
		const parser::command::token_type& word,
		context_type& ctx
	) {
		if (
			ctx.pt = conv(word.modifierView);
			!(ctx.pt == hid::pressType::SHORT || ctx.pt == hid::pressType::PRESS || ctx.pt == hid::pressType::UNSPECIFIED)
		) {
			ctx.details = "incorrect modifier";
			return false;
		}

		if constexpr (hid::composite::keyboard_included_type::value) {
			return sequence.keyboardTyping(
				word.dataView,
				ctx.pt == hid::pressType::SHORT
			);
		}
		ctx.details = "device disabled";
		return false;
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