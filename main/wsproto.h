#pragma once

#include <string>

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

hid::joystick::control_writer_type& operator<<(hid::joystick::control_writer_type& stream, const std::string_view& byteStream) {
    stream.copyFrom(&byteStream[0], byteStream.size());
    return stream;
}

class wsproto {

    public:

        typedef parsers::keyboard 	                                                        kb_parser_type;
        typedef hid::keyboard	                                                            keyboard_type;
        typedef hid::joystick                                                               joystick_type;
        typedef sessionManager                                                              sessions_type;
        typedef notificationManager<sessions_type> 										    notifications_type;
        typedef generator<uint32_t, true> 													packet_seq_generator_type;
        typedef scheduler<std::string> 	  												    scheduler_type;
        typedef ctrlmap<std::string, std::string> 	  										ctrl_map_type;
        typedef std::string	  										                        sing_type;

        kb_parser_type& 			parser;
        keyboard_type& 			    keyboard;
        joystick_type&              joystick;
        sessions_type&              sessions;
        notifications_type&	        notifications;
        packet_seq_generator_type&  packetCounter;
        ctrl_map_type&              ctrl;
        sing_type&                  persistenceSign;
        sing_type&                  bootingSign;
        scheduler_type&             tailScheduler;

        wsproto(
                parsers::keyboard& 			    parser,
                hid::keyboard& 			        keyboard,
                hid::joystick&                  joystick,
                sessions_type&                  sessions,
                notifications_type&	            notifications,
                packet_seq_generator_type&      packetCounter,
                ctrl_map_type&                  ctrl,
                sing_type&                      persistenceSign,
                sing_type&                      bootingSign,
                scheduler_type&                 tailScheduler
        ) :
                parser(parser),
                keyboard(keyboard),
                joystick(joystick),
                sessions(sessions),
                notifications(notifications),
                packetCounter(packetCounter),
                ctrl(ctrl),
                persistenceSign(persistenceSign),
                bootingSign(bootingSign),
                tailScheduler(tailScheduler)
        {

        }

        typedef std::function<resBool(http::socket::socket& remote, const http::socket::socket::message& rawMessage, const http::socket::context& ctx)> handler_type;

        inline resBool operator()(http::socket::socket& socket, const http::socket::socket::message& rawMessage, const http::socket::context& ctx) {

            debugIf(LOG_MESSAGES, "sock rcv", rawMessage);

            auto pSession = pointer_cast<session>(ctx.getSession());
            if (pSession == nullptr) {
                error("general ws-session fail");
                return ESP_FAIL;
            }

            std::string clientName = {};
            uint32_t    clientId   = 0;
            if (auto reader = pSession->read(); reader) {
                clientName = reader->clientName;
                clientId   = pSession->index();
            }

            info("session credits", clientId, " ", clientName);


            if (rawMessage.starts_with("auth:")) {

                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    error("auth block fail2parce");
                    return ESP_FAIL;
                }


                clientName = std::string(trim(pack.body));
                if (!clientName.size()) {
                    error("auth block fail2clientid");
                    return ESP_FAIL;
                }

                info("client is connecting", clientName, " ", pack.taskId);

                pSession->write()->clientName = clientName;
                pSession->authorize();

                debugIf(LOG_MESSAGES, "respond", pack.taskId, " ", resultMsg("auth", pack.taskId, true));
                socket.write(resultMsg("auth", pack.taskId, true));

                if (auto ret = notifications.notify(signNotify(packetCounter(), "storageSign", persistenceSign), clientId); !ret) {
                    error("unable send notifications (storageSign)", ret.code());
                }
                if (auto ret = notifications.notify(signNotify(packetCounter(), "bootingSign", bootingSign), clientId); !ret) {
                    error("unable send notifications (bootingSign)", ret.code());
                }
                if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, true), clientId); !ret) {
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

            if (!pSession->isAuthorized()) {
                info("authfail");
                socket.write("authfail");
                return ESP_OK;
            }

            if (rawMessage.starts_with("leave:")) {
                auto pack = unpackMsg(rawMessage);
                if (!pack.success) {
                    return ESP_FAIL;
                }

                info("client is leaving", pack.taskId, " ", clientName);

                socket.write(resultMsg("leave", pack.taskId, true));
                if (auto ret = notifications.notifyExcept(connectedNotify(packetCounter(), clientName, false), clientId); !ret) {
                    error("unable send notifications (leave)", ret.code());
                }

                sessions.close(pSession->sid());

                return ESP_OK;
            }

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

                auto controlStream = joystick.control();
                controlStream << pack.body;

                if (auto ret = notifications.notifyExcept(ctrNotify(packetCounter(), pack.body), clientId, true); !ret) {
                    error("unable send notifications (ctr)", ret.code());
                }

                return ESP_OK; //no respond
            }

            if (rawMessage.starts_with("axi:")) {
                //custom axis request must be fixed size: 2byte + buttons (32 one bit each) + 4byte header = 10 bytes
            }


            if (rawMessage.starts_with("ping:")) {
                auto pack = unpackMsg(rawMessage);
                if (pack.success) {
                    socket.write(resultMsg("ping", pack.taskId, pack.success));
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

};
