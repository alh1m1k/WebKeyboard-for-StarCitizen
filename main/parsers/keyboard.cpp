#include "keyboard.h"

#include "class/hid/hid.h"
#include "generated.h"
#include "task.h"
#include "util.h"
#include <iterator>
#include <sys/_stdint.h>

namespace parsers {
	
	using namespace std::literals;
		
	keyboard::token::press_e keyboard::view2press(std::string_view candidate) {
		debug("keyboard::view2press", candidate);
		if (candidate == "press"sv) {
			return keyboard::token::press_e::PRESS;
		}
		if (candidate == "long"sv) {
			return keyboard::token::press_e::LONGPRESS;
		}
		if (candidate == "double"sv) {
			return keyboard::token::press_e::DOUBLETAP;
		}
		if (candidate == "down"sv) {
			return keyboard::token::press_e::DOWN;
		}
		if (candidate == "up"sv) {
			return keyboard::token::press_e::UP;
		}
		if (candidate == "manual"sv) {
			return keyboard::token::press_e::MANUAL;
		}
		return keyboard::token::press_e::INVALID;
	}

	template<typename T>
	bool keyboard::tokenizer(T begin, T end, char prefix) {
		
		if (*begin != prefix || std::distance(begin, end) == 1) { //case of "bla bla" and "+"
			keyboard::token t = {.type = keyboard::token::type_e::SYMBOL, .press = keyboard::DEFAULT_PRESS_TYPE,  .view =  {begin, end}};
			_tokens.push_back(t);
			
			return true;
		}
			
		//T beginOfTocken;
		
		auto beginOfBlock = std::find(begin, end, prefix);
		while (beginOfBlock != end) {
			auto endingOfBlock = std::find(++beginOfBlock, end, prefix);
			
			auto endingOfBlockNext = endingOfBlock+1; //hungry mode impl
			while (endingOfBlockNext != end) {
				if (*endingOfBlockNext == prefix) {
					endingOfBlockNext++;
				} else {
					break;
				}
			}
			endingOfBlock = endingOfBlockNext-1;
			
			if (endingOfBlock == end) {
				//case when there are leading * but not ending *
				//so ++beginOfBlock will-be start of symbol and endingOfBlock will-be end IF distance > 0
				if (std::distance(beginOfBlock, endingOfBlock) > 0) { //there ending is out of block ie end
					keyboard::token t = {.type = keyboard::token::type_e::SYMBOL, .press = keyboard::DEFAULT_PRESS_TYPE, .view =  {beginOfBlock, endingOfBlock}};
					_tokens.push_back(t);
				}
			} else if (std::distance(beginOfBlock, endingOfBlock) > 0) {
				//if both starting and ending * exist and distance > 0 (distance dont include last) (value be "someValue*" )
				if (auto pressSeparatorPos = std::find(beginOfBlock, endingOfBlock, ':'); pressSeparatorPos == endingOfBlock) {
					keyboard::token t = {.type = keyboard::token::type_e::KEY, .press = keyboard::DEFAULT_PRESS_TYPE, .view =  {beginOfBlock, endingOfBlock}};
					_tokens.push_back(t);
				} else {
					if (std::find(pressSeparatorPos+1, endingOfBlock, ':') != endingOfBlock) {
						//only one of : allow to be in decl
						keyboard::token e = {.type = keyboard::token::type_e::ERROR, .press = keyboard::DEFAULT_PRESS_TYPE, .view =  {beginOfBlock, endingOfBlock}};
						_errors.push_back(e);
					} else {
						auto pressTypeView = std::string_view(pressSeparatorPos+1, endingOfBlock);
						if (pressTypeView == "symbols"sv || pressTypeView == "symbols-press"sv) {
							keyboard::token t = {.type = keyboard::token::type_e::SYMBOL, .press = keyboard::DEFAULT_PRESS_TYPE,  .view =  {beginOfBlock, pressSeparatorPos}};
							_tokens.push_back(t);
						} else if (pressTypeView == "symbols-short"sv) {
							keyboard::token t = {.type = keyboard::token::type_e::SYMBOL, .press = keyboard::token::press_e::SHORT,  .view =  {beginOfBlock, pressSeparatorPos}};
							_tokens.push_back(t);
						} else if (auto press = view2press(pressTypeView); press == keyboard::token::press_e::INVALID) {
							keyboard::token e = {.type = keyboard::token::type_e::ERROR, .press = keyboard::token::press_e::INVALID, .view =  {pressSeparatorPos, endingOfBlock}};
							_errors.push_back(e);
						} else {
							keyboard::token t = {.type = keyboard::token::type_e::KEY, .press = press, .view =  {beginOfBlock, pressSeparatorPos}};
							_tokens.push_back(t);
						}
					}
				}
			} else {
				error("parsers::keyboard::tokenizer undefined op");
			}
			beginOfBlock = endingOfBlock;
		}
		
		return true;
	}
	
hid::task::pressType conv(keyboard::token::press_e in) {
	if (in >= keyboard::token::press_e::INVALID) {
		return hid::task::pressType::PRESS;
	}
	return static_cast<hid::task::pressType>(in);
}	
	
#ifndef __TEST_ONLY_PURE_PARESER
	bool keyboard::specialKey(hid::keyboard::sequence_writer_type& kb, const std::string_view& str, const token& tock) {
		
		/* ------------------------------------------------------------------------------------- */
				
		if (str == "alt"sv || str ==  "lalt"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_LEFTALT);
			return true;
		}
		
		if (str == "ralt"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_RIGHTALT);
			return true;
		}
		
		if (str == "ctrl"sv || str ==  "lctrl"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_LEFTCTRL);
			return true;
		}
		
		if (str == "rctrl"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_RIGHTCTRL);
			return true;
		}
		
		if (str == "shift"sv || str ==  "lshift"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_LEFTSHIFT);
			return true;
		}
				
		if (str == "rshift"sv) {
			kb.modify(hid::keyboard::modifier::KEYBOARD_MODIFIER_RIGHTSHIFT);
			return true;
		}
		
		
		/* ------------------------------------------------------------------------------------- */
		/* Key variant of modifier key (workaround) */
		
		if (str == "alt-key"sv || str ==  "lalt-key"sv) {
			kb.special(HID_KEY_ALT_LEFT, conv(tock.press));
			return true;
		}
		
		if (str == "ralt-key"sv) {
			kb.special(HID_KEY_ALT_RIGHT, conv(tock.press));
			return true;
		}
		
		if (str == "ctrl-key"sv || str ==  "lctrl-key"sv) {
			kb.special(HID_KEY_CONTROL_LEFT, conv(tock.press));
			return true;
		}
		
		if (str == "rctrl-key"sv) {
			kb.special(HID_KEY_CONTROL_RIGHT, conv(tock.press));
			return true;
		}
		
		if (str == "shift-key"sv || str ==  "lshift-key"sv) {
			kb.special(HID_KEY_SHIFT_LEFT, conv(tock.press));
			return true;
		}
				
		if (str == "rshift-key"sv) {
			kb.special(HID_KEY_SHIFT_RIGHT, conv(tock.press));
			return true;
		}
		
		/* ------------------------------------------------------------------------------------- */
		/* Key variant of modifier key (workaround) */
		
		if (str == "special-1"sv) {
			if (auto combination = kb.combination(conv(tock.press)); combination) {
				debugIf(LOG_KEYBOARD, "special-1 init");
				combination.special(HID_KEY_ALT_LEFT);
				combination.symbol('n');
				debugIf(LOG_KEYBOARD, "special-1 n");
			}
			debugIf(LOG_KEYBOARD, "special-1 out of scope");
			return true;
		}
		
		if (str == "special-2"sv) {
			if (auto combination = kb.combination(conv(keyboard::token::press_e::LONGPRESS)); combination) {
				debugIf(LOG_KEYBOARD, "special-2 init");
				combination.special(HID_KEY_ALT_LEFT);
				combination.special(HID_KEY_CONTROL_LEFT);
				combination.special(HID_KEY_DELETE);
				debugIf(LOG_KEYBOARD, "special-2 n");
			}
			debugIf(LOG_KEYBOARD, "special-2 out of scope");
			return true;
		}
		
		if (str == "special-3"sv) {
			if (auto combination = kb.combination(conv(keyboard::token::press_e::PRESS)); combination) {
				debugIf(LOG_KEYBOARD, "special-3 init");
				combination.special(HID_KEY_ALT_LEFT);
				combination.symbol('a');
				combination.symbol('b');
				combination.symbol('c');
				debugIf(LOG_KEYBOARD, "special-3 n");
			}
			debugIf(LOG_KEYBOARD, "special-3 out of scope");
			return true;
		}
		
		if (str == "special-4"sv) {
			if (auto combination = kb.combination(conv(keyboard::token::press_e::DOUBLETAP)); combination) {
				debugIf(LOG_KEYBOARD, "special-4 init");
				combination.special(HID_KEY_ALT_LEFT);
				combination.symbol('a');
				combination.symbol('b');
				combination.symbol('c');
				combination.symbol('d');
				combination.symbol('e');
				debugIf(LOG_KEYBOARD, "special-4 n");
			}
			debugIf(LOG_KEYBOARD, "special-3 out of scope");
			return true;
		}
		
		
		/* ------------------------------------------------------------------------------------- */
		
		if (str == "scrolllock"sv) {
			kb.special(HID_KEY_SCROLL_LOCK, conv(tock.press));
			return true;
		}
		
		if (str == "capslock"sv) {
			kb.special(HID_KEY_CAPS_LOCK, conv(tock.press));
			return true;
		}
		
		if (str == "numlock"sv) {
			kb.special(HID_KEY_NUM_LOCK, conv(tock.press));
			return true;
		}
				
		if (str == "numdiv"sv) {
			kb.special(HID_KEY_KEYPAD_DIVIDE, conv(tock.press));
			return true;
		}
		
		if (str == "nummul"sv) {
			kb.special(HID_KEY_KEYPAD_MULTIPLY, conv(tock.press));
			return true;
		}
		
		if (str == "numsub"sv) {
			kb.special(HID_KEY_KEYPAD_SUBTRACT, conv(tock.press));
			return true;
		}
		
		if (str == "numadd"sv) {
			kb.special(HID_KEY_KEYPAD_ADD, conv(tock.press));
			return true;
		}
		
		if (str == "numenther"sv) {
			kb.special(HID_KEY_KEYPAD_ENTER, conv(tock.press));
			return true;
		}
	
		
		if (str == "numrigth"sv) {
			kb.special(HID_KEY_ARROW_RIGHT, conv(tock.press));
			return true;
		}
		
		if (str == "numleft"sv) {
			kb.special(HID_KEY_ARROW_LEFT, conv(tock.press));
			return true;
		}
		
		if (str == "numdown"sv) {
			kb.special(HID_KEY_ARROW_DOWN, conv(tock.press));
			return true;
		}
		
		if (str == "numup"sv) {
			kb.special(HID_KEY_ARROW_UP, conv(tock.press));
			return true;
		}
		
		if (str.size() == 4 && str.starts_with("num") && str[3] >= '1' && str[3] <= '9') {
			//WARNING HID_KEY_KEYPAD_0 is on the end of HID_KEY_KEYPAD_SEQUENCE
			//but ASCII 0 in on the top on numerical SEQUENCE
			int i = (str[3] - '1');
			debugIf(LOG_KEYBOARD, "num", i);
			kb.special(uint8_t(HID_KEY_KEYPAD_1 + i), conv(tock.press));
			return true;
		}
		
		if (str == "num0"sv) {
			//WARNING HID_KEY_KEYPAD_0 is on the end of HID_KEY_KEYPAD_SEQUENCE
			//but ASCII 0 in on the top on numerical SEQUENCE
			kb.special(HID_KEY_KEYPAD_0, conv(tock.press));
			return true;
		}
		
		if (str == "equal"sv || str == "plus") {
			kb.special(HID_KEY_EQUAL, conv(tock.press));
			return true;
		}
		
		if (str == "minus"sv) {
			kb.special(HID_KEY_MINUS, conv(tock.press));
			return true;
		}
		
		if (str == "comma"sv) {
			kb.special(HID_KEY_COMMA, conv(tock.press));
			return true;
		}
		
				
		if (str == "period"sv) {
			kb.special(HID_KEY_PERIOD, conv(tock.press));
			return true;
		}
		
		if (str == "slash"sv) {
			kb.special(HID_KEY_SLASH, conv(tock.press));
			return true;
		}

		
		if (str == "f10"sv) {
			kb.special(HID_KEY_F10, conv(tock.press));
			return true;
		}
		
		if (str == "f11"sv) {
			kb.special(HID_KEY_F11, conv(tock.press));
			return true;
		}
		
		if (str == "f12"sv) {
			kb.special(HID_KEY_F12, conv(tock.press));
			return true;
		}

		if (str.size() == 2 && str[0] == 'f' && str[1] >= '1' && str[1] <= '9') {
			int i = (str[1] - '0') - 1;
			kb.special(uint8_t(HID_KEY_F1 + i), conv(tock.press));
			return true;
		}
		
		if (str == "tab"sv) {
			kb.special(HID_KEY_TAB, conv(tock.press));
			return true;
		}
		
		if (str == "enter"sv) {
			kb.special(HID_KEY_ENTER, conv(tock.press));
			return true;
		}
		
		if (str == "backspace"sv || str == "back"sv) {
			kb.special(HID_KEY_BACKSPACE, conv(tock.press));
			return true;
		}
		
		if (str == "space"sv) {
			kb.special(HID_KEY_SPACE, conv(tock.press));
			return true;
		}
		
		if (str == "tilde"sv) {
			kb.special(HID_KEY_ESCAPE, conv(tock.press));
			return true;
		}
				
		if (str.size() == 1) {
			//probably encoded symbol like +b:long+
			kb.symbol(*(str.data()), conv(tock.press));
			return true;
		}
		
		info("parsers::keyboard::specialKey unable determinate key, type it", str);

		return false;
	}
#endif
	
	keyboard::keyboard() {};
		
	bool keyboard::parse(const std::string& message) {
		_tokens.clear();
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}
	
	bool keyboard::parse(const std::string_view& message) {
		_tokens.clear();
		tokenizer(message.begin(), message.end(), '+');
		return !hasErrors();
	}
	
#ifndef __TEST_ONLY_PURE_PARESER
	keyboard::write_result keyboard::writeTo(hid::keyboard& kb)  {
		auto sequence = kb.sequence();
		bool success = true;
		
		logIf(true, "[info] ", "extracted tokens -> [");
		const auto total = _tokens.size();
		for (auto i = 0; i < total; ++i) {
			if (_tokens[i].type == keyboard::token::type_e::KEY) {
				logIf(true, "\"", _tokens[i].view, "\"k", total == i + 1 ? "" : ", ");
				if (success = specialKey(sequence, _tokens[i].view, _tokens[i]); !success) {
					break;
				}
			} else {
				logIf(true, "\"", _tokens[i].view, "\"s", total == i + 1 ? "" : ", ");
				//std::string proxy = std::string(_tokens[i].view.begin(), _tokens[i].view.end());
				if (success = sequence.type(_tokens[i].view, _tokens[i].press == keyboard::token::press_e::SHORT); !success) {
					break;
				}
			}			
		}
		logIf(true, "]\n");
		_tokens.clear();
		if (success) { //todo simplify this
			return sequence.lastWriteResult();
		} else {
			return ESP_FAIL;
		}
	}
#endif

}