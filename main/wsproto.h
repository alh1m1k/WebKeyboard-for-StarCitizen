#pragma once

#include <string>

#include "esp_http_server.h"

#include "generated.h"
#include "util.h"
#include "http/socket/socket.h"
#include "http/socket/asyncSocket.h"
#include "http/socket/context.h"
#include "parsers/keyboard.h"
#include "hid/keyboard.h"
#include "hid/joystick.h"
#include "clients.h"
#include "notification.h"
#include "ctrlmap.h"
#include "scheduler.h"
#include "packers.h"
#include "sessionManager.h"
#include "session.h"
#include "storage.h"
#include "crypto/hash/sha256Engine.h"
#include "crypto/base64Helper.h"

hid::joystick::control_writer_type& operator<<(hid::joystick::control_writer_type& stream, const std::string_view& byteStream) {
    stream.copyFrom(&byteStream[0], byteStream.size());
    return stream;
}

#define IS(flags, entity)    ((flags&entity)==entity)
#define SET(flags, entity)   (flags |= entity)
#define UNSET(flags, entity) (flags &= ~entity)
#define SWITCH(flags, entity) (flags ^= entity)

class wsproto {

	//consume almost 500-600b of stack, most of theme is hash ctx + exceptions
    static std::string base64Hash(const std::string& val) {
        auto engine = crypto::hash::sha256Engine();
        if (auto hash = engine.hash({reinterpret_cast<const uint8_t*>(val.data()),  val.size()}); hash != crypto::hash::sha256Engine::invalidHash) {
            return crypto::base64Helper::toBase64(hash);
        }
        throw std::logic_error("error in hash op"); //consume 144bytes of stack
    }

    public:

        typedef parsers::keyboard 	                                                        kb_parser_type;
        typedef hid::keyboard	                                                            keyboard_type;
        typedef hid::joystick                                                               joystick_type;
        typedef sessionManager                                                              sessions_type;
        typedef notificationManager<sessions_type> 										    notifications_type;
        typedef generator<uint32_t, true> 													packet_seq_generator_type;
        typedef scheduler<std::string> 	  												    scheduler_type;
        typedef ctrlmap<std::string, std::string> 	  										ctrl_map_type;
        typedef hwrandom<uint32_t> 	  										                entropy_generator_type;
        typedef std::string	  										                        sing_type;

        kb_parser_type& 			parser;
        keyboard_type& 			    keyboard;
        joystick_type&              joystick;
        sessions_type&              sessions;
        notifications_type&	        notifications;
        packet_seq_generator_type&  packetCounter;
        ctrl_map_type&              ctrl;
        scheduler_type&             tailScheduler;
        entropy_generator_type&     entropySource;
        sing_type&                  persistenceSign;
        sing_type&                  bootingSign;

		mutable std::atomic<uint32_t>       joystickCounter 	= 0;
		mutable int64_t 					joystickFlushAt 	= 0;



        wsproto(
                parsers::keyboard& 			    parser,
                hid::keyboard& 			        keyboard,
                hid::joystick&                  joystick,
                sessions_type&                  sessions,
                notifications_type&	            notifications,
                packet_seq_generator_type&      packetCounter,
                ctrl_map_type&                  ctrl,
                scheduler_type&                 tailScheduler,
                entropy_generator_type&         entropySource,
                sing_type&                      persistenceSign,
                sing_type&                      bootingSign
        ) :
                parser(parser),
                keyboard(keyboard),
                joystick(joystick),
                sessions(sessions),
                notifications(notifications),
                packetCounter(packetCounter),
                ctrl(ctrl),
                tailScheduler(tailScheduler),
                entropySource(entropySource),
                persistenceSign(persistenceSign),
                bootingSign(bootingSign)
        {
            sessions.notification = [proto= this](int type, sessionManager::session_ptr_type& context, void* data) -> void  {
                proto->handleEvents(type, context, data);
            };
        }

        virtual ~wsproto() { };

        wsproto(const wsproto& copy) :
                parser(copy.parser),
                keyboard(copy.keyboard),
                joystick(copy.joystick),
                sessions(copy.sessions),
                notifications(copy.notifications),
                packetCounter(copy.packetCounter),
                ctrl(copy.ctrl),
                tailScheduler(copy.tailScheduler),
                entropySource(copy.entropySource),
                persistenceSign(copy.persistenceSign),
                bootingSign(copy.bootingSign)
        {
            error("wsproto must be newer copy");
        }

        wsproto(wsproto&& move) noexcept :
            parser(move.parser),
            keyboard(move.keyboard),
            joystick(move.joystick),
            sessions(move.sessions),
            notifications(move.notifications),
            packetCounter(move.packetCounter),
            ctrl(move.ctrl),
            tailScheduler(move.tailScheduler),
            entropySource(move.entropySource),
            persistenceSign(move.persistenceSign),
            bootingSign(move.bootingSign)
        {

        }

        auto operator=(const wsproto&) = delete;

        auto operator=(wsproto&&) = delete;

        typedef std::function<resBool(http::socket::socket& remote, const http::socket::socket::message& rawMessage, const http::socket::context& ctx)> handler_type;

        inline resBool operator()(http::socket::socket& socket, const http::socket::socket::message& rawMessage, const http::socket::context& ctx) const {

            debugIf(LOG_MESSAGES, "sock rcv", rawMessage.c_str());

            auto pSession = pointer_cast<session>(ctx.getSession());
            if (pSession == nullptr) {
                error("general ws-session fail");
                return ESP_FAIL;
            }

            std::string clientName = {};
            uint32_t    clientId   = 0;
            uint32_t    flags      = 0;
            if (auto r = pSession->read(); r) {
                clientName = r->clientName;
                clientId   = r->index;
                flags      = r->flags;
            }

            if (IS(flags, (uint32_t)sessionFlags::SOCKET_CHANGE)) {
                flags = UNSET(pSession->write()->flags, (uint32_t)sessionFlags::SOCKET_CHANGE|(uint32_t)sessionFlags::AUTHODIZED);
            }

			//ping is not member of keepAlive, it is network fail discovery
			if (rawMessage.starts_with("ping:") && IS(flags, (uint32_t)sessionFlags::AUTHODIZED)) {
				auto pack = unpackMsg(rawMessage);
				if (pack.success) {
					socket.write(resultMsg("echo", pack.taskId, pack.success));
					return ESP_OK;
				} else {
					return ESP_FAIL;
				}
			}

            if (rawMessage.starts_with("auth:")) {

                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    error("auth block fail2parce");
                    return ESP_FAIL;
                }

                bool isReconnect = IS(flags, (uint32_t)sessionFlags::DISCONECTED) || IS(flags, (uint32_t)sessionFlags::AUTHODIZED);
                clientName = std::string(trim(pack.body));
                if (clientName.empty()) {
                    error("auth block fail2clientid");
                    return ESP_FAIL;
                }

                info("client is connecting", "id: ", clientId, " name: ", clientName, " pid: ", pack.taskId);

                if (auto w = pSession->write(); w) {
                    w->clientName = clientName;
                    flags = UNSET(SET(w->flags, (uint32_t)sessionFlags::AUTHODIZED), (uint32_t)sessionFlags::DISCONECTED);
                }

                debugIf(LOG_MESSAGES, "respond", pack.taskId, " ", resultMsg("auth", pack.taskId, true));
                socket.write(resultMsg("auth", pack.taskId, true));

                if (auto ret = notifications.notify(signNotify(packetCounter(), "storageSign", base64Hash(persistenceSign)), clientId); !ret) {
                    error("unable send notifications (storageSign)", ret.code());
                }

                if (auto ret = notifications.notify(signNotify(packetCounter(), "bootingSign", base64Hash(bootingSign)), clientId); !ret) {
                    error("unable send notifications (bootingSign)", ret.code());
                }

                if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, isReconnect ? 2 : 1), clientId); !ret) {
                    error("unable send notifications (auth)", ret.code());
                }
                {
                    auto guardian = ctrl.sharedGuardian(); //it leave with scope
                    for (auto it = ctrl.begin(), endIt = ctrl.end(); it != endIt; ++it) {
                        auto pair = *it;
                        notifications.notify(kbNotify(packetCounter(), pair.first, pair.second), clientId);
                    }
                }

                std::string joystickView = {};
                joystickView.resize(20);
                joystick.control().copyTo(joystickView.data(), joystickView.size());
                notifications.notify(ctrNotify(packetCounter(), joystickView), clientId, true);

                return ESP_OK;
            }

            if (!IS(flags, (uint32_t)sessionFlags::AUTHODIZED)) {
				info("force client to authorize", "id: ", clientId, " name: ", clientName, " pid: n/a");
                socket.write("authfail");
                return ESP_OK;
            }

            if (rawMessage.starts_with("leave:")) {
                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    return ESP_FAIL;
                }

				info("client is leaving", "id: ", clientId, " name: ", clientName, " pid: ", pack.taskId);

                socket.write(resultMsg("leave", pack.taskId, true));
                if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, 3), clientId); !ret) {
                    error("unable send notifications (leave)", ret.code());
                }

                sessions.close(pSession->sid());

                return ESP_OK;
            }

            pSession->keepAlive(esp_timer_get_time());

            if (rawMessage.starts_with("ctr:")) {
                //control axis request must be fixed size: 8 axis(2byte each) + buttons(32 one bit each) = 20bytes
                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    return ESP_FAIL;
                }
                if (pack.body.size() != 20) {
                    error("ctrl axis packet has invalid size: ", rawMessage.c_str(), " ", rawMessage.size(), " ", (uint)pack.body[0], " ", (uint)pack.body[1], " ", (uint)pack.body[pack.body.size()-2], " ", (uint)pack.body[pack.body.size()-1], " ",  pack.body.size(), " ", 20);
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

                auto controlStream = joystick.control();
                controlStream << pack.body;

                if (auto ret = notifications.notifyExcept(ctrNotify(packetCounter(), pack.body), clientId, true); !ret) {
                    error("unable send notifications (ctr)", ret.code());
                }

				++joystickCounter;
				if (auto time = esp_timer_get_time(); (time - joystickFlushAt >= 1000000) && joystickCounter > 1) {
					//that probably not thread safe, add spinlock
					info("client activity", "from: ", joystickFlushAt,  " to `now` processed: ", joystickCounter, " joystick event(s)");
					joystickCounter = 0;
					joystickFlushAt = time;
				}

                return ESP_OK; //no respond
            }

			info("client activity", "id: ", pSession->index(), " name: ", clientName.c_str());

			if (rawMessage.starts_with("kb:")) {
                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    return ESP_FAIL;
                }
                auto kbPack = unpackKb(pack.body);
                debugIf(LOG_MESSAGES, "kb payload", pack.body, " ", kbPack.hasAction, " ", kbPack.input, " ", kbPack.actionId, " ", kbPack.actionType);
                if (kbPack.hasInput) {
                    if (!parser.parse(kbPack.input)) {
                        socket.write(resultMsg("kb", pack.taskId, false));
                    } else {
                        auto kbResult = parser.writeTo(keyboard);
                        socket.write(resultMsg("kb", pack.taskId, true));
                        if (kbResult) {
                            debugIf(LOG_MESSAGES, "kb schedule ok", std::get<uint32_t>(kbResult));
                        } else {
                            error("kb schedule fail", kbResult.code());
                        }
                    }
                } else {
                    socket.write(resultMsg("kb", pack.taskId, true)); //case to update of switchoff state to controls
                }


                if (kbPack.hasAction) {
                    ctrl.nextState(std::string(kbPack.actionId), std::string(kbPack.actionType));
                    try {
                        if (auto ret = notifications.notifyExcept(kbNotify(packetCounter(), kbPack.actionId, kbPack.actionType), clientId); !ret) {
                            error("unable send notifications (kba)", ret.code());
                        }
                    } catch (not_found& e) {
                        error("unable to send kb notify", e.what());
                    }
                } else {
                    debug("no action data", pack.taskId);
                }

                return ESP_OK;

            } else if (rawMessage.starts_with("settings-get:")) {

                auto pack = unpackMsg(rawMessage);

                if (!pack.success) {
                    return ESP_FAIL;
                }

                auto persistence = storage();

                socket.write(settingsGetMsg(pack.taskId, persistence.getWifiSSID(), persistence.getWifiAuth()));

                return ESP_OK;

            } else if (rawMessage.starts_with("settings-set:")) {

                auto pack = unpackMsg(rawMessage);

                if (!pack.success) {
                    return ESP_FAIL;
                }


                auto settings 	= unpackSettings(rawMessage);
                bool updated 	= false;

                if (!(settings.success && settings.valid)) {
                    socket.write(resultMsg("settings-set", pack.taskId, false));
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

                socket.write(resultMsg("settings-set", pack.taskId, true));

                if (updated) {
                    if (auto ret = notifications.notify(settingsSetNotify(packetCounter())); !ret) {
                        error("unable send notifications (settingsSetNotify)", ret.code());
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
                    socket.write(resultMsg("repeat", pack.taskId, false));
                    return ESP_OK;
                }

                bool status = false;
                std::string taskId = "repeat-";
                taskId += repeatTask.pack.actionId;
                std::string combination(repeatTask.pack.input);

                if (repeatTask.pack.actionType == "switched-on") {
                    status = tailScheduler.schedule(taskId, [this, combination]() -> void {
                        info("schedule delayed kb press", combination, " ", esp_timer_get_time() / 1000);
                        auto kbParser = parsers::keyboard();
                        if (kbParser.parse(std::string_view(combination))) {
                            auto kbResult = kbParser.writeTo(keyboard);
                        } else {
                            error("unable process repeat", combination);
                        }
                    }, repeatTask.intervalMS, repeatTask.repeat);
                    info("scheduled task", taskId, " ", status, " ", repeatTask.intervalMS, " ", repeatTask.repeat);

                } else {
                    status = tailScheduler.unschedule(taskId);
                    info("unscheduled task", taskId, " ", status);
                }

                if (repeatTask.pack.hasAction && status) {
                    ctrl.nextState(std::string(repeatTask.pack.actionId), std::string(repeatTask.pack.actionType));
                    try {
                        if (auto ret = notifications.notifyExcept(kbNotify(packetCounter(), repeatTask.pack.actionId, repeatTask.pack.actionType), clientId); !ret) {
                            error("unable send notifications (kba-repeat)", ret.code());
                        }
                    } catch (not_found& e) {
                        error("unable to send kb notify (repeat)", e.what());
                    }
                } else {
                    debug("no action data", pack.taskId);
                }

                socket.write(resultMsg("repeat", pack.taskId, status));

                return ESP_OK;
            }

            error("rcv undefined msg type", rawMessage);

            return ESP_OK;
            
        }


/*
 *      This code expects the manager to be unblocked when the event occurs. This is currently ensured by all manager methods;
        if this ever changes, deferred execution via the scheduler is necessary.
*/
        void handleEvents(int type, sessionManager::session_ptr_type& context, void* data) {
            using managerT = sessionManager::sessionNotification;
            using sessionT = session::sessionNotification;
            auto sess = pointer_cast<session>(context);
            std::string clientName;
            uint32_t    clientId;
            if (auto r = sess->read(); r) {
                clientName  = r->clientName;
                clientId    = r->index;
            }

            switch (type) {
                case (int)managerT::OPEN:
                    infoIf(LOG_SESSION_EVT, "evt session open", sess->sid().c_str(), " reason: ", ((sessionManager::note_open_type*)data)->reason);
                    break;
                case (int)managerT::CLOSE:
                    infoIf(LOG_SESSION_EVT, "evt session close", sess->sid().c_str(), " reason: ", *(uint32_t*)data);
                    if (sess->getWebSocket() != http::socket::noAsyncSocket) {
                        debug("have non empty sock");
                        //we not junk session (read about it in notification.h)
                        if (!IS(sess->read()->flags, (uint32_t)sessionFlags::DISCONECTED)) {
                            debug("not disconnected yet");
                            if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, 3), clientId); !ret) {
                                error("unable send notifications (ctr)", ret.code());
                            }
                            auto ws = sess->getWebSocket();
                            info("closing now");
                            if (httpd_sess_trigger_close(ws.serverHandler(), ws.native()) != ESP_OK) {
                                error("unable to close socket");
                            }
                        } else {
                            debug("already disconnected yet");
                        }
                    }
                    break;
                case (int)sessionT::WS_OPEN:
                    infoIf(LOG_SESSION_EVT, "evt session ws open", sess->sid().c_str(), " name: ", sess->read()->clientName.c_str());
                    break;
                case (int)sessionT::WS_CHANGE:
                    infoIf(LOG_SESSION_EVT, "evt session ws change", sess->sid().c_str(), " name: ", sess->read()->clientName.c_str());
                    break;
                case (int)sessionT::WS_CLOSE:
                    infoIf(LOG_SESSION_EVT, "evt session ws close", sess->sid().c_str(), " name: ", sess->read()->clientName.c_str());
                    SET(sess->write()->flags, (uint32_t)sessionFlags::DISCONECTED);
                    if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, 3), clientId); !ret) {
                        error("unable send notifications (ctr)", ret.code());
                    }
                    break;
                default:
                    error("sessions.notification: undefined event", type, " ", data);
            }
        }

};
