#include "mouse.h"

#include <utility>

namespace interpreter {

	using namespace std::literals;

	static int view2BtnIndex(const std::string_view word) {
		/**
		 * mouse-b[0-+10]
		 */
		constexpr auto offset = std::string_view("mouse-b").size();
		if (word.size() >= offset) {
			if (auto substr = word.substr(offset); !substr.empty()) {
				int index = 0;
				if (std::from_chars(substr.begin(), substr.end(), index).ec == std::errc{}) {
					return index <= MOUSE_BUTTON_FORWARD ? index : -1;
				}
			}
		}
		return -1;
	}

	static int view2WheelSpeed(const std::string_view word) {
		/**
		 * mouse-wheel([-128-+127])
		 */
		constexpr auto offset = std::string_view("mouse-wheel(").size();
		if (auto ending = word.find(')', offset); ending != std::string_view::npos) {
			if (auto substr = word.substr(offset,  ending - offset); !substr.empty()) {
				int value = 0;
				if (std::from_chars(substr.begin(), substr.end(), value).ec == std::errc{}) {
					return std::in_range<int8_t>(value) ? value : -10000;
				}
			}
		}
		return -1;
	}

	struct pos {
		int16_t x,y;
	};

	static pos view2XYPosition(const std::string_view word) {
		/**
		 * mouse-pos([0-+32767],[0-+32767])
		 */
		constexpr auto offset = std::string_view("mouse-pos(").size();
		if (const auto ending = word.find(')', offset); ending != std::string_view::npos) {
			if (auto substr = word.substr(offset,  ending - offset); !substr.empty()) {
				if (const auto sep = substr.find(','); ending != std::string_view::npos) {
					int16_t posX = -1, posY = -1;
					auto posStr = trim(substr.substr(0, sep));
					if (std::from_chars(posStr.begin(), posStr.end(), posX).ec == std::errc{}) {
						posX = posX >= 0 ? posX : -1;
					}
					posStr = trim(substr.substr(sep + 1));
					if (std::from_chars(posStr.begin(), posStr.end(), posY).ec == std::errc{}) {
						posY = posY >= 0 ? posY : -1;
					}
					return {posX, posY};
				}

			}
		}
		return {-1, -1};
	}

	bool mouse::executeOn(
		hid::composite::combination_writer_type& combination,
		const word_type& word,
		context_type& ctx
	) {

		if (word.dataView.starts_with("mouse-b")) {
			if (const auto index = view2BtnIndex(word.dataView); index != -1) {
				combination.mouseButtons(1 << index);
				return true;
			}
		} else if (word.dataView.starts_with("mouse-wheel(")) {
			if (const auto speed = view2WheelSpeed(word.dataView); speed != -10000) {
				combination.mouseScroll(0, (int8_t)speed);
				return true;
			}
		} else if (word.dataView.starts_with("mouse-pos(")) {
			if (const auto [x, y] = view2XYPosition(word.dataView); x != -1 && y != -1) {
				combination.mousePosition(x, y);
				return true;
			}
		} else if (word.dataView == "mouse-left"sv) {
			combination.mouseButtons(MOUSE_BUTTON_LEFT);
			return true;
		} else if (word.dataView == "mouse-right"sv) {
			combination.mouseButtons(MOUSE_BUTTON_RIGHT);
			return true;
		} else if (word.dataView == "mouse-middle"sv) {
			combination.mouseButtons(MOUSE_BUTTON_MIDDLE);
			return true;
		} else if (word.dataView == "mouse-backward"sv) {
			combination.mouseButtons(MOUSE_BUTTON_BACKWARD);
			return true;
		} else if (word.dataView == "mouse-forward"sv) {
			combination.mouseButtons(MOUSE_BUTTON_FORWARD);
			return true;
		}

		return false;
	}

} // namespace interpreter