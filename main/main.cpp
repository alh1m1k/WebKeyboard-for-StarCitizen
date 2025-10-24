#include "generated.h"

#include <algorithm>
#include <cstdlib>
#include <future>
#include <string>
#include <stdexcept>
#include <string_view>
#include <random>


#include "esp_err.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include <esp_wifi_types.h>
#include "esp_timer.h"


#include "util.h"
#include "result.h"

#include "wifi/wifi.h"
#include "hid/usbDevice.h"
#include "hid/usbDeviceImpl.h"
#include "hid/keyboard.h"
#include "hid/joystick.h"
#include "driver/gpio.h"
#include "hwrandom.h"
#include "ledStatusChange.h"

#include "contentType.h"
#include "http/server.h"
#include "socket/handler.h"
#include "resource/memory/shortcut.h"
#include "sessionManager.h"
#include "resource/memory/file.h"

#include "ctrlmap.h"
#include "scheduler.h"
#include "storage.h"
#include "notification.h"
#include "parsers/keyboard.h"
#include "wsproto.h"

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
    webServer.attachSessions<sessionManager>();
	
	if (auto status = webServer.begin(80); !status) {
		trap("fail 2 setup webServer", status.code());
	}
				
	resBool status = ESP_OK;

	if (status = webServer.addHandler("/", httpd_method_t::HTTP_GET, index_html_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /", 				status.code());
	}
	if (status = webServer.addHandler("/lib.js", 		httpd_method_t::HTTP_GET, lib_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /lib.js", 		status.code());
	}
	if (status = webServer.addHandler("/widget.js", 	httpd_method_t::HTTP_GET, widget_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /widget.js", 	status.code());
	}
	if (status = webServer.addHandler("/overlay.js", 	httpd_method_t::HTTP_GET, overlay_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /overlay.js", 	status.code());
	}
	if (status = webServer.addHandler("/favicon.ico", httpd_method_t::HTTP_GET, favicon_ico_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /favicon.ico", 	status.code());
	}
	if (status = webServer.addHandler("/index.js", 	httpd_method_t::HTTP_GET, index_js_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /index.js", 		status.code());
	}
	if (status = webServer.addHandler("/index.css", 	httpd_method_t::HTTP_GET, index_css_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /index.css", 		status.code());
	}
	if (status = webServer.addHandler("/symbols.svg", httpd_method_t::HTTP_GET, symbols_svg_memory_file); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /symbols.svg", 	status.code());
	}


/*	result = webServer.addHandler("/leave", httpd_method_t::HTTP_POST, [&closureCtx](http::request& req, http::response& resp)-> http::handlerRes {
		
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

    auto  kbMessageParser    = typename wsproto::kb_parser_type();
    auto  packetCounter 	 = typename wsproto::packet_seq_generator_type();
    auto  ctrl               = typename wsproto::ctrl_map_type();
    auto  tailScheduler		 = typename wsproto::scheduler_type();

    //this is little ugly but save until we not close server forcely
    auto& sessions     = *(sessionManager*)webServer.getSessions().get();
    auto notifications = typename wsproto::notifications_type(webServer.native(), sessions);

	if (status = webServer.addHandler("/socks", httpd_method_t::HTTP_GET, http::socket::handler(
        wsproto(
            kbMessageParser,
            kb,
            joy,
            sessions,
            notifications,
            packetCounter,
            ctrl,
            persistanceSign,
            bootingSign,
            tailScheduler
        )
    )); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /socks", status.code());
	}
	
	std::cout << "starting of webServer complete" << std::endl;
	
/*	tailOp.schedule("kbStatus1", []() -> void {
		debug("scheduler test 10sec", esp_timer_get_time() / 1000);
	}, 10000, 100);*/
	
	
	//kb.onLedStatusChange(ledStatusChange(ctrl, notifications, packetCounter, tailOp));
	kb.onLedStatusChange(ledStatusChange(ctrl, notifications, packetCounter));

    tailScheduler.begin();
	
	
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
