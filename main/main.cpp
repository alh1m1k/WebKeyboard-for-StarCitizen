#include "generated.h"

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
#if HTTP_USE_HTTPS
#include "http/secure.h"
#else
#include "http/server.h"
#endif
#include "socket/handler.h"
#include "sessionManager.h"

#include "resource/file.h"
#include "resource/shortcut.h"

#include "ctrlmap.h"
#include "make.h"
#include "notification.h"
#include "parsers/keyboard.h"
#include "scheduler.h"
#include "storage.h"
#include "wsproto.h"
#include "cache.h"
#include "filter.h"
#include "resourceChecksum.h"

#ifdef WIFI_AP_DNS
#include "dns/server.h"
#endif

#include "resultStream.h" //need for cout << result



extern "C" {
	void app_main(void);
}

#if HTTP_CACHE_USE_ETAG
	auto defaultCacheControl = eTagHandler;
#else
	auto defaultCacheControl = dummyHandler;
#endif

//http::resource::file will be created and stored in hello_js_memory_file
decl_web_resource(widget_js, 	http::resource::endings::TEXT, 		"widget.js", 		http::contentType::JS, 		defaultCacheControl);
decl_web_resource(overlay_js, 	http::resource::endings::TEXT, 		"overlay.js", 		http::contentType::JS,  	defaultCacheControl);
decl_web_resource(lib_js, 		http::resource::endings::TEXT, 		"lib.js", 			http::contentType::JS,  	defaultCacheControl);
decl_web_resource(index_html, 	http::resource::endings::TEXT, 		"index.html", 		http::contentType::HTML,  	defaultCacheControl);
decl_web_resource(favicon_ico, 	http::resource::endings::BINARY, 	"favicon.ico", 		"image/x-icon"	,  			defaultCacheControl);
decl_web_resource(index_js, 	http::resource::endings::TEXT, 		"index.js", 		http::contentType::JS,  	defaultCacheControl);
decl_web_resource(index_css,	http::resource::endings::TEXT, 		"index.css", 		http::contentType::CSS,  	defaultCacheControl);
decl_web_resource(symbols_svg,	http::resource::endings::TEXT, 		"symbols.svg", 		http::contentType::SVG,  	defaultCacheControl);

#if EMBED_CAPTIVE
decl_web_resource(captive_html,	http::resource::endings::TEXT, 		"captive.html", 	http::contentType::HTML,    filterCaptiveAcceptHtmlHandler);
#endif

#if EMBED_CERT
decl_memory_file(cert_pem, 	   http::resource::endings::TEXT);
decl_memory_file(privkey_pem,  http::resource::endings::TEXT);
#elif HTTP_USE_HTTPS
auto& cert_pem_memory_file 		= http::resource::nofile;
auto& privkey_pem_memory_file 	= http::resource::nofile;
#endif
#if EMBED_CACERT
decl_memory_file(cacert_pem,  http::resource::endings::TEXT);
#elif HTTP_USE_HTTPS
auto& cacert_pem_memory_file = http::resource::nofile;
#endif


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

#if (WIFI_AP_DNS_CAPTIVE)
static_assert(WIFI_AP_DNS, "WIFI_AP_DNS");
static_assert(!HTTP_USE_HTTPS, "!HTTP_USE_HTTPS");
static_assert(strings_equal(CAPTIVE_PORTAL_BACK_URL, WIFI_AP_DNS_DOMAIN), "PORTAL_BACK_URL != WIFI_AP_DNS_DOMAIN");
static_assert(EMBED_CAPTIVE, "Using captive without captive portal make no sense. Embed captive portal by adding flag -DEMBED_CAPTIVE=ON or disable captive");
#endif

#if (WIFI_AP_DNS)
static_assert(WIFI_MODE == WIFI_MODE_AP);
#endif

#if (HTTP_USE_HTTPS)
static_assert(!WIFI_AP_DNS_CAPTIVE);
static_assert(EMBED_CERT, "Using https without cert make no sense. Embed cert by adding flag -DEMBED_CERT=ON or disable https");
#endif

#if (HTTP_USE_HTTPS && EMBED_CERT && !EMBED_CACERT)
#warning "cacert not provided, self-signed cert may be ignored by the browser"
#endif

#if HTTP_CACHE_USE_ETAG
static_assert(RESOURCE_CHECKSUM, "Etag cache engine require checksum data. Embed checksum data by adding flag -DRESOURCE_CHECKSUM=ON or disable etag");
#endif

void app_main(void)
{


	debugIf(LOG_HTTPD_HEAP, "app_main ", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

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

	debugIf(LOG_HTTPD_HEAP, "app_main wifi", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	if (auto wifiStatus = wifi::init(); !wifiStatus) {
		trap("fail to init wifi", wifiStatus.code());
	}

	if (auto statusCode = wifi::mode(WIFI_MODE); !statusCode) {
		trap("unable setup mode", statusCode.code());
	}

	debugIf(LOG_HTTPD_HEAP, "app_main storage", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	static std::string persistenceSign;
	static std::string bootingSign;

	{

		storage persistence = storage();

		if constexpr (WIFI_MODE == wifi_mode_t::WIFI_MODE_STA) {
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

		if (persistenceSign = persistence.getStorageSign(); persistenceSign.empty()) {
			persistenceSign = randomString(32);
			infoIf(LOG_ENTROPY, "generate new storage sign", persistenceSign.c_str());
			if (auto status = persistence.setStorageSign(persistenceSign); status) {
				debug("new storage sign is set");
			} else {
				error("fail to set new storage sign", status.code());
			}
		} else {
			infoIf(LOG_ENTROPY, "storage sign", persistenceSign.c_str());
		}

	}

	{
		bootingSign = randomString(32);
	}

	debugIf(LOG_HTTPD_HEAP, "app_main dns", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());


#ifdef WIFI_AP_DNS
	std::cout << "starting dns" << std::endl;
	if (auto status = dns::server::begin(); !status) {
		trap("fail 2 setup dns", status.code());
	}
#endif

	debugIf(LOG_HTTPD_HEAP, "app_main rnd", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "starting rnd generator " << std::endl;

	struct  {
		hwrandom<uint32_t> generator;
		std::normal_distribution<float> distribution;
	} static randomCtx = {
		.generator 		= {},
		.distribution 	= std::normal_distribution<float>(0, 1.0),
	};

	debugIf(LOG_HTTPD_HEAP, "app_main usb", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "starting usb Stack " << std::endl;
	hid::UsbDevice = std::make_unique<hid::UsbDeviceImpl>();

	debugIf(LOG_HTTPD_HEAP, "app_main kb", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "starting keyboard " << std::endl;
	static auto kb = hid::keyboard();

	if (!kb.install()) {
		trap("fail 2 setup keyboard", ESP_FAIL);
	}
	kb.entroySource([]() -> float { return randomCtx.distribution(randomCtx.generator); });

	debugIf(LOG_HTTPD_HEAP, "app_main joy", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "starting joystick " << std::endl;
	static auto joy = hid::joystick();

	if (!joy.install()) {
		trap("fail 2 setup joystick", ESP_FAIL);
	}

	debugIf(LOG_HTTPD_HEAP, "app_main driver", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "installing usb driver" << std::endl;
	if (!hid::UsbDevice->install()) { //mustbe called after all usb device
		trap("fail 2 install usb driver", ESP_FAIL);
	}
	std::cout << "starting of usb stack complete" << std::endl;

	debugIf(LOG_HTTPD_HEAP, "app_main web", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

	std::cout << "starting webServer " << std::endl;
#if HTTP_USE_HTTPS
	 static auto webServer = https::server{};
	 webServer.attachSessions<sessionManager>(persistenceSign);
	 if (auto status = webServer.begin(HTTP_HTTPS_PORT, cert_pem_memory_file, privkey_pem_memory_file, cacert_pem_memory_file); !status) {
		 trap("fail 2 setup webServer", status.code());
	 }
#else
	 static auto webServer = http::server{};
	 webServer.attachSessions<sessionManager>(persistenceSign);
	 if (auto status = webServer.begin(HTTP_PORT); !status) {
		 trap("fail 2 setup webServer", status.code());
	 }
#endif

	resBool status = ESP_OK;
	debugIf(LOG_HTTPD_HEAP, "app_main web handlers", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

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

#if (WIFI_AP_DNS_CAPTIVE && EMBED_CAPTIVE)
		webServer.captiveHandler(captive_html_memory_file);
#endif

	if (status = webServer.addHandler("/leave", httpd_method_t::HTTP_POST,
	  [](http::request& req, http::response& response, http::server& serv)-> http::server::handler_res_type {
		if (auto absSess = req.getSession(); absSess != nullptr) {
			if (webServer.getSessions()->close(absSess->sid())) {
				return ESP_OK;
			}
		}
	    return ESP_FAIL;
	}); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /leave", 	status.code());
	}

	if (status = webServer.addHandler("/renew", httpd_method_t::HTTP_POST,
	  [](http::request& req, http::response& response, http::server& serv)-> http::server::handler_res_type {
		  return ESP_OK;  //rest will handle server
	  }); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /renew", 	status.code());
	}


	debugIf(LOG_HTTPD_HEAP, "app_main proto", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

    static auto  kbMessageParser    = typename wsproto::kb_parser_type();
	static auto  packetCounter 	 	= typename wsproto::packet_seq_generator_type();
	static auto  ctrl               = typename wsproto::ctrl_map_type();
	static auto  tailScheduler		= typename wsproto::scheduler_type();

    //this is little ugly but save until we not close server forcely
    auto& sessions     = *(sessionManager*)webServer.getSessions().get();
	static auto notifications = typename wsproto::notifications_type(webServer.native(), sessions);

	if (status = webServer.addHandler("/socks", httpd_method_t::HTTP_GET, http::socket::handler(
        wsproto(
            kbMessageParser,
            kb,
            joy,
            sessions,
            notifications,
            packetCounter,
            ctrl,
            tailScheduler,
            randomCtx.generator,
            persistenceSign,
            bootingSign
        )
    )); !status) {
		webServer.end();
		trap("fail 2 setup webServer path /socks", status.code());
	}
	
	std::cout << "starting of webServer complete" << std::endl;

	//kb.onLedStatusChange(ledStatusChange(ctrl, notifications, packetCounter, tailOp));
	kb.onLedStatusChange(ledStatusChange(ctrl, notifications, packetCounter));

	debugIf(LOG_HTTPD_HEAP, "app_main scheduler.begin()", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());

    tailScheduler.begin();

    tailScheduler.schedule("server gc", []() -> void {
        infoIf(LOG_SERVER_GC, "system gc start");
		webServer.collect();
		ctrl.collect();
		infoIf(LOG_SERVER_GC, "system gc end");
    }, 11000, -1);

	if constexpr (SOCKET_KEEP_ALIVE_TIMEOUT > 0) {
		tailScheduler.schedule("socket gc", []() -> void {
			//no better solution for now
			infoIf(LOG_SERVER_GC, "socket gc start");
			auto timestamp = esp_timer_get_time();
			webServer.getSessions()->walk([&timestamp](sessionManager::session_ptr_type& sessPrt, size_t index) -> bool {
				if (auto sess = pointer_cast<session>(sessPrt); sess != nullptr) {
					if (auto lastActiveAt = (int64_t)sess->read()->heartbeatAtMS * 1000;
						timestamp - lastActiveAt > ((int64_t)SOCKET_KEEP_ALIVE_TIMEOUT * 1000000)
					) {
						if (auto& sock = sess->getWebSocket(); sock != http::socket::noAsyncSocket) {
							debug("find inactive socket", sess->sid(), " last active at: ", lastActiveAt, " close it now");
							sock.close();
						}
					}
				}
				return true;
			});
			infoIf(LOG_SERVER_GC, "socket gc end");
		}, SOCKET_KEEP_ALIVE_TIMEOUT * 1000 / 3 - 1, -1);
	}

	
	//todo fix schedule faild before .begin
/*	tailOp.schedule("test", [&joy, &randomCtx]() -> void {
		debug("scheduler joystick test", esp_timer_get_time() / 1000);
		static int axis = 0;
		if (axis > (int)hid::joystick::axis::THROTTLE) {
			axis = 0;
		}
		joy.axis(axis++, std::min(std::max((uint16_t)(randomCtx.distribution(randomCtx.generator) * 1024 + 1024), (uint16_t)0), (uint16_t)2048));
	}, 250, 1000);*/

	debugIf(LOG_HTTPD_HEAP, "app_main done", esp_get_minimum_free_heap_size(), " ", esp_get_free_internal_heap_size());
	
	info("setup done, app now operational");
	
	while (true) {
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
