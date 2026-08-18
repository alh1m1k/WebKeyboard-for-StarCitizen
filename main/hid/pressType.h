#pragma once

namespace hid {

	enum class pressType: uint8_t {
		PRESS = 0x01,
		LONGPRESS,
		DOUBLETAP,
		SHORT,
		DOWN,
		UP,
		INVALID
	};

	enum class flag: uint8_t {
		COMBINATION_BEGIN =	(1UL << (7)),
		COMBINATION_END   =	(1UL << (6)),

		MASK 			  = COMBINATION_BEGIN|COMBINATION_END
	};

	enum class suffix: uint8_t {
		FORMAT_KB_KEY 				= 0x00,	//{flag, modifiers, key, press}
		FORMAT_KB_DEFAULT_PRESS,			//{flag, modifiers, key, key2}
		FORMAT_KB_ENDING_PRESS,				//{flag, key, key2, press}
		FORMAT_KB_KEYS,						//{flag, key, key2, key3}
		SPECIAL_MEANING				= 0xFF&(~(uint8_t)flag::MASK), //max avl flag
	};

}
