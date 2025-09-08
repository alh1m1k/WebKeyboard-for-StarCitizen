//import { Socket } from "./lib"

"use strict";

const pageDefaultLayout = {
    col: 5,
    row: 5,
    gap: 1,
    pad: 0.7,
    layout: ""
}

const versions = {
    protocol: 0.1,
    storage:  0.4,
    server:   0.1,
    client:   0.1,

    incremental: 2,
};

const timeouts = {
    settings:       750,
};

const invertAspectRatio = 2/3.1; //@see CSS


(function(){

    //console.log = console.warn = console.info = () => {};

    const defaultAxisParams = { step: 1, min: 0, max: 8 };

    CanvasControlWidget._protectedDescriptor = Symbol("protected");
    function CanvasControlWidget(domTemplate, WidgetImpl = null) {
        Control.call(this, domTemplate);
        this[CanvasControlWidget._protectedDescriptor] = {
            WidgetImpl, widgetsActivated: 0, widgetsAnimating: 0,
        };
    }

    Object.setPrototypeOf(CanvasControlWidget.prototype, Control.prototype);

    CanvasControlWidget.prototype.activateView = function(refOrNode) {
        const view = this.viewAt(refOrNode);
        if (!view) {
            throw new Error("view not found or argument incorrect");
        }
        const widgetProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor] = { pendingWidget: null, activeWidget: null };
        if (!widgetProtectedCtx.pendingWidget) {
            widgetProtectedCtx.pendingWidget = new Promise((resolve) => {
                //canvas integration note:
                //canvas init must be delayed until dom layout calculation complete
                //as it (canvas) aggressively cache layout data
                //there is no flush API available like window.promiseDocumentFlushed
                //so we use requestAnimationFrame to move query out of scope
                //warning: requestAnimationFrame probably not best place to query layout data
                //warning: possible race between free* like methods and animate(true)
                requestDocumentFlushTime(() => {
                    const dom = view.dom;
                    //console.log("requestDocumentFlushTime", dom, dom.querySelector);
                    const container = dom.querySelector(".canvas-wrapper") || dom;
                    let hndl = this.buildWidget(container);
                    this.setupWidget(dom, hndl);
                    if (this.disabled) {  //todo this is temporal workaround problem when widget unable to handle resize if it start in disabled state
                        hndl.initEvents();
                        hndl.freeEvents(true);
                    }
                    this.widgetActivate(view, hndl);
                    resolve(hndl);
                });
            });
        } else {
            console.warn("CanvasControlWidget.prototype.activateView", "second activation");
        }
        view.activated = true;
    }

    CanvasControlWidget.prototype.deactivateView = function(refOrNode) {
        //todo keep resize event
        const view = this.viewAt(refOrNode);
        if (!view) {
            throw new Error("view not found or argument incorrect");
        }
        const widgetProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
        if (widgetProtectedCtx && widgetProtectedCtx.pendingWidget) {
            widgetProtectedCtx.pendingWidget.then((widget) => {
                this.widgetDeactivate(view, widget);
                widget.free();
            });
            widgetProtectedCtx.pendingWidget = null;
        } else {
            console.warn("CanvasControlWidget.prototype.deactivateView", "second deactivation");
        }
        view.activated = false;
    }

    CanvasControlWidget.prototype.widgetActivate = function(view, widget) {
        view.data[CanvasControlWidget._protectedDescriptor].activeWidget = widget;
        this[CanvasControlWidget._protectedDescriptor].widgetsActivated++;
    }

    CanvasControlWidget.prototype.widgetDeactivate = function(view) {
        view.data[CanvasControlWidget._protectedDescriptor].activeWidget = null;
        this[CanvasControlWidget._protectedDescriptor].widgetsActivated--;
    }

    CanvasControlWidget.prototype.suspend = function(suspend, refOrNode, page) {
        const view = this.viewAt(refOrNode);
        if (view) {
            const widgetProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
            if (widgetProtectedCtx && widgetProtectedCtx.pendingWidget) {
                widgetProtectedCtx.pendingWidget.then((widget) => {
                    //warning: possible race between free* like methods and animate(true)
                    //animate might fire after element removed from pendingWidget list
                    //also see PDSWidgetsPad.prototype.animate WORKAROUND note
                    if (suspend) {
                        this.widgetSuspend(view, widget);
                    } else {
                        this.widgetAnimate(view, widget);
                    }
                    view.suspended = suspend;
                    if (!widgetProtectedCtx.pendingWidget) {
                        console.error("CanvasControlWidget.prototype.suspend", "promise chain probably broken");
                    }
                });
            }
        }
    }

    CanvasControlWidget.prototype.widgetSuspend = function(view, widget) {
        if (widget.isAnimating()) {
            widget.animate(false);
            this[CanvasControlWidget._protectedDescriptor].widgetsAnimating--;
        }
    }

    CanvasControlWidget.prototype.widgetAnimate = function(view, widget) {
        if (!widget.isAnimating()) {
            widget.animate(true);
            this[CanvasControlWidget._protectedDescriptor].widgetsAnimating++;
        }
    }

    CanvasControlWidget.prototype.disableNodeTransition = function (dom, disable) {
        const view = this.viewAt(dom);
        if (view) {
            const widgetProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
            if (widgetProtectedCtx && widgetProtectedCtx.pendingWidget) {
                widgetProtectedCtx.pendingWidget.then((widget) => {
                    if (disable) {
                        widget.freeEvents(true);
                    } else {
                        widget.initEvents();
                    }
                });
            }
        }
        Control.prototype.disableNodeTransition.call(this, dom, disable);
    }

    CanvasControlWidget.prototype.initEvents = function(dom) {
        return plainObjectFrom({});
    }

    CanvasControlWidget.prototype.widgetConf = function(dom) {
        return { noevents: this.disabled, noanimation: true };
    }

    CanvasControlWidget.prototype.buildWidget = function(dom) {
        const builder = this.WidgetImpl();
        return new builder(dom, this.widgetConf(dom));
    }

    CanvasControlWidget.prototype.activate = function(kbd, activationType = "activate-default", context = Control.defaultContext) {
        if (this.isCtrl(kbd)) {
            setTimeout((kbd) => this.callCtrl(kbd, context.next({ ctrl: this })), 0, kbd); //move out
            kbd = ":" + this.id + ":" + activationType;
        } else {
            kbd = kbd + ":" + this.id + ":" + activationType;
        }
        this.dispatchEvent("fire", this, kbd);
    }

    CanvasControlWidget.prototype.setupWidget = function(dom, widget) {}

    CanvasControlWidget.prototype.WidgetImpl = function() {
        return this[CanvasControlWidget._protectedDescriptor].WidgetImpl;
    }

    ShieldControlWidget[_protectedDescriptor] = {};

    function ShieldControlWidget(domTemplate, WidgetImpl = ShieldWidget) {
        CanvasControlWidget.call(this, domTemplate, WidgetImpl);
    }

    Object.setPrototypeOf(ShieldControlWidget.prototype, CanvasControlWidget.prototype);

    ShieldControlWidget.prototype.initAttributes = function(dom, attributesDefaults = {}) {
        let ctx = this;
        return Control.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
            "kbd-front": {
                label:          "kb key(front)",
                description:    "key that pressed when shield redirected to the Front",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("front") || this.getValue()
            },
            "kbd-right": {
                label:          "kb key(right)",
                description:    "key that pressed when shield redirected to the Right",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("right") || this.getValue()
            },
            "kbd-back": {
                label:          "kb key(back)",
                description:    "key that pressed when shield redirected to the Back",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("back") || this.getValue()
            },
            "kbd-left": {
                label:          "kb key(left)",
                description:    "key that pressed when shield redirected to the Left",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("left") || this.getValue()
            },
            "kbd-top": {
                label:          "kb key(top)",
                description:    "key that pressed when shield redirected to the Top",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("top") || this.getValue()
            },
            "kbd-bottom": {
                label:          "kb key(bottom)",
                description:    "key that pressed when shield redirected to the Bottom",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("bottom") || this.getValue()
            },
            "kbd-reset": {
                label:          "kb key(reset)",
                description:    "key that pressed when shield reseted",
                type:           Attribute.types.KBKEY,
                value: () => this.getValue("reset") || this.getValue()
            },
            "formula": {
                label: "formula",
                description: "widget display param (number of shield directions)",
                type: Attribute.types.ENUM,
                params: Object.assign({},{  }, attributesDefaults.formula ? attributesDefaults.formula.params : {}),
/*                validate: (value, attributes) => {
                    return { valid: true, mutableValue: +value, errors: [], };
                },*/
                apply: (value, oldValue, attributes) => {
                    for (const view of this) {
                        const viewProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
                        if (viewProtectedCtx && viewProtectedCtx.pendingWidget) {
                            viewProtectedCtx.pendingWidget.then((widget) => {
                                widget.count = view.attributes.get("formula");
                            });
                        }
                    }
                },
                value: () => attributesDefaults["formula"] !== undefined ?  +attributesDefaults["formula"].value : 5
            },
            "size": {
                label: "size",
                description: "widget shield field size",
                type: Attribute.types.INT,
                units: "px",
                params: Object.assign({},{ step: 1, min: 1, max: 100}, attributesDefaults.size ? attributesDefaults.size.params : {}),
                apply: (value, oldValue, attributes) => {
                    for (const view of this) {
                        const viewProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
                        if (viewProtectedCtx && viewProtectedCtx.pendingWidget) {
                            viewProtectedCtx.pendingWidget.then((widget) => {
                                widget.size = view.attributes.get("size");
                            });
                        }
                    }
                },
                value: () => attributesDefaults["size"] !== undefined ?  +attributesDefaults["size"].value : 50
            },
        }), ["kbd"]);
    }

    ShieldControlWidget.prototype.widgetConf = function(dom) {
        const attributes = this.viewAt(dom).attributes;
        return  Object.assign(CanvasControlWidget.prototype.widgetConf.call(this, dom), { size: attributes.get("size"),  count: attributes.get("formula") });
    }

    ShieldControlWidget[_protectedDescriptor].widgetChosed = (t,c,d,that) => {
        //ce is canvasEvent, similar to just e (inputEvent)
        that.control.activate(d.index, Control.defaultContext.next({ ce: d, view: that }));
    }

    ShieldControlWidget[_protectedDescriptor].widgetActivate = (t,c,stage,that) => {
        switch (stage) {
            case 0:
            case 1:
                HapticResponse.defaultInstance.vibrate(100, "widget activate " + stage);
                break;
            default:
                console.error("undefined activation stage", stage);
        }

    }

    ShieldControlWidget.prototype.setupWidget = function(dom, widget) {
        CanvasControlWidget.prototype.setupWidget.call(this, dom, widget);
        const ref = this.viewAt(dom);
        widget.attachEvent("chosed",    ShieldControlWidget[_protectedDescriptor].widgetChosed,   ref);
        widget.attachEvent("activate",  ShieldControlWidget[_protectedDescriptor].widgetActivate, ref);
    }

    ShieldControlWidget.prototype.activate = function(type, context = Control.defaultContext) {
        const widget = this[CanvasControlWidget._protectedDescriptor].WidgetImpl;
        const attributes = context.view ? context.view.attributes : this.attributes;
        let kbd;
        switch (type) {
            case widget.vectors.FRONT:
                kbd = attributes.get("kbd-front");
                break;
            case widget.vectors.LEFT:
                kbd = attributes.get("kbd-left");
                break;
            case widget.vectors.BACK:
                kbd = attributes.get("kbd-back");
                break;
            case widget.vectors.RIGHT:
                kbd = attributes.get("kbd-right");
                break;
            case widget.vectors.TOP:
                kbd = attributes.get("kbd-top");
                break;
            case widget.vectors.BOTTOM:
                kbd = attributes.get("kbd-bottom");
                break;
            case widget.vectors.SPECIAL:
                kbd = attributes.get("kbd-reset");
                break;
            default:
                throw new Error("invalid vector");
        }
        CanvasControlWidget.prototype.activate.call(this, kbd, "activate-" + type, context);
        HapticResponse.defaultInstance.vibrate(400, "widget set");
    }

    ShieldControlWidget.prototype.reset = function(context = Control.defaultContext) {

    }

    ShieldControlWidget.prototype.applyState = function(state) {
        const widget = this[CanvasControlWidget._protectedDescriptor].WidgetImpl;
        const strIndex = state.indexOf("-");
        if (strIndex === -1 || ((state.length - strIndex) < 2)) {
            throw new Error("invalid state");
        }
        const index = parseInt(state.substring(strIndex+1));
        if (isNaN(index) || index > widget.vectors.RESERVED || index < 0) {
            throw new Error("invalid state");
        }
        for (const view of this) {
            const viewProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
            if (viewProtectedCtx && viewProtectedCtx.pendingWidget) {
                viewProtectedCtx.pendingWidget.then((widget) => {
                    if (!widget.beginSelection) {
                        widget.chosed(index, widget.parts[index], true);
                    }
                });
            }
        }
    }

    function PDSControlWidget(domTemplate, WidgetImpl = PDSWidgetsPad) {
        CanvasControlWidget.call(this, domTemplate, WidgetImpl);
    }

    Object.setPrototypeOf(PDSControlWidget.prototype, CanvasControlWidget.prototype);

    PDSControlWidget.prototype.initAttributes = function(dom, attributesDefaults = {}) {
        let ctx = this;
        return CanvasControlWidget.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
            "kbd-weapon-pwr-up": {
                label: "kb key(weapon power up)",
                description: "key that pressed when increased weapon power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("weapon-up", this.getValue()),
            },
            "kbd-weapon-pwr-down": {
                label: "kb key(weapon power down)",
                description: "key that pressed when decreased weapon power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("weapon-down", this.getValue()),
            },
            "kbd-shield-pwr-up": {
                label: "kb key(shield power up)",
                description: "key that pressed when increased shield power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("shield-up", this.getValue()),
            },
            "kbd-shield-pwr-down": {
                label: "kb key(shield power down)",
                description: "key that pressed when decreased shield power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("shield-down", this.getValue()),
            },
            "kbd-engine-pwr-up": {
                label: "kb key(engine power up)",
                description: "key that pressed when increased engine power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("engine-up", this.getValue()),
            },
            "kbd-engine-pwr-down": {
                label: "kb key(engine power down)",
                description: "key that pressed when decreased engine power",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("engine-down", this.getValue()),
            },
            "kbd-throttle-up": {
                label: "kb key(throttle up)",
                description: "key that pressed when increased throttle",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("throttle-up", this.getValue()),
            },
            "kbd-throttle-down": {
                label: "kb key(throttle down)",
                description: "key that pressed when decreased throttle",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("throttle-down", this.getValue())
            },
            "kbd-cooler-up": {
                label: "kb key(cooler up)",
                description: "key that pressed when increased cooler rate",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("cooler-up", this.getValue()),
            },
            "kbd-cooler-down": {
                label: "kb key(cooler down)",
                description: "key that pressed when decreased cooler rate",
                type: Attribute.types.KBKEY,
                value: () => this.getValue("cooler-down", this.getValue())
            },
        }), ["kbd"]);
    }

    PDSControlWidget.prototype.widgetConf = function(dom) {
        return  Object.assign(CanvasControlWidget.prototype.widgetConf.call(this, dom),{
            widgets: [{ id: "weapon-pwr", label: { text: "WPN" } }, { id: "shield-pwr", label: { text: "SHLD" } }, { id: "engine-pwr", label: { text: "ENGN" } }, {id: "throttle", label: { text: "THRL" } }, {id: "cooler", label: { text: "COOL" } }],
            layout: {
                regionDefault: {
                    paddingLeft:    10,
                    paddingRight:   10,
                },
                regions: []
            },
            widgetDefaults: {
                background: {
                    scaleStyle: PDSWidgetBackground.scaleStyle.zeroCenteredSwitch,
                    font:       "normal 10px arial",
                },
                head: {
                    triangleEdge: 0.15,
                    selectorFillStyle: "white",
                    fillStyle: "rgba(135, 205, 250, .5)",
                    selectorScale: 0.15,
                }
            }
        });
    }

    PDSControlWidget.prototype.setupWidget = function(dom, widget) {
        CanvasControlWidget.prototype.setupWidget.call(this, dom, widget);
        const ref = this.viewAt(dom);
        widget.attachEvent("change", (t, context, d, that) => {
            if (d.value === 0) {
                return;
            } else if (d.value >= 0.5) {
                that.control.activate(context, "up",   Control.defaultContext.next({ ce: d, view: that }) );
            } else if (d.value <= -0.5) {
                that.control.activate(context, "down", Control.defaultContext.next({ ce: d, view: that }) );
            } else {

            }
            context.value = 0;
        }, ref);
        widget.attachEvent("activate", (t, context, d) => {
            HapticResponse.defaultInstance.vibrate(100, "widget activate");
        }, ref);
    }


    PDSControlWidget.prototype.activate = function(widget, direction, context = Control.defaultContext) {
        const attributes = context.view ? context.view.attributes : this.attributes;
        let kbd = attributes.get(`kbd-${widget.id}-${direction}`);
        CanvasControlWidget.prototype.activate.call(this, kbd, `activate-${widget.id}-${direction}`, context);
        HapticResponse.defaultInstance.vibrate(400, "widget set");
    }

    PDSControlWidget.prototype.reset = function(context = Control.defaultContext) {

    }

    PDSControlWidget.prototype.applyState = function(state) {

        const widgetConf = this.widgetConf();
        const strIndex = state.lastIndexOf("-");
        if (strIndex === -1 || ((state.length - strIndex) < 2)) {
            throw new Error("invalid state");
        }
        const widgetId = state.substring(state.indexOf("-") + 1, strIndex);
        const valueTag = state.substring(strIndex+1);
        const widgetIndex = widgetConf.widgets.reduce((acc, widget, index) => {
            return acc !== -1 ? acc : (widget.id === widgetId ? index : -1);
        }, -1);

        if (widgetIndex === -1) {
            throw new Error("invalid state");
        }

        const valueState = { value: valueTag === "up" ? 0.7 : -0.7 };
        for (const view of this) {
            const viewProtectedCtx = view.data[CanvasControlWidget._protectedDescriptor];
            if (viewProtectedCtx && viewProtectedCtx.pendingWidget) {
                viewProtectedCtx.pendingWidget.then((widget) => {
                    widget = widget.widgets[widgetIndex];
                    widget.applyState(valueState); //todo return promise
                    setTimeout(() => { if (widget.value === valueState.value) { widget.value = 0; } }, 750);
                });
            }
        }

    }

    LimiterControlWidget[_protectedDescriptor] = {};

    function LimiterControlWidget(domTemplate, WidgetImpl = PDSWidgetsPad) {
        CanvasControlWidget.call(this, domTemplate, WidgetImpl);

        Object.assign(this[CanvasControlWidget._protectedDescriptor], {
            netValues:          [],
            valueOwners:        [], //for netValues(local one), cancel positive feedback
            joystickInstalled:  false,
            syncRequest:        undefined,
            widgetsSyncList:    [], //for fast access
            mustUpdateSyncList: false,
            //syncAnimating:    true;
        });

        this[CanvasControlWidget._protectedDescriptor].valueOwners.length = 8;

        this.joystickChangeHandler = this.joystickChangeHandler.bind(this);
        this.sync = this.sync.bind(this);

    }

    Object.setPrototypeOf(LimiterControlWidget.prototype, CanvasControlWidget.prototype);

    LimiterControlWidget.prototype.joystickChangeHandler = function (evtId, joystick, local) {
        /*console.warn(LimiterControlWidget.prototype.joystickChangeHandler, this.id);*/
        const protectedCtx = this[CanvasControlWidget._protectedDescriptor];
        for (let axis = 0; axis < joystick.config.axisCount; ++axis) {
            protectedCtx.netValues[axis]    = joystick.getAxis(axis);
            protectedCtx.valueOwners[axis]  = joystick.localData.axisOwners[axis];
        }
        this.resync();
    }

    LimiterControlWidget.prototype.widgetActivate = function(view, widget) {
        CanvasControlWidget.prototype.widgetActivate.call(this, view, widget);
        const protectedCtx = this[CanvasControlWidget._protectedDescriptor];
        if (!protectedCtx.joystickInstalled) {
            protectedCtx.netValues.length = joystick.config.axisCount;
            for (let axis = 0; axis < joystick.config.axisCount; ++axis) {
                protectedCtx.netValues[axis] = joystick.getAxis(axis);
            }
            joystick.attachEvent("change", this.joystickChangeHandler, this);
            protectedCtx.joystickInstalled = true;
            console.log("LimiterControlWidget.prototype.widgetActivate", "joystick installed");
        }
    }

    LimiterControlWidget.prototype.widgetDeactivate = function(view, widget) {
        joystick.localData.axisOwners.map(owner => owner === widget ? null : owner); //to prevent leak
        CanvasControlWidget.prototype.widgetDeactivate.call(this, view, widget);
        const protectedCtx = this[CanvasControlWidget._protectedDescriptor];
        if (protectedCtx.joystickInstalled && protectedCtx.widgetsActivated === 0) {
            joystick.detachEvent("change", this.joystickChangeHandler);
            protectedCtx.joystickInstalled = false;
            console.log("LimiterControlWidget.prototype.widgetDeactivate", "joystick deinstalled", this.joystickChangeHandler);
        }
        protectedCtx.mustUpdateSyncList = true;
    }

    LimiterControlWidget.prototype.widgetSuspend = function(view, widget) {
        CanvasControlWidget.prototype.widgetSuspend.call(this, view, widget);
        this[CanvasControlWidget._protectedDescriptor].mustUpdateSyncList = true;
    }

    LimiterControlWidget.prototype.widgetAnimate = function(view, widget) {
        CanvasControlWidget.prototype.widgetAnimate.call(this, view, widget);
        this[CanvasControlWidget._protectedDescriptor].mustUpdateSyncList = true;
        this.resync();
    }

    LimiterControlWidget.prototype.updateWidgetsSyncList = function() {
        const protectedCtx = this[CanvasControlWidget._protectedDescriptor];
        protectedCtx.widgetsSyncList.length = 0;
        for (const view of this) {
            if (view.attached && view.activated && !view.suspended) {
                if (view.data[CanvasControlWidget._protectedDescriptor].activeWidget) {
                    protectedCtx.widgetsSyncList.push(view.data[CanvasControlWidget._protectedDescriptor].activeWidget);
                }
            }
        }
    }

    LimiterControlWidget.prototype.resync = function() {
        const protectedCtx = this[CanvasControlWidget._protectedDescriptor];
        if (protectedCtx.syncRequest === undefined && protectedCtx.widgetsAnimating > 0) {
            protectedCtx.syncRequest = setTimeout(this.sync, 0);
        }
    }

    LimiterControlWidget.prototype.sync = function() {

        const protectedCtx  = this[CanvasControlWidget._protectedDescriptor];

        //this.syncRequest.complete = true;
        protectedCtx.syncRequest = undefined; //keep in sync even if fail occur

        if (this.disabled) {
            return;
        }

        if (protectedCtx.mustUpdateSyncList) {
            this.updateWidgetsSyncList();
            protectedCtx.mustUpdateSyncList = false;
            //console.log("mustUpdateSyncList", this.id);
        }

        console.log("update", this.id);

        const widgets = protectedCtx.widgetsSyncList;
        //const keys = Object.keys(widgets);

        if (!widgets.length) {
            return;
        }

        const widget = widgets[0];

        let newState = { value: 0 };

        for (let wIndex = 0; wIndex < widget.widgets.length; ++wIndex) {
            let     wWidget = widget.widgets[wIndex];
            const   axis = this.widgetToAxis(wWidget);

            let     typicalValue    = wWidget.value;
            let     typicalNewValue = typicalValue;
            let     netNewValue     = this.toWidgetScale(wWidget, protectedCtx.netValues[axis]);

            if (this.fromWidgetScale(wWidget, typicalValue) !== protectedCtx.netValues[axis]) {
                try {
                    typicalNewValue = netNewValue;
                    if (!wWidget.locked && protectedCtx.valueOwners[axis] !== wWidget) {
                        newState.value = typicalNewValue;
                        wWidget.applyState(newState);
                    }
                } catch (e) { console.warn(e); } //may fail if widget currently handle user input
            }

            for (let index = 1; index < widgets.length; ++index) {
                wWidget = widgets[index].widgets[wIndex];
                const wWidgetValue = wWidget.value;
                if (wWidgetValue === typicalValue) {
                    if (wWidgetValue !== typicalNewValue) {
                        try {
                            if (!wWidget.locked && protectedCtx.valueOwners[axis] !== wWidget) {
                                newState.value = typicalNewValue;
                                wWidget.applyState(newState);
                            }
                        } catch (e) { console.warn(e); } //may fail if widget currently handle user input
                    }
                } else {
                    if (this.fromWidgetScale(wWidget, wWidgetValue) !== protectedCtx.netValues[axis]) {
                        try {
                            if (!wWidget.locked && protectedCtx.valueOwners[axis] !== wWidget) {
                                newState.value = netNewValue;
                                wWidget.applyState(newState);
                            }
                        } catch (e) { console.warn(e); } //may fail if widget currently handle user input
                    }
                }
                //console.log("sync", wWidget.id, wWidgetValue, this.toWidgetScale(wWidget, this.netValues[axis]));
            }
        }

    }

    LimiterControlWidget.prototype.initAttributes = function(dom, attributesDefaults = {}) {
        let ctx = this;
        return CanvasControlWidget.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
            "kbd-speed-limit": {
                label: "axis(limit speed)",
                description: "axis that used to limit ship max speed",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("speed-limit", this.getValue()),
            },
            "kbd-acceleration-limit": {
                label: "axis(acceleration limit)",
                description: "axis that used to limit ship max acceleration",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("acceleration-limit", this.getValue()),
            },
            "kbd-turret-speed-limiter": {
                label: "axis(turret rotation speed limiter)",
                description: "axis that used to limit rotation speed of the turret",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("turret-speed-limiter", this.getValue()),
            },
            "kbd-weap-intersect": {
                label: "axis(weapon intersection distance)",
                description: "axis that used to set distance where projectile launched from ship will intersect",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("weap-intersect", this.getValue()),
            },
            "kbd-opt-1": {
                label: "axis(optional-1)",
                description: "optional axis 1",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("opt-1", this.getValue()),
            },
            "kbd-opt-2": {
                label: "axis(optional-2)",
                description: "optional axis 2",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("opt-2", this.getValue()),
            },
            "kbd-opt-3": {
                label: "axis(optional-3)",
                description: "optional axis 3",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("opt-3", this.getValue()),
            },
            "kbd-opt-4": {
                label: "axis(optional-4)",
                description: "optional axis 4",
                type: Attribute.types.AXIS,
                inheritable: false, //due to limitation of sync
                params: defaultAxisParams,
                value: () => +this.getValue("opt-4", this.getValue()),
            },
        }), ["kbd"]);
    }

    LimiterControlWidget.prototype.widgetConf = function(dom) {
        return  Object.assign(CanvasControlWidget.prototype.widgetConf.call(this, dom),{
            id: "limiters",
            layout: {
                type: WidgetLayout.types.COLS,
                paddingTop:     10,
                paddingBottom:  10,
                regions: [
                    {
                        type: WidgetLayout.types.COLS,
                        flex: 1,
                        regions: [
                            {  }, {  },
                            {  }, {  },
                        ],
                        regionDefault: {
                            flex: 1, paddingLeft: 10, paddingRight: 10
                        }
                    },
                    {
                        type: WidgetLayout.types.TABLE,
                        flex: 1,
                        colCount: 2,
                        rowCount: 2,
                        regionDefault: {
                            flex: 1,
                            paddingLeft: 10, paddingRight: 10
                        },
                        regionRow: [
                            { paddingBottom: 10 }, //not need for secondary it will-be defaulted
                        ],
                        regionCol: [
                            { paddingLeft: 0 }, { paddingRight: 0 }
                        ],
                    }
                ]
            },
            widgets: [
                { id: "speed-limit",            label: { text: "speed",            id: "speed-limit-label"          },  background: { id: "speed-limit-bg",             themeConstructor: PDSWidgetBackgroundLineralThemeTrait }, head: { id: "speed-limit-head",          fillStyle: "rgb(135, 205, 250)", selectorFillStyle: "white", fontFillStyle: "white", fontFillStyleHigh: "black", fontFillStyleLow: "white", selectorScale: 0.15 } }, //fixme selectorScale value bigger than 17 render invalid (white line cause bu cursor overflow box)
                { id: "acceleration-limit",     label: { text: "acceleration",     id: "acceleration-limit-label"   },  background: { id: "acceleration-limit-bg",      themeConstructor: PDSWidgetBackgroundLineralThemeTrait }, head: { id: "acceleration-limit-head",   fillStyle: "rgb(135, 205, 250)", selectorFillStyle: "white", fontFillStyle: "white", fontFillStyleHigh: "black", fontFillStyleLow: "white", selectorScale: 0.15 } },
                { id: "turret-speed-limiter",   label: { text: "turret speed" ,    id: "turret-speed-limiter-label" },  background: { id: "turret-speed-limiter-bg",    themeConstructor: PDSWidgetBackgroundLineralThemeTrait }, head: { id: "turret-speed-limiter-head", fillStyle: "rgb(135, 205, 250)", selectorFillStyle: "white", fontFillStyle: "white", fontFillStyleHigh: "black", fontFillStyleLow: "white", selectorScale: 0.15 } },
                { id: "weap-intersect",         label: { text: "weapon intersect", id: "weap-intersect-label"       },  background: { id: "weap-intersect-bg",          themeConstructor: PDSWidgetBackgroundLineralThemeTrait }, head: { id: "weap-intersect-head",       fillStyle: "rgb(135, 205, 250)", selectorFillStyle: "white", fontFillStyle: "white", fontFillStyleHigh: "black", fontFillStyleLow: "white", zeroAt: PDSWidgetHead.zeroAt.CENTER, selectorScale: 0.15 } },

                { id: "opt-1", label: { text: "opt-1", id: "opt-1-label" }, background: { id: "opt-1-bg", themeConstructor: PDSWidgetBackgroundRadialThemeTrait }, head: { id: "opt-1-head", themeConstructor: PDSWidgetHeadRadialThemeTrait, font: "bold 25px sans-serif", selectorScale: 0.25 } },
                { id: "opt-2", label: { text: "opt-2", id: "opt-2-label" }, background: { id: "opt-2-bg", themeConstructor: PDSWidgetBackgroundRadialThemeTrait }, head: { id: "opt-2-head", themeConstructor: PDSWidgetHeadRadialThemeTrait, font: "bold 25px sans-serif", selectorScale: 0.25 } },
                { id: "opt-3", label: { text: "opt-3", id: "opt-3-label" }, background: { id: "opt-3-bg", themeConstructor: PDSWidgetBackgroundRadialThemeTrait, startAngle: PI + PI / 4, endAngle: PI - PI / 4 },         head: { id: "opt-3-head", themeConstructor: PDSWidgetHeadRadialThemeTrait, font: "bold 25px sans-serif", startAngle: PI + PI / 4, endAngle: PI - PI / 4, selectorScale: 0.25 } },
                { id: "opt-4", label: { text: "opt-4", id: "opt-4-label" }, background: { id: "opt-4-bg", themeConstructor: PDSWidgetBackgroundRadialThemeTrait, startAngle: PI_1_2 + PI / 2, endAngle: PI_1_2 - PI / 2 }, head: { id: "opt-4-head", themeConstructor: PDSWidgetHeadRadialThemeTrait, font: "bold 25px sans-serif", startAngle: PI_1_2 + PI / 2, endAngle: PI_1_2 - PI / 2, zeroAt: PDSWidgetHead.zeroAt.CENTER, selectorScale: 0.25   } },
            ],
            widgetDefaults: {
                /*
                 * Until relative units avl. padding control (indirectly) triangleEdge so increase it will left more space
                 * for pointer. But until rel. units avl visual effect will-be different on different device.
                 */
                background: {
                    padding: 10,
                    font: "normal 10px arial"
                },
                head: {
                    padding: 10,
                    changeEventPolicy:  PDSWidgetHead.changeEventPolicy.ON_CHANGE,
                    zeroAt:             PDSWidgetHead.zeroAt.END,
                    drawValue:          true,
                }
            }
        });
    }

    LimiterControlWidget[_protectedDescriptor].changeWidget = (t, widget, d, that) => {
        that.control.activate(widget, d.value, that); //due to high intensity, this one will provide view directly
    }

    LimiterControlWidget[_protectedDescriptor].activateWidget = (t, widget, d, that) => {
        HapticResponse.defaultInstance.vibrate(100, "widget activate");
    }

    LimiterControlWidget.prototype.setupWidget = function(dom, widget) {
        CanvasControlWidget.prototype.setupWidget.call(this, dom, widget);
        const ref = this.viewAt(dom);
        widget.attachEvent("change",    LimiterControlWidget[_protectedDescriptor].changeWidget,   ref);
        widget.attachEvent("activate",  LimiterControlWidget[_protectedDescriptor].activateWidget, ref);
        this.resync();
    }

    LimiterControlWidget.prototype.disableTransition = function(disable) {
        if (!disable) {
            this.resync();
        }
        CanvasControlWidget.prototype.disableTransition.call(this, disable);
    }

    LimiterControlWidget.prototype.reset = function(context = Control.defaultContext) {

    }

    LimiterControlWidget.prototype.buildWidgetToAxisPair = function() {
        const confSample = this.widgetConf();
        let result = [];
        for (const widget of confSample.widgets) {
            const axis = this.widgetToAxis(widget);
            result.push({ id: widget.id, axis });
        }
        result.sort((a, b) => {
            return a.axis - b.axis;
        });
        return result;
    }

    LimiterControlWidget.prototype.widgetToAxis = function(widget, value) {
        return +this.attributes.get("kbd-" + widget.id);
    }

    LimiterControlWidget.prototype.normalizeWidgetScale = function(widget, value) {
        if (widget.head.config.zeroAt === PDSWidgetHead.zeroAt.CENTER) {
            return (value + 1.0) / 2;
        }
        return value;
    }

    LimiterControlWidget.prototype.fromWidgetScale = function(widget, value) {
        return ~~(this.normalizeWidgetScale(widget, value) * 2048);
    }

    LimiterControlWidget.prototype.toWidgetScale = function(widget, value) {
        if (widget.head.config.zeroAt === PDSWidgetHead.zeroAt.CENTER) {
            return (value - 1024) / 1024;
        } else {
            return value / 2048;
        }
    }

    LimiterControlWidget.prototype.activate = function(widget, value, view = this) {
        const normalized = this.normalizeWidgetScale(widget, value);
        const axisValue = this.fromWidgetScale(widget, value);
        const kbd = +view.attributes.get(`kbd-${widget.id}`);
        try {
            const pattern = (1 - normalized) * 175 + 25;
            HapticResponse.defaultInstance.vibrate([50, pattern, 50, pattern, 50, pattern, 50, pattern], "joystick axis");
            joystick.setAxis(kbd, axisValue);
            joystick.localData.axisOwners[kbd] = widget; //quick and dirty, save last axis emiter to prevent positive loop
            //todo revert value
        } catch (e) {}
    }

    function SalvageLimiterControlWidget(domTemplate, WidgetImpl = PDSWidgetsPad) {
        LimiterControlWidget.call(this, domTemplate, WidgetImpl);
    }

    Object.setPrototypeOf(SalvageLimiterControlWidget.prototype, LimiterControlWidget.prototype);

    SalvageLimiterControlWidget.prototype.initAttributes = function(dom, attributesDefaults = {}) {
        let ctx = this;
        return CanvasControlWidget.prototype.initAttributes.call(this, dom, attributesDefaults).merge(new Attributes({
            "kbd-salvage-space": {
                label:          "axis(salvage spacing)",
                description:    "axis that used to set salvage beam spacing",
                type:           Attribute.types.AXIS,
                inheritable:    false, //due to limitation of sync
                value: () =>   +this.getValue("salvage-space", this.getValue()),
            },
        }), ["kbd"]);
    }

    SalvageLimiterControlWidget.prototype.widgetConf = function(dom) {
        return  Object.assign(CanvasControlWidget.prototype.widgetConf.call(this, dom),{
            id: "salvageLimiter",
            layout: {
                type: WidgetLayout.types.COLS,
                regions: [ { flex: 1, }, ]
            },
            widgets: [
                { id: "salvage-space",  label: { text: "spacing",  id: "salvage-space-label" }, background: { id: "salvage-space-bg", themeConstructor: PDSWidgetBackgroundLineralThemeTrait }, head: { id: "salvage-space-head",   themeConstructor: PDSWidgetHeadLineralThemeTrait,  fillStyle: "rgb(135, 205, 250)", selectorFillStyle: "white", fontFillStyle: "white", fontFillStyleHigh: "black", fontFillStyleLow: "white", selectorScale: 0.15 } },
            ],
            widgetDefaults: {
                /*
                 * Until relative units avl. padding control (indirectly) triangleEdge so increase it will left more space
                 * for pointer. But until rel. units avl visual effect will-be different on different device.
                 */
                background: {
                    padding: 10,
                    font: "normal 10px arial",
                    themeConstructor: PDSWidgetBackgroundRadialThemeTrait
                },
                head: {
                    themeConstructor: PDSWidgetHeadRadialThemeTrait,
                    font: "bold 25px sans-serif",
                    selectorScale: 0.25,
                    padding: 10,
                    changeEventPolicy:  PDSWidgetHead.changeEventPolicy.ON_CHANGE,
                    zeroAt:             PDSWidgetHead.zeroAt.END,
                    drawValue:          true,
                }
            }
        });
    }

    window.DISABLE_SETTINGS_FETCH   = false;
    window.ENABLE_PAGE_CHECK        = false;
    window.DISABLE_OVF_CHECK        = false;
    window.REPORT_PIXEL_RATE        = false;
    window.DISABLE_OVERLAY_WORKERS  = false;

    //dummyControl is workaround for PageLayout that allow it (PageLayout) work consistently when non custom view deleted
    //it allow to keep same gridArea order, dummyControl must replace any non custom view that need to be removed
    //it must be drop after PageLayout refactor
    function dummyControl(el) {
        const block = document.createElement("div");
        block.classList.add("control", "dummy");
        block.style.gridArea        = el.style.gridArea;
        block.style.display         = "none"
        block.style.pointerEvents   = "none";
        block.setAttribute("data-control-id", el.getAttribute("data-control-id"));
        return block;
    }

    function idle(callback, options = undefined, failbackTimeout = 40) {
        if (window.requestIdleCallback) {
            const reqId = requestIdleCallback(callback, options);
            return () => cancelIdleCallback(reqId);
        } else {
            const reqId = setTimeout(callback, failbackTimeout);
            return () => clearTimeout(reqId);
        }
    }

    function identity() {
        let settingsStore = storage.rubric("settings");
        if (settingsStore.get("clientId", false) === false) {
            if (window.crypto.subtle) {
                window.crypto.subtle.digest("SHA-256", (new TextEncoder()).encode(window.navigator.userAgent)).then(sha => {
                    const hashArray = Array.from(new Uint8Array(sha)); // convert buffer to byte array
                    return hashArray.map((b) => b.toString(16).padStart(2, "0")).join(""); // convert bytes to hex string
                })
            } else {
                return simpleHash(window.navigator.userAgent);
            }
        } else {
            return settingsStore.get("clientId", false);
        }
    }

    function findNsEntry(msg, ns){
        const nsIndex = msg.indexOf(`${ns}:`);
        if (nsIndex === -1) {
            return "";
        }
        const nsSizeStartIndex = msg.indexOf(":", nsIndex);
        if (nsSizeStartIndex === -1) {
            return "";
        }
        const nsSizeEndIndex = msg.indexOf(":", nsSizeStartIndex+1);
        if (nsSizeEndIndex === -1) {
            return "";
        }
        const size = parseInt(msg.substring(nsSizeStartIndex+1, nsSizeEndIndex));
        if (isNaN(size)) {
            return "";
        }
        return msg.substring(nsSizeEndIndex+1, nsSizeEndIndex+1+size);
    }


    function simpleHash(str) { //0-10 a-z A-Z
        let hash = 0
        for (let i = 0; i < str.length; i++) {
            hash = ((hash << 5) - hash + str.charCodeAt(i)) | 0
        }
        return (hash >>> 0).toString(36)
    }

    function validatePageArea(pageId = "current") {
        const page = pageId === "current" ? currentPage : Page.registry.get(pageId);
        const codes = Array.from(page.dom.querySelectorAll(".control:not(.dummy)")).map((e) => e.style.gridArea ? e.style.gridArea.split(" / ")[0] : "" ).sort();
        const removed = Array.from(page.dom.querySelectorAll(".dummy")).map((e) => e.style.gridArea ? e.style.gridArea.split(" / ")[0] : "").sort();

           /* .map((e) => (e.charCodeAt(0) - 97) * 26 + (e.charCodeAt(1) - 97));*/

        if ((new Set(removed)).size !== removed.length) {
            console.error("validatePageArea remove index invalid", removed);
            return { success: false, error: "remove index invalid"};
        }

        if ((new Set(codes)).size !== codes.length) {
            console.error("validatePageArea codes invalid", codes);
            return { success: false, error: "codes invalid"};
        }

        if (codes.indexOf("") !== -1 || codes.indexOf(" ") !== -1) {
            console.error("validatePageArea codes contain empty area", codes);
            return { success: false, error: "codes contain empty area"};
        }

        const area = page.layout.areas;

        for (const code of codes) {
            if (area.indexOf(code) === -1) {
                console.error("validatePageArea codes contain unmapped area", code, codes, area);
                return { success: false, error: "codes contain unmapped area"};
            }
        }

        console.info("validatePageArea page", page.id, "is valid");
        return { success: true, error: ""};
    }

    window.validatePageArea = validatePageArea;

    function extractRoute(routeEntry, index = 0, screen = []) {
        const page = Page.registry.get(routeEntry.getAttribute("data-page-id"));

        if (!page) {
            console.warn("route lead to invalid pageId", routeEntry.getAttribute("data-page-id"));
            throw new Error("route lead to invalid pageId");
        }

        const abs = routeEntry.getAttribute("data-abs") || "";
        const data = {
            page,
            abs,
            router,
            index,
            screen,
            label: routeEntry.getAttribute("data-label") || "",
        }
        screen.push(data);

        return data;
    }

    function buildScreen(screen, page = null) {
        //console.info("build new screen", screen);

        horPager.clear();

        swiper.clearItems();

        for (const routeParams of screen) {
            let page = routeParams.page;
            if (!page) {
                console.error("no page found");
            }
            page.dom.classList.add("controls", "swiper-page");
            page.dom.setAttribute("data-page-abs", routeParams.abs);
            page.dom.setAttribute("data-page-index", routeParams.index);
            swiper.addItem(page.dom);
            horPager.addPage("#"+routeParams.abs);
            page.suspend = true; //start it suspended to prevent costly animation if have
        }

        if (screen.length === 1) {
            console.log("disable swipe for single page screen");
            swipeLocker.lock("single_page");
            document.querySelector(".viewport .header .btn.lock").classList.add("disabled");
        } else {
            swipeLocker.unlock("single_page");
            document.querySelector(".viewport .header .btn.lock").classList.remove("disabled");
        }

        if (!DISABLE_OVF_CHECK) {
            if (page) {
                reportOverflowOnce(page.dom.querySelectorAll(".control"), true).then(() => {
                    return reportOverflowOnce(document.querySelectorAll(`.page:not(#${page.id}) > .control`));
                }).catch(console.error);
            } else {
                reportOverflowOnce(document.querySelectorAll(".control")).catch(console.error);
            }
        }

        //requestDocumentFlushTime(() => screen.forEach((rp) => prefetchOverlay(rp.page)));
    }


    function reportHover() {
        if (window.matchMedia) {
            const hover  = window.matchMedia('(hover: hover)').matches;
            const notHover  = window.matchMedia('(hover: none)').matches;
            if (hover && !notHover) {
                notificator.addNotification("device is capable of hover", "hover", "info");
            } else if (!hover && notHover) {
                notificator.addNotification("device is not capable of hover", "hover", "info");
            } else {
                notificator.addNotification("hover, conflicting result", "hover", "info");
            }
        }
    }

    function reportDeviceScreen() {
        if (window.matchMedia) {
            notificator.removeNotification("screen");
            const mediaQueryWidth  = window.matchMedia('(max-width:  24rem)').matches;
            const mediaQueryHeight = window.matchMedia('(max-height: 12rem)').matches;
            if (mediaQueryWidth || mediaQueryHeight) {
                notificator.addNotification("Device screen too small, try different orientation or app will work incorrectly", "screen", "warning", 30000);
            } else {
                notificator.removeNotification("screen");
            }
        }
    }

    reportOverflowOnce.request = PendingRequest();
    reportOverflowOnce.invert  = (window.innerWidth / window.innerHeight) <= invertAspectRatio;
    function reportOverflowOnce(controls, sync = false, invert = reportOverflowOnce.invert) {
        cancelRequest(reportOverflowOnce.request);
        return reportOverflow(
            controls,
            sync ? reportOverflow.syncRequest : reportOverflow.asyncRequest,
            invert,
            reportOverflowOnce.request = PendingRequest()
        );
    }


    reportOverflow.asyncRequest = {
        readScheduler:  requestDocumentFlushTime,
        writeScheduler: requestAnimationFrameTime,
        tail: {
            readScheduler:  requestDocumentFlushTime,
            writeScheduler: requestAnimationFrameTime,
        }
    };
    reportOverflow.syncRequest  = {
        readScheduler:  requestDocumentFlushTime,
        writeScheduler: requestAnimationFrameTime,
        tail: {
            readScheduler:  requestSync,
            writeScheduler: requestSync,
        }
    }

    function reportOverflow(
        controls,
        { readScheduler, writeScheduler, tail = reportOverflow.asyncRequest.tail } = reportOverflow.asyncRequest,
        invert = false,
        request = PendingRequest()
    ) {
        return new Promise((resolve, reject) => {
            if (request.canceled) {
                return reject();
            }
            request = readScheduler(() => {
                if (request.canceled) {
                    return reject();
                }

                let ovfX   = [];
                let ovfY   = [];
                let reset  = [];
                controls.forEach((el) => {
                    if (el.classList.contains("overflow-x") || el.classList.contains("overflow-y")) {
                        reset.push(el);
                        return;
                    }
                    if (el.scrollWidth > el.clientWidth) {
                        ovfX.push(el);
                    }
                    if (el.scrollHeight > el.clientHeight) {
                        ovfY.push(el);
                    }
                });
                if (ovfX.length || ovfY.length || reset.length) {
                    request = writeScheduler(() => {
                        if (request.canceled) {
                            return reject();
                        }
                        reset.map(el => el.classList.remove("overflow-x", "overflow-y"));
                        ovfX.map(el => el.classList.add(invert ? "overflow-y" : "overflow-x"));
                        ovfY.map(el => el.classList.add(invert ? "overflow-x" : "overflow-y"));
                        if (reset.length) {
                            return resolve(reportOverflow(reset, tail, invert, request));
                        } else {
                            return resolve();
                        }
                    }, request);
                } else {
                    resolve();
                }
            }, request);
        });
    }

    let ww = window.innerWidth, wh = window.innerHeight, aspect = ww / wh, deviceScreenRequest = PendingRequest();
    //setTimeout( () =>  notificator.addNotification(`initial: ${ww}/${wh}`, "initial-size", "info"), 0);
    window.addEventListener("resize", () => {
        //notificator.addNotification(`resize: ${window.innerWidth}/${window.innerHeight}`, "resize-size", "info");
        const newWW = window.innerWidth;
        const newWH = window.innerHeight;
        const newAspect = newWW / newWH;
        const prevAspect = aspect;
        //cation floating UI also trigger resize with NEW size!
        cancelRequest(deviceScreenRequest); deviceScreenRequest = requestDocumentFlushTime(() => {
            if (!deviceScreenRequest.canceled) {
                if (Math.sign(prevAspect - 1.0) !== Math.sign(newAspect - 1.0)) {
                    reportDeviceScreen();
                }
            }
        });
        if (!DISABLE_OVF_CHECK) {
            const invert = newAspect <= invertAspectRatio;
            if (currentPage) {
                reportOverflowOnce(currentPage.dom.querySelectorAll(".control"), true, reportOverflowOnce.invert = invert).then(() => {
                    return reportOverflowOnce(document.querySelectorAll(`.page:not(#${currentPage.id}) > .control`));
                }).catch(console.error);
            } else {
                reportOverflowOnce(document.querySelectorAll(".control"), false, reportOverflowOnce.invert = invert).catch(console.error);
            }
        }
        ww     = newWW;
        wh     = newWH;
        aspect = newAspect;
    });

    function transferPage(page, oldPage) {
        if (page === oldPage) {
            return;
        }

        if (page && page.suspend) {
            page.suspend = false;
        }

        if (oldPage && !oldPage.suspend) {
            idle(() => {
                if (oldPage !== currentPage && !oldPage.suspend) {
                    oldPage.suspend = true;
                }
            }, {timeout: 250});
        }

        return page;
    }
    function transferScreen(screen, oldScreen) {
        //console.log("transfer control to new screen", screen);

        const lockedInEditMode = locker.has("editmode");

        for (const routeParams of oldScreen) {
            let page = routeParams.page;
            if (page) {
                page.active = false;
                if (lockedInEditMode) {
                    editModePageTransition(false, page);
                }
            }
        }

        for (const routeParams of screen) {
            let page = routeParams.page;
            if (page) {
                page.active = true;
                if (lockedInEditMode) {
                    editModePageTransition(true, page);
                }
            }
        }

        return screen;

    }

    let currentBreadParts = [];
    function buildBreadV1(path) {
        performance.mark("bread1Start")
        bread.clear();
        let collector   = "";
        let prevInfo      = null;
        const pathParts        = router.path.split("/");
        for (let i = 0; i < pathParts.length; i++) {
            if (collector.endsWith("/")) {
                collector += pathParts[i];
            } else {
                collector += "/" + pathParts[i];
            }
            const leaf = i === pathParts.length - 1;
            let info = router.info(collector, true);
            if (info) {
                if (prevInfo && prevInfo.page === info.page) {
                    continue;
                }
                bread.push("#" + collector, info.label, leaf);
                prevInfo = info;
            } else {
                bread.push("#" + collector,"undefined", leaf);
                prevInfo = null;
            }
        }
        currentBreadParts = pathParts;
        performance.measure("bread1", "bread1Start")
    }


    function buildBreadV2() {
        //todo refactor it simplify it
        performance.mark("bread2Start");
        const   path        = (""+router.path).trim();
        let     collector   = "";
        let     prevInfo      = null;
        const   pathParts  = path === "/" ? [""] : path.split("/"); //there "" because "/" must be single path not multiple and "" result "/" later in processing
        for (let i = 0; i < pathParts.length; i++) {
            const leaf      = i === pathParts.length - 1;
            const leafPrev  = i === currentBreadParts.length - 1;
            collector += i === 0 ? pathParts[i] : ("/" + pathParts[i]);
            //console.log("collector",collector);
            if (pathParts[i] === currentBreadParts[i] && leaf === leafPrev) {
                prevInfo = router.info(collector, true) || null;
                //console.log("skip render of ", i);
                continue;
            }
            let info = router.info(collector, true);
            if (info) {
/*              if (prevInfo && prevInfo.page === info.page) { continue; not sure why }*/
                if (info.is404) {
                    bread.update(i,"#" + collector, info.label, true);
                    prevInfo = info;
                    break;
                }
                bread.update(i,"#" + collector, info.label, leaf);
                prevInfo = info;
            } else {
                bread.update(i,"#" + collector,"undefined", leaf);
                prevInfo = null;
            }
        }
        currentBreadParts = pathParts;
        performance.measure("bread2", "bread2Start")
    }


    let errorSeq = 0;
    window.onerror = (a, b, c, d, e) => {
        try {
            notificator.addNotification(`error: ${a}\n, ${b}: ${c}, ${d}, ${e}`, "error-rep-" + errorSeq++, "error");
        } catch (e) {}

        //return true;
    };

    function selectControl(elm) {
        switch (elm.getAttribute("data-control-type") || "") {
            case "switch":
                return SwitchControl;
            case "oneshot":
                return OneshotControl;
            case "link":
                return LinkControl;
            case "repeat":
                return RepeatControl;
            case "remote-repeat":
                return RemoteRepeatControl;
            case "direct":
                return DirectControl;
            case "memo":
                return MemoControl;
            case "back":
                return BackControl;
            case "shields":
                return ShieldControlWidget;
            case "pds":
                return PDSControlWidget;
            case "limiter":
                return LimiterControlWidget;
            case "salvage-limiter":
                return SalvageLimiterControlWidget;
            default:
                return InvalidControl;
        }
    }

    function selectWindow(elm) {
        switch (elm.getAttribute("data-window-id") || "") {
            case "layout-configuration":
                return LayoutConfigurationWindow;
            case "settings":
                return SettingsWindow;
            case "control-configuration":
                return ControlConfigurationWindow;
            case "control-selection":
                return ControlSelectionWindow;
            default:
                if (elm.classList.contains("modal")) {
                    return WindowModal;
                } else {
                    return Window;
                }
        }
    }

    function resetPage(id = "current", { clearLayout = true, clearCustom = true, clearView = false } = {}) {

        id = id === "current" ? currentPage.id : id;

        let pageStore = storage.rubric("page", id);
        let pageViewStore =  pageStore.rubric("views");
        if (clearCustom) {
            let customRecord = storage.rubric("custom");
            for (const [viewId, customCtrlRecord] of customRecord) {
                if (customCtrlRecord.pageId === id) {
                    customRecord.remove(viewId);
                    if (!clearView) {
                        pageViewStore.rubric(viewId).clear();
                    }
                }
            }
        }

        if (clearLayout) {
            pageStore.remove("layout");
            if (!clearView) {
                for (const [attr, value] of pageViewStore) {
                    if (attr.endsWith(".gridArea")) {
                        pageViewStore.remove(attr);
                    }
                }
            }
        }

        if (clearCustom && clearView && clearLayout) {
            pageStore.clear();
        } else if (clearView) {
            pageViewStore.clear();
        }
    }

    window.resetPage = resetPage;

    function resetCtrl(id) {
        let page = storage.rubric("controls", id);
        page.clear();
    }

    window.resetCtrl = resetCtrl;


    function unlock() {
        locker.unlock("no_connection");
    }

    window.unlock = unlock;

    generateUniqueViewId.storage = [];
    function generateUniqueViewId(ctrlId, pageId) {
        let candidate;
        do {
            candidate = Page.genViewId(ctrlId, pageId, simpleHash(new Date().toString() + Math.random().toString()));
        } while (generateUniqueViewId.storage.indexOf(candidate) !== -1);
        generateUniqueViewId.storage.push(candidate);
        return candidate;
    }

    function viewRubric(view, storage) {
        return storage.rubric(view.data.page.id, 'views');
    }

    function saveToStore(ctrlOrView, storage, attributesToSave) {

        let removal = false;
        let custom  = false;
        let attributes = ctrlOrView.attributes;

        storage = storage.rubric(ctrlOrView.id);

        if (ctrlOrView instanceof ControlView) {
            removal = attributes.get("removed");
            custom  = attributes.get("custom");
        }

        if (removal) {
            storage.clear();
            if (!custom) { //original widget must keep removed flag forever (or it will respawn), custom (user created) just purge itself completely
                storage.set("removed", true);
            }
            return true;
        }

        let force = false;
        if (attributesToSave) {
            attributesToSave = attributesToSave.map((e) => {
                return [e, attributes.descriptor(e)];
            });
            force = true;
        } else {
            attributesToSave = attributes;
        }

        for (const [name, desc] of attributesToSave) {
            if (!desc) {
                throw new Error("invalid attributesToSave name");
            }
            if (force || desc.writeable) { //allow to save only meanful field
                try {
                    if (
                        (ctrlOrView instanceof Control      && !attributes.isDefaulted(name))   ||
                        (ctrlOrView instanceof ControlView  &&  attributes.hasOverloaded(name))
                    ) {
                        //in case of Control save any non-default
                        //in case of ControlView save field than not the same at it's parent (overloaded) (hasOwn() && .value !== parent.value)
                        storage.set(name, attributes.get(name));
                    } else if (storage.has(name)) { //or remove at once if exist but not met requirements
                        storage.remove(name);
                    }
                } catch (e) {
                    console.warn("saveToStore error", ctrlOrView.id, name, e);
                }
            }
        }

        return true;

    }

    function loadFromStore(ctrlOrView, storage) {

        let attributes = ctrlOrView.attributes;

        storage = storage.rubric(ctrlOrView.id);

        let totalSuccess = true;
        if (storage.has("removed") && storage.get("removed")) {
            try {
                if (!attributes.set("removed", true)) {
                    console.warn("loadFromStore error", ctrlOrView.id, name, new Error("unable set field"));
                    totalSuccess = false;
                }
            } catch (e) {
                console.warn("loadFromStore error", ctrlOrView.id, name, e);
            }
            return totalSuccess;
        }


        for (const [name, value] of storage) {
            const desc = attributes.descriptor(name);
            if (desc && desc.writeable) {
                try {
                    if (!attributes.set(name, value)) {
                        console.warn("loadFromStore error", ctrlOrView.id, name, new Error("unable set field"));
                        totalSuccess = false;
                    }
                } catch (e) {
                    console.warn("loadFromStore error", ctrlOrView.id, name, e);
                }
            }
        }

        return totalSuccess;

    }

    let overlayEl, overlay;

    const overlayFeeder = (page) => {
        const style = getComputedStyle(page.dom);
        return [ page.dom.querySelectorAll(".control:not(.dummy)"),
            { pageId: page.id, cluster: { probe: { radius: { collision: ~~Math.ceil(
                                //style.gridRowGap for old browser
                                //columnGap may produce x2 value
                                Math.max(parseFloat(style.gridRowGap || style.rowGap), parseFloat(style.gridColumnGap || style.columnGap)) / 2
                            )
                        }
                    }
                }
            }
        ]
    }

    let enableOverlayFnResizeHandler = null;
    function enableOverlay(enable = true) {
        const vp        = document.querySelector(".viewport");
        const swipeEl   = document.querySelector(".swiper");
        if (enable) {
            if (overlay) {
                console.warn("overlay already exist");
                return;
            }
            console.info("create overlay");
            window.debugPoint ? window.debugPoint = console.log : null;

            overlayEl = document.createElement("canvas");
            overlayEl.setAttribute("id", "overlay");
            overlayEl.setAttribute("width",  vp.offsetWidth);
            overlayEl.setAttribute("height", swipeEl.offsetHeight);
            overlayEl.classList.add("underlay");

            showOverlay(false);

            swiper.underlay(overlayEl);

            overlay = Overlay(overlayEl, { groups: Array.from(Group.registry).reduce((acc, group) => {
                    if (group.logic) {
                        acc.push({ id: group.id, color: group.data ? group.data.color : undefined });
                    }
                    return acc;
                }, []), fill: "rgb(16, 50, 70)" }
            );

            let request = PendingRequest();
            window.addEventListener("resize", enableOverlayFnResizeHandler = () => {
                cancelRequest(request);
                showOverlay(false);
                request = requestTimeout(() => {
                    if (request.canceled) {
                        return;
                    }
                    request = requestDocumentFlushTime(() => {
                        if (request.canceled) {
                            return;
                        }
                        const w = vp.offsetWidth; //swipeEl after init have invalid size (vp * itemCount)
                        const h = swipeEl.offsetHeight;
                        request = requestAnimationFrameTime(() => {
                            if (request.canceled) {
                                return;
                            }
                            overlayEl.setAttribute("width",  w);
                            overlayEl.setAttribute("height", h);
                            invalidateOverlay();
                            updateOverlay();
                        }, request);
                    }, request);
                }, 50);
                //currentScreen.map((cfg) => prefetchOverlay(cfg.page));
            });

            vp.classList.add("overlayed");

            window.overlay.obj  = overlay;
            window.updateOverlay = updateOverlay;
        } else {
            if (overlay) {
                vp.classList.remove("overlayed");
                console.info("overlay removed");
                window.removeEventListener("resize", enableOverlayFnResizeHandler);
                overlay.clear();
                overlay.free();
                swiper.underlay(null);
                overlayEl.remove();
                overlay = enableOverlayFnResizeHandler = window.overlay.obj = null;
            }
        }
    }

    function showOverlay(show = true) {
        if (overlayEl) {
            if (show) {
                overlayEl.style.opacity = "1.0";
                overlayEl.style.visibility = "visible"; //safety to prevent overlayEl shown before it ready
                                                        //as swipe also control opacity of overlayEl
            } else {
                overlayEl.style.opacity = "0.0";
                overlayEl.style.visibility = "hidden";
            }
        }
    }

    function invalidateOverlay(page = "all") {
        console.log("invalidate", page);
        if (overlay) {
            if (page === "all") {
                overlay.invalidateAll();
            } else {
                overlay.invalidate(page, overlayFeeder);
            }
        }
    }

    function prefetchOverlay(page) {
        if (overlay) {
            overlay.prefetch(page, overlayFeeder);
        }
    }

    function cancelOverlay() {
        if (overlay) {
            overlay.cancel();
        }
    }

    function updateOverlay() {
        if (overlay) {
            showOverlay(false);
            overlay.clear();
            if (locker.has("editmode")) {
                return;
            }
            let info = router.info(router.path);
            let d = new Date();
            const path = router.path;
            //notificator.addNotification("overlay start " + path, "info", "overlay-" + path);
            if (info && info.page) {
                requestDocumentFlushTime(() => {
                    overlay.select(info.page, overlayFeeder).then(() => {
                        requestAnimationFrame(() => {
                            console.info("select draw", info.page.id);
                            overlay.draw();
                            showOverlay(true);
                            //notificator.addNotification("overlay end " + path + " " + ((new Date()) - d), "info", "overlay-" + router.path);
                        });
                    });
                });
            }
        }
    }

    window.overlay      = () => {
        if (!overlay) {
            enableOverlay(true);
        }
        showOverlay(parseFloat(overlayEl.style.opacity) !== 1.0);
    }

    let currentPage;

    let horPager = Pager(document.querySelector(".pager.horizontal"));
    let verPager = Pager(document.querySelector(".pager.vertical"));

    let swiper   = Swiper(document.querySelector('.swiper'));
    window.swiper     = swiper;

    swiper.afterStart((i) => requestAnimationFrame(horPager.notify) );

    let transitionStart = 0;
    swiper.afterTransitionStart((i, pageChanged = false) => {
        requestAnimationFrame(() => horPager.page(i));
        performance.mark("transitionStart");
        transitionStart = performance.now();
        //console.log("transitionStart");
        if (pageChanged) {
            //console.log("transitionStart", "page change", i);
            cancelOverlay();
        }
    });

    let routerUpdate  = PendingRequest();
    swiper.afterEnd((i, pageChanged = false) => {
        if (!pageChanged) {
            return;
        }
        performance.measure("transition", "transitionStart");
        const m = performance.now() - transitionStart;
        //setTimeout(() => notificator.addNotification("transition: " + Number.parseFloat(m).toFixed(2), "transition", "info", 1000), 2000);
        if (currentScreen[i] && currentScreen[i].page) {
            const path   = currentScreen[i].abs;
            cancelRequest(routerUpdate); routerUpdate = requestIdleTime(() => {
                router.end();
                router.overridePath(window.location.hash = path);
                router.begin();
                requestAnimationFrame(buildBreadV2);
                updateOverlay();
                if (ENABLE_PAGE_CHECK) {
                    let status = validatePageArea();
                    if (!status.success) {
                        notificator.addNotification(`Page ${currentPage.id} faild validation ` + status.error, `${currentPage.id}-validation`, 'error', 5000);
                    }
                }
            }, { timeout: 100 });
            currentPage = transferPage(currentScreen[i].page, currentPage);
        } else {
            //error, do not touch hash
            console.error("swiped page has no page index");
        }
    });

    let bread = new Breadcrumbs(document.querySelector(".header .breadcrumb"));

    let ctrlLocker = UiLocker("ctrlLocker", () => Control.disable(), () => Control.enable());
    ctrlLocker.lock("no_connection");
    window.locker = ctrlLocker;

    let swipeLocker = UiLocker("swipeLocker", () => swiper.end(), () => swiper.begin());


    let notificator = Notifications(window.document.querySelector(".notifications"));
    window.notificator = notificator;

    let generalConfiguration = Storage();
    let storage = Storage(versions.storage.toString());
    const oldVersion = generalConfiguration.get("storageVersion", "");
    if (oldVersion !== versions.storage) {
        if (oldVersion) {
            let legacyStorage = Storage(oldVersion);
            if (confirm(`version of storage is changed ${oldVersion} != ${versions.storage}, transfer storage?`)) {
                for (const [key, value] of legacyStorage) {
                    //todo migration
                    storage.set(key, value);
                }
            }
        } //or this is app init time
        generalConfiguration.set("storageVersion", versions.storage);
    }
    window.storage = storage;

    let socket = Socket("/socks");
    window.socket = socket;

    const identityV = identity();
    socket.identity(() => identityV);
    socket.open();

    notificator.addNotification("Attempting connect...", "connection_info", "info", 0);
    reportDeviceScreen();
    if (REPORT_PIXEL_RATE) {
        notificator.addNotification("device pixel ratio is: " + window.devicePixelRatio, "pixels", "info");
    }
    if (DISABLE_OVERLAY_WORKERS) {
        notificator.addNotification("WebWorkers was disabled", "worker", "warning", 0);
    }
    if (DISABLE_SETTINGS_FETCH) {
        notificator.addNotification("Settings fetch was disabled", "fetch", "warning", 0);
    }
    if (ENABLE_PAGE_CHECK) {
        notificator.addNotification("Page validator enabled", "page-validator", "warning", 0);
    }

    socket.ondisconnect = () => {
        notificator.removeNotification("connection_info");
        notificator.addNotification("Connection Lost", "connection_error", "error", 0);
        ctrlLocker.lock("no_connection");
    }
    let connectCount = 0;
    socket.onauthorize = () => {
        notificator.removeNotification("connection_info");
        notificator.removeNotification("connection_error");
        notificator.addNotification("Connection Established", "connection_success");
        if (++connectCount > 1) {
            //if this is any reconnect attempt:
            //we don't know how much server change since last connect, so controls must be reset in order to replicate state from server
            //this is because certain type of states don't store on server even if they declared by client, like:
            //[click, activate, activate-%, switch-off] so client must be reset before replicate.
            //this behavior may change
            resetClientState();
        }
        ctrlLocker.unlock("no_connection");
    }

    socket.ns("nt", (taskId, msg) => {
        if (msg && msg.length) {
            notificator.addNotification(msg, "notification-" + taskId.toString(), "info");
        } else {
            console.warn("rcv malformed msg[note] from server", taskId, msg);
        }
    });

    socket.ns("storageSign", (taskId, serverPersistenceSign) => {
        const clientPersistenceSign = storage.get("storageSign", "");
        if (!clientPersistenceSign) {
            storage.set("storageSign", serverPersistenceSign);
        } else if (serverPersistenceSign !== clientPersistenceSign) {
            if (confirm("server state indicate factory reset, reset client local storage?")) {
                storage.clear();
                storage.set("storageSign", serverPersistenceSign);
                window.location.reload();
            } else {
                storage.set("storageSign", serverPersistenceSign);
            }
        }
    });

    function resetClientState() {
        Control.reset();
        joystick.reset(); //this will reset any axis related controls
    }

    window.resetClientState = resetClientState;

    let bootingSign;
    socket.ns("bootingSign", (taskId, serverBootingSign) => {
        if (!bootingSign) {
            bootingSign = serverBootingSign;
        } else if (serverBootingSign !== bootingSign) {
            console.log("server state indicate rebooting, time to negotiate server state", "not implemented");
            //if server rebooted while client has any state, we in situation where server have no state but client does
            //and probably different clients have different state, what lead to negotiation stage based probably
            //on last serverPacketId.
            //but in real world, reconnect time not determined and server probably will use state from first client
            //reconnected or fresh new.
            //currently used fresh new state, and clients must follow.
            notificator.addNotification("Server unexpectedly rebooted", "server_error", "error");
            bootingSign = serverBootingSign;
        }
    });

    let leds = {
        "numlock": {
            el: document.querySelector(".toolbar .btn.numlock"),
            status: "switched-off"
        },
        "capslock": {
            el: document.querySelector(".toolbar .btn.capslock"),
            status: "switched-off"
        },
/*        "scrolllock": {
            el: document.querySelector(".toolbar .btn.scrolllock"),
            status: "switched-off"
        }*/
    }
    function applyKeyboardLedStatus() {
        //do not use applyKeyboardLedStatus.arguments as it can be mixed with idleDeadline
        for (const [id, led] of Object.entries(leds)) {
            if (led.status === "switched-on" && !led.el.classList.contains("pressed")) {
                led.el.classList.add("pressed");
            } else if (led.status === "switched-off" && led.el.classList.contains("pressed")) {
                led.el.classList.remove("pressed");
            }
        }

    }

    window.leds = leds;
    window.applyKeyboardLedStatus = applyKeyboardLedStatus;

    socket.ns("kba", (taskId, msg) => {
        if (msg && msg.length) {
            console.info("kba", taskId, msg);
            const [id, command] = msg.split(":");
            const ctrl = Control.registry.get(id);
            if (ctrl && command) {
                ctrl.applyState(command);
            }
            if (id === "numlock" || id === "capslock" /*|| id === "scrolllock"*/) {
                leds[id].status = command;
                idle(applyKeyboardLedStatus, { timeout: 25 });
            }
        } else {
            console.warn("rcv malformed msg[kba] from server", taskId, msg);
        }
    });

    socket.ns("ctr", (taskId, msg) => {
        if (msg) {
            console.info("ctr", taskId, msg, msg.size);
            joystick.fromBuffer(msg.slice(-20));
        } else {
            console.warn("rcv malformed msg[ctr] from server", taskId, msg);
        }
    });

    window.addEventListener("unload", (e) => {
        let type, name;

        if (window.PerformanceObserver) {
            [{ type: type, name : name}, ...rest] = e.currentTarget.performance.getEntriesByType("navigation");
        } else if (window.PerformanceNavigation)  {
            type = function (type){
                switch (type) {
                    case window.PerformanceNavigation.TYPE_NAVIGATE:
                        return "navigate";
                    //todo other
                    default:
                        return "reload";
                }
            }(e.currentTarget.performance.navigation.type);
            name = "document";
        } else {
            console.log("undefined leave type");
            socket.close();
            return;
        }

        console.log("unload", name, type);

        if (/*name === "document" &&*/ type === "navigate") {
            console.log("page is moveout");
            socket.close();
        } else {
            console.log("page is reloading");
        }
    });

    document.querySelectorAll(".group-decl .group").forEach((elm, i ) => {
        try {
            let group = Group.declare({
                id:         elm.getAttribute("data-group-id")        || "",
                logic:      !!elm.querySelector(".attribute[data-attribute-id=logic]"),
                control:    !!elm.querySelector(".attribute[data-attribute-id=control]"),
                info:       !!elm.querySelector(".attribute[data-attribute-id=info]"),
                data:       Object.fromEntries(Array.prototype.map.call(elm.querySelectorAll(".attribute[data-attribute-id^=\"data.\"]"), (el) => {
                    return [el.getAttribute("data-attribute-id").replace("data.", ""), el.innerText];
                })),
            });
            elm.remove();
        } catch (e) {
            console.warn(e, elm.getAttribute("data-group-id"));
        }
    });


    let controlsStore = storage.rubric("controls");
    document.querySelectorAll(".controls-decl .control").forEach((elm, i ) => {
        try {
            let ctrl = Control.declare({
                id:         elm.getAttribute("data-control-id")      || "",
                type:       elm.getAttribute("data-control-type")    || "",
                groups:     elm.getAttribute("data-control-groups")  || "",
            }, selectControl(elm), elm);
            elm.remove();
            if (ctrl) {
                loadFromStore(ctrl, controlsStore);
            }
        } catch (e) {
            console.warn(e);
        }
    });

    Control.attachEvent("fire", (ctrl, kbd) => {
        console.log("send", kbd);
        if (socket.available()) {
            if (ctrl instanceof RemoteRepeatControl) {
                socket.sendConf("repeat", kbd).catch((e)=>{
                    ctrl.applyState("switched-off");
                    notificator.addNotification("Server reject task for ctrl " + ctrl.id, ""+ctrl.id, 'error', 5000);
                    console.warn(e);
                });
            } else {
                socket.send("kb", kbd);
            }
        } else {
            console.warn("socket unavailable unable to send", kbd, socket.reconnectReason, socket.error);
        }
    });

    let pagesStorage = storage.rubric("page");
    let customStore = storage.rubric("custom");
    Page.hash = simpleHash;
    document.querySelectorAll(".pages-decl .page").forEach((elm, i ) => {
        try {
            let page = Page.declare({
                id:         elm.getAttribute("data-page-id")      || "",
            }, Page, elm);

            elm.remove();

            let pageId = page.id;
            let pageSpecificStorage = pagesStorage.rubric(pageId);

            const staticFingerprint = pageSpecificStorage.get("fingerprint", "");
            if (staticFingerprint !== "" && staticFingerprint !== page.staticFingerprint) {
                if (
                    pageSpecificStorage.has("layout")       || //has any custom layout
                    Object.keys(Object.fromEntries(pageSpecificStorage.rubric("views"))).find((attr) => attr.endsWith(".gridArea")) || //has any gridArea
                    Object.values(Object.fromEntries(customStore)).find((view) => view.pageId === pageId) //has any custom elements
                ) {
                    if (!confirm(
                        `Page "build in" controls for page "${pageId}" was changed, saved layout may apply incorrectly. Confirm if you want to continue use saved layout or reject to use defaults.`
                    )) {
                        //there is no real reason to flush clearCustom but initial idea of customCtrl does not include view without gridArea
                        //so to not take any chances, when clearLayout (that remove gridArea) also clearCustom.
                        resetPage(pageId, { clearLayout: true, clearCustom: true, clearView: false });
                    }
                }
            }

            pageSpecificStorage.set("fingerprint", page.staticFingerprint);

            let pageLayout = pageSpecificStorage.get("layout", null);
            if (pageLayout) {
                for (const [key, value] of Object.entries(pageLayout)) {
                    //console.info(page.layout.hasConfig(key), key);
                    if (page.layout.hasConfig(key)) {
                        page.layout[key] = value;
                    }
                }
                if (page.layout.isDefault && !pageLayout.manual) {
                    //this page in default state remove cfg
                    console.warn("remove old layout from store", pageId)
                    pageSpecificStorage.remove("layout");
                }
            }


        } catch (e) {
            console.warn(e);
        }
    });

    for (const ctrl of Control.registry) {
        for (const view of ctrl) {
            const prevEl = view.dom.previousElementSibling;
            const parentEl = view.dom.parentElement;
            loadFromStore(view, viewRubric(view, pagesStorage));
            if (view.attributes.get("removed")) { //todo event
                view.data.page.removeControl(view);
                if (!view.attributes.get("custom")) {
                    if (prevEl) {
                        prevEl.after(dummyControl(view.dom)); //see dummyControl explanation
                    } else {
                        parentEl.insertBefore(dummyControl(view.dom), parentEl.children[0]);
                    }
                }
            }
        }
    }

    for (const [viewId, customCtrlRecord] of customStore) {
        generateUniqueViewId.storage.push(viewId);
        const page = Page.registry.get(customCtrlRecord.pageId);
        if (page) {
            const bind = page.addControl(customCtrlRecord.ctrlId, { viewId: viewId });
            if (bind) {
                if (!loadFromStore(bind.ref, viewRubric(bind.ref, pagesStorage))) {
                    console.error("unable load attributes for", viewId)
                }
                if (bind.ref.attributes.get("removed")) {
                    console.error("custom element keep persistent after be removed");
                    if (!bind.ref.attributes.get("custom")) {
                        console.error("custom element marked as not custom", "force fixed");
                        bind.ref.attributes.set("custom", true);
                    }
                    customStore.remove(viewId);                                 //blame
                    saveToStore(bind.ref, viewRubric(bind.ref, pagesStorage));  //purge store
                    page.removeControl(bind.ref);                               //unlink from page
                }
            }
        }
    }

    let router = new Router();
    window.router = router;


    document.querySelectorAll("body .window").forEach((elm, i ) => {
        try {
            Window.declare({
                id:         elm.getAttribute("data-window-id")      || "",
            }, selectWindow(elm), elm);
            elm.remove();
        } catch (e) {
            console.warn(e);
        }
    });

    let currentScreen = [];
    function resolve(event, params) {
        const page = params.page;
        if (page) {
            if (currentScreen.length === 1 || currentScreen.indexOf(params) === -1) {
                //different screen or single page
                //blur();
                showOverlay(false);
                buildScreen(params.screen, page);
                swiper.setIndex(params.index);
                horPager.page(params.index);
                buildBreadV2();
                currentScreen   = transferScreen(params.screen, currentScreen);
                currentPage     = transferPage(page, currentPage); //resume animation
                updateOverlay();
                if (ENABLE_PAGE_CHECK) {
                    let status = validatePageArea();
                    if (!status.success) {
                        notificator.addNotification(`Page ${currentPage.id} faild validation ` + status.error, `${currentPage.id}-validation`, 'error', 5000);
                    }
                }
            } else {
                //same screen
                swiper.swipeToIndex(params.index);
            }

        } else {
            console.error("resolved page does not exist", event, params);
        }
    }

    for (const routeItem of Array.from(document.querySelector(".router-decl > .routes").children)) {
        if (routeItem.classList.contains("screen")) {
            let index = 0;
            let screen = [];
            for (const routeItemOfScreen of Array.from(routeItem.children)) {
                try {
                    const params = extractRoute(routeItemOfScreen, index++, screen);
                    router.addStaticRoute(params.abs, resolve, params);
                    if (routeItemOfScreen.hasAttribute("data-default-route")) {
                        router.addStaticRoute("", resolve, params);
                        router.addStaticRoute("/", resolve, params);
                    }
                    routeItemOfScreen.remove();
                } catch (e) {
                    console.warn(e);
                }
            }
            if (!routeItem.children.length) {
                routeItem.remove();
            }
        } else if (routeItem.classList.contains("route")) {
            try {
                const params = extractRoute(routeItem);
                router.addStaticRoute(params.abs, resolve, params);
                if (routeItem.hasAttribute("data-default-route")) {
                    router.addStaticRoute("", resolve, params);
                    router.addStaticRoute("/", resolve, params);
                }
                routeItem.remove();
            } catch (e) {
                console.warn(e);
            }
        } else {
            console.warn("invalid type of route", routeItem);
        }
    }

    let route404 = document.querySelector(".router-decl > .route-404");
    if (route404) {
        try {
            let params = extractRoute(route404);
            params.is404 = true;
            router.addStaticNotFoundRoute(resolve, params);
            route404.remove();
        } catch (e) {
            console.warn(e);
        }
    }


    let ml = ModalLocker(document.body);

    ml.beforeLock(() => {
        horPager.hide();
    });

    ml.beforeUnlock(() => {
        horPager.show();
    });

    let tbmPrivateCtx = {
        configurate: {},
        editmode: {},
    };

    let editModeRequest = PendingRequest();
    const editModePageTransition = (mode, page) => {

        if (!mode) {

            let items = page.dom.querySelectorAll(".layout-highlight");
            let candidate = items[0];

            if (candidate) {

                items.forEach((el) => { el.classList.add("removed"); });


                animationFinished(candidate, { deadline: 5000 }).then((reason) => {
                    console.log("animationFinished", reason);
                    items.forEach((el) => { el.remove(); });
                }).catch(console.error);

            }

            page.dom.classList.remove("editmode");

            return;
        }

        let areas = page.layout.areas || "";

        areas = areas.replace(/"/g, " ").replace(/\s+/g, " ")
            .split(" ").filter((value,index, self) => {
                return value !== "." && value !== "" && self.indexOf(value) === index;
            }).sort();

        page.dom.classList.add("editmode");

        let fragment = new DocumentFragment();
        areas.map((area) => {
            let el = document.createElement("div");
            el.classList.add("layout-highlight");
            el.style.gridArea = area;
            el.innerText = area;
            fragment.appendChild(el);
        });

        page.dom.appendChild(fragment);
    }

    const extractLayoutCfg = (storage) =>  {
        return {
            col:        storage.columnCnt       ? storage.columnCnt     : 5,
            row:        storage.rowCnt          ? storage.rowCnt        : 5,
            gap:        storage.cellGap         ? storage.cellGap       : 1,
            pad:        storage.incellPadding   ? storage.incellPadding : 0.7,
            fontSize:   storage.fontSize        ? storage.fontSize      : "120%",
            layout:     storage.areas           ? storage.areas         : "",
        }
    }

    const toolbarMenu = {
        layout: () => {

            let page        = router.info(router.path).page;
            const pageId    = page.id;
            let wnd = Window.registry.get("layout-configuration");

            let pageSpecificStorage = null
            if (pageId) {
                pageSpecificStorage = pagesStorage.rubric(pageId);
            }

            return wnd.show(document.body, Object.assign({
                    elementCount: 10,
                    defaults: extractLayoutCfg(page.layout.defaults),
                }, extractLayoutCfg(page.layout)
            )).result().then((result) => {
                if (!result.submit || !result.valid) {
                    console.log("ignore result", result);
                    return;
                } else {
                    console.log("valid result", result.data.col, result.data.row, result.data.gap);
                }

                let layout = {
                    columnCnt:      result.data.col,
                    rowCnt:         result.data.row,
                    cellGap:        result.data.gap,
                    incellPadding:  result.data.pad,
                    fontSize:       result.data.fontSize,
                    areas:          result.data.layout
                }

                if (!page.layout.equalConfig(layout)) {
                    for (const [key, value] of Object.entries(layout)) {
                        if (page.layout.hasConfig(key)) {
                            //console.log("set lyout", key, value)
                            page.layout[key] = value;
                        }
                    }
                    layout.manual      = true;
                    if (pageSpecificStorage) {
                        pageSpecificStorage.set("layout", layout);
                    }

                    showOverlay(false);
                    if (!DISABLE_OVF_CHECK) {
                        reportOverflowOnce(page.dom.querySelectorAll(".control"), true).then(() => {
                            invalidateOverlay(page);
                            updateOverlay();
                        });
                    }


                } else {
                    console.info("layout, ignore equal config");
                }

                return Promise.resolve("success");
            }).catch((e)=> {
                console.error(e);
                return Promise.reject("fail");
            });
        },
        lock: (mode) => {

            if (mode) {
                document.querySelector(".viewport").classList.add("lock-mode");
                swipeLocker.lock("user_choice");
            } else {
                document.querySelector(".viewport").classList.remove("lock-mode");
                swipeLocker.unlock("user_choice");
            }

        },
        configurate: (mode) => {

            if (!mode) {
                console.info("disable conf");
                if (tbmPrivateCtx.configurate.externalEvtHandler) {
                    tbmPrivateCtx.configurate.externalEvtHandler.free();
                }
                document.querySelector(".viewport").classList.remove("configurate-mode");
                ctrlLocker.unlock("configuration");
            } else {
                ctrlLocker.lock("configuration");

                if (locker.has("editmode")) {
                    toolbarMenu.editmode(false);
                }

                document.querySelector(".viewport").classList.add("configurate-mode");

                (tbmPrivateCtx.configurate.externalEvtHandler = touchEventHelper(document, {
                    decimator:  ({ target }) => !(target && (target.classList.contains("control") || target.closest(".control"))),
                })).ontap = (e) => {

                    let target = e.target;
                    if (!target.classList.contains("control")) {
                        target = target.closest(".control");
                    }

                    const ctrl = Control.registry.get(target.getAttribute("data-control-id"));
                    if (!ctrl) {
                        console.warn("invalid ctrl object", target, e);
                        return;
                    }
                    const view = ctrl.viewAt(target);
                    let wnd = Window.registry.get("control-configuration");
                    wnd.show(document.body, {
                        ctrl,
                        view,
                        mode: "all",
                        groups: {
                            items: Array.from(Group.registry).reduce((acc, group) => acc.push({ id: group.id, color: group.data ? group.data.color : undefined }) && acc, []),
                            defaultColors: Overlay.colors,
                        },
                    }).result().then((result) => {
                        if (!result.submit || !result.valid) {
                            console.log("ignore result", result);
                        } else {
                            console.log("valid result", result);

                            const vCtrlResult = ctrl.attributes.batch(result.data.ctrl);
                            const vViewResult = view.attributes.batch(result.data.view);

                            if (!vCtrlResult.success || !vViewResult.success) {
                                notificator.addNotification("unable update attributes", "configuration-error", "error");
                                console.error(vCtrlResult);
                            }

                            saveToStore(view, viewRubric(view, pagesStorage));
                            saveToStore(ctrl, controlsStore); //caution saveToStore(ctrl, controlsStore) must be exec even if only view is saving
                                                              //because of how non inheritable attrs work.

                            if (!DISABLE_OVF_CHECK) {
                                reportOverflow([target], true).then(() => {
                                    invalidateOverlay(currentPage);
                                    updateOverlay();
                                });
                            }
                        }
                    }).catch(console.warn);
                };
            }
        },
        settings: () => {

            let wnd = Window.registry.get("settings");

            let settingsStore = storage.rubric("settings");

            let task;
            if (DISABLE_SETTINGS_FETCH) {
                task = Promise.resolve("SSID:9:debugMode:AUTH:1:6:AUTHDATA:73:WPA3 PSK:6:WPA2 PSK:3:WPA2 WPA3 PSK:7:WPA WPA2 PSK:4:Other(Keep Same):100");
            } else {
                if (!socket.available()) {
                    notificator.addNotification("unable load setting, not connection", "settings", "error", Notifications.defaultLivetime * 2);
                    return false;
                }
                task = socket.sendConf("settings-get", "", timeouts.settings);
            }

            task.then((msg)=> {
                console.log(msg);
                const ssid      = findNsEntry(msg, "SSID");
                const auth      = findNsEntry(msg, "AUTH");
                const authData  = findNsEntry(msg, "AUTHDATA");
                if (!ssid.length || !auth.length || !authData.length) {
                    notificator.addNotification("unable load setting, invalid format", "settings", "error", Notifications.defaultLivetime * 2);
                    return false;
                }
                let authDataTokens = [];
                let startIndex  = 0;
                let partIndex   = 0;
                let endIndex    = 0;
                while ((partIndex = authData.indexOf(":", startIndex)) !== -1) {
                    if ((endIndex = authData.indexOf(":", partIndex + 1)) === -1) {
                        if (partIndex + 1 >= authData.length) {
                            notificator.addNotification("unable load setting, invalid format", "settings", "error", Notifications.defaultLivetime * 2);
                            return false;
                        }
                        authDataTokens.push({value: authData.substring(partIndex+1), label: authData.substring(startIndex, partIndex)});
                        break;
                    } else {
                        authDataTokens.push({value: authData.substring(partIndex+1, endIndex), label: authData.substring(startIndex, partIndex)});
                    }
                    startIndex = endIndex + 1;
                }
                wnd.show(document.body, {
                    mode:       auth,
                    ssid:       ssid,
                    authData:   authDataTokens,
                    password1:  "",
                    password2:  "",
                    clientId:   settingsStore.get("clientId", ""),
                    hapticResp: settingsStore.get("hapticResp", HapticResponse.defaultInstance.available),
                    overlay:    settingsStore.get("overlay",    true),
                }).result().then((result) => {
                    if (!result.submit || !result.valid) {
                        console.log("ignore result", result);
                    } else {
                        console.log("valid result", result);
                        let clientIdUpdated = false;
                        let mustUpdateOverlay= false; //updateOverlay() may be expensive(but async) it even can lead
                                                               //to mess up with deadline so wait until request finish
                        if (result.data.clientId && result.data.clientId !== settingsStore.get("clientId","")) {
                            settingsStore.set("clientId", result.data.clientId);
                            notificator.addNotification("ClientId updated", "clientId", "info");
                            clientIdUpdated = true;
                        }
                        if (result.data.hapticResp !== undefined && result.data.hapticResp !== settingsStore.get("hapticResp","")) {
                            settingsStore.set("hapticResp", result.data.hapticResp);
                            HapticResponse.defaultInstance.enabled = result.data.hapticResp;
                        }
                        if (result.data.overlay !== undefined && result.data.overlay !== settingsStore.get("overlay",true)) {
                            settingsStore.set("overlay", result.data.overlay);
                            enableOverlay(mustUpdateOverlay = !!result.data.overlay);
                        }
                        if (!socket.available()) {
                            if (mustUpdateOverlay) {
                                updateOverlay(); //@see mustUpdateOverlay;
                            }
                            notificator.addNotification("Unable save settings, no connection", "settings", "error", Notifications.defaultLivetime * 2);
                            return Promise.reject("no connection");
                        } else {
                            let msg = `SSID:${result.data.ssid.length}:${result.data.ssid}:AUTH:${result.data.mode.length}:${result.data.mode}`;
                            if (result.data.password1 && result.data.password2 && result.data.password1 !== "") {
                                msg += `:PASSWORD:${result.data.password1.length}:${result.data.password1}`;
                            }
                            return socket.sendConf("settings-set", msg, timeouts.settings).catch((reason)=> {
                                if (mustUpdateOverlay) {
                                    updateOverlay(); //@see mustUpdateOverlay;
                                }
                                if (reason === "timeout") {
                                    notificator.addNotification("Unable save settings, timeout error", "settings", "error", Notifications.defaultLivetime * 2);
                                } else {
                                    notificator.addNotification("Unable save settings, server reject query", "settings", "error", Notifications.defaultLivetime * 2);
                                }
                                throw new Error(reason);
                            }).then((a) => {
                                if (mustUpdateOverlay) {
                                    updateOverlay(); //@see mustUpdateOverlay;
                                }
                                if (clientIdUpdated) {
                                    return socket.closeAsync().then(()=> {
                                        const identityV = identity();
                                        socket.identity(() => identityV);
                                        return socket.openAsync();
                                    });
                                } else {
                                    return a;
                                }
                            });
                        }
                    }
                }).catch((e)=> console.error(e));
            }).catch((reason)=> {
                if (reason === "timeout") {
                    notificator.addNotification("unable receive setting in time, timeout error", "settings", "error", Notifications.defaultLivetime * 2);
                } else {
                    notificator.addNotification("unable receive setting, server error", "settings", "error", Notifications.defaultLivetime * 2);
                }
            });
        },
        numlock: () => {
            let ctrl = Control.registry.get("numlock");
            if (ctrl) {
                ctrl.click();
            } else {
                console.error("no such control \"numlock\"");
            }
            return Promise.reject("this must be rejected");
        },
        capslock: () => {
            let ctrl = Control.registry.get("capslock");
            if (ctrl) {
                ctrl.click();
            } else {
                console.error("no such control \"capslock\"");
            }
            return Promise.reject("this must be rejected");
        },
        scrolllock: () => {
            let ctrl = Control.registry.get("scrolllock");
            if (ctrl) {
                ctrl.click();
            } else {
                console.error("no such control \"scrolllock\"");
            }
            return Promise.reject("this must be rejected");
        },
        editmode: (mode) => {

            const pageInfo = router.info(router.path);
            const page      = pageInfo.page;
            const screen    = pageInfo.screen;
            const indexOf   = screen.indexOf(pageInfo);

            if (mode) {
                locker.lock("editmode");

                if (locker.has("configuration")) {
                    toolbarMenu.configurate(false);
                }

                let selected;

                const longTap = (e) => {

                    console.info("longtap");

                    let zone;
                    let candidate;
                    let zoneArea;

                    if (e.target.classList.contains("control")) {
                        //allow to terminate orphaned view, view in layout will be protected by its .layout-highlight
                        selected = undefined;
                        try {
                            candidate = Control.registry.get(e.target.getAttribute("data-control-id")).viewAt(e.target);
                        } catch (e) {
                            console.error(e);
                            return;
                        }
                    } else {
                        //default mode
                        zone = e.target;

                        if (zone && !zone.classList.contains("locked")) {
                            zoneArea = zone.style.gridArea;
                        } else {
                            return;
                        }

                        for (const ref of page) {
                            if (ref.gridArea === zoneArea) {
                                candidate = ref;
                                break;
                            }
                        }
                    }

                    if (candidate) {

                        if (selected && selected.zone && selected.zone.classList.contains("locked")) {
                            return;
                        }

                        const ctrl = candidate.control;
                        const area = candidate.gridArea;
                        const sel  = selected;
                        let animations = [];

                        console.log("sel", sel);

                        selected = undefined;

                        candidate.dom.classList.add("removed");
                        if (zone) {
                            zone.classList.add("locked");
                        }

                        animations.push(animationFinished(candidate.dom, { deadline: 1000 }));
                        if (sel) {
                            if (sel.zone) {
                                sel.zone.classList.add("locked");
                                sel.zone.classList.remove("selected");
                            }
                            sel.view.dom.classList.add("removed");
                            animations.push(animationFinished(sel.view.dom, { deadline: 1000 }));
                        }

                        HapticResponse.defaultInstance.vibrate(400);

                        Promise.all(animations).then((reason) => {
                            console.log("animationFinished", reason);
                            const custom = candidate.attributes.get("custom");
                            if (!custom) {
                                candidate.dom.replaceWith(dummyControl(candidate.dom)); //see replaceWithDummyControl explanation
                            }
                            if (candidate.attributes.set("removed", true)) {
                                saveToStore(candidate, viewRubric(candidate, pagesStorage));
                                if (custom) {
                                    if (storage.rubric("custom").has(candidate.id)) {
                                        storage.rubric("custom").remove(candidate.id);
                                    } else {
                                        console.error("custom storage has not element", candidate.id);
                                    }
                                }
                                page.removeControl(candidate);
                            } else {
                                console.error("unable to set attribute", "removed");
                            }
                            if (sel) {
                                if (sel.view.attributes.set("gridArea", area)) {
                                    saveToStore(sel.view, viewRubric(sel.view, pagesStorage), ["gridArea"]);
                                } else {
                                    console.error("unable to set attribute", "gridArea");
                                }
                            }

                            if (sel) {
                                if (sel.zone) {
                                    sel.zone.classList.remove("locked");
                                }
                                sel.view.dom.classList.remove("removed");
                            }

                            if (zone) {
                                zone.classList.remove("locked");
                            }
                            candidate.dom.classList.remove("removed");

                        });

                    } else if (zone) {
                        let wnd = Window.registry.get("control-selection");
                        if (wnd) {
                            zone.classList.add("locked");
                            wnd.show(document.body, {
                                groups: {
                                    items: Array.from(Group.registry).reduce((acc, group) => acc.push({ id: group.id, color: group.data ? group.data.color : undefined }) && acc, []),
                                    defaultColors: Overlay.colors,
                                },
                            }).result().then((result) => {
                                if (result.submit && result.valid) {
                                    const ctrl = result.data.ctrl;
                                    const current = router.info(router.path);
                                    if (current.page) {

                                        const pageId = page.id;
                                        const ctrlId = ctrl.id;
                                        const viewId = generateUniqueViewId(ctrlId, pageId);

                                        let customCtrlStorage = storage.rubric("custom");

                                        tbmPrivateCtx.editmode.newCtrlAppended = true;

                                        //stretch needed to abtein all avl size
                                        const bind = current.page.addControl(ctrl.id, { viewId, classList: ["removed", "stretch"] });

                                        customCtrlStorage.set(viewId, { pageId, ctrlId } );

                                        bind.ref.gridArea = ""; //temporaly fix lacking of default value
                                        bind.ref.attributes.set("custom",   true);
                                        bind.ref.attributes.set("gridArea", zoneArea);

                                        if (!saveToStore(bind.ref, viewRubric(bind.ref, pagesStorage), ["custom", "gridArea"])) {
                                            console.error("unable save custom control", bind.ref);
                                        }

                                        HapticResponse.defaultInstance.vibrate(400);

                                        requestAnimationFrame(() => {
                                            requestAnimationFrame(() => {
                                                bind.node.classList.remove("removed", "stretch");
                                                zone.classList.remove("locked");
                                                if (!DISABLE_OVF_CHECK) {
                                                    reportOverflowOnce([bind.node]); //??animationFinished?
                                                }
                                            });
                                        });

                                    }
                                } else {
                                    zone.classList.remove("locked");
                                }
                            });
                        }
                    }
                }


                let inc = 0;
                const tap = (e) => {

                    console.info("tap", e);

                    let zone;
                    let candidate;
                    let zoneArea;

                    if (e.target.classList.contains("control")) {
                        //allow to move orphaned view, view in layout will be protected by its .layout-highlight
                        selected = undefined;
                        try {
                            candidate = Control.registry.get(e.target.getAttribute("data-control-id")).viewAt(e.target);
                        } catch (e) {
                            console.error(e);
                            return;
                        }
                    } else {

                        zone = e.target;

                        if (zone && !zone.classList.contains("locked")) {
                            zoneArea = zone.style.gridArea;
                        } else {
                            return;
                        }

                        for (const ref of page) {
                            if (ref.gridArea === zoneArea) {
                                candidate = ref;
                                break;
                            }
                        }
                    }

                    if (candidate && selected) {

                        if (selected && selected.zone && selected.zone.classList.contains("locked")) {
                            return;
                        }

                        if (selected.view === candidate) {
                            selected.zone.classList.remove("selected");
                            selected = candidate = undefined;
                            return;
                        }

                        const selectedArea  = candidate.gridArea;
                        const candidateArea = selected.view.gridArea;
                        const sel = selected;

                        selected = undefined;

                        if (zone) {
                            zone.classList.add("locked");
                        }
                        if (sel.zone) {
                            sel.zone.classList.add("locked");
                            sel.zone.classList.remove("selected");
                        }
                        candidate.dom.classList.add("removed");
                        sel.view.dom.classList.add("removed");

                        HapticResponse.defaultInstance.vibrate(200);

                        Promise.all([
                            animationFinished(candidate.dom, { deadline: 1000 } ),
                            animationFinished(sel.view.dom, { deadline: 1000 } )
                        ]).then((reason) => {
                            console.log("animationFinished", reason);
                            if (candidate.attributes.set("gridArea", candidateArea)) {
                                saveToStore(candidate, viewRubric(candidate, pagesStorage), ["gridArea"]);
                            } else {
                                console.error("unable to set attribute", "gridArea");
                            }
                            if (sel.view.attributes.set("gridArea", selectedArea)) {
                                saveToStore(sel.view, viewRubric(sel.view, pagesStorage), ["gridArea"]);
                            } else {
                                console.error("unable to set attribute", "gridArea");
                            }

                            candidate.dom.classList.remove("removed");
                            sel.view.dom.classList.remove("removed");
                            if (zone) {
                                zone.classList.remove("locked");
                            }
                            if (sel.zone) {
                                sel.zone.classList.remove("locked");
                            }
                        });

                    } else if (selected) {
                        if (selected && selected.zone && selected.zone.classList.contains("locked")) {
                            return;
                        }

                        const sel = selected;
                        selected = undefined;

                        if (zone) {
                            zone.classList.add("locked");
                        }
                        if (sel.zone) {
                            sel.zone.classList.add("locked");
                            sel.zone.classList.remove("selected");
                        }

                        sel.view.dom.classList.add("removed");

                        HapticResponse.defaultInstance.vibrate(200);

                        animationFinished(sel.view.dom, { deadline: 1000 }).then((reason) => {
                            console.log("animationFinished", reason);
                            if (sel.view.attributes.set("gridArea", zoneArea)) {
                                saveToStore(sel.view,   viewRubric(sel.view, pagesStorage), ["gridArea"]);
                            } else {
                                console.error("unable to set attribute", "gridArea");
                            }

                            sel.view.dom.classList.remove("removed");
                            if (zone) {
                                zone.classList.remove("locked");
                            }
                            if (sel.zone) {
                                sel.zone.classList.remove("locked");
                            }
                        });

                    } else if (candidate && !selected) {
                        selected = { zone: zone, view: candidate };
                        if (selected.zone) {
                            selected.zone.classList.add("selected");
                        }
                    } else {
                        if (selected && selected.zone) {
                            selected.zone.classList.remove("selected");
                        }
                        selected = undefined;
                    }
                }

                //note ff52 can produce event in a weird way
                tbmPrivateCtx.editmode.externalEvtHandler = touchEventHelper(document, { decimator: (e) => {
                    if (!(e.target instanceof HTMLElement) || (!e.target.classList.contains("layout-highlight") && !e.target.classList.contains("control"))) {
                        return true;
                    }
                } });

                tbmPrivateCtx.editmode.externalEvtHandler.onlongtap = longTap;
                tbmPrivateCtx.editmode.externalEvtHandler.ontap     = tap;

            }


            cancelRequest(editModeRequest);

            editModePageTransition(mode, page); //first is synchronous

            let callback;
            let forwardIndex   = indexOf+1;
            let backwardIndex  = indexOf-1;
            let ongoingRequest = editModeRequest = requestAnimationFrameTime(callback = () => {
                if (ongoingRequest.canceled) {
                    return;
                }
                if (forwardIndex < screen.length) {
                    editModePageTransition(mode, screen[forwardIndex].page);
                    forwardIndex++;
                }
                if (backwardIndex >= 0) {
                    editModePageTransition(mode, screen[backwardIndex].page);
                    backwardIndex--;
                }
                if (forwardIndex < screen.length || backwardIndex >= 0) {
                    requestAnimationFrameTime(callback, ongoingRequest);
                }
            });

            if (!mode) {

                tbmPrivateCtx.editmode.externalEvtHandler.free();
                locker.unlock("editmode");

                let item = page.dom.querySelector(".control");
                if (item && (overlay || tbmPrivateCtx.editmode.newCtrlAppended)) {
                    showOverlay(false);
                    animationFinished(item, { deadline: 5000 }).then(() => {
                        /*
                         * new ctrl applyed with scale, make its internal size invalid
                         * most easy way to fix it, trigger resize to recalculate size
                         * but only after scale remove
                         */
                        if (tbmPrivateCtx.editmode.newCtrlAppended) {
                            window.dispatchEvent(new Event('resize'));
                            tbmPrivateCtx.editmode.newCtrlAppen = undefined;
                        }
                        showOverlay(false);
                        if (!DISABLE_OVF_CHECK) {
                            //reportOverflowOnce is called in sync mode to try to prevent flickering
                            reportOverflowOnce(page.dom.querySelectorAll(".control"), true).then(() => {
                                invalidateOverlay(page);
                                updateOverlay();
                            });
                        }
                    });
                }


            }

        },
        showhide: (mode) => {
            const tb = document.querySelector(".viewport > .header ul.toolbar");
            if (mode) {
                tb.classList.add("forced");
            } else {
                tb.classList.remove("forced");
            }
        }
    }

    window.tb = toolbarMenu;

    document.querySelectorAll(".viewport > .header ul.toolbar a").forEach((elm, i ) => {
        const actionId = elm.getAttribute("data-action");
        if (Object.hasOwn(toolbarMenu, actionId)) {
            elm.addEventListener("click", (e) => {

                e.preventDefault();

                if (!elm.classList.contains("pressed") && elm.hasAttribute("data-group-id")) {
                    const groups = elm.getAttribute("data-group-id").trim().split(",");
                    for (const group of groups) {
                        document.querySelectorAll(`.viewport > .header ul.toolbar a.group-${group.trim()}`).forEach((otherElm) => {
                            if (elm === otherElm) {
                                return;
                            }
                            otherElm.classList.remove("pressed");
                        });
                    }
                }

                let result;
                if (elm.classList.contains("switch")) {
                    //mode switcher
                    const mode =  !elm.classList.contains("pressed");
                    elm.classList.toggle("pressed");
                    try {
                        result = toolbarMenu[actionId](mode);
                    } catch (e) {
                        console.warn("unable process action", actionId, e);
                        elm.classList.toggle("pressed");
                    }
                    if (result instanceof Promise) {
                        result.catch(()=>{
                            elm.classList.toggle("pressed");
                        });
                    }
                } else {
                    //task
                    try {
                        elm.classList.add("pressed");
                        result = toolbarMenu[actionId]();
                    } catch (e) {
                        console.warn("unable process action", actionId, e);
                    }
                    if (result instanceof Promise) {
                        result.then(()=> {
                            elm.classList.remove("pressed");
                        }).catch(()=> {
                            elm.classList.remove("pressed");
                        });
                    } else {
                        setTimeout(() => elm.classList.remove("pressed"), 250);
                    }
                }

                //elm.blur();
                return false;
            });
        } else {
            console.warn("undefined actionId", actionId);
        }
    });

    document.querySelector(".viewport > .header .title .settings-btn").addEventListener("click", (e) => {
        e.preventDefault();
        toolbarMenu.settings.apply(this, arguments);
        return false
    });

   //storage.rubric("settings").remove("hapticResp");

    HapticResponse.defaultInstance.enabled = storage.rubric("settings").get("hapticResp", HapticResponse.defaultInstance.available);
    enableOverlay(!!storage.rubric("settings").get("overlay", true));


    swiper.begin();
    router.begin(true);

    addEventListener("DOMContentLoaded", (event) => {
        if (document.querySelector(".debug-shield-widget")) {
            const w = new ShieldWidget(document.querySelector(".debug-shield-widget"));
        }
        if (document.querySelector(".debug-power-widget")) {
            const w = new PDSWidgetsPad(document.querySelector(".debug-power-widget"), { widgets: [
                    { id: "t1" },
                    { id: "t2" },
                    { id: "t3" },
                    { id: "t4" },
                    { id: "t5" },
                ], widgetPadding: 10 });

        }
    });

    Joystick.defaults = {
        refreshTime:    20, //50hz
        axisCount:      8,
        buttonsCount:   32,
    }

    /**
     * todo add joystickSign[4byte]: clientId[byte]joystickId[byte]sequenceId[byte,byte]
     * where sequence in server? or client? ns
     * @param socket
     * @param config
     * @constructor
     */
    function Joystick(socket, config = Joystick.defaults) {

        this.config = Object.assign({}, Joystick.defaults, config);

        this.state = new ArrayBuffer(this.config.axisCount * 2 + ~~((this.config.buttonsCount + 7) / 8));
        this.view  = new DataView(this.state);
        this.pendingOp  = undefined;
        this.socket     = socket;
        this.localData  = {
            axisOwners: [],
        };

        this.localData.axisOwners.length = 8;

        //console.info("Joystick data len", this.state.byteLength);

        for (let axis = 0; axis < this.config.axisCount; axis++) {
            this.view.setUint16(axis*2, 1024, true);
        }
        this.sendState = this.sendState.bind(this);

        if (!EventDispatcherTrait.isInited(this)) {
            EventDispatcherTrait.call(this);
        }
    }

    mixin(Joystick, EventDispatcherTrait);

    Joystick.prototype.setAxis = function(index, value) {
        if (index < 0 || index > 8) {
            throw new Error("bad axis index");
        }
        value = Math.max(Math.min(2048, value), 0);
        this.view.setUint16(index*2, value, true);
        if (!this.pendingOp) {
            this.pendingOp = setTimeout(this.sendState, this.config.refreshTime);
        }
    }

    Joystick.prototype.getAxis = function(index) {
        if (index < 0 || index > 8) {
            throw new Error("bad axis index");
        }
        return this.view.getUint16(index*2, true);
    }

    Joystick.prototype.fromString = function(string) {
        console.warn("Joystick.prototype.fromString", string);
        for (let i = 0; i < string.length && i < 20; i++) {
            this.view.setUint8(i, string.charCodeAt(i));
        }
        for (let axis = 0; axis < this.localData.axisOwners.length; axis++) {
            this.localData.axisOwners[axis] = null;
        }
        this.dispatchEvent("change", this, true); //remoteOne
    }

    Joystick.prototype.fromBuffer = function(buffer) {
        console.warn("Joystick.prototype.fromBuffer", buffer);
        this.state = buffer;
        this.view  = new DataView(this.state);
        for (let axis = 0; axis < this.localData.axisOwners.length; axis++) {
            this.localData.axisOwners[axis] = null;
        }
        this.dispatchEvent("change", this, true); //remoteOne
    }

    Joystick.prototype.sendState = function() {
        this.pendingOp = undefined; //keep in sync event if socket fail
        try {
            this.socket.sendBinary("ctr", this.state);
        } catch (e) {
            console.error(e);
        } finally {
            this.dispatchEvent("change", this, false); //local
        }
    }

    Joystick.prototype.reset = function() {
        this.state = new ArrayBuffer(this.config.axisCount * 2 + ~~((this.config.buttonsCount + 7) / 8));
        this.view  = new DataView(this.state);
        for (let axis = 0; axis < this.config.axisCount; axis++) {
            this.view.setUint16(axis*2, 1024, true);
        }
        this.dispatchEvent("change", this, false); //local
    }

    let joystick = new Joystick(socket);
    window.joystick = joystick;

/*
    Group.registry.get("target").logic          = false;
    Group.registry.get("turret").logic          = false;
    Group.registry.get("echo").logic            = false;
    Group.registry.get("missile-pack").logic    = false;
    Group.registry.get("memo").logic            = false;
    Group.registry.get("countermeasures").logic = false;
    Group.registry.get("salvage-cycle").logic   = false;
*/
    //Probe.default.radius.collision = 4

})();