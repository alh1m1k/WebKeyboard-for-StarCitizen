#include "asyncSocket.h"

#include "esp_http_server.h"
#include "lwip/def.h"
#include "work.h"

namespace http::socket {
	
	#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
	std::mutex m {};	
	#endif

	asyncSocket::asyncSocket(httpd_handle_t hd, const int fd) noexcept : hd(hd), fd(fd) {};

	bool asyncSocket::valid() const noexcept {
		return (hd != nullptr && fd != 0) && httpd_ws_get_fd_info(hd, fd) == HTTPD_WS_CLIENT_WEBSOCKET;
	}

	resBool asyncSocket::write(const uint8_t* buffer, const size_t size, const httpd_ws_type_t type) noexcept {
		#if defined(SYNC_WRITE_OP_WITH_MUTEX) 
		std::unique_lock guardian {m};	
		#endif
		httpd_ws_frame_t 	ws_pkt = {};
		ws_pkt.type 	= type;
		ws_pkt.payload  = (uint8_t*)buffer; //dangerous?
		ws_pkt.len 		= size;
		return httpd_ws_send_frame_async(hd, fd, &ws_pkt);
	}
	
	resBool asyncSocket::write(const message& msg, const httpd_ws_type_t type) noexcept {
		debugIf(LOG_SOCKET, "socket::write", msg.size());
		return write((const uint8_t*)msg.data(), msg.size(), type);
	}
	
	resBool asyncSocket::write(const char* msg, const httpd_ws_type_t type) noexcept {
		return write((const uint8_t*)msg, strlen(msg), type);
	}

	resBool asyncSocket::close(
		const wscodes code,
		const uint8_t* buffer,
		const size_t size,
		asyncSocket::callback_type&& callback
	) noexcept {

		const auto serverHandle = this->serverHandle();
		const auto fileDescriptor = this->fileDescriptor();
		const auto txBuffSize = size+2;
		const auto codeNetOrder = lwip_htons((int16_t)code);
		auto txBuff = std::make_unique_for_overwrite<uint8_t[]>(txBuffSize);
		memcpy(txBuff.get(), &codeNetOrder, 2);
		memcpy(&txBuff[2], buffer, size); //memcpy from zero-sized must-be noop

		return work::schedule([=, txBuff = std::move(txBuff), callback = std::move(callback)]() mutable -> esp_err_t {
			//create new interface object in case previous was dead
			auto socket = asyncSocket(serverHandle, fileDescriptor);
			if (const auto result = socket.write(txBuff.get(), txBuffSize, httpd_ws_type_t::HTTPD_WS_TYPE_CLOSE); !result) {
				error("asyncSocket::close unable send frame", result.code());
			}
			//terminate are self queued, no need for work there
			if (const auto ret = socket.terminate(); !ret || callback == nullptr) {
				return ret.code();
			}
			//schedule callback after terminate no way to check that it succeed
			return work::schedule(std::move(callback), serverHandle).code();
		}, serverHandle);
	}

	resBool asyncSocket::closeWait(const wscodes code, const uint8_t* buffer, const size_t size) noexcept {
		const auto handle = xTaskGetCurrentTaskHandle();
		/**
		 *	httpd is executer for 'close', so it can't wait for 'close'
		 */
		assert(strcmp(pcTaskGetName(handle), "httpd") != 0 && "call from invalid context");
		if (const auto ret =  close(code, buffer, size, [=]() -> esp_err_t {
			xTaskNotifyGive(handle);
			return ESP_OK;
		}); ret) {
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			return ret;
		} else {
			return ret;
		}
	}
}