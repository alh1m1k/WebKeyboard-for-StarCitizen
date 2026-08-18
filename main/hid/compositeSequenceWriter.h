#pragma once

#include <sys/_stdint.h>

#include "generated.h"

#include "compositeCombinationWriter.h"
#include "compositeWriter.h"
#include "result.h"
#include "util.h"

namespace hid {

	template<class BACK_PUSHER, class MUTEX>
	class compositeSequenceWriter: public compositeWriter<BACK_PUSHER> {
		
		using compositeWriter<BACK_PUSHER>::backend;
		using enum report::PacketType;
					
		std::unique_lock<MUTEX> guardian;
		
		public:

			using compositeWriter<BACK_PUSHER>::keyboardModifier;
			using compositeWriter<BACK_PUSHER>::keyboardSymbol;
			using compositeWriter<BACK_PUSHER>::keyboardKey;

			using compositeWriter<BACK_PUSHER>::joystickAxis;
			using compositeWriter<BACK_PUSHER>::joystickButtons;

			using compositeWriter<BACK_PUSHER>::mousePosition;
			using compositeWriter<BACK_PUSHER>::mouseScroll;
			using compositeWriter<BACK_PUSHER>::mouseButtons;
		
			typedef compositeCombinationWriter<BACK_PUSHER> combination_writer_type;
			using write_result_type = compositeCombinationWriter<BACK_PUSHER>::write_result_type;
							
			compositeSequenceWriter(BACK_PUSHER& bp, std::unique_lock<MUTEX>&& guardian)
				: compositeWriter<BACK_PUSHER>(bp, 0), guardian(std::move(guardian)) { }
			
			~compositeSequenceWriter() = default;
			
			compositeSequenceWriter(compositeSequenceWriter&& other) = default;
			
			combination_writer_type combination(const pressType press = pressType::PRESS, const uint8_t modifiers = 0) {
				if ((uint8_t)press == 0) {
					error("sequenceWriter::combination invalid press type", (uint8_t)press, " ", (uint8_t)modifiers);
				}
				return compositeCombinationWriter(backend, press, modifiers);
			}

			template<typename iterable>
			write_result_type keyboardTyping(iterable str, const bool fast = false) {
				reportPacker packer = {};
				packer.pack_back(
					TEXT,
					std::string_view(str.begin(), str.end()),
					fast ? (uint8_t)pressType::SHORT : (uint8_t)pressType::PRESS
				);
				report report = {};
				packer.repack(report);
				return (uint32_t)backend->push_back(report);
			}
						
			operator bool() const noexcept {
				return !!backend;
			}

			write_result_type lastWriteResult() { return backend->receivedCnt(); }
	};

}