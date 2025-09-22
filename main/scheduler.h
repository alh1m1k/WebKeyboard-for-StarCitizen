#pragma once

#include "generated.h"

#include "esp_timer.h"
#include "generated.h"
#include "projdefs.h"
#include "util.h"
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/_stdint.h>
#include "../exception/bad_api_call.h"

template<typename TAG, typename TASK = std::function<void()>>
class scheduler {
	
	public: typedef TASK task_type;

    protected:
        
    	template<class CALLABLE>
    	class threadHandler {
			TaskHandle_t  hndl = NULL;
			public:
				threadHandler(CALLABLE* owner, const char* pcName, const configSTACK_DEPTH_TYPE usStackDepth = 4096, const UBaseType_t uxPriority = 10) {
					debugIf(LOG_SCHEDULER, "threadHandler::threadHandler", pcName);
					auto status = xTaskCreate([](void* o){
						while (true) {
							(*static_cast<CALLABLE*>(o))();
						}
					}, pcName, usStackDepth, (void*)owner, uxPriority, &hndl);
					if (status != pdPASS) {
						throw bad_api_call("threadHandler::threadHandler", ESP_FAIL);
					}
				}
				~threadHandler() {
					debugIf(LOG_SCHEDULER, "threadHandler::~threadHandler");
					vTaskDelete(hndl);
				}
				threadHandler(threadHandler&) 				= delete;
				threadHandler& operator=(threadHandler&) 	= delete;
				inline TaskHandle_t handler() {
					return hndl;
				}
		};
	
		struct unit {
			int64_t     invoked;
			task_type 	handler;
			int32_t 	repeat;
			int64_t     intervalUS;
			TAG         tagId;
			bool 		erase = false;
		};
		
		struct next_unit {
			int64_t 	timeleft;
			typename 	std::vector<unit>::iterator it; 
		};
		
		struct finder {
			typename 	std::vector<unit>&          vect;
			typename 	std::vector<unit>::iterator it; 
		};
	
		std::unique_ptr<threadHandler<scheduler>> thread = nullptr;
		std::vector<unit> units = {};
		std::vector<unit> unitsShadow = {};
		std::mutex mutex = {};
		
		finder find(const TAG& tagId) {
			
			if (auto it = std::find_if(unitsShadow.begin(), unitsShadow.end(), [&tagId](const unit& u) -> bool {
				return u.tagId == tagId && u.erase == false;
			}); it != unitsShadow.end()) {
				return {unitsShadow, it};
			}
			
			return {units, std::find_if(units.begin(), units.end(), [&tagId](const unit& u) -> bool {
				return u.tagId == tagId && u.erase == false;
			})};
			
		}
		
 		next_unit next() {
			auto closest = units.begin();
			if (closest == units.end()) {
				return {0, closest};
			}
			auto time = esp_timer_get_time();
			int64_t closestTimeleft = (int64_t)closest->intervalUS - (time - closest->invoked);
			for (auto it = units.begin(), end = units.end(); it != end; ++it) {
				auto timeleftIt = (int64_t)it->intervalUS - (time - it->invoked);
				if (timeleftIt < closestTimeleft) {
					closestTimeleft = timeleftIt;
					closest = it;
				}
			}
			return {
				closestTimeleft, closest
			};
		}
		
		inline void throtleUntil(int64_t prev, int64_t interval) {
			
			int64_t yeldInterval = std::max(interval - 500, (int64_t)0);
			while (esp_timer_get_time() - prev < yeldInterval) {
				taskYIELD();
			}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"
			volatile uint32_t cycles = 0;
			while (esp_timer_get_time() - prev < yeldInterval) {
				cycles++;
			}
#pragma GCC diagnostic pop

		}	
		
		inline void operator()() {
					
			next_unit nx;
						
			//if iterator exist and (time passes or timeleft less than 1 freertos tick)
			for (nx = next(); nx.it != units.end() && (nx.timeleft <= 0 || (nx.timeleft > 0 && (pdMS_TO_TICKS((TickType_t)(nx.timeleft / 1000)) == 0))); nx = next()) {
				if (nx.it->erase) {
					mutex.lock();
					eraseByReplace(units, nx.it);
					mutex.unlock();
					continue;
				}
				if constexpr(strictTimemanagment) {
					//FIXME incorrect fix me
					throtleUntil(nx.it->invoked, (int64_t)nx.it->intervalUS);
				}
				nx.it->handler();
				nx.it->invoked = esp_timer_get_time();
				if (nx.it->repeat > 0) {
					if (--nx.it->repeat == 0) {
						mutex.lock();
						eraseByReplace(units, nx.it);
						mutex.unlock();
					}
				}
			}
			
			BaseType_t rcvStatus = pdFALSE;
			uint32_t flags = 0;
			if (nx.it != units.end()) {
				//debug("scheduler() sleep", nx.timeleft, " ", (TickType_t)((nx.timeleft + 999)/ 1000), " ", pdMS_TO_TICKS((TickType_t)((nx.timeleft + 999) / 1000)));
				rcvStatus = xTaskNotifyWait(0x00, ULONG_MAX, &flags, pdMS_TO_TICKS((TickType_t)((nx.timeleft + 999) / 1000)));
			} else {
				debugIf(LOG_SCHEDULER, "scheduler() default sleep");
				rcvStatus = xTaskNotifyWait(0x00, ULONG_MAX, &flags, pdMS_TO_TICKS((TickType_t)10000));
			}
			
			if (rcvStatus == pdTRUE) {
				mutex.lock();
				debugIf(LOG_SCHEDULER, "scheduler() wakeup with new units", flags, " ", unitsShadow.size());
				units.reserve( units.size() + unitsShadow.size() ); // preallocate memory
				units.insert(units.end(), unitsShadow.begin(), unitsShadow.end());
				unitsShadow.clear();
				mutex.unlock();
			}			
		}
	
	public:
	
		static const bool strictTimemanagment = false;
	
		scheduler()	{}
		scheduler(scheduler &&) 			= default;
		scheduler &operator=(scheduler &&) 	= default; 
			
		//in case std::is_default_constructible_v<TAG> == true) ie "" for string or 0 for num... ect is reserved for using by anonymous task
		//DO NOT USE "" or 0 as TAG
		bool schedule(const TAG& tagId, const task_type& handler, const uint32_t intervalMs, const int32_t repeat = -1) {
			
			auto guardian= std::unique_lock(mutex);
			
			if (auto finder = find(tagId); finder.it != finder.vect.end()) {
				return false;
			}
			
			unitsShadow.emplace_back(esp_timer_get_time(), handler, repeat, intervalMs * 1000, tagId, false);
			
			xTaskNotify(thread->handler(), 0, eNoAction);
			
			return true;			
		}
		
		//anonymous self managed task
		bool schedule(const task_type& handler, const uint32_t intervalMs, int32_t const repeat = 1) {
			
			static_assert(std::is_default_constructible_v<TAG> == true);
			
			if (repeat <= 0) {
				return false;
			}
			
			auto guardian= std::unique_lock(mutex);
						
			//todo check TAG()
			unitsShadow.emplace_back(esp_timer_get_time(), handler, repeat, intervalMs * 1000, TAG(), false);
			
			xTaskNotify(thread->handler(), 0, eNoAction);
			
			return true;
			
		}
		
		//in case std::is_default_constructible_v<TAG> == true) unschedule must not be use to unschedule Task{} (default one)
		//to simplify impl such task are interpreted as anonymous and self managed
		bool unschedule(const TAG& tagId) {
			
			auto guardian= std::unique_lock(mutex);
			
			if (auto finder = find(tagId); finder.it == finder.vect.end()) {
				return false;
			} else {
				finder.it->erase = true;
				return true;
			}
			
		}
		
		bool exist(const TAG& tagId) {
			auto guardian = std::unique_lock(mutex);
			if (auto finder = find(tagId); finder.it != finder.vect.end()) {
				return true;
			} else {
				return false;
			}
		}
		
		
		void begin() {
			thread = std::make_unique<threadHandler<scheduler>>(this, "schedulerTask");
		}
		
		void end() {
			thread = nullptr;
		}
	
};
