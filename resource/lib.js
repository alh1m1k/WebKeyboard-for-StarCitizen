"use strict";

const SocketInit        = 0;
const SocketOpen        = 1;
const SocketAuthorized  = 2;
const SocketClose       = 3;
const SocketError       = 4;
const SocketReconnect   = 5;
const SocketAuthorize   = 6;

const quarterFingerPixels = ((2.75 - 2.25) / 2 + 2.25) / (Math.PI * 2) / 2 * ((window.devicePixelRatio || 1.0) * 96);

const defaultLinkPattern =  { pattern: "^(\\/[\\w\\-]*)(\\/[\\w\\-]+)*\\/?" };

('hasOwn' in Object) || (Object.hasOwn = Object.call.bind(Object.hasOwnProperty));

//('vibrate' in navigator) || (navigator.vibrate = function (){ console.error("no vibrate impl") });

('fromEntries' in Object) || (Object.fromEntries = (entries) => {
    let result = {}
    for (let [index, value] of entries) {
        //console.log("FormData", index, value);
        result[index] = value;
    }
    return result;
});

if (!Object.hasOwn(Promise, "withResolvers")) {
    Promise.withResolvers = () => {
        let resolve, reject;
        const promise = new Promise((res, rej) => {
            resolve = res;
            reject = rej;
        });
        return {promise, resolve, reject};
    }
}

if (!DOMTokenList.prototype.replace) {
    DOMTokenList.prototype.replace = function(oldToken, newToken) {
        // If the oldToken doesn't exist, return false without adding the new token.
        if (!this.contains(oldToken)) {
            return false;
        }
        // Remove the old token and add the new token.
        this.remove(oldToken);
        this.add(newToken);
        return true; // Indicate success
    };
}


function normalizeColor(color) {
    if (color.startsWith("rgb")) {
        return color;
    } else if (color === "white") {
        return "rgb(255, 255, 255)";
    } else if (color === "black") {
        return "rgb(0, 0, 0)";
    } else if (color.startsWith("#")) {
        const hexColor = +('0x' + color.slice(1).replace(color.length < 5 && /./g, '$&$&'));
        return `rgb(${hexColor >> 16}, ${(hexColor >> 8) & 255}, ${hexColor & 255})`;
    } else {
        if (!normalizeColor.converter) {
            normalizeColor.converter = document.createElement("div");
            normalizeColor.converter.style.display = "none";
            document.body.appendChild(normalizeColor.converter);
            queueMicrotask(() => {
                normalizeColor.converter.remove();
                normalizeColor.converter = null;
            });
        }
        normalizeColor.converter.style.color = color;
        return getComputedStyle(normalizeColor.converter).color;
    }
}

function isLightColor(color) {
    let r, g, b;

    const rgb = normalizeColor(color).match(
        /^rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+(?:\.\d+)?))?\)$/
    );

    r = rgb[1];
    g = rgb[2];
    b = rgb[3];

    // HSP equation from http://alienryderflex.com/hsp.html
    const hsp = Math.sqrt(0.299 * (r * r) + 0.587 * (g * g) + 0.114 * (b * b));

    // Using the HSP value, determine whether the color is light or dark
    // > 127.5 is 'light', <= 127.5 is 'dark'
    return hsp > 127.5;
}

function unify(e, source = e.changedTouches) { return  source ? source[0] : e }

function touchById(e, touchId, source = e.changedTouches) {
    if (!source && touchId === "mouse") {
        return e;
    } else if (source) {
        for (const touch of source) {
            if (touch.identifier === touchId) {
                return touch;
            }
        }
    } else {
        return null;
    }
}

function distance(x1, y1, x2, y2) {
    return Math.sqrt(Math.pow(x2-x1, 2) + Math.pow(y2-y1, 2));
}

function isCallable(fn) {
    return fn instanceof Function;
}

function countEntrys(str, subst) {
    let count = 0, index = 0;
    for (index = str.indexOf(subst); index !== -1; count++, index = str.indexOf(subst, index + subst.length )) {};
    return count;
}

function arrayIntersect(array, array2) {
    return array.filter(value => array2.includes(value));
}

function plainObjectFrom(object = {}) {
    let candy = Object.assign(Object.create(null), object);
    Object.defineProperties(candy, Object.getOwnPropertyDescriptors(object));
    return candy;
}

function createWithProtectedSlot(prototype, propertiesObject) {
    let object = Object.create(prototype, propertiesObject);
    object[_protectedDescriptor] = Object.create(prototype[_protectedDescriptor]);
    return object;
}

function blur() {
    if (document.activeElement && document.activeElement.blur) {
        document.activeElement.blur();
    }
}

animationFinished.defaultOptions = {
    deadline:           1000,
    fallbackDeadline:   undefined,
    deadlineReject:     false,
}
function animationFinished(el, options = animationFinished.defaultOptions) {

    options = Object.assign({}, animationFinished.defaultOptions, options);
    let deadline;
    let awaitingAnimations;

    if (isCallable(el.getAnimations)) {

        awaitingAnimations =  el.getAnimations({ subtree: false }).map((animation) => animation.finished);
        deadline = options.deadline;

    } else {

        deadline = options.fallbackDeadline || options.deadline;
        awaitingAnimations  = [];
        let style = getComputedStyle(el);

        //todo delay (need css values parser)
        const waitAnimation= (style.animationName && style.animationName !== "none") && style.animationPlayState === "running";
        //it seems we do not support "all" transition in failsafe mode
        //!!style.transition not really work as transition initial value not determinated
        const waitTransition= style.transitionDuration !== '0s' && (style.transitionProperty !== "all" /*default value*/ && style.transitionProperty !== "none");
        const opt = { once: true };

        if (waitAnimation) {
            awaitingAnimations.push(new Promise((resolve, reject) => {
                const pendingResolve = (e) => resolve("animation");
                //chrome57 does not support: el.onanimationend = el.onanimationcancel
                el.addEventListener("animationend",    pendingResolve, opt);
                el.addEventListener("animationcancel", pendingResolve, opt);
            }));
        }

        if (waitTransition) {
            awaitingAnimations.push(new Promise((resolve, reject) => {
                const pendingResolve = (e) => resolve("transition");
                //chrome57 does not support: el.ontransitionend = el.ontransitioncancel
                el.addEventListener("transitionend",    pendingResolve, opt);
                el.addEventListener("transitioncancel", pendingResolve, opt);
            }));
        }
    }

    if (!awaitingAnimations.length) {
        return Promise.all([]);
    }

    if (deadline) {
        return Promise.race([ Promise.all(awaitingAnimations), new Promise((resolve, reject) => {
                setTimeout(options.deadlineReject ? () => reject("deadline") : () => resolve("deadline"), deadline);
            })
        ]);
    } else {
        return Promise.all(awaitingAnimations);
    }

}

animationMonitor.symbol = Symbol("animationMonitor");

animationMonitor.report = (el) => {
    return el[animationMonitor.symbol];
}

function animationMonitor(el) {

    this.animating  = false;
    this.transition = false;

    if (el[animationMonitor.symbol]) {
        throw new Error("animationMonitor is already captured");
    } else {
        el[animationMonitor.symbol] = this;
    }

    //todo distinguish different transition in transitions list
    //todo distinguish different animation in animations list

    const transitionStart = ()  => { this.transition = true  };
    const animationStart = ()   => { this.animating = true   }
    const transitionEnd = ()    => { this.transition = false };
    const animationEnd = ()     => { this.animating = false  }

    el.addEventListener("transitionstart",  transitionStart );
    el.addEventListener("transitionrun",    transitionStart );
    el.addEventListener("transitionend",    transitionEnd   );
    el.addEventListener("transitioncancel", transitionEnd   );

    el.addEventListener("animationstart",   animationStart  );
    el.addEventListener("animationend",     animationEnd    );
    el.addEventListener("animationcancel",  animationEnd    );

    this.release = () => {
        el.removeEventListener("transitionstart",  transitionStart );
        el.removeEventListener("transitionrun",    transitionStart );
        el.removeEventListener("transitionend",    transitionEnd   );
        el.removeEventListener("transitioncancel", transitionEnd   );

        el.removeEventListener("animationstart",   animationStart  );
        el.removeEventListener("animationend",     animationEnd    );
        el.removeEventListener("animationcancel",  animationEnd    );
    }

}


const _modalLockerSymbol = Symbol("modalLocker");
const _isShowingSymbol = Symbol("isShowing");
const _protectedDescriptor = Symbol("protected");
const _disabledDescriptor = Symbol("disabled");

const indentity = Math.random().toString(36).replace(/:/g, "x");

function Socket(target) {

    const DeadSocket = {
        close() {
            console.log("closing dead socket");
        }
    };

    let privateCtx = {
        _socket: null,
        set socket(socket) {

            if (socket === privateCtx._socket) {
                return;
            }

            let oldSocket = privateCtx._socket;
            privateCtx._socket = socket;

            socket.binaryType = "arraybuffer";

            socket.onopen = function(e){
                //alert("[open] Соединение установлено");
                // alert("Отправляем данные на сервер");
                //socket.send("Меня зовут Джон");
                if (this !== privateCtx._socket) {
                    console.log("context changed between call :: open");
                    return;
                }

                console.info("socket open");

                clearTimeout(privateCtx.reconnectHndl);
                privateCtx.reconnectHndl = undefined;
                privateCtx.status = SocketOpen;

                if (privateCtx._internalWait.has("open")) {
                    privateCtx._internalWait.get("open").resolve(e);
                    privateCtx._internalWait.delete("open");
                } else {
                    console.warn("no handler");
                }

                privateCtx.lastOpenAt = new Date();
                privateCtx.wasOpen = true;

                if (isCallable(privateCtx.onconnectnect)) {
                    privateCtx.onconnectnect.call(publicCtx, e);
                }
            };

            socket.onclose = function(event) {
                if (this !== privateCtx._socket) {
                    console.log("context changed between call :: close", privateCtx._socket);
                    return;
                }
                privateCtx.lastDisconnectReason = {
                    wasClean:   event.wasClean,
                    code:       event.code,
                    reason:     event.reason,
                    socketStatus: event.wasClean ? SocketClose : SocketError,
                }
                const prevStatus = privateCtx.status;
                if (event.wasClean) {
                    //alert(`[close] Соединение закрыто чисто, код=${event.code} причина=${event.reason}`);
                    privateCtx.status = SocketClose;
                } else {
                    // например, сервер убил процесс или сеть недоступна
                    // обычно в этом случае event.code 1006
                    //alert('[close] Соединение прервано');
                    privateCtx.status = SocketError;

                }

                if (privateCtx._internalWait.has("close")) {
                    privateCtx._internalWait.get("close").resolve(event);
                    privateCtx._internalWait.delete("close");
                } else {
                    //console.warn("no handler");
                }
                if (privateCtx._internalWait.has("open")) {
                    privateCtx._internalWait.get("open").reject("socket closed", event);
                    privateCtx._internalWait.delete("open");
                } else {
                    //may not exist (disconnect)
                }

                if (prevStatus === SocketAuthorized && isCallable(privateCtx.ondeauthorized)) {
                    privateCtx.ondeauthorized.call(publicCtx, "socketClosed", event);
                }
                if (isCallable(privateCtx.ondisconnect)) {
                    privateCtx.ondisconnect.call(publicCtx, event);
                }

                if (!event.wasClean) {
                    if ( privateCtx.sess !== emptyFn && privateCtx.wasAuthorizedOnce && privateCtx.wasOpen &&
                        privateCtx.lastOpenAt && new Date() - privateCtx.lastOpenAt <= 1000)
                    {
                        privateCtx.sess().then(() => {
                            clearTimeout(privateCtx.reconnectHndl);
                            privateCtx.reconnectHndl = setTimeout(() => privateCtx.reconnect(), 0); //move out of stack
                            //temporal fix race of socket open/close
                        });
                    } else {
                        clearTimeout(privateCtx.reconnectHndl);
                        privateCtx.reconnectHndl = setTimeout(() => privateCtx.reconnect(), 0); //move out of stack
                        console.warn("trigger reconnect")
                    }
                } else {
                    console.warn("socket closed clean");
                }
            };

            socket.onmessage = function(evt, ...args) {

                const msg = evt.data
                console.log(msg, typeof msg, ...args);

                if (this !== privateCtx._socket) {
                    console.log("context changed between call :: onmessage");
                    return;
                }

                if (msg === "authfail") {
                    console.error("auth fail");
                    if (privateCtx.status === SocketAuthorized) {
                        privateCtx.status = SocketOpen;
                        privateCtx.reauthorize();
                        setTimeout(privateCtx.ondeauthorized.call(publicCtx), 0);
                    }
                }

                let namespace;
                if (msg instanceof Blob) {

                    if (privateCtx._ns.has("ctr")) {
                        const nsHandler = privateCtx._ns.get("ctr");
                        if (nsHandler.async) {
                            nsHandler.callbacks.forEach(store => setTimeout(store.callback, 0, 0, msg, ...store.args));
                        } else {
                            nsHandler.callbacks.forEach(store => store.callback(0, msg, ...store.args));
                        }
                    }

                } else if (msg instanceof ArrayBuffer) {

                    if (privateCtx._ns.has("ctr")) {
                        const nsHandler = privateCtx._ns.get("ctr");
                        if (nsHandler.async) {
                            nsHandler.callbacks.forEach(store => setTimeout(store.callback, 0, 0, msg, ...store.args));
                        } else {
                            nsHandler.callbacks.forEach(store => store.callback(0, msg, ...store.args));
                        }
                    }

                } else {
                    let index = -1;
                    if ((index = msg.indexOf(":")) !== -1) {
                        namespace = msg.substring(0, index);
                        let taskId = undefined;
                        let data = undefined;
                        if ((index = msg.indexOf(":", index+1)) !== -1) {
                            taskId = msg.substring(0, index);
                            if (privateCtx.hachiko.has(taskId)) {
                                const ctrl = privateCtx.hachiko.get(taskId);
                                if (msg.substring(index+1, index+1+2) === "ok") {
                                    data = msg.substring(index+1+2+1);
                                    ctrl.resolve(data);
                                } else {
                                    index = msg.indexOf(":", index+1);
                                    data = msg.substring(index === -1 ? msg.length : index+1);
                                    ctrl.reject(data);
                                }
                                privateCtx.hachiko.delete(taskId);
                            }
                        } else {
                            console.warn("probably malformed message", msg);
                        }

                        if (privateCtx._ns.has(namespace)) {
                            const nsHandler = privateCtx._ns.get(namespace);
                            if (data === undefined) {
                                data = msg.substring(index+1);
                            }
                            if (taskId && (index = taskId.indexOf(":")) !== -1) {
                                taskId = taskId.substring(index+1);
                            }
                            if (nsHandler.async) {
                                nsHandler.callbacks.forEach(store => setTimeout(store.callback, 0, taskId, data, ...store.args));
                            } else {
                                nsHandler.callbacks.forEach(store => store.callback(taskId, data, ...store.args));
                            }
                        }
                    }
                }


                if (isCallable(privateCtx.onmessage)) {
                    return privateCtx.onmessage.call(publicCtx, msg, ...args);
                }

            };

            socket.onerror = function(error) {
                if (this !== privateCtx._socket) {
                    //console.log("context changed between call :: error");
                    return;
                }
                const prevStatus = privateCtx.status;
                privateCtx.status = SocketError;
                privateCtx.lastError = {
                    time: new Date(),
                    subject: error
                }

                console.error("socket", error);

                if (prevStatus === SocketAuthorized && isCallable(privateCtx.ondeauthorized)) {
                    privateCtx.ondeauthorized.call(publicCtx, "socketError", error);
                }
            };

        },
        get socket() {
            return privateCtx._socket;
        },
        tasks: {
            connect: null,
            disconnect: null,
            auth: null,
        },
        taskId: 0,
        status: SocketInit,
        wasOpen: false,
        wasAuthorizedOnce: false,
        lastError: {},
        lastDisconnectReason: {},
        connectAttempts: 0,
        lastConnectTime: null,
        lastOpenAt: null,
        onconnectnect: null,
        ondisconnect: null,
        onauthorized: null,
        ondeauthorized: null,
        onmessage: null,
        hachiko: new Map(),
        _internalWait: new Map(),
        _ns: new Map(),
        auth: () => {
            return indentity;
        },
        sess: emptyFn(),
        connect: ()=> {
            console.info("trigger connect")
            if (privateCtx.tasks.connect) {
                console.warn("connect task ongoing");
                //todo check why mo than one socket cannot exist with same add?
                //socket.onclose not call for second
                return;
            }
            //todo check why more than one socket with same add cannot persist
            privateCtx.tasks.connect =  privateCtx.connectTask().then(() => {
                privateCtx.tasks.connect = null;
            }).catch(() => {
                privateCtx.tasks.connect = null;
            });
        },
        connectAsync: async ()=> {
            privateCtx.tasks.connect = privateCtx.connectTask().catch(console.warn);
            await privateCtx.tasks.connect;
            privateCtx.tasks.connect = null;
        },
        reconnect: () => {
            privateCtx.wasOpen = false;
            privateCtx.lastOpenAt = null;
            clearTimeout(privateCtx.reconnectHndl); //privateCtx.reconnectHndl is shared betwen reconnect impl and reconnect itself
            let passes = (new Date) - privateCtx.lastConnectTime;
            let time = 1000 *  Math.min(Math.floor(privateCtx.connectAttempts / 5)+1, 10);
            if (passes < time) {
                console.info("delay connect evt by", time - passes);
                privateCtx.reconnectHndl = setTimeout(() => { privateCtx.connect(); }, time - passes);
            } else {
                //it seems timeout is crucial to proper resolve connect promise //do not remove even if reconnect now itself out of stack
                privateCtx.reconnectHndl = setTimeout(() => { privateCtx.connect(); }, 250);
            }
        },
        reauthorize: () => {
            privateCtx.authorizeTask().then(() => {
                privateCtx.reauthorizeAttempts = 0;
                if (isCallable(privateCtx.onauthorized)) {
                    console.info("onauthorized");
                    privateCtx.onauthorized.call(publicCtx, privateCtx.auth());
                } else {
                    console.info("not auth task");
                }
            }).catch(e => {
                privateCtx.reauthorizeAttempts++;
                privateCtx.disconnect();
            })
        },
        disconnect: () => {
            if (privateCtx.status === SocketOpen) {
                privateCtx.disconnectTask().catch(console.error);
            } else {
                privateCtx._socket.close();
                privateCtx.socket = DeadSocket; //move current socket away of scope
                privateCtx.status = SocketClose;
            }
        },
        disconnectAsync: async () => {
            const identity = isCallable(privateCtx.auth) ? privateCtx.auth() : "";
            if (indentity.trim().length === 0) {
                throw new Error("invalid identity");
            }
            if (privateCtx.status === SocketOpen) {
                await privateCtx.disconnectTask().catch(console.error);
            } else {
                privateCtx.socket.close();
                privateCtx.socket = DeadSocket; //move current socket away of scope
                privateCtx.status = SocketClose;
            }
        },
        disconnectTask: (timeoutMs = 250) => {
            const identity = isCallable(privateCtx.auth) ? privateCtx.auth() : "";
            if (indentity.trim().length === 0) {
                throw new Error("invalid identity");
            }
            return privateCtx.deauthorizeTask.then((msg) => {
                return new Promise((resolve, reject) => {
                    privateCtx._internalWait.set("close", {
                        resolve,
                        reject,
                    });
                    setTimeout(() => {
                        privateCtx._internalWait.delete("close");
                        reject("timeout");
                    }, timeoutMs);
                    privateCtx._socket.close();
                });
            }).then(() => {
                privateCtx.socket = DeadSocket;
                privateCtx.status = SocketClose;
                return true;
            }).catch((reason) => {
                privateCtx.socket = DeadSocket; //move current socket away of scope
                privateCtx.status = SocketClose;
                throw new Error(reason);
            });
        },
        authorizeTask: (timeoutMs = 1000) => {
            const identity = isCallable(privateCtx.auth) ? privateCtx.auth() : "";
            if (indentity.trim().length === 0) {
                throw new Error("invalid identity");
            }
            return privateCtx.sendConf("auth", identity, timeoutMs).then(()=>{
                console.info("successful auth");
                if (privateCtx.status === SocketOpen) {
                    privateCtx.status = SocketAuthorized;
                    privateCtx.wasAuthorizedOnce = true;
                } else {
                    console.warn("SocketAuthorized of socket that no open!");
                }
                return true;
            }).catch((e) => {
                if (privateCtx.status === SocketAuthorized) {
                    privateCtx.status = SocketOpen;
                }
                throw new Error(e);
            });
        },
        deauthorizeTask: (timeoutMs = 1000) => {
            const identity = isCallable(privateCtx.auth) ? privateCtx.auth() : "";
            if (indentity.trim().length === 0) {
                throw new Error("invalid identity");
            }
            return privateCtx.sendConf("leave", identity, timeoutMs).then(()=>{
                console.info("successful deauth");
                if (privateCtx.status === SocketAuthorized) {
                    privateCtx.status = SocketOpen;
                } else {
                    console.warn("deauthorize socket that not authorized!");
                }
                return true;
            }).catch((e) => {
                throw new Error(e);
            });
        },
        connectTask: () => {
            const identity = isCallable(privateCtx.auth) ? privateCtx.auth() : "";
            if (indentity.trim().length === 0) {
                throw new Error("invalid identity");
            }
            let immediateClose = new Promise((resolve, reject) => {
                privateCtx._internalWait.set("close", {
                    resolve: () => reject("socket closed"),
                });
                setTimeout(resolve, 250);
            });
            return new Promise((resolve, reject) => {
                privateCtx.socket = DeadSocket;
                privateCtx._internalWait.set("open", {
                    resolve,
                    reject,
                });
                privateCtx.connectAttempts++;
                privateCtx.lastConnectTime = new Date();
                console.log("connection attempt: ", privateCtx.connectAttempts);
                privateCtx.socket = new WebSocket("ws://" + location.hostname + target);
            }).then((status) => {
                return immediateClose; //fixme todo19
            }).then((status) => {
                //when server close session it first open and emediatly close
                //this not work as reject not work after resolve
                return privateCtx.authorizeTask();
            }).then((status) => {
                if (isCallable(privateCtx.onauthorized)) {
                    console.info("onauthorized");
                    privateCtx.onauthorized.call(publicCtx, privateCtx.auth());
                } else {
                    console.info("not auth task");
                }
                return status;
            }).catch((e) => {
                console.debug("catch on connect")
                if (privateCtx.status === SocketAuthorized) {
                    console.info("fail2connect execute disconnect");
                    return privateCtx.disconnectTask().then(status => {
                        throw new Error(e);
                    });
                } else {
                    privateCtx.socket = DeadSocket; //safeguard of socket retry
                    throw new Error(e);
                }
            });
        },
        send: (topic, msg) => {
            if (!(privateCtx.status === SocketOpen || privateCtx.status === SocketAuthorized)) {
                throw new Error("socket was not open");
            }
            privateCtx.socket.send(topic + ":" + privateCtx.taskId.toString() + ":" + msg);
            return privateCtx.taskId++;
        },
        sendBinary: (topic, buffer) => { //todo remove me
            if (!(privateCtx.status === SocketOpen || privateCtx.status === SocketAuthorized)) {
                throw new Error("socket was not open");
            }
            const prefix = (new TextEncoder()).encode(topic + ":" + privateCtx.taskId.toString() + ":");
            let msg = new Uint8Array(prefix.byteLength + buffer.byteLength);
            msg.set(prefix, 0);
            msg.set(new Uint8Array(buffer), prefix.byteLength);
            privateCtx.socket.send(msg);
            return privateCtx.taskId++;
        },
        sendConf: (topic, msg, timeoutMs = 1000) => {
            return new Promise((resolve, reject) => {
                const taskId = privateCtx.taskId;
                privateCtx.hachiko.set(topic + ":" + taskId.toString(), {
                    resolve,
                    reject
                });
                if (privateCtx.send(topic, msg) !== taskId) {
                    privateCtx.hachiko.delete(topic + ":" + taskId.toString());
                    reject("malformed impl of send");
                }
                setTimeout(() => {
                    privateCtx.hachiko.delete(topic + ":" + taskId.toString());
                    reject("timeout");
                }, timeoutMs);
            });
        },
    };

    let publicCtx = plainObjectFrom({
        send(topic, msg) {
            if (privateCtx.status !== SocketAuthorized) {
                throw new Error("socket was not authorize");
            }
            return privateCtx.send(topic, msg);
        },
        sendBinary(topic, msg) {
            if (privateCtx.status !== SocketAuthorized) {
                throw new Error("socket was not authorize");
            }
            return privateCtx.sendBinary(topic, msg);
        },
        sendConf(topic, msg, timeoutMs = 1000) {
            if (privateCtx.status !== SocketAuthorized) {
                throw new Error("socket was not authorize");
            }
            return privateCtx.sendConf(topic, msg, timeoutMs);
        },
        open() {
            return privateCtx.connect();
        },
        async openAsync() {
            return privateCtx.connectAsync();
        },
        close() {
            console.log("begin disconnect seq");
            clearTimeout(privateCtx.reconnectHndl);
            return privateCtx.disconnect();
        },
        async closeAsync() {
            clearTimeout(privateCtx.reconnectHndl);
            return privateCtx.disconnectAsync();
        },
        renew() {
            return privateCtx.disconnectTask().then(() => {
               return privateCtx.connectTask();
            });
        },
/*        async renewAsync() {
            await publicCtx.close();
            return publicCtx.open();
        },*/
        set onmessage(callback) {
            privateCtx.onmessage = callback;
        },
        identity(callback) {
            privateCtx.auth = callback;
        },
        session(callback) {
            privateCtx.sess = callback;
        },
        ns(namespace, callback = function (){}, async = true, ...args) {
            let ctrl;
            if (!privateCtx._ns.has(namespace)) {
                privateCtx._ns.set(namespace, ctrl = {
                    async: true,
                    callbacks: [],
                });
            } else {
                ctrl = privateCtx._ns.get(namespace);
            }
            if (arguments.length >= 2) {
                ctrl.async &= async;
                ctrl.callbacks.push({
                    callback,
                    args,
                });
            } else {
                return {
                    add: (callback, async = true, ...args) => {
                        ctrl.async &= async;
                        ctrl.callbacks.push({
                            callback,
                            args,
                        });
                    },
                    remove: (callback) => {
                        const indexOf = ctrl.callbacks.indexOf(callback);
                        if (indexOf !== -1) {
                            ctrl.callbacks.splice(indexOf, 1);
                        }
                    },
                    has: (callback) => {
                        return ctrl.callbacks.indexOf(callback) !== -1
                    },
                }
            }
        },
        available()  { return privateCtx.status === SocketAuthorized; },
        status(){ return privateCtx.status  },
        get disconnectReason() { return privateCtx.lastDisconnectReason; },
        get reconnectReason() { return privateCtx.lastDisconnectReason; },
        get error() { return privateCtx.lastError },
        set onconnect(callback) { privateCtx.onconnectnect = callback },
        get onconnect() { return privateCtx.onconnectnect },
        set onauthorize(callback) { privateCtx.onauthorized  = callback},
        get onauthorize() { return privateCtx.onauthorized },
        set ondeauthorize(callback) { privateCtx.ondeauthorized  = callback},
        get ondeauthorize() { return privateCtx.ondeauthorized },
        set ondisconnect(callback) { privateCtx.ondisconnect = callback },
        get ondisconnect() { return privateCtx.ondisconnect },
    });

    return publicCtx;
}

function Registry() {

    let privateCtx = {
        storage: Object.create(null),
        incCounter: 0,
        count: 0,
        iterate: () => {
            let ctx = {
                keys: Object.keys(privateCtx.storage),
                index: 0,
            };
            return {
                next: () => {
                    if (ctx.index < ctx.keys.length) {
                        return { value: privateCtx.storage[ctx.keys[ctx.index]], done: ctx.index++ >= ctx.keys.length };
                    } else {
                        return { value: null, done: true };
                    }
                }
            };
        }
    }

    return plainObjectFrom({
        has: (id) => {
            return Object.hasOwn(privateCtx.storage, id);
        },
        add: (id, instance) => {
            if (Object.hasOwn(privateCtx.storage, id)) {
                privateCtx.storage[id] = instance;
            } else {
                privateCtx.storage[id] = instance;
                privateCtx.incCounter++;
                privateCtx.count++;
            }
        },
        get: (id, def = null) => {
            if (Object.hasOwn(privateCtx.storage, id)) {
                return privateCtx.storage[id];
            } else {
                return def;
            }
        },
        remove: (id) => {
            if (Object.hasOwn(privateCtx.storage, id)) {
                delete privateCtx.storage[id];
                privateCtx.count--;
            }
        },
        incremental: () => {
            return privateCtx.incCounter;
        },
        count: () => {
            return privateCtx.count;
        },
        at: (index) => {
            if (privateCtx.count <= index) {
                return null;
            }
            return privateCtx.storage[(Object.keys(privateCtx.storage)[index])];
        },
        iterate:            privateCtx.iterate,
        [Symbol.iterator]:  privateCtx.iterate,
    });
}

function emptyReturnFn(arg) {
    return arg;
}
function emptyFn() {}


function defaultValidator(value, attributes) {
    return { valid: true, mutableValue: value, errors: [], };
}

Attribute.defaults = {
    enumerable: true, writeable: true, readable: true, required: true, inheritable: true,
    apply: emptyFn, validate: defaultValidator, retrieve: emptyReturnFn,
    //value: undefined,
    label: "no label", description: "no desc", type: "int", params: {}, units: ""
}

/*
    note: Attribute.readable and Attribute.writeable not symmetrical

    when Attribute.writeable always checking before .set Attribute.readable mostly note

    Attribute.enumerable: using when iterating over attributes

    Attribute.required: for note

    note: Attribute.inheritable === false

    attribute with (Attribute.inheritable === false) represent internal "Control" State like ["state", "activationLeft"]
    as is this attribute must not be inherited via COW operation, because
    inheritance will lead to state splitting between "Control" and its "ControlView" and even between  "ControlViews" itself.

 */

Attribute.validationResult = {
    valid:          false,
    mutableValue:   undefined,
    errors:         [],
}

Attribute.create = createWithProtectedSlot;

function Attribute(descriptor = {}) {
    this[_protectedDescriptor] = {
        value: descriptor.value,
    };
    delete descriptor.value;
    Object.assign(this, Attribute.defaults, descriptor);
}

Object.defineProperties(Attribute.prototype, {
    optional: {
        get() {
            return !this.required;
        },
    },
    defaultValue: {
        get() {
            return this[_protectedDescriptor].value;
        },
    },
    value: {
        get() {
            throw new Error("not implemented");
        },
        set() {
            throw new Error("not implemented");
        },
    },
    defaulted: {
        get() {
            throw new Error("not implemented");
        },
    },
    empty: {
        get() {
            throw new Error("not implemented");
        },
    },
    incremental: {
        get() {
            return this.type === Attribute.types.INT;
        },
    },
});

Attribute.prototype.getValue = function(storage) {
    throw new Error("not implemented");
}

Attribute.prototype.setValue = function(storage, value) {
    throw new Error("not implemented");
}

Attribute.prototype.isDefaulted = function(storage) {
    throw new Error("not implemented");
}

Attribute.prototype.isEmpty = function(storage) {
    throw new Error("not implemented");
}

Attribute.prototype.increment = function(sign) {
    throw new Error("no inc imp and no backward setter impl");
}

Attribute.types = {
    STRING:     "string",
    TEXT:       "text",
    INT:        "int",
    FLOAT:      "float",
    BOOLEAN:    "bool",
    ENUM:       "enum",
    INTERVAL:   "interval",
    KBKEY:      "kbkey",
    AXIS:       "axis",
    LINK:       "link",
}

Attribute.scalarTypes = {
    [Attribute.types.STRING]:     "string",
    [Attribute.types.TEXT]:       "string",
    [Attribute.types.INT]:        "number",
    [Attribute.types.FLOAT]:      "number",
    [Attribute.types.BOOLEAN]:    "boolean",
    [Attribute.types.ENUM]:       undefined,
    [Attribute.types.INTERVAL]:   "number",
    [Attribute.types.KBKEY]:      "string",
    [Attribute.types.AXIS]:       "number",
    [Attribute.types.LINK]:       "string",
}

Attributes.create = (prototype, propertiesObject) => {
    const pt = createWithProtectedSlot(prototype, propertiesObject);
    pt[_protectedDescriptor].values     = Object.create(pt[_protectedDescriptor].values);
    pt[_protectedDescriptor].handlers   = Object.create(pt[_protectedDescriptor].handlers);
    pt[_protectedDescriptor].keys       = new Set(pt[_protectedDescriptor].keys);
    return pt;
};

function Attributes(descriptors = {}) {
    let original;
    for (let that = this; that instanceof Attributes; original = that, that = Object.getPrototypeOf(that)) {}
    this[_protectedDescriptor] = {
        keys:       null,
        values:     Object.create(null),
        handlers:   Object.assign(Object.create(null), Object.fromEntries(Object.entries(descriptors).map((e) => {
            return [e[0], Object.freeze(new Attribute(e[1]))];
        }))),
    };
    this[_protectedDescriptor].keys = new Set(Object.keys(this[_protectedDescriptor].handlers));
    this[_protectedDescriptor].originalValues = original[_protectedDescriptor].values;  //counter inheritance field, this must always lead to root of Attributes inheritance tree
}

Attributes.prototype.merge = function(attributes, removeList = []) {
    let protectedCtx = this[_protectedDescriptor];
    if (!(attributes instanceof Attributes)) {
        throw new Error("invalid type of attributes");
    }
    //will not copy object correctly if both attributes are have super
    protectedCtx.handlers   = Object.assign(protectedCtx.handlers, attributes[_protectedDescriptor].handlers);
    protectedCtx.values     = Object.assign(protectedCtx.values, attributes[_protectedDescriptor].values);
    protectedCtx.keys       = new Set([...protectedCtx.keys, ...attributes[_protectedDescriptor].keys]);
    if (removeList.length) {
        for (const removal of removeList) {
            delete protectedCtx.handlers[removal];
            delete protectedCtx.values[removal];
            protectedCtx.keys.delete(removal);
        }
    }
    return this;
}

Attributes.prototype.get = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    const value = protectedCtx.values[id];
    if (value === undefined && !Object.hasOwn(protectedCtx.values, id)) { //not sure how it accurate
        return this.default(id);
    }
    return protectedCtx.handlers[id].retrieve(protectedCtx.values[id], this);
}

Attributes.prototype.set = function(id, value, validate = true) {
    const protectedCtx  = this[_protectedDescriptor];
    const handler       = protectedCtx.handlers[id];
    let   values        = handler.inheritable ? protectedCtx.values : protectedCtx.originalValues;

    if (!handler.writeable) {
        throw new Error("attribute is not writeable");
    }

    let valid = !validate;
    if (!validate || handler.validate === defaultValidator) {
        if (validate) {
            value = this.defaultValidator(handler, value);
            valid = !(typeof value === "number" && isNaN(value));
        }
    } else {
        const vResult = handler.validate(value, this);
        value = vResult.mutableValue;
        valid = vResult.valid;
    }


    if (valid) {
        const oldValue = this.get(id);
        if (oldValue !== value) {
            values[id] = value;
            if (handler.apply(value, oldValue, this) === false) {
                values[id] = oldValue;
            } else {
                //todo change event
            }
        }
    }

    return valid;
}

Attributes.prototype.batch = function(keyValue, validate = true) {
    let success = {};
    let totalSuccess = true;
    for (const [name, value] of Object.entries(keyValue)) {
        let desc = this.descriptor(name);
        if (desc && desc.writeable) {
            try {
                if (!this.isEqualTo(name, value)) {
                    success[name] = this.set(name, value, validate);
                    totalSuccess &= success[name];
                } else {
                    success[name] = true;
                }
            } catch (e) {
                success[name] = totalSuccess = false;
                console.warn(e);
            }
        }
    }
    return { success: totalSuccess, validate, details: success };
}

Attributes.prototype.increment = function(id, offset) {
    const protectedCtx = this[_protectedDescriptor];
    const handler = protectedCtx.handlers[id];
    let   values = handler.inheritable ? protectedCtx.values : protectedCtx.originalValues;

    if (protectedCtx.values[id] === undefined && !Object.hasOwn(protectedCtx.values, id)) {
        protectedCtx.values[id] = this.default(id);
    }

    const oldValue = values[id];
    const value = values[id] += offset;

    handler.apply(value, oldValue, this);

    return value;
}

Attributes.prototype.default = function(id) {
    if (isCallable(this[_protectedDescriptor].handlers[id].defaultValue)) {
        return this[_protectedDescriptor].handlers[id].defaultValue(this);
    }
    return this[_protectedDescriptor].handlers[id].defaultValue;
}

Attributes.prototype.raw = function(id) {
    return this[_protectedDescriptor].values[id];
}

Attributes.prototype.validate = function(id, value) {
    const protectedCtx = this[_protectedDescriptor];
    const handler = protectedCtx.handlers[id];
    if (handler.validate === defaultValidator) {
        value = this.defaultValidator(handler, value);
        return { valid: !(typeof value === "number" && isNaN(value)), mutableValue: value, errors: [] };
    }
    return handler.validate(value, this);
}

Attributes.prototype.isValid = function(id, value, strict = true) {
    return this.validate(id, value, strict).valid;
}

Attributes.prototype.isEqualTo = function(id, value) {
    let protectedCtx = this[_protectedDescriptor];
    const handler = protectedCtx.handlers[id];

    if (handler.validate === defaultValidator) {
        return this.get(id) === this.defaultValidator(handler, value);
    } else {
        const vResult = handler.validate(value, this);
        if (!vResult.valid) {
            return false;
        }
        return this.get(id) === vResult.mutableValue;
    }

}

Attributes.prototype.defaultValidatorEnumOpExist = function(handler, valueHandler, value, options) {
    for (let opt of options) {
        if (opt.type === "optgroup") {
            if (this.defaultValidatorEnumOpExist(handler, valueHandler, value, opt.items)) {
                return true;
            }
        } else if (opt.type === "option") {
            if (this.defaultValidator(valueHandler, opt.value) === value) {
                return true;
            }
        }
    }
    return false;
}

Attributes.defaultValidatorSynteticHandler = { type: undefined };

Attributes.prototype.defaultValidator = function(handler, value) {
    switch (handler.type) {
        case Attribute.types.BOOLEAN:
            return !!value;
        case Attribute.types.INT:
        case Attribute.types.AXIS:
        case Attribute.types.INTERVAL:
            return +value;
        case Attribute.types.FLOAT:
            return parseFloat(value);
        case Attribute.types.STRING:
        case Attribute.types.TEXT:
        case Attribute.types.LINK:
        case Attribute.types.KBKEY:
            return "" + value;
        case Attribute.types.ENUM:
            Attributes.defaultValidatorSynteticHandler.type = (handler.params.valueType && handler.params.valueType !== Attribute.types.ENUM) ? handler.params.valueType : undefined;
            value = this.defaultValidator(Attributes.defaultValidatorSynteticHandler, value);
            if (handler.params.options) {
                if (this.defaultValidatorEnumOpExist(handler, Attributes.defaultValidatorSynteticHandler, value, handler.params.options)) {
                    return value;
                }
            }
        default:
            return value;
    }
}

Attributes.prototype.isEmpty = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    const value = protectedCtx.values[id];
    if (value === undefined && !Object.hasOwn(protectedCtx.values, id)) { //not sure how it accurate
        return this.default(id) === undefined;
    }
    return false;
}

Attributes.prototype.isOwned = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    if (protectedCtx.values === protectedCtx.originalValues) { //we are at the root
        return !this.isEmpty(id); //only root of inherit chain are owner for default values (that not materialized yet)
    } else {
        return this.hasOwn(id);
    }
}

Attributes.prototype.isInherited = function(id) {
    return this.hasInherit(id);
}

Attributes.prototype.hasOwn = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    return Object.hasOwn(protectedCtx.values, id);
}

Attributes.prototype.hasInherit = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    for (let values = protectedCtx.values; values !== null; values = Object.getPrototypeOf(values)) {
        if (Object.hasOwn(values, id)) {
            return values !== protectedCtx.values;
        }
    }
    return false;
}

Attributes.prototype.hasOverloaded = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    if (!Object.hasOwn(protectedCtx.values, id)) {
        return false;
    }
    for (let values = Object.getPrototypeOf(protectedCtx.values); values !== null; values = Object.getPrototypeOf(values)) {
        if (Object.hasOwn(values, id)) {
            return values[id] !== protectedCtx.values[id];
        }
    }
    return !this.isDefaulted(id);
}

Attributes.prototype.isDefaulted = function(id) {
    const defaulted = this.default(id);
    if (defaulted === undefined) {
        return false;
    }
    return defaulted === this.get(id);
}

Attributes.prototype.getAttribute = function(id) {
    throw new Error("Attributes.prototype.getAttribute");
}

Attributes.prototype.descriptor = function(id) {
    let protectedCtx = this[_protectedDescriptor];
    return protectedCtx.handlers[id];
}

Attributes.prototype[Symbol.iterator] = function*() {
    const protectedCtx = this[_protectedDescriptor];
    for (const key of protectedCtx.keys) {
        const handler = protectedCtx.handlers[key];
        if (handler.enumerable) {
            yield [key, handler];
        }
    }
}

/*function Plugable() {
    function attach(plugable, options) {
        this.connect(plugable, options);
        plugable.connect(this, options);
    }
    function detach(plugable, options)  {
        this.disconnect(plugable, options);
        plugable.disconnect(this, options);
    }

    function connect(abstract, options) {}

    function disconnect(abstract, options) {

    }
}*/

touchEventHelper.events = ["mousedown", "touchstart", "mouseup", "touchend", "touchcancel", "mousemove", "touchmove", "contextmenu"];

/**
 *
 *  Note: that firefox52(mobile) can process single touch like (exact order): (touchStart, touchEnd, mouseDown, mouseUp)
 *  other browser work as expected (touchStart, mouseDown, touchEnd, mouseUp)
 *  that one happened at least at document attach level, probably some kind of "race" between event emulation and userinput at task ending.
 *  So decouple flag now in game.
 *
 *  todo respect touchId
 *
 * @param el
 * @param decimator callback for filter input event
 * @param decouple  filter synthetic event like touchStart <=> mouseDown
 * @returns {any}
 */
function touchEventHelper(el, { decimator = emptyFn, decouple = true } = {}) {

    let longtapTimer;
    let tapInProgress;
    let startTouchPos = { x: 0, y: 0 };

    const context = Object.assign(Object.create(null), {
        free: () => {
            for (const evtId of touchEventHelper.events) {
                el.removeEventListener(evtId, callback);
            }
        },
        ontap:          emptyFn,
        onlongtap:      emptyFn,
    });

    const complete = (e) => {
        clearTimeout(longtapTimer);
        tapInProgress = false;
        e.target.classList.remove("active");
        startTouchPos.x = startTouchPos.y = NaN;
    }

    const needDecimate = decimator !== emptyFn;
    const callback = (e) => {
        if (needDecimate && decimator(e) === true) {
            return;
        }
        const unified = unify(e);
        switch (e.type) {
            case "mousedown":
            case "touchstart":
                if (!tapInProgress) {
                    tapInProgress = true;
                    e.target.classList.add("active");
                    clearTimeout(longtapTimer);
                    longtapTimer = setTimeout(() => {
                        if (tapInProgress) {
                            context.onlongtap(e);
                            complete(e);
                        }
                    }, 1000);
                    startTouchPos.x = unified.pageX; startTouchPos.y = unified.pageY;
                    if (decouple) {
                        e.preventDefault();
                        e.stopPropagation();
                    }
                }
                break;
            case "mouseup":
            case "touchend":
            case "touchcancel":
                if (tapInProgress) {
                    clearTimeout(longtapTimer);
                    context.ontap(e);
                    if (decouple) {
                        e.preventDefault();
                        e.stopPropagation();
                    }
                }
                complete(e);
                break;
            case "mousemove":
            case "touchmove":
                if (tapInProgress) {
                    if (distance(startTouchPos.x, startTouchPos.y, unified.pageX, unified.pageY) > quarterFingerPixels) {
                        complete(e);
                    }
                }
                break;
            case "contextmenu":
                e.preventDefault();
                break;
        }
    }

    for (const evtId of touchEventHelper.events) {
        el.addEventListener(evtId, callback);
    }

    return context;
}

Group.autocreated = [];
Group.registry = new Registry();

Group.join = (member, groups) => {
    for (const groupId of groups) {
        if (!groupId) {
            console.error("Group.join", "empty groupId")
        }
        let group = Group.registry.get(groupId);
        if (!group) {
            group = new Group(groupId);
            Group.autocreated.push(group);
            Group.registry.add(groupId, group);
        }
        group.add(member);
    }
}

Group.isMember = (member, groupId) => {
    const group = Group.registry.get(groupId);
    if (!group) {
        return false;
    }
    return group.has(member);
}

Group.config = {
    id:         undefined,
    logic:      true,
    control:    false,
}

Group.declare = (config, constructor = Group) => {

    config = Object.assign({}, Group.config, config);

    if (!config.id) {
        throw new Error("group must have valid id");
    }

    if (Group.registry.has(config.id)) {
        throw new Error("group already regin");
    } else {
        let group = new Group(config.id);
        Group.registry.add(config.id, group);
        delete config.id;
        for (const [id, value] of Object.entries(config)) {
            if (Object.hasOwn(group, id)) {
                group[id] = value;
            }
        }

        return group;
    }
}

Group.empty = new Group("");

function Group(name) {

    this[_protectedDescriptor] = { name, registry: new Registry() };

    this.logic   = false; //graphical designation for group members
    this.control = false; //controlling group, all ctrl member in group will-be turned off if any of it turned on
    this.info    = false; //information group

    this.data    = undefined;  //group data related to it's behavior flags

}

Group.prototype.add = function(member) {
    this[_protectedDescriptor].registry.add(member.id, member);
}

Group.prototype.remove = function(member) {
    this[_protectedDescriptor].registry.remove(member.id);
}

Group.prototype.has = function(member) {
    return this[_protectedDescriptor].registry.has(member.id);
}

Group.prototype.get = function(id) {
    return this[_protectedDescriptor].registry.get(id);
}

Group.prototype[Symbol.iterator] = function() {
    const protectedCtx = this[_protectedDescriptor];
    return () => protectedCtx.registry[Symbol.iterator]();
}

Group.prototype.forEach = function(callback, thisArg = this) {
    const protectedCtx = this[_protectedDescriptor];
    for (const item of protectedCtx.registry) {
        callback.call(thisArg, item, item.id);
    }
}

Object.defineProperties(Group.prototype, {
    id:     { get() { return this[_protectedDescriptor].name; } },
});

function ControlView(ctrl, dom) {

    if (!(ctrl instanceof Control)) {
        throw new Error("ctrl must be instance of Control");
    }

    if (!(dom instanceof Element)) {
        throw new Error("ctrl must be instance of Element");
    }

    let attributes = Attributes.create(ctrl.attributes);

    attributes.merge(new Attributes({
        viewId:  {
            label:          "view",
            //description: "unique id of this view",
            type:           Attribute.types.STRING,
            writeable:      false,
            readable:       true,
            enumerable:     true,
            value:          () => this.id,
        },
        gridArea: {
            label:          "area",
            description:    "position of widget in page layout",
            type:           Attribute.types.STRING,
            writeable:      true,
            readable:       false,
            enumerable:     false,
            value:          (attributes) => {
                if (this[_protectedDescriptor].originalGridArea === undefined) {
                    this[_protectedDescriptor].originalGridArea = this.gridArea;
                }
                return this[_protectedDescriptor].originalGridArea;
            },
            apply: (value, oldValue, attributes) => {
                if (this[_protectedDescriptor].originalGridArea === undefined) {
                    this[_protectedDescriptor].originalGridArea = this.gridArea;
                }
                this.gridArea = value;
            }
        },
        custom: {
            type:       Attribute.types.BOOLEAN,
            writeable:  true,
            readable:   false,
            enumerable: false,
            value:      false,
        },
        removed: {
            type:       Attribute.types.BOOLEAN,
            writeable:  true,
            readable:   false,
            enumerable: false,
            value:      false,
            apply: (value, oldValue, attributes) => {
                if (oldValue && !value) {
                    throw new Error("removal of ControlView is irreversible");
                }
                if (value) {
                    this.doTerminate();
                }
            }
        }
    }));

    //current life cycle is bit strange, as suspend is optional
    //attached is always true as ControlView cannot exist without Control
    //activated === true (alive) also mean wakeup suspended = false if Control is suspendable
    //but activated === false (dead) dont mean goto sleep suspended = true
    //but! activated === false also mean gracefuly shutdown all long term op and release any resource allocated with this view
    //so suspend impl is on suspentable ctrl
    this.attached   = false;
    this.activated  = false;
    this.suspended  = false;

    this[_protectedDescriptor] = {
        ctrl,
        dom,
        attributes,
        originalGridArea: "",
        data: Object.create(null),
    }
    const overrideAttrs = ctrl.extractAttributesDefault(dom);
    for (let [id, attr] of Object.entries(overrideAttrs)) {
        attributes.set(id, attr.value);
    }
    this[_protectedDescriptor].dom = this.make(ctrl, dom);

    dom.replaceWith(this[_protectedDescriptor].dom);

}

Object.defineProperties(ControlView.prototype, {
    id: {
        get() {
            return this[_protectedDescriptor].dom.getAttribute("id");
        },
        set(value) {
            return this[_protectedDescriptor].dom.setAttribute("id", value);
        }
    },
    attributes: {
        get() {
            return this[_protectedDescriptor].attributes;
        }
    },
    control: {
        get() {
            return this[_protectedDescriptor].ctrl;
        }
    },
    dom: {
        get() {
            return this[_protectedDescriptor].dom;
        }
    },
    data: {
        get() {
            return this[_protectedDescriptor].data;
        }
    },
    gridArea: {
        get() {
            return this[_protectedDescriptor].dom.style.gridArea;
        },
        set(value) {
            this[_protectedDescriptor].dom.style.gridArea = value;
        }
    }
});

ControlView.prototype.make = function(ctrl, dom) {

    let node = ctrl.template.cloneNode(true);

    if (dom.hasAttribute("id")) {
        node.setAttribute("id", dom.getAttribute("id"));
    } else {
        node.removeAttribute("id");
    }

    if (dom.classList.length) {
        node.classList.add(...dom.classList);
    }

    if (dom.children.length) {
        for (const element of dom.children) {
            if (element.hasAttribute("data-selector")) {
                //todo encoding
                const selector = element.getAttribute("data-selector");
                const qElems = node.querySelectorAll(selector);
                if (qElems.length > 1 || qElems.length <= 0) {
                    throw new Error("ControlView unable inherit dom (provided selector are invalid)")
                }
                qElems[0].replaceWith(element.cloneNode(true));
            } else {
                let selector = this.buildSelectorCandidate(element);
                while (selector.length) {
                    const qElems = node.querySelectorAll(selector.join("."));
                    if (qElems.length === 1) {
                        qElems[0].replaceWith(element.cloneNode(true));
                        break;
                    } else if (qElems.length > 1) {
                        const qElemsRev = dom.querySelectorAll(selector.join("."));
                        const indexOf = Array.prototype.indexOf.call(qElemsRev, element);
                        if (indexOf === -1 || !qElems[indexOf]) {
                            throw new Error("ControlView unable inherit dom (unable to determinate parity between elements)");
                        }
                        qElems[indexOf].replaceWith(element.cloneNode(true));
                    }
                    selector.pop();
                }
            }
            //console.log("ControlView.prototype.make", element.selector)
        }
    }

    return node;
}

ControlView.prototype.buildSelectorCandidate = function(dom) {
    let candidate = [];
    candidate.push(dom.tagName);
    candidate.push(...dom.classList.values());
    return candidate;
}

ControlView.prototype.doTerminate = function() {
    console.info("terminate", this.id)
    const protectedCtx = this[_protectedDescriptor];
    if (this.attached) {
        protectedCtx.ctrl.detachView(this);
    }
    protectedCtx.dom.remove();
}

Control.registry = new Registry();

Control._attachedEvents = plainObjectFrom();

Control._defaultHandler = function(evtId, object, ...arg) {
    if (Control._attachedEvents[evtId] != null) {
        Control._attachedEvents[evtId](object, ...arg);
    }
}

Control.genId = function (type, id) {
    return "control-" + type + "-" + id;
}

Control.declare = function (config, constructor, domTemplate) {

    config = Object.assign({}, Control.config, config);

    if (!(constructor === Control || constructor.prototype instanceof Control)) {
        throw new Error("constructor must be instance of Control");
    }
    if (!(domTemplate instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }
    if (!config.type.trim()) {
        throw new Error("config.type must be instance non empty string");
    }

    config.type     = config.type.trim();
    config.id       = config.id.trim() || Control.genId(config.type, Control.registry.incremental()).trim();
    config.groups   = config.groups.trim().split(",").map((s) => s.trim()).filter((s) => !!s);


    if (Control.registry.has(config.id)) {
        console.warn("control id aready regin", config.id);
        throw new Error("control id aready regin");
    } else {

        domTemplate.setAttribute("id",                config.id+"-decl" );
        domTemplate.setAttribute("data-control-id",         config.id         );
        domTemplate.setAttribute("data-control-type",       config.type       );
        domTemplate.setAttribute("data-control-groups",     config.groups.join(",")     );

        domTemplate.classList.add("control", config.type);
        if (config.groups.length) {
            domTemplate.classList.add("group");
            config.groups.forEach((grp) => {
                domTemplate.classList.add("group-"+grp);
            });
        }

        let instance = new constructor(domTemplate);

        if (Control.isDisabled()) {
            instance.disabled = true;
        }

        Control.registry.add(config.id, instance);

        if (config.groups.length) {
            Group.join(instance, config.groups);
        }

        return instance;
    }
}

Control.reference = function (ctrlId, domElem) {

    if (!(domElem instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    if (!Control.registry.has(ctrlId)) {
        throw new Error("element " + ctrlId + " not declared");
    }

    let ctrl = Control.registry.get(ctrlId);
    let ref  = ctrl.attachView(domElem);

    return {
        ctrl,
        ref,
        node: ref.dom
    }
}

Control.enable = function (){
    for (let ctr of Control.registry) {
        ctr.disabled = false;
    }
    //be careful with this descriptor
    Control.prototype[_disabledDescriptor] = false;
}

Control.disable = function (){
    for (let ctr of Control.registry) {
        ctr.disabled = true;
    }
    Control.prototype[_disabledDescriptor] = true;
}

Control.reset = function (context = Control.defaultContext){
    for (let ctrl of Control.registry) {
        try {
            ctrl.reset(context);
        } catch (e) { console.error(ctrl.type + ":" + ctrl.id, e); }
    }
}

Control.isDisabled = function (){
    return !!Control.prototype[_disabledDescriptor];
}

Control.attachEvent = function (eventId, callback) {
    Control._attachedEvents[eventId] = callback;
}

Control.detachEvent = function (eventId, callback) {
    delete Control._attachedEvents[eventId];
}

Control.config = {
    id:         "",
    type:       "",
    groups:     ""
}

function eraseFromChain(object, index, onlyFirst = true) {
    for (; object !== null ; object = Object.getPrototypeOf(object) ) {
        if (Object.hasOwn(object, index)) {
            delete object[index];
            if (onlyFirst) {
                break;
            }
        }
    }
}

//calling context mostly to prevent ctrl recursion
Control.defaultContext = plainObjectFrom({
    ctrl:           null,
    event:          null,
    silent:         false,
    next(context) {
        return Object.assign(Object.create(this), context);
    },
    contain(ctrl) {
        for (let context = this; context !== Control.defaultContext; context = Object.getPrototypeOf(context)) {
            if (context.ctrl === ctrl) {
                return true;
            }
        }
        return false;
    }
});

function Control(domTemplate) {
    if (!(domTemplate instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    this[_protectedDescriptor] = {
        domTemplate,
        views:          new Map(),
        handler:        Control._defaultHandler,
        globalEvents:   plainObjectFrom({}),
        events:         this.initEvents(domTemplate),
        attributes:     this.initAttributes(domTemplate, this.extractAttributesDefault(domTemplate)),
        viewsIncremental: 0,
    }
}

//extract html attributes from Control Attribute
Control.prototype.extractAttributesOptionsAttrs = function(e) {
    let attributes = {};
    for (const attr of e.attributes) {
        if (attr.name.startsWith("data-attribute-option-")) {
            const attrId = attr.name.replace("data-attribute-option-", "");
            if (attrId.length) {
                attributes[attrId] = attr.value;
            } else {
                console.warn("extractAttributesOptionsAttrs probably malformed attribute", attr);
            }
        }
    }
    return attributes;
}

Control.prototype.extractAttributesOptions = function(e) {
    let options = [];
    for (const child of e.children) {
        if (child.classList.contains("optgroup")) {
            let mock = this.extractAttributesOptionsAttrs(child);
            let mockList = this.extractAttributesOptions(child);
            mock.type  = "optgroup";
            mock.items = mockList;
            options.push(mock);
        } else if (child.classList.contains("option")) {
            let mock = this.extractAttributesOptionsAttrs(child);
            mock.type = "option";
            mock.label = child.innerText;
            options.push(mock);
        } else {
            console.warn("extractAttributesOptions invalid members present");
        }
    }
    return options;
}

//operate differently than other declFunction, it will not keep invalid
//decl due to impl limitation
Control.prototype.extractAttributesDefault = function(domTemplate) {
    let attributes = {};
    let attributesScope =  domTemplate.querySelector(".attributes");
    attributesScope = attributesScope ? attributesScope : domTemplate;
    attributesScope.querySelectorAll(":scope > .attribute").forEach((e, i) => {
        if (e.children.length) {
            const value = e.querySelector(".value");
            if (value) {
                attributes[e.getAttribute("data-attribute-id")] = {
                    value:  value.innerText,
                    params: Object.fromEntries(Array.from(e.querySelectorAll(".param").values()).map((e) => {
                        if (e.getAttribute("data-attribute-param-id") === "options") {
                            return [e.getAttribute("data-attribute-param-id"), this.extractAttributesOptions(e)];
                        } else {
                            return [e.getAttribute("data-attribute-param-id"), e.innerText];
                        }
                    })),
                }
            }
        } else {
            attributes[e.getAttribute("data-attribute-id")] = {
                value:  e.innerText,
                params: {},
            }
        }
        e.remove();
    });
    if (attributesScope !== domTemplate) {
        attributesScope.remove();
    }
    return attributes;
}

Control.prototype.initAttributes = function (dom, attributesDefaults = {}) {
    let ctx = this;
    return new Attributes({
        id: {
            label:          "id",
            //description: "unique id of this control",
            type:           Attribute.types.STRING,
            writeable:      false,
            enumerable:     false,
            value:          () => this.id,
        },
        type: {
            label:          "type",
            //description: "type of this control",
            type:           Attribute.types.STRING,
            writeable:      false,
            enumerable:     false,
            value:          () => this.type,
        },
        label: {
            label:      "label",
            //description: "label of this control",
            type:       Attribute.types.STRING,
            writeable:      false,
            enumerable:     false,
            value:      () => this.label,
        },
        groups: {
            label:          "groups",
            description:    "comma separated list of groups of this object",
            type:           Attribute.types.STRING,
            writeable:      false,
            enumerable:     false,
            value:          () => this.groupIds,
        },
        kbd: {
            label:          "kb key",
            description:    "key that pressed when control is activated",
            type:           Attribute.types.KBKEY,
            value:          () => this.getValue()
        },
        description: {
            label:          "description",
            //description: "",
            type:           Attribute.types.TEXT,
            writeable:      false,
            value:          () => this.description,
        }
    });
}

Control.prototype.initEvents = function(dom) {

    let ctx = this;
    let clickInProgress = false;
    let longTapTimer = 0;
    let startTouchPos = { x: 0, y: 0 };

    const complete = (e) => {
        clearTimeout(longTapTimer);
        clickInProgress = false;
        e.target.classList.remove("highlight");
        startTouchPos.x = NaN; startTouchPos.y = NaN;
    }

    const evtHandler = (e) => {

        if (ctx.disabled) {
            complete(e);
            if (e.type === "contextmenu") {
                e.preventDefault();
            }
            return;
        }

        const unified = unify(e);
        switch (e.type) {
            case "mousedown":
            case "touchstart":
                if (!clickInProgress) {
                    clickInProgress  = true;
                    e.target.classList.add("highlight"); //preactivation
                    longTapTimer = setTimeout(() => {
                        complete(e);
                        ctx.longclick( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                        HapticResponse.defaultInstance.vibrate([400]);
                    }, 1000);
                    startTouchPos.x = unified.pageX; startTouchPos.y = unified.pageY;
                }
                e.preventDefault(); //is it true needed?
                break;
            case "touchmove":
            case "mousemove":
                if (clickInProgress) {
                    if (distance(startTouchPos.x, startTouchPos.y, unified.pageX, unified.pageY) > quarterFingerPixels) {
                        complete(e);
                        console.info("touch canceled, too much move");
                    }
                }
                e.preventDefault();
                break;
            case "touchcancel":
                if (clickInProgress) {
                    complete(e);
                }
                e.preventDefault();
                break;
            case "mouseup":
            case "touchend":
                if (clickInProgress) {
                    complete(e);
                    ctx.click( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                    HapticResponse.defaultInstance.vibrate([200]);
                }
                e.preventDefault();
                break;
            case "contextmenu":
                e.preventDefault();
                break;
        }
    }

    return plainObjectFrom({
        mouseup:        evtHandler,
        mousedown:      evtHandler,
        mousemove:      evtHandler,
        touchstart:     evtHandler,
        touchend:       evtHandler,
        touchmove:      evtHandler,
        touchcancel:    evtHandler,
        contextmenu:    evtHandler,
    });
}

Control.prototype.hasValue = function(namespace = "") {
    const protectedCtx = this[_protectedDescriptor];
    const ns    = namespace.trim();
    return !!protectedCtx.domTemplate.querySelector(ns ? "kdb."+ns : "kbd:not([class]), kbd[class='']");
}

Control.prototype.getValue = function(namespace = "", defaultValue = null) {
    const protectedCtx = this[_protectedDescriptor];
    const ns    = namespace.trim();
    const kdb = protectedCtx.domTemplate.querySelector(ns ? "kbd."+ns : "kbd:not([class]), kbd[class='']");
    /*
    * the idea that kdb must have namespace (like default) or empty(class) kdb will-be used
    * do not overthink it, do not mess kbd with kbd.ns or if it necessary use kbd.default instead
    */
    if (kdb) {
        return kdb.innerText;
    } else {
        return defaultValue;
    }
}

Control.prototype.viewAt = function(candidate) {
    if (candidate instanceof ControlView) {
        if (candidate.control === this) {
            return candidate;
        }
    } else if (candidate instanceof Element) {
        if (candidate.hasAttribute("id")) {
            return this[_protectedDescriptor].views.get(candidate.getAttribute("id") || "");
        }
    } else if (candidate) {
        return this[_protectedDescriptor].views.get(candidate || "");
    }
    return undefined; //map return undefined
}

Control.prototype.isCtrl = function(value) {
    return value.trim().startsWith("call:");
}

Control.prototype.callCtrl = function(value, context = Control.defaultContext) {
    let ctrlId = value.substring("call:".length).trim();
    let action = "click";

    let sep = ctrlId.lastIndexOf(".");
    if (sep !== -1) {
        action = ctrlId.substring(sep+1, sep);
        ctrlId = ctrlId.substring(0, sep);
    }

    //eraseFromChain(context, "view");
    context.view = null; //just faster
    //view must be reset between control transfer or infinity recursion occur

    console.log("callctrl", ctrlId, action);

    if (Control.registry.has(ctrlId)) {
        let ctrl = Control.registry.get(ctrlId);
        switch (action) {
            case "click":
                ctrl.click(context.next({ ctrl: this }));
                break;
            case "dbclick":
                ctrl.dbclick(context.next({ ctrl: this }));
                break;
            case "longclick":
                ctrl.longclick(context.next({ ctrl: this }));
                break;
            default:
                throw new Error(`invalid ctrlId ${ctrlId} action ${action}`);
        }
    } else {
        throw new Error(`invalid ctrlId ${ctrlId}`);
    }
}

Control.prototype.dispatchKbd = function(kbd, ns, context = Control.defaultContext) {

    if (this.isCtrl(kbd)) {
        setTimeout((kbd) => this.callCtrl(kbd, context), 0, kbd); //move out
        kbd = ":" + this.id + ":" + ns;
    } else {
        kbd = kbd + ":" + this.id + ":" + ns;
    }

    this[_protectedDescriptor].handler("fire", this, kbd);

}

Control.prototype.attachView = function (refOrNode){
    const protectedCtx = this[_protectedDescriptor];
    if (refOrNode instanceof Element) {
        refOrNode = new ControlView(this, refOrNode);
    }
    if (refOrNode.attached) {
        throw new Error("view already attached");
    }
    if (!refOrNode.id) {
        console.log("genId", this.id + "-view-" + protectedCtx.viewsIncremental, protectedCtx.views);
        refOrNode.id = this.id + "-view-" + protectedCtx.viewsIncremental;
    }
    protectedCtx.views.set(refOrNode.id, refOrNode);
    refOrNode.attached = true;
    protectedCtx.viewsIncremental++;
    return refOrNode;
}

Control.prototype.detachView = function (refOrNode){
    const protectedCtx = this[_protectedDescriptor];
    let view = this.viewAt(refOrNode);
    if (!view) {
        throw new Error("view not found or argument incorrect");
    }
    if (view.activated) {
        this.deactivateView(view);
    }
    protectedCtx.views.delete(view.id);
    view.attached = false;
    return refOrNode;
}

Control.prototype.activateView = function(refOrNode){
    let view = this.viewAt(refOrNode);
    if (!view) {
        throw new Error("view not found or argument incorrect");
    }
    this.handleEvents(view.dom);
    view.activated = true;
}

Control.prototype.deactivateView = function(refOrNode){
    let view = this.viewAt(refOrNode);
    if (!view) {
        throw new Error("view not found or argument incorrect");
    }
    this.releaseEvents(view.dom);
    view.activated = false;
}

Control.prototype.handleEvents = function (view) {
    for (const [ns, handler] of Object.entries(this[_protectedDescriptor].events)) {
        view.addEventListener(ns, handler);
    }
}

Control.prototype.releaseEvents = function (view) {
    for (const [ns, handler] of Object.entries(this[_protectedDescriptor].events)) {
        view.removeEventListener(ns, handler);
    }
}

Control.prototype.reset = function(context = Control.defaultContext){
    throw new Error("not implemented");
}

Control.prototype.notifyGroupMembers = function(context = Control.defaultContext) {
    this.groupIds.split(",").forEach((grp) => {
        const group = Group.registry.get(grp);
        if (group && group.control) {
            group.forEach((ctrl) => {
                if (ctrl instanceof SwitchControl && ctrl !== this) {
                    const initialState = ctrl.state();
                    ctrl.switchOff(context.next({ ctrl: this, silent: true }));
                    if (!context.silent && initialState) {
                        this[_protectedDescriptor].handler("fire", this, ":" + ctrl.id + ":" + "switched-off");
                    }
                }
            });
        }
    });
}

Control.prototype.applyState = function(state){
    throw new Error("not implemented");
}

Control.prototype.click = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }
}

Control.prototype.dbclick = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }
}

Control.prototype.longclick = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }
}

Control.prototype.disableNodeTransition = function (dom, disable) {
    if (disable) {
        dom.classList.add("disabled");
    } else {
        dom.classList.remove("disabled");
    }
}

Control.prototype.disableTransition = function (disable) {
    let ctx = this;
    ctx.disableNodeTransition(ctx[_protectedDescriptor].domTemplate, disable);
    this[_protectedDescriptor].views.forEach((view) => ctx.disableNodeTransition(view.dom, disable));
}

Control.prototype.dispatchEvent = function(type, context, eData) {
    return this[_protectedDescriptor].handler.call(this, type, context, eData);
}

Control.prototype[Symbol.iterator] = function*() {
    const protectedCtx = this[_protectedDescriptor];
    for (const record of protectedCtx.views) {
        yield record[1];
    }
}

Object.defineProperties(Control.prototype, {
    id: {
        get: function() {
            return this[_protectedDescriptor].domTemplate.getAttribute("data-control-id");
        }
    },
    template: {
        get: function() {
            return  this[_protectedDescriptor].domTemplate;
        }
    },
/*    views: {
        get: function() {
            return  this[_protectedDescriptor].views;
        }
    },*/
    attributes: {
        get: function() {
            return  this[_protectedDescriptor].attributes;
        }
    },
    groupIds: {
        get: function() {
            return this[_protectedDescriptor].domTemplate.getAttribute("data-control-groups");
        }
    },
    type: {
        get: function() {
            return this[_protectedDescriptor].domTemplate.getAttribute("data-control-type");
        }
    },
    label: {
        get: function() {
            return this[_protectedDescriptor].domTemplate.querySelector("p").innerText;
        }
    },
    description: {
        get: function() {
            const dom = this[_protectedDescriptor].domTemplate.querySelector(".description");
            return dom ? dom.innerText : "";
        }
    },
    disabled: {
        get: function() {
            return !!this[_protectedDescriptor].disabled;
        },
        set: function(value) {
            this[_protectedDescriptor].disabled = !!value;
            this.disableTransition(this[_protectedDescriptor].disabled);
        }
    }
});


function OneshotControl(domTemplate) {
    Control.call(this, domTemplate);
}
Object.setPrototypeOf(OneshotControl.prototype, Control.prototype);

OneshotControl.prototype.click = function(context = Control.defaultContext) {

    Control.prototype.click.call(this, context);

    const protectedCtx = this[_protectedDescriptor];

    this.notifyGroupMembers(context);

    this.clickTransition();

    if (!context.silent) {
        let attributes = context.view ? context.view.attributes : protectedCtx.attributes;
        let kbd = attributes.get("kbd");
        if (this.isCtrl(kbd)) {
            setTimeout((kbd) => this.callCtrl(kbd, context), 0, kbd); //move out
            kbd = ":" + this.id + ":" + "click";
        } else {
            kbd = attributes.get("kbd") + ":" + this.id + ":" + "click";
        }

        protectedCtx.handler("fire", this, kbd);
    }
}

OneshotControl.prototype.reset = function(context = Control.defaultContext) {

}

OneshotControl.prototype.applyState = function(state, context = Control.defaultContext) {
    if (state === "click") {
        this.clickTransition();
    } else {
        throw new Error("undefined state");
    }
}

OneshotControl.prototype.clickTransition = function () {
    let ctx = this;
    ctx[_protectedDescriptor].domTemplate.classList.add("click");
    ctx[_protectedDescriptor].views.forEach((e) => {
        e.dom.classList.add("click");
    });
    setTimeout(() => {
        ctx[_protectedDescriptor].views.forEach((e) => {
            e.dom.classList.remove("click");
        });
        ctx[_protectedDescriptor].domTemplate.classList.remove("click");
    }, 250);
}

function SwitchControl(domTemplate) {
    Control.call(this, domTemplate);
    if (!domTemplate.classList.contains("switched-on") && !domTemplate.classList.contains("switched-off")) {
        domTemplate.classList.add("switched-off");
    }
    if (this[_protectedDescriptor].attributes.get("simple")) {
        this.simpleModeTransition(true);
    }
}
Object.setPrototypeOf(SwitchControl.prototype, Control.prototype);

SwitchControl.prototype.initAttributes = function(dom, attributesDefaults = {}) {

    let ctx = this;

    return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
        kbd: {
            label:          "kb key(now)",
            description:    "key that pressed when control is activated now",
            type:           Attribute.types.KBKEY,
            writeable:      false,
            value: (attributes)=> this.state() ? attributes.get("kbd-switched-on") : attributes.get("kbd-switched-off"),
        },
        "kbd-switched-on": {
            label:          "kb key(switched-on)",
            description:    "key that pressed activated when in state \"ON\"",
            type:           Attribute.types.KBKEY,
            apply: (value, oldValue, attributes) => {
                ctx.simpleModeTransition(attributes.get("simple") && (value === attributes.get("kbd-switched-off")));
            },
            value: () => this.getValue("switched-on") || this.getValue("", "")
        },
        "kbd-switched-off": {
            label:          "kb key(switched-off)",
            description:    "key that pressed activated when in state \"OFF\"",
            type:           Attribute.types.KBKEY,
            apply: (value, oldValue, attributes) => {
                ctx.simpleModeTransition(attributes.get("simple") && (value === attributes.get("kbd-switched-on")));
            },
            value: () => this.getValue("switched-off") || this.getValue("", "")
        },
        "href-switched-on": {
            label:          "href(switched-on)",
            description:    "href to go when activated in state \"ON\"",
            type:           Attribute.types.LINK,
            params:         defaultLinkPattern,
            value: () => (attributesDefaults["href-switched-on"] && attributesDefaults["href-switched-on"].value) || ""
        },
        "href-switched-off": {
            label:          "href(switched-off)",
            description:    "href to go when activated in state \"OFF\"",
            type:           Attribute.types.LINK,
            params:         defaultLinkPattern,
            value: () => (attributesDefaults["href-switched-off"] && attributesDefaults["href-switched-off"].value) || ""
        },
        state: {
            label:          "state",
            description:    "",
            type:           Attribute.types.STRING,
            writeable:      false,
            inheritable:    false,
            value: () => this.state() ? "switched-on" : "switched-off",
        },
        simple: {
            label:          "simplified transition",
            description:    "when active: if both state have equal behavior then control behave like oneshot (activate, without state change)",
            type:           Attribute.types.BOOLEAN,
            inheritable:    false,
            apply: function(value, oldValue, attributes) {
                ctx.simpleModeTransition(value && (attributes.get("kbd-switched-on") === attributes.get("kbd-switched-off")));
            },
            //if "simple" is set  its true false overwise
            value:          Object.hasOwn(attributesDefaults, "simple"),
        }
    }));

}

SwitchControl.prototype.simpleModeTransition = function(state) {
    if (state) {
        this[_protectedDescriptor].domTemplate.querySelectorAll(":scope > span").forEach(e => e.classList.add("hidden_block"));
        this[_protectedDescriptor].views.forEach(e => e.dom.querySelectorAll(":scope > span").forEach(e => e.classList.add("hidden_block")));
    } else {
        this[_protectedDescriptor].domTemplate.querySelectorAll(":scope > span").forEach(e => e.classList.remove("hidden_block"));
        this[_protectedDescriptor].views.forEach(e => e.dom.querySelectorAll(":scope > span").forEach(e => e.classList.remove("hidden_block")));
    }
}


SwitchControl.prototype.click = function(context = Control.defaultContext) {

    if (this.isSimplyfied(context)) {
        console.log("simple mode impl");
        const state = this.state();
        this.switchTo(!state, context);
        if (!state) { //if we are on -> turnoff, if we off -> turnon -> turnoff
            setTimeout(() => {
                this.switchTo(state, context.next({ silent: true }));
                this[_protectedDescriptor].handler("fire", this, ":" + this.id + ":" + "switched-off");
            }, 250);
        }
    } else {
        this.switchTo(!this.state(), context);
    }

}

SwitchControl.prototype.longclick = function(context = Control.defaultContext) {
    this.switchTo(!this.state(), context.next({ silent: true }));
}

SwitchControl.prototype.isSimplyfied = function(context) {
    const protectedCtx = this[_protectedDescriptor];
    const attributes = context.view ? context.view.attributes : protectedCtx.attributes;
    if (attributes.get("simple")) {
        let kbdOn  =  attributes.get("kbd-switched-on");
        let kbdOff =  attributes.get("kbd-switched-off");
        if (kbdOn === kbdOff) {
            return true;
        }
    }
    return false
}

SwitchControl.prototype.switchOn = function(context = Control.defaultContext) {
    this.switchTo(true, context);
}

SwitchControl.prototype.switchOff = function(context = Control.defaultContext) {
    this.switchTo(false, context);
}

SwitchControl.prototype.switchTo = function(state, context = Control.defaultContext) {

    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }

    const protectedCtx = this[_protectedDescriptor];
    const stateOn = this.state();
    if (stateOn === state) {
        return;
    }

    if (!stateOn) {
        this.notifyGroupMembers(context);
    }

    this.stateTransition(stateOn);

    if (!context.silent) {
        const attributes = context.view ? context.view.attributes : protectedCtx.attributes;
        let kbd  =  attributes.get(stateOn ? "kbd-switched-on"  : "kbd-switched-off");
        let href =  attributes.get(stateOn ? "href-switched-on" : "href-switched-off");
        this.dispatchKbd(kbd, stateOn ? "switched-off" :  "switched-on", context);
        if (href.trim() && !this.isSimplyfied(context)) {
            LinkControl.prototype.goTo.call(this, href);
        }
    }
}

SwitchControl.prototype.state = function() {
    return this[_protectedDescriptor].domTemplate.classList.contains("switched-on");
}

SwitchControl.prototype.stateTransition = function(stateOn) {
    this[_protectedDescriptor].views.forEach((e) => {
        e.dom.classList.replace(stateOn ? "switched-on" :  "switched-off", stateOn ? "switched-off" : "switched-on");
    });
    this[_protectedDescriptor].domTemplate.classList.replace(stateOn ? "switched-on" :  "switched-off", stateOn ? "switched-off" : "switched-on");
}

SwitchControl.prototype.reset = function(context = Control.defaultContext) {
    this.applyState("switched-off", context);
}

SwitchControl.prototype.applyState = function(state, context = Control.defaultContext) {
    console.info("SwitchControl apply state", state)
    if (state === "switched-on" || state === "switched-off") {
        this.switchTo(state === "switched-on", context.next({ silent: true }));
    } else {
        throw new Error("undefined state");
    }
}

function LinkControl(domTemplate) {
    if (!domTemplate.getAttribute("data-route")) {
        throw new Error("domTemplate must be have route");
    }
    Control.call(this, domTemplate);
}
Object.setPrototypeOf(LinkControl.prototype, Control.prototype);

LinkControl.prototype.initAttributes = function(dom, attributesDefaults = {}) {
    return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
        routeTo: {
            label:          "routeTo",
            description:    "route to follow when activated",
            type:           Attribute.types.LINK,
            params:         defaultLinkPattern,
            value: () => this[_protectedDescriptor].domTemplate.getAttribute("data-route") || "",
        },
    }));
}

LinkControl.prototype.click = function(context = Control.defaultContext) {
    OneshotControl.prototype.clickTransition.call(this);
    this.goTo(context.view.attributes.get("routeTo"));
}

LinkControl.prototype.reset = function(context = Control.defaultContext) {

}

LinkControl.prototype.goTo = function(route) {
    route = route.replace("#", "");
    if (route.startsWith("/")) {
        window.location.hash = route;
    } else {
        if (window.location.hash.endsWith("/")) {
            window.location.hash += route;
        } else {
            window.location.hash = "/" + route;
        }
    }
}

function RepeatControl(domTemplate) {
    SwitchControl.call(this, domTemplate);
    this[_protectedDescriptor].task  = {
        intervalHandler: 0,
        finish: true
    };
}
Object.setPrototypeOf(RepeatControl.prototype, SwitchControl.prototype);

RepeatControl.prototype.initAttributes = function(dom, attributesDefaults = {}) {
    return SwitchControl.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
        repeatDelay: {
            label:          "delay",
            description:    "delay in ms between auto activate",
            units:          "ms",
            type:           Attribute.types.INTERVAL,
            params: Object.assign({},{ step: 1, min: 100, max: 10000}, attributesDefaults.repeatDelay ? attributesDefaults.repeatDelay.params : {}),
            value: () => {
                return attributesDefaults["repeatDelay"] !== undefined ?  +attributesDefaults["repeatDelay"].value : 2000;
            },
            validate(value) {
                let validationResult = Object.assign({}, Attribute.validationResult);
                validationResult.mutableValue = +value;
                validationResult.valid = true;
                if (validationResult.mutableValue !== parseInt(value)) {
                    validationResult.valid = false
                    validationResult.mutableValue = value;
                    validationResult.errors.push("value not integer");
                    return validationResult;
                }
                if (validationResult.mutableValue < this.params.min || validationResult.mutableValue > this.params.max) {
                    validationResult.valid = false
                    validationResult.errors.push("value not in range");
                }
                return validationResult;
            }
        },
        repeatCount: {
            label:          "repeat count",
            description:    "total count of repeats, 0 meaning infinity",
            type:           Attribute.types.INT,
            params: Object.assign({},{ step: 1, min: 0, max: 100}, attributesDefaults.repeatCount ? attributesDefaults.repeatCount.params : {}),
            value: () => {
                return attributesDefaults["repeatCount"] !== undefined ?  attributesDefaults["repeatCount"].value : 0;
            },
            validate(value) {
                let validationResult = Object.assign({}, Attribute.validationResult);
                validationResult.mutableValue = +value;
                validationResult.valid = true;
                if (validationResult.mutableValue !== parseInt(value)) {
                    validationResult.valid = false
                    validationResult.mutableValue = value;
                    validationResult.errors.push("value not integer");
                    return validationResult;
                }
                if (validationResult.mutableValue < this.params.min || validationResult.mutableValue > this.params.max) {
                    validationResult.valid = false
                    validationResult.errors.push("value not in range");
                }
                return validationResult;
            }
        },
        timeLeft: {
            label:      "",
            enumerable:  false,
            inheritable: false,
            type: Attribute.types.INTERVAL,
            description: "",
            value: 0,
        },
        leftRepeats: {
            label: "",
            enumerable:  false,
            inheritable: false,
            description: "",
            value: 0,
            type: Attribute.types.INT,
        },
        simple: {
            type: Attribute.types.BOOLEAN,
            enumerable: false,
            writeable:  false,
            inheritable:false,
            value: false,
        }
    }));
}

RepeatControl.prototype.switchTo = function(state, context = Control.defaultContext) {
    const prevState = this.state();
    SwitchControl.prototype.switchTo.call(this, state, context.next({ silent: true }));

    let ctx = this;
    let protectedCtx = this[_protectedDescriptor];

    if (!prevState && (prevState !== state)) {
        this.notifyGroupMembers(context);
    }

    if (prevState !== state) {

        if (state && !context.silent) {

            let attributes =  context.view ? context.view.attributes : protectedCtx.attributes;

            const kbd   = attributes.get("kbd-switched-on");
            const delay = attributes.get("repeatDelay");
            const count = attributes.get("repeatCount");
            attributes.set("leftRepeats", count);

            protectedCtx.handler("fire", this, ":" + this.id + ":" + "switched-on" );

            protectedCtx.task = {
                intervalHandler: 0,
                finish: false,
                state,
                delay,
                kbd,
                count,
                attributes,
                context
            }

            this.handleTask(protectedCtx.task);
            if (!protectedCtx.task.finish) {
                protectedCtx.task.intervalHandler = setInterval(this.handleTask.bind(this), delay, protectedCtx.task);
            }

        } else {
            console.log("cancel repeat")
            clearInterval(protectedCtx.task.intervalHandler);
            protectedCtx.task.finish = true;
            if (!context.silent) {
                protectedCtx.handler("fire", this, ":" + this.id + ":" + "switched-off" );
            }
        }
    }
}

RepeatControl.prototype.disableTransition = function(disable) {
    SwitchControl.prototype.disableTransition.call(this, disable);
    let protectedCtx = this[_protectedDescriptor];
    if (disable) {
        if (!protectedCtx.task.finish) {
            clearInterval(this[_protectedDescriptor].task.intervalHandler);
        }
    } else {
        if (!protectedCtx.task.finish) {
            protectedCtx.task.intervalHandler = setInterval(this.handleTask.bind(this), protectedCtx.task.delay, protectedCtx.task);
        }
    }
}

RepeatControl.prototype.handleTask = function(task) {
    const protectedCtx = this[_protectedDescriptor];
    if (this.isCtrl(task.kbd)) {
        this.callCtrl(task.kbd, task.context.next({ ctrl: this }));
    } else {
        protectedCtx.handler("fire", this, task.kbd);
    }
    if (task.count) {
        console.warn(task.attributes.get("leftRepeats"))
        if (task.attributes.increment("leftRepeats", -1) <= 0) {
            console.log("term")
            clearInterval(task.intervalHandler);
            task.finish = true;
            //no next because we not transfer control to anyone
            setTimeout((state, context) => this.switchTo(false, context), 0, false, task.context);
        }
    }
}

function RemoteRepeatControl(domTemplate) {
    RepeatControl.call(this, domTemplate);
    if (!domTemplate.classList.contains("switched-on") && !domTemplate.classList.contains("switched-off")) {
        domTemplate.classList.add("switched-off");
    }
}
Object.setPrototypeOf(RemoteRepeatControl.prototype, RepeatControl.prototype);

RemoteRepeatControl.prototype.switchTo = function(state, context = Control.defaultContext) {
    const prevState = this.state();
    SwitchControl.prototype.switchTo.call(this, state, context.next({ silent: true }));
    if (prevState !== state) {
        let protectedCtx = this[_protectedDescriptor];
        if (!context.silent) {
            const attributes =  context.view ?  context.view.attributes : protectedCtx.attributes;
            const kbd   = attributes.get("kbd-switched-on");
            const delay = attributes.get("repeatDelay") || 0;
            const count = attributes.get("repeatCount") || -1;
            if (state) {
                protectedCtx.handler("fire", this, kbd + ":" + this.id + ":" + (state ? "switched-on" : "switched-off") + ":" + delay + ":" + count);
            } else {
                protectedCtx.handler("fire", this, ":" + this.id + ":" + "switched-off" + "::");
            }

        }
    }
}

function DirectControl(domTemplate) {
    Control.call(this, domTemplate);
    if (!domTemplate.classList.contains("switched-on") && !domTemplate.classList.contains("switched-off")) {
        domTemplate.classList.add("switched-off");
    }
}
Object.setPrototypeOf(DirectControl.prototype, Control.prototype);

DirectControl.prototype.initAttributes = function(dom, attributesDefaults = {}) {

    let ctx = this;

    return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
        kbd: {
            label:          "kb key(now)",
            description:    "key that pressed when control is activated now",
            type:           Attribute.types.KBKEY,
            writeable:      false,
            value: (attributes) => this.state() ? attributes.get("kbd-switched-on") : attributes.get("kbd-switched-off"),
        },
        "kbd-switched-on": {
            label:          "kb key(switched-on)",
            description:    "key that pressed activated when in state \"ON\"",
            type:           Attribute.types.KBKEY,
            value: () => this.getValue("switched-on") || this.getValue("", "")
        },
        "kbd-switched-off": {
            label: "kb key(switched-off)",
            description: "key that pressed activated when in state \"OFF\"",
            type: Attribute.types.KBKEY,
            value: () => this.getValue("switched-off") || this.getValue("", ""),
        },
        state: {
            label: "state",
            description: "",
            type: Attribute.types.STRING,
            writeable:      false,
            inheritable:    false,
            value: () => this.state() ? "switched-on" : "switched-off",
        }
    }));

}

DirectControl.prototype.initEvents = function (dom) {

    let ctx = this;

    const evtHandler = (e) => {
        if (ctx.disabled) {
            if (e.type === "contextmenu") {
                e.preventDefault();
            }
            return;
        }

        const unified = unify(e);

        switch (e.type) {
            case "mousedown":
            case "touchstart":
                ctx.switchOn( Control.defaultContext.next({ event: e, unified }) );
                HapticResponse.defaultInstance.vibrate([200]);
                break;
            case "touchcancel":
                //ctx.switchOff(); //cause shadow cancel
            case "touchmove":
            case "mousemove":
                e.preventDefault();
                break;
            case "mouseup":
            case "touchend":
                ctx.switchOff( Control.defaultContext.next({ event: e, unified }) );
                break;
            case "contextmenu":
                e.preventDefault();
                break;
        }
    }

    return plainObjectFrom({
        mouseup:        evtHandler,
        mousedown:      evtHandler,
        mousemove:      evtHandler,
        touchstart:     evtHandler,
        touchend:       evtHandler,
        touchmove:      evtHandler,
        touchcancel:    evtHandler,
        contextmenu:    evtHandler,
    });
}

DirectControl.prototype.switchOn = function(context = Control.defaultContext) {
    this.switchTo(true, context);
}

DirectControl.prototype.switchOff = function(context = Control.defaultContext) {
    this.switchTo(false, context);
}

DirectControl.prototype.switchTo = function(state, context = Control.defaultContext) {

    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }

    const stateOn = this.state();
    if (stateOn === state) {
        return;
    }

    this.stateTransition(stateOn);

    const protectedCtx = this[_protectedDescriptor];

    const attributes = context.view ? context.view.attributes : protectedCtx.attributes;
    let kbd =  attributes.get(stateOn ? "kbd-switched-on" :  "kbd-switched-off");
    if (this.isCtrl(kbd)) {
        if (!context.silent) {
            setTimeout((kbd) => this.callCtrl(kbd, context), 0, kbd); //move out
        }
        kbd = ":" + this.id + ":" + (stateOn ? "switched-off" :  "switched-on");
    } else {
        kbd = kbd + ":" + this.id + ":" + (stateOn ? "switched-off" :  "switched-on");
    }

    if (!context.silent) {
        protectedCtx.handler("fire", this, kbd);
    }
}

DirectControl.prototype.state = function() {
    return this[_protectedDescriptor].domTemplate.classList.contains("switched-on");
}

DirectControl.prototype.stateTransition = function(stateOn) {
    this[_protectedDescriptor].views.forEach((e) => {
        e.dom.classList.replace(stateOn ? "switched-on" :  "switched-off", stateOn ? "switched-off" : "switched-on");
    });
    this[_protectedDescriptor].domTemplate.classList.replace(stateOn ? "switched-on" :  "switched-off", stateOn ? "switched-off" : "switched-on");
}

DirectControl.prototype.reset = function(context = Control.defaultContext) {
    this.applyState("switched-off", context);
}

DirectControl.prototype.applyState = function(state, context = Control.defaultContext) {
    console.info("DirectControl apply state", state)
    if (state === "switched-on" || state === "switched-off") {
        this.switchTo(state === "switched-on", context.next({ silent: true }));
    } else {
        throw new Error("undefined state");
    }
}

function MemoControl(domTemplate) {
    Control.call(this, domTemplate);
    if (!domTemplate.classList.contains("switched-on") && !domTemplate.classList.contains("switched-off")) {
        domTemplate.classList.add("switched-off");
    }
}
Object.setPrototypeOf(MemoControl.prototype, Control.prototype);

MemoControl.prototype.initAttributes = function(dom, attributesDefaults) {
    return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
        kbd: {
            label:          "kb key(activate)",
            description:    "key that pressed when control is activated",
            type:           Attribute.types.KBKEY,
            value: () => this.getValue()
        },
        "kbd-set": {
            label: "kb key(setup)",
            description: "key that pressed when \"setup\" happened",
            type: Attribute.types.KBKEY,
            value: () => this.getValue("set") || ""
        },
        "kbd-clear": {
            label: "kb key(reset)",
            description: "key that pressed when \"clear\" happened",
            type: Attribute.types.KBKEY,
            value: () => this.getValue("clear") || ""
        },
        state: {
            label: "state",
            description: "",
            type: Attribute.types.STRING,
            writeable:   false,
            inheritable: false,
            value: () => this.state() ? "switched-on" : "switched-off",
        },
        description: {
            label: "description",
            type: Attribute.types.TEXT,
            writeable: false,
            value: () => this.description || "this is three stage component - set, activate, clear\n set - double tap (or tap if ctrl is not \"set\")\n activate - tap (available only if ctrl is \"set\" )\n clear - longtap",
        }
    }));
}

MemoControl.prototype.initEvents = function(dom) {

    let ctx = this;
    let keydownTime, prevKeydownTime;
    let clickInProgress = false;
    let touches = 0;
    let taps = 0;
    let tapTimer, longTapTimer = 0;
    let startTouchPos = { identifier: "mouse", x: 0, y: 0 };

    const complete = (e) => {
        clearTimeout(tapTimer);
        clearTimeout(longTapTimer);
        clickInProgress = false;
        e.target.classList.remove("highlight");
        startTouchPos.identifier = "mouse"; startTouchPos.x = startTouchPos.y = NaN;
    }

    const evtHandler = (e) => {

        if (ctx.disabled) {
            complete(e);
            if (e.type === "contextmenu") {
                e.preventDefault();
            }
            return;
        }

        const unified = unify(e);
        const touchId = e.identifier === undefined ? "mouse" : e.identifier;

        //note to future me about touchId and "check memo and other touchId usage"
        //this widget actually do not support multitouch as .set fire immediately after touchstart targetTouches.length === 2
        //any other cases processed in mouse compatible fation

        switch (e.type) {
            case "touchstart":
                if (e.targetTouches.length === 2) {
                    complete(e);
                    ctx.set(Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }));
                    HapticResponse.defaultInstance.vibrate([100, 100]);
                    e.preventDefault();
                    break;
                }
            case "mousedown":
                prevKeydownTime = keydownTime; keydownTime = new Date();
                if (clickInProgress && (keydownTime - prevKeydownTime < 300)) {
                    complete(e);
                    ctx.set(Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }));
                    HapticResponse.defaultInstance.vibrate([100, 100]);
                } else {
                    clickInProgress  = true;
                    e.target.classList.add("highlight"); //preactivation
                    longTapTimer = setTimeout(() => {
                        complete(e);
                        ctx.clear( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                        HapticResponse.defaultInstance.vibrate([400]);
                    }, 1000);
                }
                //not really need? to keep all touch points as e.targetTouches > 1 auto resolve as set
                //and swipe check only first event anyway
                startTouchPos.identifier = touchId; startTouchPos.x = unified.pageX; startTouchPos.y = unified.pageY;
                e.preventDefault();
                break;
            case "touchcancel":
                complete(e);
                e.preventDefault();
                break;
            case "touchmove":
            case "mousemove":
                console.log("move");
                if (clickInProgress) {
                    //console.log("move inner", startTouchPos.identifier !== touchId, distance(startTouchPos.x, startTouchPos.y, unified.pageX, unified.pageY) );
                    if (startTouchPos.identifier !== touchId || distance(startTouchPos.x, startTouchPos.y, unified.pageX, unified.pageY) > quarterFingerPixels) {
                        complete(e);
                    }
                }
                e.preventDefault();
                break;
            case "mouseup":
            case "touchend":
                const pressTime = (new Date()) - keydownTime;
                if (clickInProgress) {
                    clearTimeout(longTapTimer);
                    if (pressTime >= 300 && pressTime < 1000) {
                        complete(e);
                        ctx.click( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                        HapticResponse.defaultInstance.vibrate([200]);
                    } else {
                        tapTimer = setTimeout(() => {
                            complete(e);
                            ctx.click( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                            HapticResponse.defaultInstance.vibrate([200]);
                        }, 300 - pressTime);
                    }
                }
                e.preventDefault();
                break;
            case "contextmenu":
                if (e.buttons === 2) {
                    complete(e);
                    ctx.clear( Control.defaultContext.next({ event: e, unified, view: ctx.viewAt(e.currentTarget) }) );
                    HapticResponse.defaultInstance.vibrate([400]);
                }
                e.preventDefault();
                break;
        }
    }

    return plainObjectFrom({
        mouseup:        evtHandler,
        mousedown:      evtHandler,
        mousemove:      evtHandler,
        touchstart:     evtHandler,
        touchend:       evtHandler,
        touchmove:      evtHandler,
        touchcancel:    evtHandler,
        contextmenu:    evtHandler,
    });
}

MemoControl.prototype.set = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }
    SwitchControl.prototype.switchTo.call(this,true, context.next({ silent:true }));

    if (!context.silent) {
        const attributes =  context.view ?  context.view.attributes : this[_protectedDescriptor].attributes;
        let kbd = attributes.get("kbd-set");
        this.dispatchKbd(kbd, "switched-on", context);
    }
}

MemoControl.prototype.clear = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }
    SwitchControl.prototype.switchTo.call(this, false, context.next({ silent:true }));

    if (!context.silent) {
        const attributes =  context.view ?  context.view.attributes : this[_protectedDescriptor].attributes;
        let kbd = attributes.get("kbd-set");
        this.dispatchKbd(kbd, "switched-off", context);
    }
}

MemoControl.prototype.activate = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.contain(this)) {
        throw new Error("Ctrl activation must not lead to recursion of any kind");
    }

    this.notifyGroupMembers(context);

    if (!context.silent) {
        const attributes =  context.view ?  context.view.attributes : this[_protectedDescriptor].attributes;
        let kbd = attributes.get("kbd-set");
        this.dispatchKbd(kbd, "click", context);
    }
}

MemoControl.prototype.dispatchKbd = function(kbd, ns, context = Control.defaultContext) {

    if (this.isCtrl(kbd)) {
        setTimeout((kbd) => this.callCtrl(kbd, context), 0, kbd); //move out
        kbd = ":" + this.id + ":" + ns;
    } else {
        kbd = kbd + ":" + this.id + ":" + ns;
    }

    this[_protectedDescriptor].handler("fire", this, kbd);

}

MemoControl.prototype.click = function(context = Control.defaultContext) {
    if (this.state()) {
        this.activate(context);
    } else {
        this.set(context);
    }
}

MemoControl.prototype.longclick = function(context = Control.defaultContext) {
    if (context !== Control.defaultContext && context.touchesCnt >= 2) {
        this.clear(context);
    } else {
        this.set(context);
    }
}

MemoControl.prototype.state = SwitchControl.prototype.state;

MemoControl.prototype.stateTransition = SwitchControl.prototype.stateTransition;

MemoControl.prototype.reset = function(context = Control.defaultContext) {
    this.applyState("switched-off", context);
}

MemoControl.prototype.applyState = function(state, context = Control.defaultContext) {
    if (state === "switched-on") {
        this.set(context.next({ silent: true }));
    } else if (state === "switched-off") {
        this.clear(context.next({ silent: true }));
    } else {
        throw new Error("undefined state");
    }
}

function BackControl(domTemplate) {
    Control.call(this, domTemplate);
}
Object.setPrototypeOf(BackControl.prototype, Control.prototype);

BackControl.prototype.initAttributes = function(dom, attributesDefaults) {
    return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes(), ["kbd"]);
}

BackControl.prototype.click = function(context = Control.defaultContext) {
    Control.prototype.click.call(this, context)
    window.history.back();
}

BackControl.prototype.reset = function(context = Control.defaultContext) {

}


function InvalidControl(domTemplate) {
    Control.call(this, domTemplate);
}
Object.setPrototypeOf(InvalidControl.prototype, Control.prototype);


function Router() {
    window.addEventListener("hashchange", console.info);
    this[_protectedDescriptor] = {
        beginning:              false,
        hashHandlerInstance:    this.hashHandler.bind(this),
        routes:                 Object.create(null),
        current:                this.pathFromHash((new URL(window.location)).hash),
        notFoundRoute:          null,
    };
}

Router.prototype.begin = function (trigger = false) {
    if (this[_protectedDescriptor].beginning) {
        return;
    }
    console.info("Router begin")
    window.addEventListener("hashchange", this[_protectedDescriptor].hashHandlerInstance);
    this[_protectedDescriptor].beginning = true;
    if (trigger) {
        this[_protectedDescriptor].current = null;
        this[_protectedDescriptor].hashHandlerInstance({
            newURL: window.location,
            oldURL: window.location
        });
    }
}

Router.prototype.end = function () {
    if (this[_protectedDescriptor].beginning) {
        console.info("Router end");
        window.removeEventListener("hashchange", this[_protectedDescriptor].hashHandlerInstance);
        this[_protectedDescriptor].beginning = false;
    }
}

Router.prototype.hashHandler = function (event) {
    let newHash = this.pathFromHash("" + (new URL(event.newURL)).hash);
    if (newHash === this[_protectedDescriptor].current) {
        console.info("suppress same location route", this[_protectedDescriptor].current);
        return true;
    }
    this[_protectedDescriptor].current = newHash;
    if (this.hasStaticRoute(newHash)) {
        this[_protectedDescriptor].routes[newHash].callback({
            newURL: newHash,
            oldURL: this.pathFromHash("" + (new URL(event.oldURL)).hash),
        }, this[_protectedDescriptor].routes[newHash].params);
    } else {
        if (this[_protectedDescriptor].notFoundRoute && isCallable(this[_protectedDescriptor].notFoundRoute.callback)) {
            this[_protectedDescriptor].notFoundRoute.callback({
                newURL: newHash,
                oldURL: this.pathFromHash("" + (new URL(event.oldURL)).hash),
            }, this[_protectedDescriptor].notFoundRoute.params);
            console.warn("path not set or not implemented", "using 404 handler");
        } else {
            throw new Error("path not set or not implemented");
        }
    }
}

Router.prototype.addStaticRoute = function (path, callback, params = {}) {
    if (Object.hasOwn(this[_protectedDescriptor].routes, path)) {
        throw new Error("path already set");
    }
    this[_protectedDescriptor].routes[path] = {
        callback: callback,
        params: params,
        static: true,
    };

}

Router.prototype.addStaticNotFoundRoute = function (callback, params = {}) {
    this[_protectedDescriptor].notFoundRoute = {
        callback,
        params,
        static: true,
    }
}

Router.prototype.hasStaticRoute = function (path) {
    return Object.hasOwn(this[_protectedDescriptor].routes, path) && this[_protectedDescriptor].routes[path].static;
}

Router.prototype.info = function (path, include404 = true) {
    if (Object.hasOwn(this[_protectedDescriptor].routes, path)) {
        return this[_protectedDescriptor].routes[path].params;
    } else {
        if (include404 && this[_protectedDescriptor].notFoundRoute) {
            return this[_protectedDescriptor].notFoundRoute.params;
        }
    }
}

Router.prototype.info404 = function () {
    return this[_protectedDescriptor].notFoundRoute.params;
}

Router.prototype.pathFromHash = function (hash) {
    if (hash.trim().startsWith("#")) {
        return hash.replace('#', "");
    } else {
        return hash;
    }
}

Object.defineProperties(Router.prototype,  {
    path: {
        get() {
            return this.pathFromHash(window.location.hash);
        }
    },
    isBeginning: {
        get() {
            return this[_protectedDescriptor].beginning;
        }
    },
    current: {
        get() {
            return this[_protectedDescriptor].current;
        }
    }
})

Router.prototype.overridePath = function (path) {
    this[_protectedDescriptor].current = path;
}

PageLayout.defaultConfig = {
    columnCnt:      5,
    rowCnt:         5,
    cellGap:        1,
    incellPadding:  0.7,
    fontSize:       "120%",
    areas:          "",
}

PageLayout.normalizeValue = (key, value, config) => {
    if (Number.isInteger(PageLayout.defaultConfig[key]) || Number.isFinite(PageLayout.defaultConfig[key])) {
        return +value;
    } else if (key === "areas") {
        return PageLayout.prototype.parseAreasValue.call(undefined, value);
    } else if (typeof PageLayout.defaultConfig[key] === "string") {
        return value.trim();
    } else {
        return  value;
    }
}

PageLayout.normalizeConfig = (config) => {
    for (const [key, value] of Object.entries(PageLayout.defaultConfig)) {
        config[key] = PageLayout.normalizeValue(key, config[key], config);
    }
    return config;
}

PageLayout.equalConfig = (c1, c2, normalize = true) => {
    let c1v, c2v;
    let keys = 0;
    for (const [key, value] of Object.entries(PageLayout.defaultConfig)) {
        if (normalize) {
            c1v = PageLayout.normalizeValue(key, c1[key], c1);
            c2v = PageLayout.normalizeValue(key, c2[key], c2);
        } else {
            c1v = c1[key];
            c2v = c2[key];
        }
        if (c1v !== c2v) {
            return false
        }
        keys++;
    }
    return !!keys;
}

PageLayout.hasConfig = (key) => {
    return Object.hasOwn(PageLayout.defaultConfig, key);
}

function PageLayout(pg, defaultValues = {}) {

    if (!(pg instanceof Page)) {
        throw new Error("pg must be instance of Page");
    }

    const protectedCtx = this[_protectedDescriptor] = {
        owner:              pg,
        innerCellPadding:   undefined,
        fontSize:           undefined,
        defaultValues:      defaultValues = Object.assign({}, PageLayout.defaultConfig, defaultValues),
    }

    for (const [key, value] of Object.entries(PageLayout.defaultConfig)) {
        const thisValue = this[key];
        const newValue  = PageLayout.normalizeValue(key, protectedCtx.defaultValues[key], protectedCtx.defaultValues);
        protectedCtx.defaultValues[key] = newValue;
        if (thisValue === undefined) {
            //layout not init
            if (newValue) {
                //console.log("set layout value", key, thisValue, newValue);
                this[key] = newValue;
            }
        } else if (thisValue !== newValue) {
            //console.log("updated layout value", key, thisValue, newValue);
            this[key] = newValue;
        } else {
            //console.log("discarded layout value", key, newValue);
        }
    }

    Object.freeze(defaultValues);

    const normalized = this.parseAreasValue(""+defaultValues.areas);
    if (normalized.length) {
        const dimensions = this.parseAreasDimensions(normalized);
        if (dimensions.invalid) {
            throw new Error("invalid layout params (dimensions)", protectedCtx.owner.id);
        }
        const layoutSlots = dimensions.rowCnt * dimensions.colCnt;
        protectedCtx.owner.dom.querySelectorAll(".control").forEach((elm, i) => {
            elm.style.gridArea = Page.elementTag(i, layoutSlots);
        });
    } else {
        console.info("page", protectedCtx.owner.id, "have no any layout");
        //opt protectedCtx.owner.dom.querySelectorAll(".control").length
        //but it's problematic as if normalized.length === 0 there is no slots => no slot indexes
    }

}

PageLayout.prototype.parseTemplColRowCount = function(strval) {
    let candidate = 0;
    if (strval.indexOf("repeat(") === -1) {
        //incase gridTemplateColumns is normalized
        return  countEntrys(strval, "minmax");
    } else {
        return  parseInt(strval.substring("repeat(".length));
    }
}

PageLayout.prototype.parseAreasValue = function(strval) {
    return strval.replace(/\s+/g, " ").trim().replace(/"\s+|\s+"/g, '"');
}

PageLayout.prototype.parseAreasDimensions = function(strval) {
    let rowCnt = 0;
    let colCnt = 0;
    let invalid = false;
    for (let index = strval.indexOf("\""); index !== -1; index = strval.indexOf("\"", index+1)) {
        let rangeStart = index;
        if ((index = strval.indexOf("\"", rangeStart + 1)) !== -1) {
            let rangeEnd = index;
            let rowColCnt = 0;
            rowCnt++;
            while ((rangeStart = strval.indexOf(" ", rangeStart + 1)) !== -1 && rangeStart < rangeEnd) {
                rowColCnt++;
            }
            rowColCnt++;
            colCnt = Math.max(rowColCnt, colCnt);
            invalid |= colCnt !== rowColCnt;
        }
        //console.info("parseAreasDimensions", index);
    }
    return {
        rowCnt,
        colCnt: colCnt || 1,
        invalid
    }
}

PageLayout.prototype.equalConfig = function(config, normalize = true) {
    let keys = 0;
    let cv;
    for (const [key, value] of Object.entries(PageLayout.defaultConfig)) {
        if (normalize) {
            cv = PageLayout.normalizeValue(key, config[key], config);
        } else {
            cv = config[key];
        }
        console.log(this[key] !== cv, this[key], cv, typeof this[key], typeof cv);
        if (this[key] !== cv) {
            return false
        }
        keys++;
    }
    return !!keys;
}

PageLayout.prototype.hasConfig = function(key) {
    return PageLayout.hasConfig(key);
}

PageLayout.prototype.applyTo = function(ref) {
    if (!(ref instanceof ControlView)) {
        throw new Error("invalid ref");
    }

    const incellPadding = this.incellPadding;
    if (incellPadding) {
        ref.dom.style.padding = `${incellPadding}rem`;
    }

    const fontSize = this.fontSize;
    if (fontSize) {
        ref.dom.style.fontSize = fontSize;
    }

}

Object.defineProperties(PageLayout.prototype,  {
    columnCnt: {
        get: function() {
            const candidate = this.parseTemplColRowCount(this[_protectedDescriptor].owner.dom.style.gridTemplateColumns.trim());
            if (isNaN(candidate) || candidate === 0) {
                return undefined;
            }
            return candidate;
        },
        set: function(cnt) {
            cnt = +cnt;
            if (isNaN(cnt) || cnt === undefined || cnt === null) {
                throw new Error("invalid layout params")
            }
            this[_protectedDescriptor].owner.dom.style.gridTemplateColumns = `repeat(${cnt}, minmax(0, 1fr))`;
        }
    },
    rowCnt: {
        get: function() {
            const candidate = this.parseTemplColRowCount(this[_protectedDescriptor].owner.dom.style.gridTemplateRows.trim());
            if (isNaN(candidate) || candidate === 0) {
                return undefined;
            }
            return candidate;
        },
        set: function(cnt) {
            cnt = +cnt;
            if (isNaN(cnt) || cnt === undefined || cnt === null) {
                throw new Error("invalid layout params")
            }
            this[_protectedDescriptor].owner.dom.style.gridTemplateRows = `repeat(${cnt}, minmax(0, 1fr))`;
        }
    },
    cellGap: {
        get: function() {
            let candidate = parseFloat(this[_protectedDescriptor].owner.dom.style.gridGap);
            if (isNaN(candidate)) {
                return undefined;
            }
            return candidate;
        },
        set: function(value) {
            value = parseFloat(value);
            if (isNaN(value) || value === undefined || value === null) {
                throw new Error("invalid layout params")
            }
            this[_protectedDescriptor].owner.dom.style.gridGap = `${value}rem`;
        }
    },
    incellPadding: {
        get: function() {
            return this[_protectedDescriptor].innerCellPadding;
        },
        set: function(value) {
            value = parseFloat(value);
            if (isNaN(value) || value === undefined || value === null) {
                throw new Error("invalid layout params");
            }
            this[_protectedDescriptor].owner.dom.querySelectorAll(".control").forEach((elm, i)=>{
                elm.style.padding = `${value}rem`;
            });
            this[_protectedDescriptor].innerCellPadding = value;
        }
    },
    fontSize: {
        get: function() {
            const protectedCtx = this[_protectedDescriptor];
            //getComputedStyle(dom).fontSize;
            return protectedCtx.fontSize;
        },
        set: function(value) {
            const protectedCtx = this[_protectedDescriptor];
            if (!/\d+(%|px|em|pt)/.test(value)) {
                throw new Error("invalid layout params");
            }
            protectedCtx.fontSize = value;
            protectedCtx.owner.dom.querySelectorAll(".control").forEach((elm, i)=>{
                elm.style.fontSize = value;
            });
        }
    },
    areas: {
        get: function() {
            let areasValue = this[_protectedDescriptor].owner.dom.style.gridTemplateAreas || undefined;
            if (areasValue) {
                areasValue = this.parseAreasValue("" + areasValue);
            }
            return areasValue;
        },
        set: function(value) {
            //const normalized = this.parseAreasValue(""+value);
/*            if (!normalized.length) {
                throw new Error("invalid layout params");
            }
            const dimensions = this.parseAreasDimensions(normalized);
            if (dimensions.invalid) {
                throw new Error("invalid layout params (dimensions)", this[_protectedDescriptor].owner.id);
            }
            this[_protectedDescriptor].owner.dom.querySelectorAll(".control").forEach((elm, i) => {
                elm.style.gridArea = Page.elementTag(i, dimensions.rowCnt * dimensions.colCnt);
            });*/
            this[_protectedDescriptor].owner.dom.style.gridTemplateAreas = value;
        }
    },
    defaults: {
        get: function() {
            return this[_protectedDescriptor].defaultValues;
        }
    },
    isDefault: {
        get: function() {
            let protectedCtx = this[_protectedDescriptor];
            //value must be normalized for this
            for (const [key, value] of Object.entries(PageLayout.defaultConfig)) {
                if (this[key] !== protectedCtx.defaultValues[key]) {
                    return false;
                }
            }
            return true;
        },
    }
});

Page.registry = new Registry();

Page.declare = function (config, constructor, domTemplate) {

    config = Object.assign({}, Page.config, config);

    if (!(constructor === Page || constructor.prototype instanceof Page)) {
        throw new Error("constructor must be instance of Page");
    }
    if (!(domTemplate instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    config.id = config.id.trim() || Page.genId(Page.registry.incremental()).trim();

    if (Page.registry.has(config.id)) {
        throw new Error("page id already regin");
    } else {
        domTemplate.setAttribute("id", config.id);
        domTemplate.setAttribute("data-page-id", config.id);

        domTemplate.classList.add("page");
        let instance = new constructor(domTemplate);
        Page.registry.add(config.id, instance);

        return instance;
    }
}

Page.config = {
    id: "",
}

Page.genId = function (type, id) {
    return "page-" + id;
}

Page.genViewId = function(ctrlId, pageId, viewIndex) {
    return`${ctrlId}-${pageId}-view-${viewIndex}`;
}

Page.elementTag = function (index, total) {
    const length = total.toString(26).length;
    const asciiStart =  "a".charCodeAt(0);
    return String.fromCharCode.apply(String, index.toString(26).split("").map(e => {
        return !isNaN(+e) ? (+e + asciiStart) : (e.charCodeAt(0) + 10);
    })).padStart(length, "a");
    //return "item-" + index.toString().padStart(length, '0');
}

Page.hash = function (value) {
    return value;
}

Page.defaultArea = function (col, row) {
    let itemId = 0;
    let layout = ""
    const total = (row * col);
    for (let i = 0; i < row; i++) {
        layout += "\""
        for (let j = 0; j < col; j++) {
            layout += Page.elementTag(itemId++, total);
            if (j !== col-1) {
                layout += " ";
            }
        }
        layout += "\"\n"
    }
    return layout;
}

function Page(domTemplate) {
    if (!(domTemplate instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    let layoutCfgDecl = domTemplate.querySelector(".layout-decl");
    let layoutCfg = {};
    if (layoutCfgDecl) {
        layoutCfgDecl.querySelectorAll(".value").forEach((value) => {
            const valueType = value.getAttribute("data-layout-value") || "";
            if (valueType) {
                layoutCfg[valueType] = value.innerText;
            }
        });
        layoutCfgDecl.remove();
    }

    const protectedCtx = this[_protectedDescriptor] = {
        domTemplate,
        layout:         null,
        domEvents:      {},
        refs:           new Map,
        ctrlNamingCounter: Object.create(null),
        staticFingerprint: "", //"compileTime" fingerprint
    }

    domTemplate.querySelectorAll(".control").forEach((elm, i ) => {
        const ctrlId = elm.getAttribute("data-control-id") || "";
        if (!ctrlId) {
            console.warn("on page: ", domTemplate.getAttribute("id"), "unspecified control:", elm);
            return;
        }
        try {
            this.addControl(ctrlId, { element: elm, validateRoot: false, applyLayout: false });
            //there is no read need for store Control in internal page registry as Control.registry do it for us.
        } catch (e) {
            console.warn("on page: ", domTemplate.getAttribute("id"), "error on control init:",  e);
        }
        protectedCtx.staticFingerprint = Page.hash(protectedCtx.staticFingerprint + ctrlId);
    });

    protectedCtx.layout = new PageLayout(this, layoutCfg); //the chicken and the egg problem
    protectedCtx.active = false;
}

Page.prototype.fingerprint = function() {
    let finger = "";
    this[_protectedDescriptor].domTemplate.querySelectorAll(".control").forEach((elm, i) => {
        finger = Page.hash(finger + (elm.getAttribute("data-control-id") || ""));
    });
    return finger;
}

Page.prototype.generateViewId = function(ctrlId) {
    const protectedCtx = this[_protectedDescriptor];
    if (protectedCtx.ctrlNamingCounter[ctrlId] === undefined) {
        protectedCtx.ctrlNamingCounter[ctrlId] = 0;
    }
    return Page.genViewId(ctrlId, this.id, protectedCtx.ctrlNamingCounter[ctrlId]++);
}

Page.defaultAddControlConfig = {
    element:        null,
    viewId:         undefined,
    validateRoot:   undefined,
    classList:      undefined,
    applyLayout:    true,
}

Page.prototype.addControl = function(ctrlId, refConfig = Page.defaultAddControlConfig) {

    const protectedCtx = this[_protectedDescriptor];
    let {element, viewId, validateRoot} = refConfig = Object.assign({}, Page.defaultAddControlConfig, refConfig);
    validateRoot = validateRoot === undefined ? !!element : validateRoot;

    if (!element) {
        element = document.createElement("div");
        element.classList.add("control");
        if (refConfig.classList) {
            element.classList.add(refConfig.classList);
        }
        element.setAttribute("data-control-id", ctrlId);

        protectedCtx.domTemplate.appendChild(element);
    } else {
        if (validateRoot) {
            const page = element.closest("[data-page-id=" + this.id + "]");
            if (!page || page !== this[_protectedDescriptor].domTemplate) {
                throw new Error("invalid element root");
            }
        }
        const cvid = element.getAttribute("data-view-id");
        if (cvid) {
            if (!cvid.startsWith("_")) {
                throw new Error("data-view-id attribute must be prefixed with \"_\"");
            }
            if (refConfig.classList) {
                element.classList.add(refConfig.classList);
            }
            viewId = Page.genViewId(ctrlId, this.id, cvid);
            console.info(`page ${this.id}, ctrl ${ctrlId} override its viewId to ${viewId}`);
        }
    }

    if (!viewId) {
        viewId = this.generateViewId(ctrlId);
        if (protectedCtx.ctrlNamingCounter[ctrlId] > 1) {
            console.warn(`ctrl ${ctrlId} on page ${this.id} have more than one view instance (${protectedCtx.ctrlNamingCounter[ctrlId]}) use data-view-id to distinguish it`);
        }
    }


    element.setAttribute("id", viewId);


    const bind = Control.reference(ctrlId, element);
    bind.ref.data.page = this;

    if (refConfig.applyLayout) {
        protectedCtx.layout.applyTo(bind.ref);
    }

    protectedCtx.refs.set(bind.ref.id, bind.ref);

    if (this.active) {
        bind.ctrl.activateView(bind.ref);
        if (bind.ctrl.suspend instanceof Function) {
            bind.ctrl.suspend(this.suspend, bind.ref, this);
        }
    }

    return bind;
}

Page.prototype.removeControl = function(nodeOrRef) {
    const protectedCtx = this[_protectedDescriptor];
    if (nodeOrRef instanceof Element) {
        nodeOrRef = protectedCtx.refs.get(nodeOrRef.id);
    }
    if (!(nodeOrRef instanceof ControlView)) {
        throw new Error("nodeOrRef invalid type");
    }
    if (!nodeOrRef.attributes.get("removed")) {
        if (!nodeOrRef.attributes.set("removed", true)) {
            throw new Error("nodeOrRef unable remove");
        }
    }
    protectedCtx.refs.delete(nodeOrRef.id);
}

Page.prototype.activate = function() {
    this.doActivate(true);
    this[_protectedDescriptor].active = true;
}

Page.prototype.deactivate = function() {
    this.doActivate(false);
    this[_protectedDescriptor].active = false;
}

Page.prototype.doActivate = function(activate = true) {
    for (const ref of this) {
        if (activate) {
            ref.control.activateView(ref);
            if (ref.control.suspend instanceof Function) {
                ref.control.suspend(this.suspend, ref, this);
            }
        } else {
            ref.control.deactivateView(ref.dom);
        }
    }
}

Page.prototype.doSuspend = function(suspend = true) {
    for (let [, ref] of this[_protectedDescriptor].refs) {
        if (ref.control.suspend instanceof Function) {
            ref.control.suspend(suspend, ref, this);
        }
    }
}

Page.prototype[Symbol.iterator] = function* () {
    const protectedCtx = this[_protectedDescriptor];
    for (const record of protectedCtx.refs) {
        yield record[1];
    }
}

Object.defineProperties(Page.prototype, {
        id: {
            get: function() {
                return this[_protectedDescriptor].domTemplate.getAttribute("data-page-id");
            }
        },
        dom: {
            get: function() {
                return this[_protectedDescriptor].domTemplate;
            }
        },
        layout: {
            get: function() {
                return this[_protectedDescriptor].layout;
            }
        },
        active: {
            get: function() {
                return this[_protectedDescriptor].active;
            },
            set: function(active) {
                if (this[_protectedDescriptor].active !== active) {
                    this.doActivate(active);
                    this[_protectedDescriptor].active = active;
                }
            }
        },
        suspend: {
            get: function() {
                return this[_protectedDescriptor].suspend;
            },
            set: function(suspend) {
                if (this[_protectedDescriptor].suspend !== suspend) {
                    this.doSuspend(suspend);
                    this[_protectedDescriptor].suspend = suspend;
                }
            }
        },
        suspendable: {
            get: function() {
                return !!this[_protectedDescriptor].ref.reduce((acc, ref) => {
                    return acc + (ref.control.suspend instanceof Function ? 1 : 0);
                }, 0);
            },
        },
        staticFingerprint: {
            get: function() {
                return this[_protectedDescriptor].staticFingerprint;
            },
        }
});

function Pager(elem) {

    if (!(elem instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    let ctx = {
        container:  elem,
        template:   elem.querySelector("li"),
        active:     elem.querySelector(".active"),
        activityHandler: 0,
    };

    ctx.template.classList.remove("active");
    ctx.container.innerHTML = "";
    ctx.active = null;

    return plainObjectFrom({
        hide() {
            ctx.container.classList.add("hidden_block");
        },
        show() {
            ctx.container.classList.remove("hidden_block");
        },
        notify() {
            ctx.container.classList.remove("hidden_opacity");
            clearTimeout(ctx.activityHandler);
            ctx.activityHandler = setTimeout(() => {
                ctx.container.classList.add("hidden_opacity");
            }, 1000);
        },
        addPage(href) {
            ctx.container.classList.add("hidden_opacity");
            clearTimeout(ctx.activityHandler);
            let clone = ctx.template.cloneNode(true);
            clone.querySelector("a").href = href;
            ctx.container.appendChild(clone);
        },
        clear() {
            ctx.container.classList.add("hidden_opacity");
            clearTimeout(ctx.activityHandler);
            ctx.container.innerHTML = "";
        },
        page(i) {
            ctx.container.classList.remove("hidden_opacity");
            const allOpt = ctx.container.children;
            if (!allOpt[i]) {
                console.error("swipe to non exist page");
                return;
            }
            if (ctx.active) {
                ctx.active.classList.remove("active");
            }
            allOpt[i].classList.add("active");
            ctx.active = allOpt[i];
            clearTimeout(ctx.activityHandler);
            ctx.activityHandler = setTimeout(() => ctx.container.classList.add("hidden_opacity"), 3000);
        }
    });
}

Swiper.default = {
    underlayFadeOutOnNewPage: true,
}
function Swiper(container, config = Swiper.default) {
    if (!(container instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    if (config !== Swiper.default) {
        config = Object.assign({}, Swiper.default, config);
    }

    let count = 0;

    let privateCtx = {
        container: container,
        config,

        itemsUpdaterFrame       : undefined,
/*        sizePromise             : null,*/

        _afterStart             : emptyFn,
        _afterDragStart         : emptyFn,
        _drag                   : emptyFn,
        _afterTransitionStart   : emptyFn,
        _afterEnd               : emptyFn,
        _underlay               : null,

        nextTransition: "",
        nextTransform:  "",
        transition: "",
        transform:  "",

        nextUnderlayTransition: "",
        nextUnderlayTransform:  "",
        nextUnderlayOpacity:    "",

        locked:     false,
        dragging:   true,
        i: 0,
        w:  undefined,
        fw: undefined,
        N: container.children.length,
        x0: null,
        touchId: undefined,

        nextState: () => {
            if (!privateCtx.nextFrame) {
                privateCtx.nextFrame = requestAnimationFrame(privateCtx.animationFrame);
            }
        },

        animationFrame: (timepast) => {

            if (privateCtx.nextTransition) {
                privateCtx.container.style.transition = privateCtx.transition = privateCtx.nextTransition; privateCtx.nextTransition = "";
            }
            if (privateCtx.nextTransform) {
                privateCtx.container.style.transform = privateCtx.transform = privateCtx.nextTransform; privateCtx.nextTransform = "";
            }

            if (privateCtx.nextUnderlayTransition) {
                privateCtx._underlay.style.transition = privateCtx.nextUnderlayTransition; privateCtx.nextUnderlayTransition = "";
            }
            if (privateCtx.nextUnderlayTransform) {
                privateCtx._underlay.style.transform = privateCtx.nextUnderlayTransform; privateCtx.nextUnderlayTransform = "";
            }
            if (privateCtx.nextUnderlayOpacity) {
                privateCtx._underlay.style.opacity = privateCtx.nextUnderlayOpacity; privateCtx.nextUnderlayOpacity = "";
            }

            privateCtx.nextFrame = undefined;
        },

        lock: (e) => {
            /*e.preventDefault();*/ //preventDefault in touchStart kill mouse event emulation and as it kill any click event
            /*e.stopPropagation();*/

            if (!privateCtx.locked) {
                const unified = unify(e);

                privateCtx.locked   = true;
                privateCtx.dragging = false;
                privateCtx.x0       = unified.clientX;
                privateCtx.touchId  = unified.identifier === undefined ? "mouse" : unified.identifier;

                privateCtx.nextTransition = "initial";
                privateCtx.nextTransform  = "";
                if (privateCtx._underlay) {
                    privateCtx.nextUnderlayTransition = privateCtx.nextTransition;
                    privateCtx.nextUnderlayTransform  = privateCtx.nextTransform;
                }
                privateCtx.nextState();

                setTimeout(privateCtx._afterStart, 0, privateCtx.i);
            }

        },

        leave: (e) => {
            e.preventDefault();
            /*e.stopPropagation();*/

            if (touchById(e, privateCtx.touchId)) {

                privateCtx.x0 = null;
                privateCtx.tx = 0;

                if (privateCtx.locked ) {
                    privateCtx.locked   = false;
                    privateCtx.dragging = false;
                    privateCtx.touchId  = undefined;

                    privateCtx.swipeToIndex(privateCtx.i, 0.4, privateCtx._afterEnd);
                    privateCtx._afterTransitionStart( privateCtx.i,  false);
                }
            }

        },

        move: (e) => {
            /*e.preventDefault();*/ //preventDefault in touchStart kill mouse event emulation and as it kill any click event
            /*e.stopPropagation();*/

            let unified;
            if(privateCtx.locked && (unified = touchById(e, privateCtx.touchId))) {

                let dx = unified.clientX - privateCtx.x0, s = Math.sign(dx),
                    f = +(s*dx/privateCtx.w).toFixed(2),
                    pageIndex = privateCtx.i, prevPageIndex = privateCtx.i;

                if((pageIndex > 0 || s < 0) && (pageIndex < privateCtx.N - 1 || s > 0) && f > .2) {
                    pageIndex -= s;
                    f = 1 - f;
                }

                privateCtx.locked   = false;
                privateCtx.dragging = false;
                privateCtx.touchId  = undefined;
                privateCtx.tx = 0;
                privateCtx.delay = (f * 0.5);
                privateCtx.x0 = null;

                privateCtx.swipeToIndex(pageIndex, privateCtx.delay, privateCtx._afterEnd);
                privateCtx._afterTransitionStart(pageIndex,  pageIndex !== prevPageIndex);
            }
        },

        drag: (e) => {
            //e.preventDefault();
            /*e.stopPropagation();*/

            /**
             * preventDefault ie "cancel event" also will kill browse pan
             * in that case manual pan for browser UI must be implemented
             *
             * there we are protected somehow from event emulation (if it may)
             * by keeping event identity that mast be differ for different pointer type (mouse dont have id)
             * */
            let unified;
            if(privateCtx.locked && (unified = touchById(e, privateCtx.touchId))) {

                const offset = Math.round(unified.clientX - privateCtx.x0);
                if (privateCtx.i === 0) { //scroll before 0 allow only for 25%
                    if (offset >= privateCtx.w * .25) {
                        return;
                    }
                }
                if (privateCtx.i === privateCtx.N -1) { //scroll after last allow only for 25%
                    if (offset <= -privateCtx.w * .25) {
                        return;
                    }
                }

                const abs = Math.round((privateCtx.i / privateCtx.N) * -privateCtx.fw + (privateCtx.tx = offset));
                privateCtx.nextTransform = 'translate(' + abs + 'px)';
                if (privateCtx._underlay) {
                    privateCtx.nextUnderlayOpacity =  Math.max(1.0 - Math.abs(offset) / (privateCtx.w * .5), 0.0);
                }
                privateCtx.nextState();

                if (!privateCtx.dragging) {
                    privateCtx.dragging = true;
                    privateCtx._afterDragStart(privateCtx.i, offset, abs);
                }

                privateCtx._drag(privateCtx.i, offset, abs);

            }

        },

        updateSize: () => {
            //is always sync to reduce complexity then both page changing and size calculation are ongoing
            //this may be change
            //there boundingClientRect for compatibility with narrow phones
            //todo respect padding
            const sizeRoot = privateCtx.container.parentElement || document.body;
            privateCtx.w  = sizeRoot.clientWidth || window.innerWidth;
            privateCtx.fw = privateCtx.N > 0 ? privateCtx.N * privateCtx.w : privateCtx.container.offsetWidth;
            console.info("resize", privateCtx.w, privateCtx.fw);
        },

        size: (e) =>{
            privateCtx.locked   = false;
            privateCtx.updateSize();
            privateCtx.setIndex(privateCtx.i);
        },

        setIndex: (index) => {
            const offset = Math.round(((privateCtx.i = index) / privateCtx.N) * -privateCtx.fw);
            privateCtx.nextTransition = "initial";
            privateCtx.nextTransform  = 'translate(' + offset + 'px)';
            if (privateCtx._underlay) {
                privateCtx.nextUnderlayTransition = privateCtx.nextTransition;
                privateCtx.nextUnderlayTransform  = 'translate(' + -offset + 'px)';
            }
            privateCtx.nextState();
        },

        swipeToIndex: (index, duration = 0.4, callback = emptyFn) => {
            const prevIndex = privateCtx.i;
            const offset = Math.round(((privateCtx.i = index) / privateCtx.N) * -privateCtx.fw);
            const nextTransform  = 'translate(' + offset + 'px)';
            const pageChanged = prevIndex !== index;
            if (nextTransform !== privateCtx.transform) {
                privateCtx.nextTransition = "transform " + duration + "s ease-out";
                privateCtx.nextTransform  = nextTransform;
                if (privateCtx._underlay) {
                    privateCtx.nextUnderlayTransition = "opacity " + duration + "s ease-out";
                    privateCtx.nextUnderlayOpacity    = pageChanged ? "0.0" : "1.0";
                }
                privateCtx.nextState();
                privateCtx.container.addEventListener("transitionend", () => {
                    if (privateCtx._underlay && pageChanged) {
                        privateCtx.nextUnderlayTransform = 'translate(' + -offset + 'px)';
                        if (!privateCtx.config.underlayFadeOutOnNewPage) {
                            privateCtx.nextUnderlayOpacity = "1.0";
                        }
                        privateCtx.nextState();
                    }
                    callback(privateCtx.i, pageChanged);
                }, { once: true });
            } else {
                setTimeout(callback, 0, privateCtx.i, pageChanged);
            }
        },

        updateItemCount: () => {
            privateCtx.N  = privateCtx._underlay ? privateCtx.container.children.length - 1 : privateCtx.container.children.length;
            privateCtx.fw = privateCtx.w * privateCtx.N;
            if (privateCtx.itemsUpdaterFrame === undefined) {
                privateCtx.itemsUpdaterFrame = requestAnimationFrame(() => {
                    //console.log("privateCtx.itemsUpdaterFrame2",  privateCtx.fw, privateCtx.container.children.length)
                    privateCtx.container.style.width = (privateCtx.N * 100) + "%";
                    for (let child of privateCtx.container.children) {
                        if (child === privateCtx._underlay) {
                            //specific size or continue
                            //continue;
                        }
                        const width = (100 / privateCtx.N) + "%";
                        child.style.width    = width;
                        child.style.minWidth = width;
                        child.style.maxWidth = width;
                    }
                    privateCtx.itemsUpdaterFrame = undefined;
                });
            }
        },

    };

    return plainObjectFrom({
        begin: () => {

            addEventListener('resize', privateCtx.size, false);
            privateCtx.container.addEventListener('mousedown',  privateCtx.lock,  false);
            privateCtx.container.addEventListener('touchstart', privateCtx.lock,  false);
            privateCtx.container.addEventListener('mouseup',    privateCtx.move,  false);
            privateCtx.container.addEventListener('touchend',   privateCtx.move,  false);
            privateCtx.container.addEventListener('mousemove',  privateCtx.drag,  false);
            privateCtx.container.addEventListener('touchmove',  privateCtx.drag,  false);
            privateCtx.container.addEventListener('mouseleave', privateCtx.leave, false);
            privateCtx.updateSize();

        },
        end: () => {

            removeEventListener('resize', privateCtx.size);
            privateCtx.container.removeEventListener('mousedown',   privateCtx.lock);
            privateCtx.container.removeEventListener('touchstart',  privateCtx.lock);
            privateCtx.container.removeEventListener('mouseup',     privateCtx.move);
            privateCtx.container.removeEventListener('touchend',    privateCtx.move);
            privateCtx.container.removeEventListener('mousemove',   privateCtx.drag);
            privateCtx.container.removeEventListener('touchmove',   privateCtx.drag);
            privateCtx.container.removeEventListener('mouseleave',  privateCtx.leave);

        },
        index: () => {
            return privateCtx.i;
        },
        setIndex: (index) => {
            if (privateCtx.i === index) {
                return;
            }
            privateCtx.setIndex( index);
        },
        swipeToIndex: (index) => {
            const prevPageIndex = privateCtx.i;
            privateCtx.swipeToIndex(index, 0.4, privateCtx._afterEnd);
            privateCtx._afterTransitionStart(index, index !== prevPageIndex);
        },
        next: () => {
            if (privateCtx.i + 1 < privateCtx.N) {
                privateCtx.swipeToIndex(privateCtx.i+1);
            }
        },
        prev: () => {
            if (privateCtx.i - 1 >= 0) {
                privateCtx.swipeToIndex(privateCtx.i-1);
            }
        },
        addItem: (el) => {
            if (privateCtx._underlay) {
                privateCtx.container.insertBefore(el, privateCtx._underlay);
            } else {
                privateCtx.container.appendChild(el);
            }
            //console.log("privateCtx.itemsUpdaterFrame0", privateCtx.container.children.length);
            privateCtx.updateItemCount();
        },
        removeItem: (el) => {
            for (const child of privateCtx.container) {
                if (child === el) {
                    if (child === privateCtx._underlay) {
                        throw new Error("incorrect API call");
                    }
                    child.remove();
                    break;
                }
            }
            console.log("privateCtx.itemsUpdaterFrame00", privateCtx.container.children.length);
            privateCtx.updateItemCount();
        },
        clearItems: () => {
            let     children = privateCtx.container.children;
            const   length = privateCtx._underlay ? 1 : 0;
            while (children.length > length) {
                children[0].remove();
            }
            privateCtx.updateItemCount();
        },
        itemCount: () => {
            return privateCtx.N;
        },
        afterStart: (as) => {
            as = isCallable(as) ? as : emptyFn;
            privateCtx._afterStart = as;
        },
        afterTransitionStart: (at) => {
            at = isCallable(at) ? at : emptyFn;
            privateCtx._afterTransitionStart = at;
        },
        afterDragStart: (ads) => {
            ads = isCallable(ads) ? ads : emptyFn;
            privateCtx._afterDragStart = ads;
        },
        drag: (ds) => {
            ds = isCallable(ds) ? ds : emptyFn;
            privateCtx._afterDragStart = ds;
        },
        afterEnd: (ae) => {
            ae = isCallable(ae) ? ae : emptyFn;
            privateCtx._afterEnd = ae;
        },
        underlay: (underlay) => {
            if (underlay && !(underlay instanceof Element)) {
                throw new Error("domTemplate must be instance of Element");
            }
            let prev = privateCtx._underlay;
            if (prev) {
                prev.remove();
            }
            if (underlay) {
                if (underlay.parentElement !== privateCtx.container) {
                    privateCtx.container.appendChild(underlay);
                }
                const offset = -Math.round((privateCtx.i  / privateCtx.N) * -privateCtx.fw);
                if (offset) {
                    privateCtx.nextUnderlayTransform  = 'translate(' + offset + 'px)';
                    privateCtx.nextUnderlayTransition = 'initial';
                    privateCtx.nextState();
                }
            } else {
                //check it
                privateCtx.nextUnderlayTransform = privateCtx.nextUnderlayTransition = privateCtx.nextUnderlayOpacity = "";
            }
            privateCtx._underlay = underlay;
            return prev;
        }
    });
}

function HapticResponse() {
    this[_protectedDescriptor] = { enabled: !!navigator.vibrate }
}

HapticResponse.prototype.vibrate = function(pattern, reason = undefined) {
    if (this.enabled) {
        navigator.vibrate(pattern);
    } else {
        console.warn("vibrate", pattern, reason);
    }
}

Object.defineProperties(HapticResponse.prototype, {
    available: {
        get() {
            return !!navigator.vibrate;
        }
    },
    enabled: {
        get() {
            return !!navigator.vibrate && this[_protectedDescriptor].enabled;
        },
        set(value) {
            console.warn("HapticResponse", value);
            this[_protectedDescriptor].enabled = value;
        }
    }
});

HapticResponse.defaultInstance = new HapticResponse();

function ModalLocker(container, cssList) {

    if (!(container instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    if (Object.hasOwn(container, _modalLockerSymbol)) { // low possibility of memory leak there. We bind view and ctrl object totherer
        console.info("reuse of locker");
        return container[_modalLockerSymbol];
    } else {

        let ctx = {
            container: container,
            elem: null,
            pthtElm: null,
            pthtElmParent: null,
            beforeLock: null,
            beforeUnlock: null,
            make: () => {
                const res = document.createElement("div");
                res.classList.add("modal-bg-wrapper");
                return res;
            }
        }

        ctx.elem = ctx.make();
        ctx.elem.style.display = "none";
        ctx.elem.classList.add.apply(ctx.elem.classList, cssList);
        container.appendChild(ctx.elem);

        const handler = plainObjectFrom({
            beforeLock: (cb) => {ctx.beforeLock = cb},
            lock: () => { isCallable(ctx.beforeLock) ? ctx.beforeLock() : null; ctx.elem.style.display = "flex"; },
            beforeUnlock: (cb) => {ctx.beforeUnlock = cb},
            unlock: () => { isCallable(ctx.beforeUnlock) ? ctx.beforeUnlock() : null; ctx.elem.style.display = "none"; },
            paththrough: (modelElem) => {
                if (!(modelElem instanceof Element)) {
                    throw new Error("domTemplate must be instance of Element");
                }
                if (ctx.pthtElmParent != null) {
                    ctx.pthtElm.remove();
                    ctx.pthtElmParent.appendChild(ctx.pthtElm);
                }
                ctx.pthtElmParent = modelElem.parentElement;
                ctx.pthtElm = modelElem
                modelElem.remove();
                ctx.elem.appendChild(modelElem);
            },
            get dom() {
                return ctx.elem;
            },
        });

        container[_modalLockerSymbol] = handler;

        return handler;
    }


}

Window.registry = new Registry();

Window.config = {
    id: "",
}

Window.genId = function (type, id) {
    return "window-" + id;
}

Window.declare = function (config, constructor, domTemplate) {

    config = Object.assign({}, Window.config, config);

    if (!(constructor === Window || constructor.prototype instanceof Window)) {
        throw new Error("constructor must be instance of Window");
    }
    if (!(domTemplate instanceof Element)) {
        throw new Error("domTemplate must be instance of Element");
    }

    config.id = config.id.trim() || Window.genId(Window.registry.incremental()).trim();

    if (Window.registry.has(config.id)) {
        throw new Error("window id aready regin");
    } else {
        domTemplate.setAttribute("id", config.id+"-decl");
        domTemplate.setAttribute("data-window-id",   config.id);
        domTemplate.classList.add("window");
        let instance = new constructor(domTemplate);
        Window.registry.add(config.id, instance);
    }
}

function Window(elem) {
    if (!(elem instanceof Element)) {
        throw new Error("elem must be instance of Element");
    }

    this[_protectedDescriptor] = {

    }

    this.elem      = elem;
    this.display   = null;
    this.titleElem = elem.querySelector(".header .title");
    this.submitBtn = elem.querySelector(".header .submit");
    this.cancelBtn = elem.querySelector(".header .cancel");
    this[_isShowingSymbol] = false;
    this._result   = null;
    this.onsubmit  = null;
    this.onreject  = null;

    let ctx = this;
    if (this.submitBtn) {
        this.submitBtn.addEventListener("click", (e) => {
            e.preventDefault();
            if (isCallable(ctx.onsubmit)) {
                const data = ctx.data;
                if (ctx.validate(data)) {
                    ctx.hide();
                    ctx.onsubmit({
                        submit: true,
                        valid:  true,
                        data:   data
                    });
                }
            } else {
                ctx.hide();
            }
            return false;
        });
    }

    if (this.cancelBtn) {
        this.cancelBtn.addEventListener("click", (e) => {
            e.preventDefault();
            if (isCallable(ctx.onsubmit)) {
                ctx.hide();
                ctx.onsubmit({
                    submit: false,
                    valid:  true,
                    data:   {}
                });
            } else {
                ctx.hide();
            }
            return false;
        });
    }

    this.elem.addEventListener("keydown", (e) => {
        if (e.target === elem) {
            ctx.processShortcuts(e);
        }
    });

}

Window.prototype.processShortcuts = function(e) {
    switch (e.keyCode) {
        case 13:
            if (this.submitBtn) {
                this.submitBtn.dispatchEvent(new MouseEvent("click", { bubbles: false, cancelable: true }));
            }
            break;
        case 27:
            if (this.cancelBtn) {
                this.cancelBtn.dispatchEvent(new MouseEvent("click", { bubbles: false, cancelable: true }));
            }
            break;
    }
}

Window.prototype.show = function (display = null, config = {}) {
    if (this.isShowing) {
        throw new Error("window is already showing");
    }
    display = display ? display : document.body;
    if (!(display instanceof Element)) {
        throw new Error("display must be instance of Element");
    }
    display.appendChild(this.elem);

    if (Object.keys(config).length > 0) {
        this.data = config;
    }

    this.elem.classList.add("showing", "before");
    this.elem.focus();

    setTimeout(() => this.elem.classList.remove("before"), 10); //0 is not working at old browser

    let ctx = this;
    this._result = new Promise((resolve, reject) => {
        ctx.onsubmit = resolve;
        ctx.onreject = reject;
    });
    this[_isShowingSymbol] = true;

    return this;
}

Window.prototype.hide = function (remove = true) {

    return new Promise((resolve, reject) => {
        this.elem.classList.add("after");
        setTimeout(() => {
            this.elem.classList.remove("showing");
            this.elem.classList.remove("after");
            if (remove) {
                this.elem.remove();
            }
            this[_isShowingSymbol] = false;
            resolve();
        }, 300); //todo check animation is complete*/
    });

}

Window.prototype.result = function () {
    return this._result;
}

Window.prototype.processFormData = function (formData) {
    return Object.fromEntries(formData);
}

Window.prototype.validate = function (data) {
    return true;
}

Object.defineProperties(Window.prototype, {
    dom: {
        get: function() {
            return this.elem;
        }
    },
    isShowing: {
        get: function() {
            return this[_isShowingSymbol];
        }
    },
    title: {
        get: function() {
            return this.titleElem.textContent;
        },
        set: function(title) {
            this.titleElem.textContent = title;
        }
    },
    data: {
        get: function() {
            let form = this.elem.querySelector("form");
            if (!form) {
                return {};
            }
            return Object.assign(this._dataLeft ? this._dataLeft : {},  this.processFormData(new FormData(form)));
        },
        set: function(data) {
            let ownData = Object.assign({}, data)
            let elements = this.elem.querySelector("form").elements;
            for (let input of elements) {
                if (input.name && Object.hasOwn(ownData, input.name)) {
                    if (input.type === "checkbox") {
                        input.checked = !!ownData[input.name];
                    } else {
                        input.value = ownData[input.name];
                    }
                    delete ownData[input.name];
                }
            }
            this._dataLeft = ownData;
        }
    }
});

function WindowModal(elem) {
    Window.call(this, elem);
    this.activeLocker = null;
}
Object.setPrototypeOf(WindowModal.prototype, Window.prototype);

WindowModal.prototype.show = function (display = null, config= {}) {
    display = display ? display : document.body;
    if (!(display instanceof Element)) {
        throw new Error("display must be instance of Element");
    }
    this.activeLocker = ModalLocker(display);
    this.activeLocker.lock();
    return Window.prototype.show.call(this, this.activeLocker.dom, config);
}

WindowModal.prototype.hide = function (remove = true) {
    return Window.prototype.hide.call(this, display).then(() => {
        this.activeLocker.unlock();
    })
}

function LayoutConfigurationWindow(elem) {
    WindowModal.call(this, elem);

    this[_protectedDescriptor] = Object.assign(this[_protectedDescriptor], {
        errors:     Object.create(null),
        formElem:   elem.querySelector("form"),
        errorsElem: elem.querySelector("form .errors"),
        layoutResetHandler: null,
        formResetHandler:   null,
    });

    let ctx = this;
    let protectedCtx = this[_protectedDescriptor];
    this.elem.querySelector(".layout-reset").addEventListener("click", protectedCtx.layoutResetHandler = (e) => {
        e.preventDefault();
        ctx.resetArea();
        return false;
    });
    this.elem.querySelector(".toolbar .btn[data-action='reset']").addEventListener("click", protectedCtx.formResetHandler = (e) => {
        e.preventDefault();
        if (ctx._dataLeft || typeof ctx._dataLeft.defaults === "object") {
            const left  = ctx._dataLeft;
            ctx.data    = Object.assign({}, left, left.defaults);
            ctx._dataLeft = left;
        } else {
            alert("required data not available");
        }
        return false;
    });
}
Object.setPrototypeOf(LayoutConfigurationWindow.prototype, WindowModal.prototype);

LayoutConfigurationWindow.showConfig = {
    elementCount: 0,
    col: 5,
    row: 5,
    gap: 1,
    pad: 0.7,
    fontSize: "120%",
    layout: "",
}

LayoutConfigurationWindow.validationErrors = {
    rowColInvalid:      "colrow",
    padInvalid:         "pad",
    gapInvalid:         "gap",
    fontSizeInvalid:    "fontSize",
    layoutQuote:        "layout-quote",
    layoutRows:         "layout-rows",
    layoutColsAtRow:    "layout-cols-at",
    layoutAreasGeneral: "layout-areas-general",
}

LayoutConfigurationWindow.prototype.processShortcuts = function(e) {
    console.log(e)
    switch (e.keyCode) {
        case 68: //d key
            if (confirm("Reset form to its default?")) {
                this.elem.querySelector(".toolbar .btn[data-action='reset']").dispatchEvent(new MouseEvent("click", { bubbles: false, cancelable: true }));
            }
            break;
        default:
            WindowModal.prototype.processShortcuts.call(this, e);
    }
}

LayoutConfigurationWindow.prototype.show = function (display = null, config= {}) {

    config = Object.assign({}, LayoutConfigurationWindow.showConfig, config);
    config.defaults = Object.assign({}, LayoutConfigurationWindow.showConfig, config.defaults);

    if (config.layout === "" && config.col > 0 && config.row > 0) {
        config.default.layout = Page.defaultArea(config.col, config.row)
    } else {
        config.layout = this.humanize(config.layout);
    }

    if (config.defaults.layout.trim() !== "") {
        config.defaults.layout = this.humanize(config.defaults.layout);
    }

    return WindowModal.prototype.show.call(this, display, config);
}

LayoutConfigurationWindow.prototype.hide = function () {
    return WindowModal.prototype.hide.call(this).then(()=>{
        this.clearErrors();
    }).catch((e)=>{
        this.clearErrors();
        throw e;
    });
}

LayoutConfigurationWindow.prototype.resetArea = function () {
    let form =  this[_protectedDescriptor].formElem;
    const col = form.elements["col"];
    const row = form.elements["row"];

    if (!col.checkValidity() || !row.checkValidity() || isNaN(+col.value) || isNaN(+row.value)) {
        this.addError("Col or/and Row count invalid", LayoutConfigurationWindow.validationErrors.rowColInvalid);
        return;
    }

    form.elements["layout"].value = this.humanize(Page.defaultArea(+col.value, +row.value));
}

LayoutConfigurationWindow.prototype.validate = function () {

    let form =  this[_protectedDescriptor].formElem;
    const col = form.elements["col"];
    const row = form.elements["row"];
    const pad = form.elements["pad"];
    const gap = form.elements["gap"];
    const fontSize = form.elements["fontSize"];
    const layout = form.elements["layout"];

    let success = true;

    if (!col.checkValidity() || !row.checkValidity() || isNaN(+col.value) || isNaN(+row.value)) {
        this.addError("Col or/and Row count invalid", LayoutConfigurationWindow.validationErrors.rowColInvalid);
        success = false;
    } else {
        this.removeError(LayoutConfigurationWindow.validationErrors.rowColInvalid);
    }

    if (!pad.checkValidity() || isNaN(+pad.value)) {
        this.addError("pad value invalid", LayoutConfigurationWindow.validationErrors.padInvalid);
        success = false;
    } else {
        this.removeError(LayoutConfigurationWindow.validationErrors.padInvalid);
    }

    if (!gap.checkValidity() || isNaN(+gap.value)) {
        this.addError("gap value invalid", LayoutConfigurationWindow.validationErrors.gapInvalid);
        success = false;
    } else {
        this.removeError(LayoutConfigurationWindow.validationErrors.gapInvalid);
    }

    if (!fontSize.checkValidity()) {
        this.addError("fontSize value invalid", LayoutConfigurationWindow.validationErrors.fontSizeInvalid);
        success = false;
    } else {
        this.removeError(LayoutConfigurationWindow.validationErrors.fontSizeInvalid);
    }

    let detail = this.validateDetailLayout(layout.value);
    if (!detail.success) {
        if (detail[LayoutConfigurationWindow.validationErrors.layoutQuote]) {
            this.addError("layout quote place invalid", LayoutConfigurationWindow.validationErrors.layoutQuote);
        } else {
            this.removeError(LayoutConfigurationWindow.validationErrors.layoutQuote);
        }
        if (detail[LayoutConfigurationWindow.validationErrors.layoutRows]) {
            this.addError("layout rows count invalid", LayoutConfigurationWindow.validationErrors.layoutRows);
        } else {
            this.removeError(LayoutConfigurationWindow.validationErrors.layoutRows);
        }
        if (detail[LayoutConfigurationWindow.validationErrors.layoutColsAtRow]) {
            let rowsId = detail[LayoutConfigurationWindow.validationErrors.layoutColsAtRow+"-rows"];
            let err = `\"layout cols count on row ${rowsId[0]+1} invalid`;
            if (rowsId.length > 1) {
                err += `, and other ${rowsId.length - 1} rows`;
            }
            err += "\"";
            this.addError(err, LayoutConfigurationWindow.validationErrors.layoutColsAtRow);
        } else {
            this.removeError(LayoutConfigurationWindow.validationErrors.layoutColsAtRow);
        }
        if (detail[LayoutConfigurationWindow.validationErrors.layoutAreasGeneral]) {
            this.addError("general template-areas validation", LayoutConfigurationWindow.validationErrors.layoutAreasGeneral);
        } else {
            this.removeError(LayoutConfigurationWindow.validationErrors.layoutAreasGeneral);
        }
        success = false
    }

    return success;
}

LayoutConfigurationWindow.prototype.humanize = function (text) {
    let result = "";
    let pre = text.replace(/\s+/g, " ").trim();
    let lines = pre.split('"');

    const maxTokenSize = lines.reduce(function (acc, value){
        if (lines.length) {
            return Math.max(acc, ...value.trim().split(" ").map((t) => t.length));
        }
        return acc;
    }, 0);

    console.log("maxTokenSize", maxTokenSize);

    lines.forEach((sub) => {
        sub = sub.trim();
        if (sub.length) {
            sub = sub.split(" ").map((s) => {
                return s.padEnd(maxTokenSize, " ");
            }).join(" ").trim();
            result += `"${sub}"\n`;
        }
    });
    return result;
}

LayoutConfigurationWindow.prototype.validateDetailLayout = function (text) {

    text = text.replace(/\s+/g, " ").trim();

    console.log("simplified", text);

    let errors = Object.create(null);

    let form =  this[_protectedDescriptor].formElem;
    const col = form.elements["col"];
    const row = form.elements["row"];

    errors.success = true;
    let rowsColsErrors = [];
    errors[LayoutConfigurationWindow.validationErrors.layoutColsAtRow+"-rows"] = rowsColsErrors;

    let symbolCnt = 0;
    let startOfRow = 0;
    let started = false;
    let rows = [];
    for (let i = 0; i < text.length; i++) {
        if (text[i] === '"') {
            symbolCnt++
            if (!started) {
                started = true;
                startOfRow = i;
            } else {
                started = false;
                rows.push(text.substring(startOfRow+1, i));
            }
        }
    }
    if (symbolCnt % 2 !== 0) {
        errors[LayoutConfigurationWindow.validationErrors.layoutQuote] = true;
        errors.success = false;
    } else {
        errors[LayoutConfigurationWindow.validationErrors.layoutQuote] = false;
    }

    if (symbolCnt / 2 !== +row.value) {
        errors[LayoutConfigurationWindow.validationErrors.layoutRows] = true;
        errors.success = false;
    } else {
        errors[LayoutConfigurationWindow.validationErrors.layoutRows] = false;
    }

    for (let i = 0; i < rows.length; i++) {
        if (rows[i].indexOf(".") === -1) {
            if (rows[i].trim().replace(/\s+/i, " ").split(" ").length !== +col.value) {
                errors[LayoutConfigurationWindow.validationErrors.layoutColsAtRow] = true;
                rowsColsErrors.push(i);
                errors.success = false;
            }
        } else {
            let r = rows[i].trim().replace(/\.+/ig, (a) => {return a.replace(/\./g, " .").trim()});
            if (r.split(" ").length !== +col.value) {
                errors[LayoutConfigurationWindow.validationErrors.layoutColsAtRow] = true;
                rowsColsErrors.push(i);
                errors.success = false;
            }
        }
    }


    //dom validation
    const dom       = document.createElement("div");
    dom.style.display               = "grid";
    dom.style.gridTemplateColumns   = `repeat(${col.value}, minmax(0, 1fr))`;
    dom.style.gridTemplateRows      = `repeat(${row.value}, minmax(0, 1fr))`;
    dom.style.gridTemplateAreas     = text;

    //note if there is problems use getComputedStyle(dom)
    if (!dom.style.gridTemplateAreas) {
        errors[LayoutConfigurationWindow.validationErrors.layoutAreasGeneral] = true;
        errors.success = false;
    } else {
        errors[LayoutConfigurationWindow.validationErrors.layoutAreasGeneral] = false;
    }

    return errors;
}

LayoutConfigurationWindow.prototype.addError = function (msg, msgId) {
    let protectedCtx = this[_protectedDescriptor];
    if (Object.hasOwn(protectedCtx.errors, msgId)) {
        this.removeError(msgId);
    }
    protectedCtx.errors[msgId] = msg;
    let newError = document.createElement("span");
    newError.classList.add("error", `error-${msgId}`);
    newError.innerText = msg;
    protectedCtx.errorsElem.appendChild(newError);
    this.elem.classList.add("has-errors");
}

LayoutConfigurationWindow.prototype.removeError = function (msgId) {
    let protectedCtx = this[_protectedDescriptor];
    if (Object.hasOwn(protectedCtx.errors, msgId)) {
        delete protectedCtx.errors[msgId];
        protectedCtx.errorsElem.querySelector(`.error-${msgId}`).remove();
    }
    if (Object.keys(protectedCtx.errors).length === 0) {
        this.elem.classList.remove("has-errors");
    }
}

LayoutConfigurationWindow.prototype.clearErrors = function () {
    let protectedCtx = this[_protectedDescriptor];
    protectedCtx.errors = Object.create(null);
    protectedCtx.errorsElem.innerHTML = "";
    this.elem.classList.remove("has-errors");
}

SettingsWindow.validationErrors = {
    SSID_INVALID:           "ssidInvalid",
    MODE_INVALID:           "modeInvalid",
    PWD_NOT_MATCH:          "pwdNotMatch",
    PWD_INVALID:            "pwdInvalid",
    HAPT_UNAVAILABLE:       "haptunavailable",
    OVERLAY_UNAVAILABLE:    "overlayunavailable",
}

SettingsWindow.showConfig = {
    mode:       "",
    ssid:       "",
    authData:   [],
    password1:  "",
    password2:  "",
    clientId:   "",
    hapticResp: HapticResponse.defaultInstance.enabled,
    overlay:    true,
}

function SettingsWindow(elem) {
    WindowModal.call(this, elem);
    this[_protectedDescriptor] = Object.assign(this[_protectedDescriptor], {
        errors:     Object.create(null),
        formElem:   elem.querySelector("form"),
        formSelect: elem.querySelector("form").querySelector("select[name=\"" + "mode" + "\"]"),
        errorsElem: elem.querySelector("form .errors"),
    });
}
Object.setPrototypeOf(SettingsWindow.prototype, WindowModal.prototype);

SettingsWindow.prototype.makeSelectVariant = function (item) {
    const opt = document.createElement("option");
    opt.setAttribute("value", item.value);
    opt.innerText = item.label;
    return opt;
}

SettingsWindow.prototype.show = function (display = null, config= {}) {

    config = Object.assign({}, SettingsWindow.showConfig, config);

    let protectedDescriptor = this[_protectedDescriptor];

    if (config.authData.length) {
        protectedDescriptor.formSelect.innerHTML = "";
    }
    config.authData.forEach((ad) => {
        protectedDescriptor.formSelect.appendChild(this.makeSelectVariant(ad));
    });

    if (!HapticResponse.defaultInstance.available) {
        if (protectedDescriptor.formElem.elements["hapticResp"]) {
            protectedDescriptor.formElem.elements["hapticResp"].setAttribute("readonly", "");
            protectedDescriptor.formElem.elements["hapticResp"].setAttribute("disabled", "");
            config.hapticResp = false;
        }
    }

    return WindowModal.prototype.show.call(this, display, config);
}

SettingsWindow.prototype.hide = function() {
    return WindowModal.prototype.hide.call(this).then(()=>{
        this.clearErrors();
    }).catch((e)=>{
        this.clearErrors();
        throw e;
    });
}

SettingsWindow.prototype.addError = function (msg, msgId) {
    let protectedCtx = this[_protectedDescriptor];
    if (Object.hasOwn(protectedCtx.errors, msgId)) {
        this.removeError(msgId);
    }
    protectedCtx.errors[msgId] = msg;
    let newError = document.createElement("span");
    newError.classList.add("error", `error-${msgId}`);
    newError.innerText = msg;
    protectedCtx.errorsElem.appendChild(newError);
    this.elem.classList.add("has-errors");
}

SettingsWindow.prototype.removeError = function (msgId) {
    let protectedCtx = this[_protectedDescriptor];
    if (Object.hasOwn(protectedCtx.errors, msgId)) {
        delete protectedCtx.errors[msgId];
        protectedCtx.errorsElem.querySelector(`.error-${msgId}`).remove();
    }
    if (Object.keys(protectedCtx.errors).length === 0) {
        this.elem.classList.remove("has-errors");
    }
}

SettingsWindow.prototype.clearErrors = function () {
    let protectedCtx = this[_protectedDescriptor];
    protectedCtx.errors = Object.create(null);
    protectedCtx.errorsElem.innerHTML = "";
    this.elem.classList.remove("has-errors");
}

SettingsWindow.prototype.validate = function () {
    let form =  this[_protectedDescriptor].formElem;

    const ssid = form.elements["ssid"];
    const mode = form.elements["mode"];
    const password1 = form.elements["password1"];
    const password2 = form.elements["password2"];

    let success = true;

    if (!ssid.checkValidity() ) {
        this.addError("ssid invalid", SettingsWindow.validationErrors.SSID_INVALID);
        success = false;
    } else {
        this.removeError(SettingsWindow.validationErrors.SSID_INVALID);
    }

    if (!mode.checkValidity()) {
        this.addError("mode invalid", SettingsWindow.validationErrors.MODE_INVALID);
        success = false;
    } else {
        this.removeError(SettingsWindow.validationErrors.MODE_INVALID);
    }

    if (password1.value || password2.value) {
        if (password1.value !== password2.value) {
            this.addError("password not match", SettingsWindow.validationErrors.PWD_NOT_MATCH);
            success = false;
        } else {
            this.removeError(SettingsWindow.validationErrors.PWD_NOT_MATCH);
        }
        if (!password1.checkValidity() || !password2.checkValidity()) {
            this.addError("password invalid", SettingsWindow.validationErrors.PWD_INVALID);
            success = false;
        } else {
            this.removeError(SettingsWindow.validationErrors.PWD_INVALID);
        }
    }

    const hapticResp = form.elements["hapticResp"];
    if (!hapticResp.checkValidity() || (!HapticResponse.defaultInstance.available && hapticResp.checked)) {
        this.addError("haptic response unavailable or invalid", SettingsWindow.validationErrors.HAPT_UNAVAILABLE);
        success = false;
    } else {
        this.removeError(SettingsWindow.validationErrors.HAPT_UNAVAILABLE);
    }

    const overlay = form.elements["overlay"];
    if (!overlay.checkValidity()) {
        this.addError("overlay unavailable or invalid", SettingsWindow.validationErrors.OVERLAY_UNAVAILABLE);
        success = false;
    } else {
        this.removeError(SettingsWindow.validationErrors.OVERLAY_UNAVAILABLE);
    }

    return success;
}

SettingsWindow.prototype.processFormData = function(formData) {

    let data = WindowModal.prototype.processFormData.call(this, formData);

    if (!HapticResponse.defaultInstance.available) {
        delete data.hapticResp;
    } else {
        data.hapticResp = !!data.hapticResp;
    }

    data.overlay = !!data.overlay;

    return data;
}

ControlConfigurationWindow.showConfig = {
    ctrl: null,
    view: null,
    mode: "all", // all||view||ctrl
    groups: {
        items:          [],
        defaultColors:  [],
    },
}

function ControlConfigurationWindow(elem) {
    WindowModal.call(this, elem);
    this[_protectedDescriptor] = Object.assign(this[_protectedDescriptor], {
        errors:     Object.create(null),
        scopeBtn:   elem.querySelector(".header .scope"),
        formCtrl:   elem.querySelector("form.ctrl"),
        formView:   elem.querySelector("form.view"),
        ctrl:       undefined,
        view:       undefined,
        mode:       "all",
        groups:     {},
    });
    this.changeScopeHandler     = this.changeScopeHandler.bind(this);
    this.trackOwnershipHandler  = this.trackOwnershipHandler.bind(this);
}

Object.setPrototypeOf(ControlConfigurationWindow.prototype, WindowModal.prototype);

ControlConfigurationWindow.prototype.validate = function () {
    const protectedCtx = this[_protectedDescriptor];
    let isValid = true;

    protectedCtx.formView.querySelectorAll("input, select").forEach(el => el.setCustomValidity(""));
    protectedCtx.formCtrl.querySelectorAll("input, select").forEach(el => el.setCustomValidity(""));

    if (protectedCtx.mode === "all" || protectedCtx.mode === "view") {
        isValid &= protectedCtx.formView.reportValidity ? protectedCtx.formView.reportValidity() : protectedCtx.formView.checkValidity();
        isValid &= this.validateKbKey(protectedCtx.formView, protectedCtx.view.attributes);
    }

    if (protectedCtx.mode === "all" || protectedCtx.mode === "ctrl") {
        isValid &= protectedCtx.formCtrl.reportValidity ? protectedCtx.formCtrl.reportValidity() : protectedCtx.formCtrl.checkValidity();
        isValid &= this.validateKbKey(protectedCtx.formCtrl, protectedCtx.ctrl.attributes);
    }

    return isValid;
}

ControlConfigurationWindow.prototype.validateKbKey = function (form, attributes) {
    let isValid = true;

    //it is better to use attributes to determinate id of kb field but this one is simpler
    //:valid first buildin validation error then logical validation
    for (const el of form.querySelectorAll("input.type-kbkey:valid, .type-kbkey input:valid")) {
        const value = el.value;
        if (value && value.startsWith("call:")) {
            if (!Control.registry.has(value.substring(5))) {
                el.setCustomValidity("call: is invalid, control does not exist or mistyped");
                el.reportValidity ? el.reportValidity() : el.checkValidity();
                isValid = false;
            }
        }
    }

    return isValid;
}

ControlConfigurationWindow.prototype.processLabel = function(text) {
    text = text.endsWith(":") ? text.substring(0, (text.length - 1) - 1) :  text;
    if (text.startsWith("kb key")) {
        text = text.replace("kb key", "keybind");
    }
    return text;
}

/*
* Why (otherStuff, name, descriptor, attributes) and not (otherStuff, name, attributes)
* main argument there is descriptor it's operate as config in this context and can be plain object if needed
* it is essential to make this code reusable, but it nameless so name must be provided also.
* Attributes in this case, work only as lookup table, if field query's more than one attribute to cook.
* yes (name, attributes) can replace (name, descriptor, attributes) as descriptor = attributes.descriptor(name)
* but this make fieldCreator dependent of attributes, it current state (render field that not in attributes) and implementation
* that was not my intension.
*/

ControlConfigurationWindow.prototype.makeLabeable = function(text, { name, type } ) {
    const label = document.createElement("span");
    label.innerText = this.processLabel(text);
    label.classList.add("capture", `type-${type}`, `name-${name}`);
    return label;
}

ControlConfigurationWindow.prototype.makeDescription = function(text, descriptor) {
    const description   = document.createElement("span");
    description.classList.add("description");
    description.innerText = text.trim();

    return description;
}

ControlConfigurationWindow.prototype.makeOwnership = function(text, descriptor) {
    const own   = document.createElement("span");
    own.classList.add("ownership");
    if (descriptor.inheritable) {
        if (descriptor.owned) {
            own.classList.add("owned");
        } else {
            own.classList.add("inherited");
        }
    } else {
        own.classList.add("not-inheritable");
    }
    own.innerText = text.trim();
    return own;
}

ControlConfigurationWindow.prototype.makeFieldReadOnly = function(textContent, descriptor) {
    const label= document.createElement("label");
    const text  = document.createElement("span");
    const scope = descriptor.htmlContent ? "innerHTML" : "innerText";
    if (descriptor.type === Attribute.types.STRING || descriptor.type === Attribute.types.TEXT) { //todo enum
        text[scope] = textContent.trim();
    } else {
        text[scope] = textContent;
    }
    text.classList.add("value", "readonly", `type-${descriptor.type}`, `name-${descriptor.name}`);
    label.classList.add("wrapper", `type-${descriptor.type}`, `name-${descriptor.name}`, ...(descriptor.classList || []));
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(text);
    return label;
}

ControlConfigurationWindow.prototype.makeFieldText = function(textContent, descriptor) {
    const label = document.createElement("label");
    const text = document.createElement("textarea");
    if (descriptor.tabindex !== undefined) {
        text.setAttribute("tabindex", descriptor.tabindex);
    }
    text.innerText = textContent.trim();
    text.classList.add("value")
    label.classList.add("wrapper", "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []));
    if (descriptor.writeable) {
        label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
    }
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(text);
    if (descriptor.description) {
        label.appendChild(this.makeDescription(descriptor.description || "", descriptor));
    }
    return label;
}

ControlConfigurationWindow.prototype.makeFieldINT = function(value, descriptor) {
    const label = document.createElement("label");
    const input = document.createElement("input");


    input.setAttribute("value", value);
    input.setAttribute("name", descriptor.name);
    input.setAttribute("type", "number");
    input.setAttribute("required", "");
    if (descriptor.tabindex !== undefined) {
        input.setAttribute("tabindex", descriptor.tabindex);
    }
    if (descriptor.params.max !== undefined) {
        input.setAttribute("max", descriptor.params.max);
    }
    if (descriptor.params.min !== undefined) {
        input.setAttribute("min", descriptor.params.min);
    }
    if (descriptor.params.step !== undefined) {
        input.setAttribute("step", descriptor.params.step);
    }
    input.classList.add("value", "type-"+descriptor.type, "name-"+name);
    label.classList.add("wrapper", "type-"+descriptor.type, "name-"+name, ...(descriptor.classList || []))

    if (descriptor.writeable) {
        label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
    }
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(input);
    if (descriptor.description) {
        label.appendChild(this.makeDescription(descriptor.description || "", descriptor));
    }

    return label;
}

ControlConfigurationWindow.prototype.makeFieldBoolean = function(boolValue, descriptor) {
    const label     = document.createElement("label");
    const input     = document.createElement("input");
    const slider    = document.createElement("span");
    const sliderWrapper = document.createElement("span");
    const container     = document.createElement("span");

    input.setAttribute("type", "checkbox");
    input.setAttribute("name", descriptor.name);
    input.setAttribute("value", "1");
    if (descriptor.tabindex !== undefined) {
        input.setAttribute("tabindex", descriptor.tabindex);
    }
    if (boolValue) {
        input.setAttribute("checked", "");
    }
    slider.classList.add("slider");
    sliderWrapper.classList.add("slider-wrapper");
    container.classList.add("value", "type-"+descriptor.type, "name-"+descriptor.name);
    label.classList.add("wrapper", "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []))
    sliderWrapper.appendChild(input);
    sliderWrapper.appendChild(slider);
    container.appendChild(sliderWrapper);

    if (descriptor.writeable) {
        label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
    }
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(container);
    if (descriptor.description) {
        label.appendChild(this.makeDescription(descriptor.description || "", descriptor));
    }

    return label;
}

ControlConfigurationWindow.prototype.makeGroupsCloud = function(textContent, descriptor) {
    const groups = document.createElement("label");

    const tinyConverter = document.createElement("div");
    tinyConverter.style.display = "none";
    document.body.appendChild(tinyConverter);

    const protectedCtx = this[_protectedDescriptor];
    const lightColor = "255, 255, 255";
    const darkColor  = "0, 0, 0";
    let index = 100;
    let defaultColorIndex = 0;
    for (let label of textContent.split(",")) {
        if (!label) {
            continue;
        }
        let group = document.createElement("span");
        let gdata = protectedCtx.groups.items.find((g) => g.id === label);
        let color = (gdata && gdata.color) || protectedCtx.groups.defaultColors[(defaultColorIndex++) % protectedCtx.groups.defaultColors.length];
        const isLightCl = isLightColor(color);
        group.classList.add("value", "readonly");
        group.style.setProperty("--cloud-item-index", index--); //go in reverse dirrections to support old browser that cant do calc on zIndex
        group.style.setProperty("--cloud-item-content", `"${label}"`);
        group.style.setProperty("--cloud-item-bg", color);
        group.style.setProperty("--cloud-item-is-light",        isLightCl ? "true" : "false");
        group.style.setProperty("--cloud-item-color",        isLightCl ? darkColor : lightColor);
        group.style.setProperty("--cloud-item-invert-color", isLightCl ? lightColor : darkColor);
        //why not tag attribute? tag attribute requre some rules to meet html spec.
        groups.appendChild(group);
    }
    groups.classList.add("wrapper", "cloud", `type-${descriptor.type}`, `name-${descriptor.name}`);
    tinyConverter.remove();
    return groups;
}

ControlConfigurationWindow.prototype.makeEnumOpt = function(opt, descriptor, attributeValue) {
    if (opt.type === "optgroup") {
        let el = document.createElement("optgroup");
        for (const [name, value] of Object.entries(opt)) {
            if (name === "items" || name === "type") {
                continue;
            }
            el.setAttribute(name, value);
        }
        for (let item of opt.items) {
            el.appendChild(this.makeEnumOpt(item, descriptor, attributeValue));
        }
        return el;
    } else if (opt.type === "option") {
        let el = document.createElement("option");
        for (const [name, value] of Object.entries(opt)) {
            if (name === "label" || name === "type") {
                continue;
            }
            el.setAttribute(name, value);
        }
        el.innerText = opt.label;
        if (("" + attributeValue) === ("" + el.value)) {
            el.setAttribute("selected", "")
        }
        return el;
    } else {
        console.warn("invalid enum opt");
        return null;
    }
}

ControlConfigurationWindow.prototype.makeEnum = function(value, descriptor) {
    const label = document.createElement("label");
    const input = document.createElement("select");

    console.warn(descriptor.name, descriptor)

    input.setAttribute("name", descriptor.name);
    input.setAttribute("required", "");
    input.classList.add("value",    "type-"+descriptor.type, "name-"+descriptor.name);
    label.classList.add("wrapper",  "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []))

    for (let opt of descriptor.params.options) {
        input.appendChild(this.makeEnumOpt(opt, descriptor, value));
    }

    if (descriptor.writeable) {
        if (descriptor.tabindex !== undefined) {
            input.setAttribute("tabindex", descriptor.tabindex);
        }
        label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
    }
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(input);
    if (descriptor.description) {
        label.appendChild(this.makeDescription(descriptor.description || "", descriptor));
    }

    return label;
}

ControlConfigurationWindow.prototype.makeKBKey = function(attributeValue, descriptor) {
    const label = document.createElement("label");
    const input = document.createElement("input");
    const hidden = document.createElement("input");
    const wrapper= document.createElement("div");
    const switcher = document.createElement("a");

/*    let tokens = [...this.makeKnownTokensList(), ...this.tokensFromTokensText(descriptor.value)];
    tokens = [...new Set(tokens)];*/

    //input.setAttribute("value", this.tokensToText(descriptor.value, tokens));

    input.setAttribute("value", attributeValue);
    input.setAttribute("type", "text");
    input.setAttribute("name", descriptor.name);  //remove me
    if (descriptor.tabindex !== undefined) {
        input.setAttribute("tabindex", descriptor.tabindex);
    }
    input.classList.add("simplified");

    switcher.setAttribute("href", "#");
    switcher.classList.add("btn", "kbk-switcher");
    switcher.innerText = "T";

    hidden.setAttribute("type", "hidden");
    //hidden.setAttribute("name", name);
    hidden.setAttribute("value", attributeValue);

    wrapper.classList.add("kbk-wrapper", "value", "type-"+descriptor.type, "name-"+descriptor.name);
    wrapper.appendChild(input);
    wrapper.appendChild(hidden);
    wrapper.appendChild(switcher);

    if (descriptor.writeable) {
        label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
    }
    label.classList.add("wrapper", "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []))
    label.appendChild(this.makeLabeable(descriptor.label, descriptor));
    label.appendChild(wrapper);

    switcher.onclick = e => e.preventDefault(); //workaround


    /*switcher.addEventListener("click", (e) => {
        e.preventDefault();

        const simplified = !input.classList.contains("simplified");

        try {
            if (simplified) {
                input.value  = this.tokensToText(hidden.value, tokens);
                input.classList.add("simplified");
                switcher.classList.remove("pressed");
            } else {
                input.value  = hidden.value;
                input.classList.remove("simplified");
                switcher.classList.add("pressed");
            }
        } catch (e) {
            input.classList.remove("simplified");
            switcher.classList.add("pressed");
            input.value = hidden.value;
        }


        return false;
    });

    const callback = () => {
        const simplified = input.classList.contains("simplified");
        if (simplified) {
            const original = hidden.value;
            try {
                hidden.value  = this.textToTokens(input.value, tokens);
                input.value   = this.tokensToText(hidden.value, tokens);
            } catch (e) {
                input.classList.remove("simplified");
                switcher.classList.add("pressed");
                input.value = hidden.value = original;
            }
        } else {
            hidden.value = input.value;
        }
    }

    input.addEventListener("blur", () => {
        console.log("blur");
        callback();
    });
    input.addEventListener("keypress", (e) => {
        if (e.code === "enter") {
            callback();
        }
        console.log("keycode", e.code);
    });*/

    return label
}

ControlConfigurationWindow.prototype.makeEmptyField = function(emptyText, labeable = false, descriptor) {
    const label  = document.createElement("label");
    const noitem = document.createElement("span");
    noitem.classList.add("empty");
    label.classList.add("wrapper", "empty", "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []));
    noitem.innerText = emptyText;
    label.appendChild(noitem);
    return label;
}

ControlConfigurationWindow.prototype.makeField = function(value, descriptor) {

    switch (descriptor.name) {
        case "groups":
            return this.makeGroupsCloud(value || "", descriptor);
    }

    if (!descriptor.writeable) {
        return this.makeFieldReadOnly(value || "", descriptor);
    }

    switch (descriptor.type) {
        case Attribute.types.TEXT:
            return this.makeFieldText(value || "", descriptor);
        case Attribute.types.BOOLEAN:
            return this.makeFieldBoolean(!!value, descriptor);
        case Attribute.types.KBKEY:
            return this.makeKBKey(value, descriptor);
        case Attribute.types.ENUM:
            return this.makeEnum(value, descriptor);
        case Attribute.types.INT:
        case Attribute.types.INTERVAL:
        case Attribute.types.AXIS:
            return this.makeFieldINT(value, descriptor);
        default:
            const label = document.createElement("label");
            const input = document.createElement("input");


            input.setAttribute("value", value);
            input.setAttribute("name", descriptor.name);
            if (descriptor.type === Attribute.types.LINK) {
                input.setAttribute("type", "href");
            } else {
                input.setAttribute("type", "text");
            }
            if (descriptor.params.pattern !== undefined) {
                input.setAttribute("pattern", descriptor.params.pattern);
            }
            if (descriptor.tabindex !== undefined) {
                input.setAttribute("tabindex", descriptor.tabindex);
            }
            input.classList.add("value", "type-"+descriptor.type, "name-"+descriptor.name);

            if (descriptor.writeable) {
                label.appendChild(this.makeOwnership(this.evaluateOwnership(descriptor), descriptor));
            }
            label.appendChild(this.makeLabeable(descriptor.label, descriptor));
            label.appendChild(input);
            if (descriptor.description) {
                label.appendChild(this.makeDescription(descriptor.description || "", descriptor));
            }

            label.classList.add("wrapper", "type-"+descriptor.type, "name-"+descriptor.name, ...(descriptor.classList || []));

            return label
    }

}

/*ControlConfigurationWindow.prototype.makeKnownTokensList = function() {

    let modKb = ["ctrl", "shift", "alt"];
    let result = [];

    for (let basic of modKb) {
        result.push(basic, "r"+basic, "l"+basic);
    }

    for (let f = 1; f <= 12; f++) {
        result.push("f"+f);
    }

    result.push("enter", "return", "backspace", "numdiv", "nummul", "numsub",
        "numadd", "numenther", "numlock", "numrigth", "numleft", "numdown", "numup");

    return result;
}

ControlConfigurationWindow.prototype.textToTokens = function(text, validTokens) {

    if (text.indexOf("++") !== -1) {
        throw new Error("unable to process in easy mode 1");
    }

    return text.replace(/\s+/g, " ").split("+").map((t, i, array) => {
        t = t.trim();
        if (validTokens.indexOf(t.toLowerCase()) !== -1) {
            if (i === 0) {
                return "+" + t.toLowerCase();
            } else if (i === array.length - 1) {
                return t.toLowerCase() + "+";
            } else {
                return t.toLowerCase();
            }
        }
        if (i === array.length - 1) {
            return t.toLowerCase();
        }
        console.info("textToTokens", t.toLowerCase(), i, array, validTokens)
        throw new Error("unable to process in easy mode 2");
    }).join("+");
}

ControlConfigurationWindow.prototype.tokensToText = function(text, validTokens) {

    if (text.indexOf("+") === -1) {
        return text;
    }

    try {
        let tokens = text.match(/(?<=\+)(\w+?)(?=\+)/g);

        tokens = tokens.map(t => {
            t = t.toLowerCase();
            if (validTokens.indexOf(t) === -1) {
                throw new Error("unable 2 convert to easy mode");
            }
            return t.charAt(0).toUpperCase() + t.slice(1);
        });

        tokens.push(...(text.match(/(?<=\+)(\w+?)$/g) || []).map(t => t.toUpperCase()));

        return tokens.join(" + ");
    } catch (e) {
        return text;
    }

}*/

/*ControlConfigurationWindow.prototype.tokensFromTokensText = function(text) {
    if (text.length === 1) {
        return [];
    }
    return text.match(/(?<=\+)(\w+?)(?=\+)/g) || [];
}*/

ControlConfigurationWindow.prototype.evaluateOwnership = function(descriptor) {
    if (descriptor.inheritable) {
        if (descriptor.owned) {
            return "owned";
        } else {
            return "inherited";
        }
    } else {
        return "not inheritable";
    }
}

ControlConfigurationWindow.prototype.buildForm = function (htmlForm, attributes, blacklisted) {

    const protectedCtx = this[_protectedDescriptor];

    let tabindex = 2;

    for (const [name, descriptor] of attributes) {
        if (descriptor.readable) {
            if (blacklisted.includes(name)) {
                continue;
            }
            if (!descriptor.writeable && attributes.isEmpty(name)) {
                continue;
            }
            const owned = attributes.isOwned(name);
            htmlForm.appendChild(this.makeField(attributes.get(name), Object.assign({ name, owned, tabindex: descriptor.writeable ? tabindex++ : undefined }, descriptor )));
        }
    }

    for (const name of ["description", "groups"]) {
        const descriptor  = attributes.descriptor(name);
        const owned       = attributes.isOwned(name);
        const emptyText   = name === "description" ? "no description" : "no groups";
        if (descriptor.writeable || attributes.get(name)) {
            htmlForm.appendChild(this.makeField(attributes.get(name), Object.assign({ name, owned, tabindex }, descriptor )));
        } else {
            htmlForm.appendChild(this.makeEmptyField(emptyText, false, Object.assign({ name, owned, tabindex }, descriptor )));
        }
    }

    const markupable = htmlForm.querySelector("label.name-description span.name-description.value");
    if (markupable) {
        markupable.innerHTML = protectedCtx.markupProcessor.process(markupable.innerHTML).result;
    }

}

ControlConfigurationWindow.prototype.processShortcuts = function (e) {
    switch (e.keyCode) {
        case 83:
            const protectedCtx = this[_protectedDescriptor];
            if (protectedCtx.scopeBtn) {
                protectedCtx.scopeBtn.dispatchEvent(new MouseEvent("click", { bubbles: false, cancelable: true }));
            }
            break;
        default:
            WindowModal.prototype.processShortcuts.call(this, e);
    }
}

ControlConfigurationWindow.prototype.show = function (display = null, config= {}) {

    config = Object.assign({},  ControlConfigurationWindow.showConfig, config);
    config.groups = Object.assign({}, ControlConfigurationWindow.showConfig.groups, config.groups);

    const protectedCtx = this[_protectedDescriptor];

    if (!config.ctrl || !(config.ctrl instanceof Control) || !config.ctrl.attributes) {
        throw new Error("not ctrl"); //regardes of mode, ctrl must be provided (or editing non-inheritable attr make little sense)
    }

    let attributes = config.view ? config.view.attributes : config.ctrl.attributes;

    const blacklisted = ["id", "type", "description", "groups"];

    this.title = attributes.get("type") + "::" + attributes.get("id");
    this.dom.classList.remove("mode-all", "mode-view", "mode-ctrl", "scope-view", "scope-ctrl");
    this.dom.classList.add("mode-"+config.mode, (config.mode === "all" || config.mode === "view") ? "scope-view" : "scope-ctrl");

    protectedCtx.markupProcessor = new SettingsMarkupParser();
    protectedCtx.ctrl   = config.ctrl;
    protectedCtx.view   = config.view;
    protectedCtx.mode   = config.mode;
    protectedCtx.groups = config.groups;

    if (config.ctrl) {
        protectedCtx.formCtrl.innerHTML = "";
        protectedCtx.formCtrl.appendChild(
            this.makeFieldReadOnly(
                `<span class="full-size">*this is ctrl scope. Any changes made there will populate to any reference of this ctrl on any page, that not override changed value but it's own</span>` +
                `<a class="compact control-tip" title="this is ctrl scope. Any changes made there will populate to any reference of this ctrl on any page, that not override changed value but it's own." href="#" >*ctrl scope. changes for all widgets of this type</a>`,
                { name: "ctrl-scope-notes", label: "notes", type: "text", htmlContent: true }
            )
        )
        this.buildForm(protectedCtx.formCtrl, config.ctrl.attributes, blacklisted);
    }
    if (config.view) {
        protectedCtx.formView.innerHTML = "";
        protectedCtx.formView.appendChild(
            this.makeFieldReadOnly(
                `<span class="full-size">*this is ref. scope. Any changes made there will only affect this particular widget, only at this page. Attribute marked as "owned" will not affect by changing it in ctrl scope, attribute marked as "noninheritable" when changing will change self in ctrl scope</span>` +
                `<a class="compact control-tip" title="this is ref. scope. Any changes made there will only affect this particular widget, only at this page. Attribute marked as &quot;owned&quot; will not affect by changing it in ctrl scope, attribute marked as &quot;noninheritable&quot; when changing will change self in ctrl scope" href="#" >*ref. scope. changes only for this widget at that page</a>`,
                { name: "ref-scope-notes", label: "notes", type: "text", htmlContent: true }
            )
        );
        this.buildForm(protectedCtx.formView, config.view.attributes, blacklisted);
    } else {
        if (config.mode === "view" || config.mode === "all") {
            throw new Error("view must be provided");
        }
    }

    this.elem.addEventListener("click", this.showTipHandler);
    protectedCtx.scopeBtn.addEventListener("click",     this.changeScopeHandler);
    protectedCtx.formView.addEventListener("change",    this.trackOwnershipHandler);

    return WindowModal.prototype.show.call(this, display, config);
}

ControlConfigurationWindow.prototype.hide = function (remove = true) {
    let protectedCtx = this[_protectedDescriptor];
    this.elem.removeEventListener("click",  this.showTipHandler);
    protectedCtx.scopeBtn.removeEventListener("click",  this.changeScopeHandler);
    protectedCtx.formView.removeEventListener("change", this.trackOwnershipHandler);
    return WindowModal.prototype.hide.call(this, remove);
}

ControlConfigurationWindow.prototype.changeScopeHandler = function(e) {

    e.preventDefault();
    e.stopPropagation();

    let protectedCtx = this[_protectedDescriptor];

    const dom = this.dom;
    dom.classList.toggle("scope-ctrl");
    dom.classList.toggle("scope-view");

    if (dom.classList.contains("scope-view")) {
        this.syncFormValues(protectedCtx.formView, protectedCtx.formCtrl, protectedCtx.view.attributes);
    } else {
        this.syncFormValues(protectedCtx.formCtrl, protectedCtx.formView, protectedCtx.ctrl.attributes);
    }

}

ControlConfigurationWindow.prototype.showTipHandler = function(e) {
    let protectedCtx = this[_protectedDescriptor];
    const target = e.target.closest("a.control-tip");
    if (target) {

        e.preventDefault();
        e.stopPropagation();

        if (target.title) {
            alert(target.title)
        }

    }
}

ControlConfigurationWindow.prototype.updateOwnership = function(el) {
    const ctrlForm   = this[_protectedDescriptor].formCtrl;
    const attributes = this[_protectedDescriptor].view.attributes;
    if (!attributes.hasOwn(el.name)) {
        const ownership = el.closest(".wrapper").querySelector(".ownership");
        const refField  = ctrlForm.querySelector(`[name=${el.name}]`);
        if (ownership && refField) {
            if (ownership.classList.contains("inherited")) {
                if (refField.value.toString().trim() !== el.value.toString().trim()) {
                    ownership.classList.remove("inherited");
                    ownership.classList.add("owned");
                    ownership.innerText = "owned";
                }
            } else if (ownership.classList.contains("owned")) {
                if (refField.value.toString().trim() === el.value.toString().trim()) {
                    //track only field that were owned before and changed to "initial state"
                    ownership.classList.remove("owned");
                    ownership.classList.add("inherited");
                    ownership.innerText = "inherited";
                }
            }
        }
    }
}

ControlConfigurationWindow.prototype.trackOwnershipHandler = function(e) {
    const target = e.target;
    if (e.target) {
        this.updateOwnership(e.target);
        if (e.isTrusted) {
            e.target.classList.add("manual");
        }
    }
}

ControlConfigurationWindow.prototype.syncFormValues = function(formTo, formFrom, referenceAttributes) {
    for (const [name, desc] of referenceAttributes) {
        const inputFrom = formFrom.querySelector(`[name=${name}]`);
        const inputTo   = formTo.querySelector(`[name=${name}]`);
        let syncRequired = false;
        if (inputFrom && inputTo) {
            if (desc.inheritable) {
                syncRequired = !!(!inputTo.classList.contains("manual") && //it is user input do not sync it
                    inputTo.closest(".wrapper").querySelector(".ownership.inherited") &&
                    inputFrom.closest(".wrapper").querySelector(".ownership.owned"));
                if (formTo === this[_protectedDescriptor].formView && !syncRequired) {
                    this.updateOwnership(inputTo); //but continue track ownership
                }
            } else {
                //not inheritable value must-be sync between form to prevent unable to change it in some cases
                syncRequired = true;
            }
            if (syncRequired) {
                if (inputFrom.type === "checkbox") {
                    inputTo.checked = inputFrom.checked;
                } else {
                    inputTo.value   = inputFrom.value;
                }
            }
        }
    }
}

ControlConfigurationWindow.prototype.processFormData = function(formData, formName = "") {

    let data = WindowModal.prototype.processFormData.call(this, formData);

    for (let attr of this[_protectedDescriptor].ctrl.attributes) {
        if (attr[1].type === Attribute.types.BOOLEAN) {
            if (data[attr[0]] === undefined) {
                data[attr[0]] = false;
            } else {
                data[attr[0]] = !!data[attr[0]];
            }
        }
    }

    return data;
}

Object.defineProperties(ControlConfigurationWindow.prototype, {
    data: {
        get: function() {
            const protectedCtx = this[_protectedDescriptor];
            if (this.dom.classList.contains("scope-view")) {
                this.syncFormValues(protectedCtx.formCtrl, protectedCtx.formView, protectedCtx.view.attributes);
            } else {
                this.syncFormValues(protectedCtx.formView, protectedCtx.formCtrl, protectedCtx.ctrl.attributes);
            }
            return Object.assign(this._dataLeft ? this._dataLeft : {}, {
                [protectedCtx.formCtrl.name]: this.processFormData(new FormData(protectedCtx.formCtrl), protectedCtx.formCtrl.name),
                [protectedCtx.formView.name]: this.processFormData(new FormData(protectedCtx.formView), protectedCtx.formView.name),
            });
        },
        set: function(data) {
            //warning syncFormValues not using there check values!
            const protectedCtx = this[_protectedDescriptor];
            let ownData = Object.assign({}, data, {
                [protectedCtx.formCtrl.name]: Object.assign({}, data[protectedCtx.formCtrl.name]),
                [protectedCtx.formView.name]: Object.assign({}, data[protectedCtx.formView.name]),
            });
            const updateForm = (form, storage) => {
                for (let input of form.elements) {
                    if (input.name && Object.hasOwn(storage, input.name)) {
                        if (input.type === "checkbox") {
                            input.checked = !!storage[input.name];
                        } else {
                            input.value = storage[input.name];
                        }
                        delete storage[input.name];
                    }
                }
            }
            updateForm(protectedCtx.formCtrl, ownData[protectedCtx.formCtrl.name]);
            updateForm(protectedCtx.formView, ownData[protectedCtx.formView.name]);
            this._dataLeft = ownData;
        }
    }
});

RegistryBathExec.state = {
    INITIAL:  0,
    CANCELED: 1,
    COMPLETE: 2,
}

function RegistryBathExec(registry, callback, ctx = this) {
    this.callback   = callback;
    this.ctx        = ctx;
    this.batchSize  = 50;
    this.beforeBath = emptyFn;
    this.afterBatch = emptyFn;
    const protectedCtx = this[_protectedDescriptor] = {
        it:         registry[Symbol.iterator](),
        ticketId:   undefined,
        state:      0,
        resolve:    null,
        reject:     null,
    };
    this.result    = new Promise((rsl, rjc) => {
        protectedCtx.resolve = rsl;
        protectedCtx.reject  = rjc;
    });
}

RegistryBathExec.prototype.execute = function() {
    if (this.batchSize === 0) {
        return false;
    }
    const protectedCtx = this[_protectedDescriptor];
    let index = 0;
    for (const ent of { [Symbol.iterator]: () => protectedCtx.it }) {
        if (this.callback.call(this.ctx, ent, index++) === false) {
            protectedCtx.state = RegistryBathExec.state.CANCELED;
            return true;
        }
        if (index >= this.batchSize) {
            return false;
        }
    }
    protectedCtx.state = RegistryBathExec.state.COMPLETE;
    return true;
}

RegistryBathExec.prototype.scheduleVia = function(scheduler, ...args) {
    const protectedCtx = this[_protectedDescriptor];
    let actualEx;
    if (protectedCtx.ticketId) {
        throw new Error("RegistryBathExec already schedule");
    }
    protectedCtx.ticketId = scheduler(actualEx = () => {
        this.beforeBath.call(this.ctx);
        this.execute();
        this.afterBatch.call(this.ctx);
        if (protectedCtx.state === RegistryBathExec.state.COMPLETE) {
            protectedCtx.resolve();
        } else if (protectedCtx.state === RegistryBathExec.state.CANCELED) {
            protectedCtx.reject();
        } else {
            protectedCtx.ticketId = scheduler(actualEx, ...args);
        }
    }, ...args);
}

RegistryBathExec.prototype.cancelVia = function(canceler) {
    const protectedCtx = this[_protectedDescriptor];
    protectedCtx.state = RegistryBathExec.state.CANCELED;
    canceler(protectedCtx.ticketId);
    protectedCtx.reject();
}


ControlSelectionWindow.showConfig = {
    batchSize: 50,
    groups: {
        items:          [],
        defaultColors:  [],
    },
}

function ControlSelectionWindow(elem) {
    WindowModal.call(this, elem);
    Object.assign(this[_protectedDescriptor], {
        searchEl:       elem.querySelector("input[type=search]"),
        resultBoxEl:    elem.querySelector(".content"),
        emptyBoxEl:     elem.querySelector(".empty-container"),
        statusBoxEl:    elem.querySelector(".footer"),
        searchTask:     null,
        spawnedControls: [],
        externalEvtHandler: null,
        markupProcessor: new SettingsMarkupParser(),
        groups:         {},
    });
    this.handleSearchElChange = this.handleSearchElChange.bind(this);
}

Object.setPrototypeOf(ControlSelectionWindow.prototype, WindowModal.prototype);

ControlSelectionWindow.prototype.show = function (display = null, config= {}) {

    let protectedCtx = this[_protectedDescriptor];

    config = Object.assign({},  ControlSelectionWindow.showConfig, config);
    config.groups = Object.assign( {}, ControlSelectionWindow.showConfig.groups, config.groups);


    protectedCtx.groups         = config.groups;
    protectedCtx.searchEl.value = "";

    protectedCtx.searchEl.removeEventListener("change", this.handleSearchElChange);
    protectedCtx.searchEl.addEventListener("change", this.handleSearchElChange);


    let ctx = this;

    //see touchEventHelper notes about ff52 event generation
    protectedCtx.externalEvtHandler = touchEventHelper(protectedCtx.resultBoxEl, { decouple: false });
    protectedCtx.externalEvtHandler.onlongtap = (e) => {

        let ref;
        if (e.target.classList.contains("control")) {
            ref = e.target;
        } else {
            ref = e.target.closest(".search-result-item-wrapper");
            if (ref) {
                ref = ref.querySelector(".control");
            }
        }

        if (!ref) {
            return;
        }

        const ctrl = Control.registry.get(ref.getAttribute("data-control-id"));

        if (ctrl) {
            HapticResponse.defaultInstance.vibrate(400);
            ctx.onsubmit({
                submit: !!ctrl,
                valid:  true,
                data:   { ctrl, }
            });
            ctx.hide();
        }

    }

    return WindowModal.prototype.show.call(this, display, config);
}

ControlSelectionWindow.prototype.hide = function (remove = true) {
    let protectedCtx = this[_protectedDescriptor];

    if (protectedCtx.searchTask) {
        protectedCtx.searchTask.cancelVia(cancelAnimationFrame);
        protectedCtx.searchTask = null;
    }

    protectedCtx.resultBoxEl.querySelectorAll(".search-result-item-wrapper").forEach((e) => {
        e.remove();
    });

    protectedCtx.resultBoxEl.appendChild(protectedCtx.emptyBoxEl);

    protectedCtx.statusBoxEl.innerText = "";

    protectedCtx.searchEl.removeEventListener("change", this.handleSearchElChange);

    protectedCtx.searchEl.value = "";

    this.destroySpawned();

    protectedCtx.externalEvtHandler.free();

    return WindowModal.prototype.hide.call(this, remove);
}

ControlSelectionWindow.prototype.handleSearchElChange = function(e) {

    if (!e.target.checkValidity()) {
        console.error("invalid input value");
        return;
    }

    const protectedCtx = this[_protectedDescriptor];

    protectedCtx.resultBoxEl.querySelectorAll(".search-result-item-wrapper").forEach((e) => {
        e.remove();
    });

    protectedCtx.resultBoxEl.appendChild(protectedCtx.emptyBoxEl);

    this.destroySpawned();

    protectedCtx.statusBoxEl.innerText = "Searching...";

    if (protectedCtx.searchTask) {
        protectedCtx.searchTask.cancelVia(cancelAnimationFrame);
    }

    let value = e.target.value.trim();
    try {
        if (value.startsWith("/") && value.endsWith("/")) {
            value = new RegExp(value.substring(1, value.length - 1), "im");
        } else {
            if (RegExp.escape) {
                value = new RegExp(RegExp.escape(value), "im");
            } else if (value.match(/^[\w\s-]*$/)) {
                value = new RegExp(value, "im");
            } else {
                protectedCtx.statusBoxEl.innerText = "invalid query";
                return;
            }
        }
    } catch (e) {
        console.error(e);
        protectedCtx.statusBoxEl.innerText = "invalid query";
        return;
    }


    let chunk = document.createDocumentFragment();

    protectedCtx.searchTask = new RegistryBathExec(Control.registry, (ctrl, index) => {
        const sResult = this.searchInCtrl(value, ctrl);
        if (sResult.entry) {
            console.info("match", value, sResult.matches);
            const bind = this.makeResultEnt(ctrl, sResult.matches);
            chunk.appendChild(bind.node);
            protectedCtx.spawnedControls.push(bind.ref);
        }
    });

    protectedCtx.searchTask.afterBatch = () => {
        if (chunk.children.length) {
            protectedCtx.emptyBoxEl.remove();
            protectedCtx.resultBoxEl.appendChild(chunk);
            chunk = document.createDocumentFragment();
        }
    }

    protectedCtx.searchTask.scheduleVia(requestAnimationFrame);

    protectedCtx.searchTask.result.then(() => {
        protectedCtx.statusBoxEl.innerText = "done";
    }).catch(() => {
        protectedCtx.statusBoxEl.innerText = "canceled";
    });

}

ControlSelectionWindow.prototype.destroySpawned = function() {
    const protectedCtx = this[_protectedDescriptor];
    for (const view of protectedCtx.spawnedControls) {
/*        if (view.control.suspend) { //see note about suspend in ControlView
            view.control.suspend(true, view);
        }*/
        view.attributes.set("removed", true); //todo make better
    }
    protectedCtx.spawnedControls.length = 0;
}

ControlSelectionWindow.prototype.searchInCtrl = function(query, ctrl) {
    const ctrlProtectedCtx = ctrl[_protectedDescriptor];
    const domText = ctrlProtectedCtx.domTemplate.querySelector("p").innerText;
    let matches;
    if ((matches = ctrl.attributes.get("id").match(query))) {
        return { entry: true, matches: [{ text: ctrl.attributes.get("id"), tokens: matches }] };
    } else if ((matches = domText.match(query))) {
        return { entry: true, matches: [{ text: domText, tokens: matches            }] };
    } else if ((matches = ctrl.attributes.get("description").match(query))) {
        return { entry: true, matches: [{ text: ctrl.attributes.get("description"), tokens: matches }] };
    } else if ((matches = ctrl.groupIds.match(query))) {
        return { entry: true, matches: [{ text: ctrl.groupIds, tokens: matches      }] };
    }
    return { entry: false, text: undefined };
}

ControlSelectionWindow.prototype.makeResultEnt = function (ctrl) {

    const protectedCtx = this[_protectedDescriptor];

    let wrapper = document.createElement("div");
    let preview = document.createElement("div");
    let identifier = document.createElement("p");
    let description = document.createElement("span");
    let groups = document.createElement("div");

    wrapper.appendChild(preview);
    wrapper.appendChild(identifier);
    wrapper.appendChild(description);
    wrapper.appendChild(groups);


    wrapper.classList.add("search-result-item-wrapper");
    preview.classList.add("preview");
    identifier.classList.add("identifier");
    description.classList.add("description");
    groups.classList.add("groups");

    let ref = document.createElement("div");
    ref.setAttribute("data-control-id", ctrl.id);
    ref.classList.add("control");
    preview.appendChild(ref);

    const bind = Control.reference(ctrl.id, ref);
    ctrl.activateView(bind.ref);
    if (ctrl.suspend) {
        wrapper.classList.add("widget"); //todo refactor me
        ctrl.suspend(false, bind.ref);
    }

    identifier.innerText    = ctrl.attributes.get("id").trim() || "no identifier";
    description.innerText   = ctrl.attributes.get("description").trim() || "no description";
    description.innerHTML   = protectedCtx.markupProcessor.process(description.innerHTML).result;

    if (ctrl.attributes.get("groups").trim()) {
        groups.appendChild(ControlConfigurationWindow.prototype.makeGroupsCloud.call(this, ctrl.attributes.get("groups") || "", Object.assign({ name: "groups" }, ctrl.attributes.descriptor("groups") )));
    } else {
        const noitem = document.createElement("span");
        noitem.classList.add("empty");
        noitem.innerText = 'no groups';
        groups.appendChild(noitem);
    }


    bind.node = wrapper;

    return bind;
}


function Storage(prefix = "") {
    return {
        get: (key, defaultValue = null) => {
            //console.log(`${prefix}.${key}`)
            let itm = window.localStorage.getItem(`${prefix}.${key}`);
            if (itm == null) {
                return defaultValue;
            }
            return JSON.parse(itm);
        },
        getRaw: (key, defaultValue = null) => {
            return window.localStorage.getItem(`${prefix}.${key}`);
        },
        set: (key, value) => {
            console.info("set", `${prefix}.${key}`, JSON.stringify(value));
            window.localStorage.setItem(`${prefix}.${key}`, JSON.stringify(value));
        },
        remove: (key) => {
            console.info("remove", `${prefix}.${key}`, key);
            window.localStorage.removeItem(`${prefix}.${key}`);
        },
        removeAny: (key) => {
            for (let [index, value] of Object.entries(window.localStorage)) {
                if (index.startsWith(`${prefix}.${key}`)) {
                    console.info("remove", index);
                    window.localStorage.removeItem(index);
                }
            }
        },
        has: (key) => {
            return window.localStorage.getItem(`${prefix}.${key}`) != null;
        },
        hasAny: (key) => {
            for (let [index, value] of Object.entries(window.localStorage)) {
                if (index.startsWith(`${prefix}.${key}`)) {
                    return true
                }
            }
        },
        clear: () => {
            for (let [key, value] of Object.entries(window.localStorage)) {
                if (key.startsWith(prefix)) {
                    console.info("remove", key);
                    window.localStorage.removeItem(key);
                }
            }
        },
        rubric: (...keys) => {
            return Storage((prefix.length ? prefix + "." : prefix) + keys.join("."));
        },
        [Symbol.iterator]: function*(){
            for (let i = 0; i < window.localStorage.length; i++) {
                let key     = localStorage.key(i);
                let value   = localStorage.getItem(key);
                let pref= prefix ? prefix + "." : prefix
                if (key.startsWith(pref)) {
                    yield [key.replace(pref, ""), JSON.parse(value)];
                }
            }
        }
    }
}

function Breadcrumbs(elem) {
    if (!(elem instanceof Element)) {
        throw new Error("elem must be instance of Element");
    }
    this[_protectedDescriptor] = {
        elem: elem,
    }
    this.clear();
}

Breadcrumbs.prototype.update = function (index, path, pretty, leaf) {
    let protectedCtx = this[_protectedDescriptor];
    let child = protectedCtx.elem.children[index];
    if (child) {
        const isLeaf = child.firstChild.classList.contains("static");
        if (leaf !== isLeaf) {
            child.firstChild.classList.toggle("static");
        }
        child.firstChild.textContent    = pretty;
        child.firstChild.href           = path;
        child.style.visibility          = "visible";
        if (leaf) {
            //for (let elm = child.nextElementSibling; elm !== null; elm = child.nextElementSibling) { for deletion in live list
            for (let elm = child.nextElementSibling; elm !== null; elm = elm.nextElementSibling) {
                elm.style.visibility = "hidden";
            }
        }
    } else {
        this.push(path, pretty, leaf);
    }
}

Breadcrumbs.prototype.push = function (path, pretty, leafHint = undefined) {
    let protectedCtx = this[_protectedDescriptor];
    let newPrefix = this.make(path, pretty, leafHint !== false);
    protectedCtx.elem.appendChild(newPrefix);
    if (newPrefix.previousElementSibling && newPrefix.previousElementSibling.classList.contains("static")) {
        newPrefix.previousElementSibling.replaceWith(this.make(
            newPrefix.previousElementSibling.firstElementChild.getAttribute("data-href") || "",
            newPrefix.previousElementSibling.firstElementChild.textContent,
            false
        ));
    }
}

Breadcrumbs.prototype.pop = function (path, pretty) {
    let protectedCtx = this[_protectedDescriptor];
    if (protectedCtx.elem.lastElementChild) {
       let last = protectedCtx.elem.lastElementChild;
       last.remove();
        last = protectedCtx.elem.lastElementChild;
        if (last) {
            last.replaceWith(this.make(
                last.firstElementChild.href,
                last.firstElementChild.textContent,
                true
            ));
        }
    }
}

Breadcrumbs.prototype.clear = function () {
    this[_protectedDescriptor].elem.innerHTML = "";
}

Breadcrumbs.prototype.make = function (path, pretty, leaf = false) {
    let item;
    if (leaf) {
        //in order to speed up layout update when swipe comme, all node must be same (no reuse it rather than replace)
        //so... leaf of Breadcrumbs is a too
        item = document.createElement("a");
        item.classList.add("breadcrumb-label", "static");
        item.href = path;
    } else {
        item = document.createElement("a");
        item.classList.add("breadcrumb-label", "active");
        item.href = path;
    }
    item.textContent = pretty;
    let node = document.createElement("li");
    node.appendChild(item);
    return node;
}

function SettingsMarkupParser(config = {}) {
    this[_protectedDescriptor] = {
        errors: [],
        result: null,
    };
}

SettingsMarkupParser.prototype.process = function(markup) {
    const protectedCtx = this[_protectedDescriptor];
    protectedCtx.errors.length = 0;
    protectedCtx.result = null;

    markup = markup.trim();
    const wrapperConverter = document.createElement('div');
    wrapperConverter.appendChild( document.createElement('div') );
    (markup.match(/(^|\s|>)#[^\n#]*#[^\n#]*#[^\n#]*#(\s|<|$)/g) || []).map((token) => {
        token = token.trim().replace(/^>/, "").replace(/<$/, "");
        if (token.length && !token.match("\n|<br>|<br\/>")) {
            wrapperConverter.firstChild.replaceWith(this.makeEl(token));
            markup = markup.replace(token, wrapperConverter.innerHTML);
        }
    });

    protectedCtx.result = markup;

    return this;
}

SettingsMarkupParser.prototype.makeEl = function(token) {
    const protectedCtx = this[_protectedDescriptor];

    token = token.split("#");
    if (token.length !== 5) {
        protectedCtx.errors.push({ error: "invalid items count"  });
    }
    if (token[0] !== "" || token[4] !== "") {
        protectedCtx.errors.push({ error: "invalid items layout" });
    }

    // #activate#ctrl+a#/settings/layout/control/any# => "", "activate", "ctrl+a", "/settings/layout/control/any", ""

    const [,action, shortcut, path, ...other] = token;

    const linkEl = document.createElement("a");
    linkEl.setAttribute("href", "#");
    linkEl.setAttribute("title", linkEl.innerText = path); linkEl.innerText = "";
    linkEl.classList.add("control-tip");
    const actionEl = document.createElement("span");
    actionEl.classList.add("action");
    actionEl.innerText = action.trim();
    const shortcutEl = document.createElement("span");
    shortcutEl.classList.add("shortcut");
    if (!shortcut.trim()) {
        actionEl.classList.add("unset");
    }
    shortcutEl.innerText = shortcut.trim();
    linkEl.appendChild(actionEl);
    linkEl.appendChild(shortcutEl);

    return linkEl;
}

Object.defineProperties(SettingsMarkupParser.prototype, {
    errors: {
        get() { return this[_protectedDescriptor].errors            }
    },
    valid: {
        get() { return !!this[_protectedDescriptor].errors.length   }
    },
    result: {
        get() { return this[_protectedDescriptor].result            }
    }
});

function Notifications(container) {

    if (!(container instanceof Element)) {
        throw new Error("elem must be instance of Element");
    }

    let privateCtx = {
        container,
        make(message, category = "", severity = "info") {
            let p = document.createElement("p");
            p.innerText = message;
            let d = document.createElement("div");
            d.appendChild(p);
            d.classList.add(`category-${privateCtx.category2class(category)}`, severity, "notification");
            d[privateCtx.nodeStorage] = {};
            return d;
        },
        category2class(cat) {
            return cat.replace("_", "-").replace(" ", "-").toLowerCase();
        },
        nodeStorage: Symbol("nodeStorage"),
    }


    return plainObjectFrom({
        addNotification(message, category = "", severity = "info", time = Notifications.defaultLivetime) {

            const node = privateCtx.make(message, category, severity);
            if (time) {
                node.classList.add("timed")
                node.style.setProperty('--time', time + "ms");
            }

            let isNew = true;
            if (category) {
                const elm = privateCtx.container.querySelector(`.category-${privateCtx.category2class(category)}`);
                if (elm) {
                    if (elm.textContent === node.textContent) {
                        console.log("content is same, spam protection");
                        return;
                    }
                    elm.replaceWith(node);
                    isNew = false;
                    clearTimeout(elm[privateCtx.nodeStorage].timeout);
                    node.classList.remove("started");
                }
            }

            if (isNew) {
                node.classList.add("before");
                privateCtx.container.appendChild(node);
                setTimeout(() => node.classList.remove("before"), 10);
            }

            if (time > 0) {
                node[privateCtx.nodeStorage].timeout = setTimeout((node) => {
                    node.classList.add("after");
                    animationFinished(node, { deadline: time + 200 }).then(() => {
                        node.classList.remove(`category-${privateCtx.category2class(category)}`);
                        //little wait to achieve better remove effect
                        animationFinished(node, { deadline: 300 }).then(() => node.remove());
                    });
                }, time, node);
                setTimeout(() => node.classList.add("started"), 250);
            }

        },
        removeNotification(category) {
            const elm = privateCtx.container.querySelector(`.category-${privateCtx.category2class(category)}`);
            if (elm) {
                elm.remove();
            }
        },
        remover(node, readline = 1000) {

        },
        clear() {
            privateCtx.container.innerHTML = "";
        }
    });
}
Notifications.defaultLivetime = 5000;

function UiLocker(name, onLock, onUnlock) {
    let privateCtr = {
        name,
        onLock,
        onUnlock,
        reasons: [],
        state: false,
    }
    return {
        lock: (reason) => {
            if (privateCtr.reasons.indexOf(reason) === -1) {
                privateCtr.reasons.push(reason);
                if (privateCtr.state) {
                    console.log("UiLocker updated reasons", privateCtr.name, privateCtr.reasons);
                }
            }
            if (!privateCtr.state && privateCtr.reasons.length) {
                if (isCallable(privateCtr.onLock)) {
                    privateCtr.onLock(privateCtr.reasons);
                    console.info("UiLocker locked",  privateCtr.name, privateCtr.reasons);
                } else {
                    console.warn("UiLocker lock", privateCtr.name, "no callback");
                }
                privateCtr.state = true;
                return true
            }
            return false;
        },
        unlock: (lockReason) => {
            const index = privateCtr.reasons.indexOf(lockReason);
            if (index === -1) {
                return false
            }
            privateCtr.reasons.splice(index, 1);
            if (!privateCtr.reasons.length && privateCtr.state) {
                if (isCallable(privateCtr.onUnlock)) {
                    privateCtr.onUnlock(privateCtr.reasons);
                    console.info("UiLocker unlocked", privateCtr.name);
                } else {
                    console.warn("UiLocker unlock", privateCtr.name, "no callback");
                }
                privateCtr.state = false;
                return true;
            } else if (privateCtr.state) {
                console.info("UiLocker still locked", privateCtr.name, privateCtr.reasons);
            }
            return false;
        },
        locked: () => {
            return privateCtr.state;
        },
        has: (reason) => {
            return privateCtr.reasons.indexOf(reason) !== -1;
        }
    }
}

function convFormat(data) {
    let mvalue = 0;
    const row = data.length;
    let col = 0;
    return data.map((e, i)=>{
        e.split(" ").map((e, i) => {
            if (e.startsWith("item-")) {
                mvalue = Math.max(parseInt(e.substring("item-".length)), mvalue);
            }
            col = i+1;
        });
        for (let i = 0; i <= mvalue; i++) {
            e = e.replaceAll("item-" + i.toString().padStart(3, "0"), Page.elementTag(i, row * col));
        }
        return e;
    });
}