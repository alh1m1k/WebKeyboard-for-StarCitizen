#pragma once

#include "task.h"
#include "tinyusb.h"
#include <sys/_stdint.h>

#include "generated.h"

#include "shortcutWriter.h"
#include "writer.h"
#include "result.h"
#include "util.h"


namespace hid {

	template<class BACK_PUSHER, class MUTEX>
	class sequenceWriter: public writer<BACK_PUSHER> {
		
		using writer<BACK_PUSHER>::backend;
					
		std::unique_lock<MUTEX> guardian;
		
		public:
		
			using writer<BACK_PUSHER>::lastResult;
			using writer<BACK_PUSHER>::lastWriteResult;
			using writer<BACK_PUSHER>::symbol;
			using writer<BACK_PUSHER>::special;
			using writer<BACK_PUSHER>::modify;
		
			typedef shortcutWriter<BACK_PUSHER> shortcut_writer_type;
							
			sequenceWriter(BACK_PUSHER& bp, std::unique_lock<MUTEX>&& guardian): writer<BACK_PUSHER>(bp, 0), guardian(std::move(guardian)) {

			}
			
			~sequenceWriter() {

			}
			
			sequenceWriter(sequenceWriter&& other) = default;
			
			shortcut_writer_type combination(task::pressType press = task::pressType::PRESS, uint8_t modifiers = 0) {
				if ((uint8_t)press == 0) {
					error("sequenceWriter::combination invalid press type", (uint8_t)press, " ", (uint8_t)modifiers);
				}
				return shortcutWriter(backend, press, modifiers, &lastResult);
			}
			
			template<typename iterable>
			bool type(iterable str, bool fast = false) {
				for (auto it = std::begin(str), end = std::end(str); it != end; ++it) {
					debugIf(LOG_KEYBOARD, "sequenceWriter::type", *it);
					if (!symbol(*it, fast ? task::pressType::SHORT : task::pressType::PRESS)) {
						return false;
					}
				}
				return true;
			}
						
			operator bool() {
				return !!backend;
			}
	};

}