#pragma once

#include "hid/composite.h"
#include "parser/command.h"

#include <utility>

namespace interpreter {

	class joystick {

		struct result_type {
			int joystickId;
			int axisIndex;
			int value;
		};

		static result_type view2AxisValue(const std::string_view word) {
			/**
			 * axis values [0-+2048] constant low, high, middle [0, 2048, 1024]
			 * js-axis[0-+7]([0-+2048]) or js[n]-axis[0-+7]([0-+2048])
			 * ie js-axis0(255),js-axis6(high) or js1-axis1(0), js0-axis5(middle)
			 */

			std::string_view substr = {};
			int joystickId = 0, axisIndex = -1;
			if (word.starts_with("js-axis")) {
				substr = word.substr(7);
			} else if (word.size() >= 8 && word.starts_with("js") && word.substr(3).starts_with("-axis")) {
				substr = word.substr(8);
				joystickId = word[2] - '0';
				joystickId = joystickId >= 0 && joystickId <= 1 ? joystickId : -1;
			}

			if (substr.size() >= 2 && substr[1] == '(') {
				axisIndex = substr[0] - '0';
				axisIndex = axisIndex >= 0 && axisIndex <= 7 ? axisIndex : -1;
				substr = substr.substr(2);
			} else {
				return {-1, -1, -1};
			}

			if (const auto ending = substr.find(')'); ending != std::string_view::npos) {
				if (
					substr = trim(substr.substr(0, ending));
					!substr.empty() && joystickId != -1 && axisIndex != -1)
				{
					if (substr == "low")    { return {joystickId, axisIndex, 0}; }
					if (substr == "middle") { return {joystickId, axisIndex, 1024}; }
					if (substr == "high")   { return {joystickId, axisIndex, 2048}; }
					int value;
					if (std::from_chars(substr.begin(), substr.end(), value).ec == std::errc{}) {
						return {joystickId, axisIndex,  value >= 0 && value <= 2048 ? value : -1};
					}
				}
			}

			return {-1, -1, -1};
		}

		struct joystick_button_type {
			int joystickId;
			int buttonIndex;
		};

		static joystick_button_type view2BtnIndex(const std::string_view word) {
			/**
			 * js-b[0-+31] or js[n]-b[0-+31] ie js-b1,js-b30 or js1-b1, js0-b15
			 */

			std::string_view substr = {};
			int joystickId = 0;
			if (word.starts_with("js-b")) {
				substr = word.substr(4);
			} else if (word.size() >= 5 && word.starts_with("js") && word.substr(3).starts_with("-b")) {
				substr = word.substr(5);
				joystickId = word[2] - '0';
				joystickId = joystickId >= 0 && joystickId <= 1 ? joystickId : -1;
			}

			if (!substr.empty() && joystickId != -1) {
				int index = 0;
				if (std::from_chars(substr.begin(), substr.end(), index).ec == std::errc{}) {
					return {joystickId, index <= 31 ? index : -1} ;
				}
			}

			return {-1, -1};
		}

		public:

			using word_type = parser::command::token_type;
			using sentence_type = parser::command::tokens_type;

			bool executeOn(
				hid::composite::combination_writer_type& combination, const word_type& word
			) {
				if (const auto [joystickId, btnIndex] = view2BtnIndex(word.view); btnIndex != -1) {
					combination.joystickButtons(0x01 << btnIndex, joystickId);
					return true;
				}
				if (const auto [joystickId, axisIndex, value] = view2AxisValue(word.view); value != -1) {
					combination.joystickAxis((uint8_t)axisIndex, (int16_t)value, (uint8_t)joystickId);
					return true;
				}

				return false;
			}
	};

}