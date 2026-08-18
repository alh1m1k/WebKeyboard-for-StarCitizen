#pragma once

#include "report.h"
#include "reportHelper.h"

namespace hid {

	using enum report::PacketType;

	/**
	 * reportPacker take advantages of esp32s3 byte alignment (for flash data)
	 * according to `google` overheads of using byte alignment ~10% compared to 4byte alignment
	 * but if needed repack probably may be adapted to 4byte alignment
	 */
	class reportPacker {

		std::vector<uint8_t> writeBuffer = {};
		size_t writeCount = 0;

	public:

		reportPacker() noexcept = default;

		~reportPacker() = default;

		#define REPORT_MASK(maskedType) ((maskedType)|KEYBOARD_MASKING_FLAG)

		void repack(report& report) {
			constexpr size_t headerWithoutPtrSize = sizeof(report.storage) - sizeof((int*)report.advancedPtr);
			const auto firstPacketSizeWithoutType = sizeOfType(writeBuffer[0]);
			if (report.storage[0] == TOTAL) {
				report.storage[0] = EMPTY;
				report.advancedPtr = nullptr; //reset prt also, for safety
			}
			if (report.storage[0] == EMPTY) { // fresh report
				std::span<uint8_t> advance, storage;
				if (writeCount == 1 && writeBuffer[0] != TEXT) {  //report able to fit in header any single type, except TEXT
					if (writeBuffer[0] == KB_BUTTONS) { //but KB_BUTTONS will-be without type and masked to fit as buttons_size == storage_size
						writeBuffer[1] |= KEYBOARD_MASKING_FLAG; //emulate type by masking flag
						storage = {writeBuffer.begin()+1, writeBuffer.end()};
					} else {
						storage = {writeBuffer.begin(), writeBuffer.end()}; //any other type gust go to storage as is
					}
					assert(storage.size() <= sizeof(report.storage) && "malformed buffer");
				} else if (firstPacketSizeWithoutType != -1 && (firstPacketSizeWithoutType + 1 <= headerWithoutPtrSize)) { //small packet that able to fit in 3 bytes
					storage = {writeBuffer.begin(), (size_t)firstPacketSizeWithoutType + 1}; //go to storage
					advance = {writeBuffer.begin() + (size_t)firstPacketSizeWithoutType + 1, writeBuffer.end()}; //rest will go to advanced part
				} else { //dynamic sizes, TEXT
					report.storage[0] = EMPTY_HEADER; //TEXT, flush header and go to advanced part
					advance = {writeBuffer.begin(), writeBuffer.end()};
				}
				if (!storage.empty()) {
					memcpy(report.storage, storage.data(), storage.size());
				}
				if (!advance.empty()) {
					report.advancedPtr = new advanced_report(advance.data(), advance.size());
				}
			} else { //this packet not empty, must check that header have sufficient space to store advanced ptr, or it will-be flushed
				if (const auto headerSizeWithoutType = sizeOfType(report.storage[0]);
					headerSizeWithoutType + 1 <= headerWithoutPtrSize
				) { //header able to fit advanced ptr or already have advanced ptr
					if (report.advancedPtr != nullptr) { //recreate advanced part to store new data
						auto ptr = new advanced_report(
							( (advanced_report*)report.advancedPtr )->buffer,
							( (advanced_report*)report.advancedPtr )->size,
							writeBuffer.data(),
							writeBuffer.size()
						);
						delete (advanced_report*)report.advancedPtr;
						report.advancedPtr = ptr;
					} else { //transform packet to advanced by create advanced ptr
						report.advancedPtr = new advanced_report( writeBuffer.data(), writeBuffer.size());
					}
				} else { //this header is big scalar there is no space for advanced ptr, it must be flushed
					//note: KB_BUTTONS may or may not have type byte
					auto const headerSize = (report.storage[0]&KEYBOARD_MASKING_FLAG) == KEYBOARD_MASKING_FLAG ? headerSizeWithoutType : headerSizeWithoutType + 1;
					assert(headerSize <= sizeof(report.storage));
					auto ptr = new advanced_report(report.storage, headerSize, writeBuffer.data(), writeBuffer.size());
					report.storage[0] = EMPTY_HEADER;
					report.advancedPtr = ptr;
				}
			}
			reset();
		}

		template <typename T>
		size_t pack_back(const uint8_t type, const T& payload) {
			static_assert(std::is_trivially_copyable<T>::value, "T must be memcpy compatible");
			static_assert(sizeof(T) != sizeof(report::text_type), "use specialized packer for strings");
			assert(sizeof(T) == sizeOfType(type) && "incorrect sizes");
			writeBuffer.push_back(type);
			writeBuffer.insert(writeBuffer.end(), (uint8_t*)&payload, (uint8_t*)&payload + sizeof(T));
			writeCount++;
			return sizeof(T);
		}

		//will it work for std::string?
		size_t pack_back(const uint8_t type, const std::string_view& payload, const uint8_t flags = 0x00) {
			assert(type == report::PacketType::TEXT && "incorrect type");
			const auto size = payload.size();
			if (size > std::numeric_limits<uint16_t>::max()) {
				throw std::invalid_argument("string too big");
			}
			writeBuffer.push_back(type);
			writeBuffer.push_back(flags);
			writeBuffer.push_back((uint8_t)size);
			writeBuffer.push_back((uint8_t)(size >> 8));
			//debug("pack_back f", (uint32_t)size, " l ", (uint32_t)(size >> 8), "  ", *(uint16_t*)&writeBuffer[writeBuffer.size() - 2]);
			writeBuffer.insert(writeBuffer.end(), payload.begin(), payload.end());
			writeCount++;
			return size;
		}

		size_t pack_back(const uint8_t type, const std::string& payload, const uint8_t flags = 0x00) {
			return pack_back(type, std::string_view(payload), flags);
		}

		void reset() {
			writeBuffer.clear();
			writeCount = 0;
		}

	};

}