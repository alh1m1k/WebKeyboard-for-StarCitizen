#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <future>
#include <iterator>
#include <mutex>
#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <sys/_stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <charconv>
#include <random>

#include <esp_wifi_types.h>


#include "asyncSocket.h"
#include "bad_api_call.h"
#include "ctrlmap.h"
#include "esp_timer.h"
#include "keyboardTask.h"
#include "not_found.h"
#include "scheduler.h"
#include "storage.h"
#include "unsupported.h"
#include "clients.h"
#include "codes.h"
#include "config.h"
#include "define.h"
#include "generated.h"
#include "contentType.h"
#include "esp_err.h"
#include "handler.h"
#include "http/server.h"
#include "hid/keyboard.h"
#include "notification.h"
#include "parsers/keyboard.h"
#include "resource/memory/file.h"
#include "result.h"
#include "socket/handler.h"
#include "socket/socket.h"
#include "usbDevice.h"
#include "util.h"
#include "wifi/wifi.h"
#include "resource/memory/shortcut.h"
#include "driver/gpio.h"
#include "hwrandom.h"
#include "esp_random.h"
#include "task.h"
#include "packers.h"
#include "ledStatusChange.h"
#include "usbDeviceImpl.h"
#include "joystick.h"
#include "bootloader_random.h"

#ifdef WIFI_AP_DNS
#include "dns/server.h"
#endif

#include "resultStream.h" //need for cout << result


extern "C" {
	void app_main(void);
}

//http::resource::memory::file will be created and stored in hello_js_memory_file
decl_memory_file(widget_js, 			http::resource::memory::endings::TEXT, 		"widget.js", 		http::contentType::JS		);
decl_memory_file(overlay_js, 			http::resource::memory::endings::TEXT, 		"overlay.js", 		http::contentType::JS		);
decl_memory_file(lib_js, 				http::resource::memory::endings::TEXT, 		"lib.js", 			http::contentType::JS		);
decl_memory_file(index_html, 			http::resource::memory::endings::TEXT, 		"index.html", 		http::contentType::TEXT		);
decl_memory_file(favicon_ico, 			http::resource::memory::endings::BINARY, 	"favicon.ico", 		"image/x-icon"				);
decl_memory_file(index_js, 				http::resource::memory::endings::TEXT, 		"index.js", 		http::contentType::JS		);
decl_memory_file(index_css,				http::resource::memory::endings::TEXT, 		"index.css", 		http::contentType::CSS		);
decl_memory_file(symbols_svg,			http::resource::memory::endings::TEXT, 		"symbols.svg", 		http::contentType::SVG		);

 
using namespace std::literals;

const uint32_t DELAY_INITAL 		= 1000;
const uint32_t DELAY_INCRIMENT 		= 1000;
const uint32_t DELAY_MAX 			= 10000;

uint32_t delay = DELAY_INITAL;


#define CHECK_FOR_FACTORY_RESET(pressedTime, success) { 								\
	if (gpio_get_level(RESET_TRIGGER_BTN) == 0) {										\
		if (pressedTime == 0) {															\
			pressedTime = esp_timer_get_time();											\
		} else if (esp_timer_get_time() - pressedTime > 3000000) {						\
			info("trigger factory reset");												\
			storage persistence = storage();											\
			if (persistence.reset()) {													\
				for (int i = 0; i < 3 && RESET_LED_INDICATOR != NO_LED; i++) {			\
					gpio_set_level(RESET_LED_INDICATOR, 1);								\
					vTaskDelay(pdMS_TO_TICKS(400));										\
					gpio_set_level(RESET_LED_INDICATOR, 0);								\
					vTaskDelay(pdMS_TO_TICKS(400));										\
				}																		\
				info("storage cleared");												\
				success = true;														    \
			} else {																	\
				error("while storage clear");											\
				success = false;														\
			}																			\
		}																				\
	} else if (pressedTime) {															\
		pressedTime = 0;																\
		info("storage clear wd reset");													\
		success = false;																\
	}																					\
}

void trap(const char* msg, esp_err_t code = ESP_FAIL) {
	int64_t 	pressedTime = 0;
	uint32_t 	counter = 0;
	bool        status = false;
	panic(msg, code);
	while (1) {
		if (counter++ % 50 == 0) {
			info("board in fail state, wait for reset or factory reset");
		}
		CHECK_FOR_FACTORY_RESET(pressedTime, status);
		if (status) {
			status = false;
			esp_restart();
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	throw std::runtime_error("trap func must newer ended");
}

hid::joystick::control_writer_type& operator<<(hid::joystick::control_writer_type& stream, const std::string_view& byteStream) {
	
	stream.copyFrom(&byteStream[0], byteStream.size());
	
	return stream;
}

typedef clients<http::socket::asyncSocket, SESSION_MAX_CLIENT_COUNT, std::string> 	socketsFinal;
typedef notificationManager<socketsFinal> 											notificationManagerFinal;
typedef generator<uint32_t, true> 													packetIdGeneratorFinal;
typedef scheduler<std::string> 	  													tailSchedulerFinal;
typedef ctrlmap<std::string, std::string> 	  										controlsMapFinal;


void app_main(void)
{

	info("app init");
	
	bootloader_random_enable();
	srand(esp_random()); //todo replace srand with c++ analog
	bootloader_random_disable();
	
	
	info("wait for possible factory reset", "hold \"RESET_TRIGGER_BTN\" for more than second to perform reset");
	{
		
		gpio_config_t trigger = {
		    .pin_bit_mask 	 = (1ULL << RESET_TRIGGER_BTN),
		    .mode 			 = GPIO_MODE_INPUT,
		    .pull_up_en      = GPIO_PULLUP_ENABLE,
			.pull_down_en    = GPIO_PULLDOWN_DISABLE,
			.intr_type       = GPIO_INTR_DISABLE,
		};
		gpio_config(&trigger);
		
		if (RESET_LED_INDICATOR != NO_LED) {
		    gpio_config_t ledConfig = {
			    .pin_bit_mask 	 = (1ULL << RESET_LED_INDICATOR),
			    .mode 			 = GPIO_MODE_OUTPUT,
			    .pull_up_en      = GPIO_PULLUP_DISABLE,
				.pull_down_en    = GPIO_PULLDOWN_DISABLE,
				.intr_type       = GPIO_INTR_DISABLE,
			};
	 		gpio_config(&ledConfig);
		}

		
		int64_t startTime 	= esp_timer_get_time();
		int64_t pressedTime = 0;
		bool    done = false;
		while (esp_timer_get_time() - startTime < 1000000 || pressedTime) {
			CHECK_FOR_FACTORY_RESET(pressedTime, done)
			if (done) {
				done = false;
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(100));
	  	}
	}

  	info("factory reset interval passed");
			
	if (auto wifiStatus = wifi::init(); !wifiStatus) {
		trap("fail to init wifi", wifiStatus.code());
	}
		
	if (auto statusCode = wifi::mode(WIFI_MODE); !statusCode) {
		trap("unable setup mode", statusCode.code());
	}
	
	std::string persistanceSign;
	std::string bootingSign;
	
	{
		
		storage persistence = storage();
				 
		if (WIFI_MODE == wifi_mode_t::WIFI_MODE_STA) {
			while (wifi::status() != wifi::status_e::CONNECTED) {
				if (!wifi::connect(persistence.getWifiSSID(), persistence.getWifiPWD())) {
					vTaskDelay(pdMS_TO_TICKS(delay));
					delay = std::min(delay+DELAY_INCRIMENT, DELAY_MAX);
				} else {
					delay = DELAY_INITAL;
				}
			}
		} else {
			auto status = wifi::hotspotBegin(persistence.getWifiSSID(), persistence.getWifiPWD(), persistence.getWifiAuth());
			if (!status) {
				trap("fail to begin wifi hotspot", status.code());
			}
		}
		
		if (persistanceSign = persistence.getStorageSign(); persistanceSign == "") {
			persistanceSign = genGandom(32);
			infoIf(LOG_ENTROPY, "generate new storage sign", persistanceSign.c_str());
			if (auto status = persistence.setStorageSign(persistanceSign); status) {
				debug("new storage sign is set");
			} else {
				error("fail to set new storage sign", status.code());
			}
		} else {
			infoIf(LOG_ENTROPY, "storage sign", persistanceSign.c_str());
		}
	
	}
	
	{
		bootingSign = genGandom(32);
	}
	
	
	
#ifdef WIFI_AP_DNS
	std::cout << "starting mdns " << std::endl;
	auto dns = dns::server(WIFI_AP_DNS_SERVICE_NAME, WIFI_AP_DNS_SERVICE_DESCRIPTION);
	dns.addService("_http", "_tcp", 80);
	if (auto status = dns.begin(); !status) {
		trap("fail 2 setup mDNS", status.code());
	}
#endif

	std::cout << "starting rnd generator " << std::endl;
	
	struct  {
		hwrandom<uint32_t> generator;
		std::normal_distribution<float> distribution;
	} randomCtx = {
		.generator 		= {},
		.distribution 	= std::normal_distribution<float>(0, 1.0),
	};
	
	
	
	std::cout << "starting usb Stack " << std::endl;
	hid::UsbDevice = std::make_unique<hid::UsbDeviceImpl>();
	
	std::cout << "starting keyboard " << std::endl;
	auto kb = hid::keyboard();
	
	if (!kb.install()) {
		trap("fail 2 setup keyboard", ESP_FAIL);
	}
	kb.entroySource([&randomCtx]() -> float { return randomCtx.distribution(randomCtx.generator); });
	
	std::cout << "starting joystick " << std::endl;
	auto joy = hid::joystick();
	
	if (!joy.install()) {
		trap("fail 2 setup joystick", ESP_FAIL);
	}
	
	std::cout << "installing usb driver" << std::endl;
	if (!hid::UsbDevice->install()) { //mustbe called after all usb device
		trap("fail 2 install usb driver", ESP_FAIL);
	}
	std::cout << "starting of usb stack complete" << std::endl;
	
	
	

	std::cout << "starting webServer " << std::endl;
	http::server webServer = {};
	
	if (auto status = webServer.begin(80); !status) {
		trap("fail 2 setup webServer", status.code());
	}
				
	resBool status = ESP_OK;

	if (status = webServer.addHandler("/"sv, httpd_method_t::HTTP_GET, index_html_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /", 				status.code());
	}
	if (status = webServer.addHandler("/lib.js"sv, 		httpd_method_t::HTTP_GET, lib_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /lib.js", 		status.code());
	}
	if (status = webServer.addHandler("/widget.js"sv, 	httpd_method_t::HTTP_GET, widget_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /widget.js", 	status.code());
	}
	if (status = webServer.addHandler("/overlay.js"sv, 	httpd_method_t::HTTP_GET, overlay_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /overlay.js", 	status.code());
	}
	if (status = webServer.addHandler("/favicon.ico"sv, httpd_method_t::HTTP_GET, favicon_ico_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /favicon.ico", 	status.code());
	}
	if (status = webServer.addHandler("/index.js"sv, 	httpd_method_t::HTTP_GET, index_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /index.js", 		status.code());
	}
	if (status = webServer.addHandler("/index.css"sv, 	httpd_method_t::HTTP_GET, index_css_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /index.css", 		status.code());
	}
	if (status = webServer.addHandler("/symbols.svg"sv, httpd_method_t::HTTP_GET, symbols_svg_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /symbols.svg", 	status.code());
	}

	
	
	/*
	* TODO 
	* Httpd Stuck problem
	* it is not Static router (no control transfer to it)
	* it is not Clients or Notification deadlock (i disable it)
	* it is not httpd chunk file transfer optimization (i disable it)
	* it feel like httpd socket or thread pool depletion
	* or as variant incorrect http route handling leading to depletion (mission of proper ending)
	*/
	
	
	auto kbMessageParser = parsers::keyboard();
	auto packetCounter 	 = packetIdGeneratorFinal();
	auto ctrl = controlsMapFinal();
#ifdef _SESSION_ENABLE
	auto sockets  = socketsFinal();
#endif
#ifdef _NOTIFICATION_ENABLE
	auto notification 	= notificationManagerFinal(sockets);
#endif
	auto tailOp				= tailSchedulerFinal();
	
	struct  {
		parsers::keyboard& 			parser;
		hid::keyboard& 			keyboard;
		hid::joystick&             joy;
#ifdef _SESSION_ENABLE
		socketsFinal& 				sockets;
#endif
#ifdef _NOTIFICATION_ENABLE
		notificationManagerFinal&	notification;
#endif
		packetIdGeneratorFinal&     packetCounter;
		ctrlmap<std::string, std::string>& ctrl;
		std::string& persistanceSign;
		std::string& bootingSign;
		tailSchedulerFinal& tailOp;
	} closureCtx = {
		.parser 			= kbMessageParser,
		.keyboard 			= kb,
		.joy				= joy,
#ifdef _SESSION_ENABLE
		.sockets 			= sockets,
#endif
#ifdef _NOTIFICATION_ENABLE
		.notification		= notification,
#endif
		.packetCounter      = packetCounter,
		.ctrl				= ctrl,
		.persistanceSign    = persistanceSign,
		.bootingSign		= bootingSign,
		.tailOp 			= tailOp,
	};
	
/*	result = webServer.addHandler("/leave"sv, httpd_method_t::HTTP_POST, [&closureCtx](http::request& req, http::response& resp)-> http::handlerRes {
		
		debug("call leave handler");
		
		auto socket = http::socket::socket(req.native()).keep();
		
		
		auto clientId = closureCtx.sockets.get(socket);
		bool success = closureCtx.sockets.remove(socket);
		
		#ifdef _NOTIFICATION_ENABLE
			if (success) {
				if (auto ret = closureCtx.notification.notifyExept(connectedNotify(closureCtx.packetCounter(), clientId, false), clientId); !ret) {
					debug("unable send notification (leave)", ret.code());
				}
			}
		#endif


		return (esp_err_t)ESP_OK;
	});*/
	
	
	if (status = webServer.addHandler("/socks"sv, httpd_method_t::HTTP_GET, http::socket::handler([&closureCtx](http::socket::socket& context, const http::socket::socket::message& rawMessage) -> resBool {
		
		debugIf(LOG_MESSAGES, "sock rcv", rawMessage);
		
#ifdef _SESSION_ENABLE

		auto longSocks = context.keep();

#endif
			
		if (rawMessage.starts_with("auth:")) {
			auto pack = unpackMsg(rawMessage);
			if (!pack.success) {
				error("auth block fail2parce");
				return ESP_FAIL;
			}	
			
			
			std::string clientId = std::string(trim(pack.body));
			if (!clientId.size()) {
				error("auth block fail2clientid");
				return ESP_FAIL;
			}	
			
			info("client is connecting", clientId, " ", pack.taskId);
#ifdef _SESSION_ENABLE				
			bool success = false; bool isNew = false;
			if (closureCtx.sockets.has(clientId)) {
				success = closureCtx.sockets.update(clientId, std::move(longSocks));
				info("updated client to new socket", closureCtx.sockets.count());
			} else {
				success = closureCtx.sockets.add(std::move(longSocks), clientId);
				isNew   = true;
				info("add new client", closureCtx.sockets.count());
			}
#else 
			bool success = true;
#endif
	
			debugIf(LOG_MESSAGES, "respond", pack.taskId, " ", resultMsg("auth", pack.taskId, success));
			context.write(resultMsg("auth", pack.taskId, success));
#ifdef _NOTIFICATION_ENABLE
			if (success) {
				if (isNew) {
					if (auto ret = closureCtx.notification.notify(signNotify(closureCtx.packetCounter(), "storageSign", closureCtx.persistanceSign), clientId); !ret) {
						error("unable send notification (storageSign)", ret.code());
					}
					if (auto ret = closureCtx.notification.notify(signNotify(closureCtx.packetCounter(), "bootingSign", closureCtx.bootingSign), clientId); !ret) {
						error("unable send notification (bootingSign)", ret.code());
					}
					if (auto ret = closureCtx.notification.notifyExept(connectedNotify(closureCtx.packetCounter(), clientId, true), clientId); !ret) {
						error("unable send notification (auth)", ret.code());
					}
				} else {
					
				}
				{
					auto guardian = closureCtx.ctrl.sharedGuardian(); //it leave with scope
					for (auto it = closureCtx.ctrl.begin(), endIt = closureCtx.ctrl.end(); it != endIt; ++it) {
						auto pair = *it;
						closureCtx.notification.notify(kbNotify(closureCtx.packetCounter(), pair.first, pair.second), clientId);
					}
				}
				
				std::string joystickView = {};
				joystickView.resize(20);
				closureCtx.joy.control().copyTo(joystickView.data(), joystickView.size());
				closureCtx.notification.notify(ctrNotify(closureCtx.packetCounter(), joystickView), clientId, true);
			}
#endif			
			
			return ESP_OK;
			
		} 
		
#ifdef _SESSION_ENABLE			
		if (!closureCtx.sockets.has(longSocks)) {
			error("authfail");
			context.write("authfail");
			return ESP_OK;
		}
#endif
		
		if (rawMessage.starts_with("leave:")) {
			auto pack = unpackMsg(rawMessage);
			if (!pack.success) {
				return ESP_FAIL;
			} 
			std::string clientId = std::string(trim(pack.body));
			//std::string clientId = std::string(pack.body);
			if (!clientId.size()) {
				return ESP_FAIL;
			}	

			info("client is leaving", pack.taskId, " ", pack.body);
#ifdef _SESSION_ENABLE
			bool success = closureCtx.sockets.remove(longSocks);
			if (!success) {
				success = closureCtx.sockets.remove(clientId);
			} else {
				closureCtx.sockets.remove(clientId);
			}
#else
			bool success = true;
#endif
			
			context.write(resultMsg("leave", pack.taskId, success));
#ifdef _NOTIFICATION_ENABLE
			if (success) {
				if (auto ret = closureCtx.notification.notifyExept(connectedNotify(closureCtx.packetCounter(), clientId, false), clientId); !ret) {
					error("unable send notification (leave)", ret.code());
				}
			}
#endif
			
			return ESP_OK;
				
		} 
		
		
		//static uint32_t joystickPrevTaskId = 0;
		
		if (rawMessage.starts_with("ctr:")) { 
			//control axis request must be fixed size: 8 axis(2byte each) + buttons(32 one bit each) = 20bytes
			auto pack = unpackMsg(rawMessage);
			if (!pack.success) {
				return ESP_FAIL;
			}
			if (pack.body.size() != 20) {
				error("ctrl axis packet has invalid size: ", rawMessage, " ", rawMessage.size(), " ", (uint)pack.body[0], " ", (uint)pack.body[1], " ", (uint)pack.body[pack.body.size()-2], " ", (uint)pack.body[pack.body.size()-1], " ",  pack.body.size(), " ", 20);
				return ESP_FAIL;
			} 
/*			if (auto taskId = packetIdFromView(pack.taskId); !taskId) {
				error("rcv \"ctr\" packet is malformed", pack.taskId, " ", taskId.code());
				return ESP_FAIL;
			} else {
				//ban OutOfOrder execution
				if (auto uTaskId = std::get<uint32_t>(taskId); uTaskId < joystickPrevTaskId) {
					error("rcv \"ctr\" packet outoforder", uTaskId, " ", joystickPrevTaskId);
					return ESP_FAIL;
				} else {
					joystickPrevTaskId = uTaskId;
				}
			}*/
			
			auto controlStream = closureCtx.joy.control();
			controlStream << pack.body;
						
			auto clientId = closureCtx.sockets.get(longSocks);
			if (auto ret = closureCtx.notification.notifyExept(ctrNotify(closureCtx.packetCounter(), pack.body), clientId, true); !ret) {
				error("unable send notification (ctr)", ret.code());
			}
			
			return ESP_OK; //no respond
		}
		
		if (rawMessage.starts_with("axi:")) { 
			//custom axis request must be fixed size: 2byte + buttons (32 one bit each) + 4byte header = 10 bytes
		}
		
		
		if (rawMessage.starts_with("ping:")) {
			auto pack = unpackMsg(rawMessage);
			if (pack.success) {
				context.write(resultMsg("ping", pack.taskId, pack.success));		
				return ESP_OK;
			} else {
				return ESP_FAIL;
			}
		} else if (rawMessage.starts_with("kb:")) {
			auto pack = unpackMsg(rawMessage);
			if (!pack.success) {
				return ESP_FAIL;
			}
			auto kbPack = unpackKb(pack.body);
			debugIf(LOG_MESSAGES, "kb payload", pack.body, " ", kbPack.hasAction, " ", kbPack.input, " ", kbPack.actionId, " ", kbPack.actionType);
			if (kbPack.hasInput) {
				if (!closureCtx.parser.parse(kbPack.input)) {
					context.write(resultMsg("kb", pack.taskId, false));
				} else {
					auto kbResult = closureCtx.parser.writeTo(closureCtx.keyboard);
					context.write(resultMsg("kb", pack.taskId, true));
					if (kbResult) {
						debugIf(LOG_MESSAGES, "kb schedule ok", std::get<uint32_t>(kbResult));
					} else {
						error("kb schedule fail", kbResult.code());
					}
				}
			} else {
				context.write(resultMsg("kb", pack.taskId, true)); //case to update of switchoff state to controls
			}


#ifdef _NOTIFICATION_ENABLE
			if (kbPack.hasAction) {
				closureCtx.ctrl.nextState(std::string(kbPack.actionId), std::string(kbPack.actionType));
				try {
					auto clientId = closureCtx.sockets.get(longSocks);
					//todo check chat kbNotify move is performed
					if (auto ret = closureCtx.notification.notifyExept(kbNotify(closureCtx.packetCounter(), kbPack.actionId, kbPack.actionType), clientId); !ret) {
						error("unable send notification (kba)", ret.code());
					}
				} catch (not_found& e) {
					error("unable to send kb notify", e.what());
				}
			} else {
				debug("no action data", pack.taskId);
			}
#endif
			
			return ESP_OK;
			
		} else if (rawMessage.starts_with("settings-get:")) {
			
			auto pack = unpackMsg(rawMessage);
			
			if (!pack.success) {
				return ESP_FAIL;
			}
						
			auto persistence = storage();
														
			context.write(settingsGetMsg(pack.taskId, persistence.getWifiSSID(), persistence.getWifiAuth()));
			
			return ESP_OK;
			
		} else if (rawMessage.starts_with("settings-set:")) { 
			
			auto pack = unpackMsg(rawMessage);
					
			if (!pack.success) {
				return ESP_FAIL;
			}
			
			
			auto settings 	= unpackSettings(rawMessage);
			bool updated 	= false;
						
			if (!(settings.success && settings.valid)) {
				context.write(resultMsg("settings-set", pack.taskId, false));
				return ESP_OK;
			}
			
			storage persistence = storage();
			
			if (settings.auth != persistence.getWifiAuth() && settings.auth != wifi_auth_mode_t::WIFI_AUTH_MAX) {
				if (auto r = persistence.setWifiAuth(settings.auth); r) {
					info("AUTH type updated", settings.auth);
					updated = true;
				} else {
					error("AUTH type update fail");
				}
			}
			
			if (auto ssid = std::string(settings.ssid); ssid != persistence.getWifiSSID()) {
				if (auto r = persistence.setWifiSSID(ssid); r) {
					info("SSID updated", ssid);
					updated = true;
				} else {
					error("SSID update fail");
				}
			}
						
			if (settings.hasPassword) {
				if (auto password = std::string(settings.password); password != persistence.getWifiPWD()) {
					if (auto r = persistence.setWifiPWD(password); r) {
						info("password updated");
						updated = true;
					} else {
						error("password update fail");
					}
				}
			}
			
			context.write(resultMsg("settings-set", pack.taskId, true));
			
			if (updated) {
				if (auto ret = closureCtx.notification.notify(settingsSetNotify(closureCtx.packetCounter())); !ret) {
					error("unable send notification (settingsSetNotify)", ret.code());
				}
			}
			
			return ESP_OK;			
		} else if (rawMessage.starts_with("repeat:")) { 
			
			debugIf(LOG_MESSAGES, "repeat:");
			
			auto pack = unpackMsg(rawMessage);
			
			if (!pack.success) {
				debugIf(LOG_MESSAGES, "general fail");
				return ESP_FAIL;
			}
			
			auto repeatTask = unpackKbRepeatPack(pack.body);
			
			if (!(repeatTask.success && repeatTask.valid)) {
				errorIf(LOG_MESSAGES, "repeatTask fail", 
						 repeatTask.success, 
					" ", repeatTask.valid, 
					" ", repeatTask.validIntervalMS, 
					" ", repeatTask.validRepeat,
					" ", repeatTask.validActionType,
					" ", repeatTask.validActionId,
					" ", repeatTask.validInput
				);
				context.write(resultMsg("repeat", pack.taskId, false));
				return ESP_OK;
			}
			
			bool status = false;
			std::string taskId = "repeat-";
			taskId += repeatTask.pack.actionId;
			std::string combination(repeatTask.pack.input);
			
			if (repeatTask.pack.actionType == "switched-on") {
				status = closureCtx.tailOp.schedule(taskId, [&closureCtx, combination]() -> void {
					info("schedule delayed kb press", combination, " ", esp_timer_get_time() / 1000);
					auto parser = parsers::keyboard();
					if (parser.parse(std::string_view(combination))) {
						auto kbResult = parser.writeTo(closureCtx.keyboard);
					} else {
						error("unable process repeat", combination);
					}
				}, repeatTask.intervalMS, repeatTask.repeat);
				info("scheduled task", taskId, " ", status, " ", repeatTask.intervalMS, " ", repeatTask.repeat);
								
			} else {
				status = closureCtx.tailOp.unschedule(taskId);
				info("unscheduled task", taskId, " ", status);
			}
			
#ifdef _NOTIFICATION_ENABLE
			if (repeatTask.pack.hasAction && status) {
				closureCtx.ctrl.nextState(std::string(repeatTask.pack.actionId), std::string(repeatTask.pack.actionType));
				try {
					auto clientId = closureCtx.sockets.get(longSocks);
					//todo check chat kbNotify move is performed
					if (auto ret = closureCtx.notification.notifyExept(kbNotify(closureCtx.packetCounter(), repeatTask.pack.actionId, repeatTask.pack.actionType), clientId); !ret) {
						error("unable send notification (kba-repeat)", ret.code());
					}
				} catch (not_found& e) {
					error("unable to send kb notify (repeat)", e.what());
				}
			} else {
				debug("no action data", pack.taskId);
			}
#endif
				
			context.write(resultMsg("repeat", pack.taskId, status));
			
			return ESP_OK;
		}
		
		error("rcv undefined msg type", rawMessage);
				
		return ESP_OK;
		
	})); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /socks", status.code());
	}
	
	std::cout << "starting of webServer complete" << std::endl;
	
/*	tailOp.schedule("kbStatus1", []() -> void {
		debug("scheduler test 10sec", esp_timer_get_time() / 1000);
	}, 10000, 100);*/
	
	
	//kb.onLedStatusChange(ledStatusChange(closureCtx.ctrl, closureCtx.notification, closureCtx.packetCounter, closureCtx.tailOp));
	kb.onLedStatusChange(ledStatusChange(closureCtx.ctrl, closureCtx.notification, closureCtx.packetCounter));

	tailOp.begin();
	
	
	//todo fix schedule faild before .begin
/*	tailOp.schedule("test", [&joy, &randomCtx]() -> void {
		debug("scheduler joystick test", esp_timer_get_time() / 1000);
		static int axis = 0;
		if (axis > (int)hid::joystick::axis::THROTTLE) {
			axis = 0;
		}
		joy.axis(axis++, std::min(std::max((uint16_t)(randomCtx.distribution(randomCtx.generator) * 1024 + 1024), (uint16_t)0), (uint16_t)2048));
	}, 250, 1000);*/

	
	info("setup done, app now operational");
	
	while  (true) {
		if (WIFI_MODE == wifi_mode_t::WIFI_MODE_STA && wifi::status() != wifi::status_e::CONNECTED) {
			storage persistence = storage();
			while (wifi::status() != wifi::status_e::CONNECTED) {
				info("try to reconnect");
				if (!wifi::connect(persistence.getWifiSSID(), persistence.getWifiPWD())) {
					vTaskDelay(pdMS_TO_TICKS(delay));
					delay = std::min(delay+DELAY_INCRIMENT, DELAY_MAX);
					error("reconnect faild");
				} else {
					delay = DELAY_INITAL;
					info("reconnect success");
				}
			}
		} else {
			vTaskDelay(pdMS_TO_TICKS(500));
		}
	}
}
