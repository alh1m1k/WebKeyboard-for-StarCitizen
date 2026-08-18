#pragma once

#include "report.h"
#include "reportHelper.h"

namespace hid {

	using enum report::PacketType;

	class reportUnpacker {

		report& report_;
		size_t readBytes = 0;
		size_t readEntities = 0;
		bool done = false;

		union uReportVariant {
			report::text_type text{};
			report::joystick_axis_type joystick_axis;
			report::joystick_buttons_type joystick_buttons;
			report::mouse_buttons_type mouse_buttons;
			report::mouse_scrolls_type mouse_scroll;
			report::mouse_axes_type mouse_axis;
			report::keyboard_buttons_type keyboard_buttons;
		} ;

		struct base_result_type {
			uint8_t type = EMPTY;
			uReportVariant* report = nullptr;
		};

		class iterator {

			reportUnpacker& owner;
			base_result_type result;

			public:

				using iterator_category = std::input_iterator_tag;
				using value_type        = base_result_type;
				using difference_type   = std::ptrdiff_t;
				using pointer           = const base_result_type*;
				using reference         = const base_result_type&;

				explicit iterator(reportUnpacker& owner, base_result_type&& result) : owner(owner), result(result) { }

				iterator& operator++() {
					result = owner.unpack_front();
					return *this;
				}

				reference operator*() const {
					return result;
				}

				pointer operator->() const {
					return &result;
				}

				bool operator==(const iterator& other) const {
					return result.type == other.result.type &&
						result.report == other.result.report;
				}

				bool operator!=(const iterator& other) const {
					return !(*this == other);
				}
		};


		static size_t actualSizeOfType(const uint8_t* pointer, const size_t size) {
			switch (const auto type = pointer[0]) {
			case TEXT: //for dynamic size type
				assert(size > 4);
				return *(uint16_t*)&pointer[2] + 4;
			default:
				return sizeOfType(type) + 1;
			}
		}

		inline base_result_type buildReport(const uint8_t* pointer, const size_t size, const bool consume = true) {
			//KB_BUTTONS may have or not have type byte, if it not have type it must be masked
			const auto wasMasked = (*pointer&KEYBOARD_MASKING_FLAG) == KEYBOARD_MASKING_FLAG;
			if (consume) { //readout data
				auto const tSize = actualSizeOfType(pointer, size);
				assert(tSize <= size && tSize > 0);
				readBytes += wasMasked ? tSize - 1 : tSize;
			}
			if (wasMasked) {
				return {KB_BUTTONS, (uReportVariant*)pointer };
			} else {
				return {pointer[0], (uReportVariant*)&pointer[1] };
			}
		}

	public:

		using result_type = base_result_type;
		using iterator_type = iterator;
		using report_type = report;

		bool destructive = true;

		explicit reportUnpacker(report_type& report) noexcept : report_(report) {}

		~reportUnpacker() = default;

		#define REPORT_UNMASK(maskedType) ((maskedType)&~KEYBOARD_MASKING_FLAG)
		
		result_type unpack_front() {
			if (done) { return {}; }
			if (readEntities == 0) { //first read, read header
				readEntities++;
				if (report_.storage[0] == EMPTY || report_.storage[0] == TOTAL) {
					done = true; //packet new or terminated
					return {};
				}
				if (report_.storage[0] == EMPTY_HEADER) {
					//header flushed out, read first data from advanced part
					assert(report_.advancedPtr != nullptr);
					readEntities++;
					//debug("unpack_front", report_.advancedPtr->size, " ", readBytes);
					return buildReport(
						&( (advanced_report*)report_.advancedPtr )->buffer[readBytes],
						( (advanced_report*)report_.advancedPtr )->size - readBytes
					);
				}
				//if this report not advanced we are done
				if (!isAdvancedReport(report_)) { done = true; }
				return buildReport(report_.storage, 8, false);
			} else { //read advanced part until it bytes not ended
				assert(report_.advancedPtr != nullptr);
				//debug("unpack_front", report_.advancedPtr->size, " ", readBytes);
				if (readBytes >= ( (advanced_report*)report_.advancedPtr )->size) {
					done = true;
					if (destructive) { recycle(report_); }
					return {};
				}
				readEntities++;
				return buildReport(
					&( (advanced_report*)report_.advancedPtr )->buffer[readBytes],
					( (advanced_report*)report_.advancedPtr )->size - readBytes
				);
			}
		}

		void reset() noexcept {
			readEntities = 0;
			readBytes = 0;
		}

		iterator begin() {
			return iterator(*this, unpack_front());
		}

		iterator end() {
			return iterator(*this, {});
		}

	};

}