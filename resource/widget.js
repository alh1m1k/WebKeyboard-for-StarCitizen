"use strict";

const PI_1_4            = Math.PI / 4;
const PI_1_2            = Math.PI / 2;
const PI_3_4            = PI_1_2 + PI_1_4;
const PI                = Math.PI;
const PI_1_1_4          = PI + PI_1_4;
const PI_1_1_2          = PI + PI_1_2;
const PI_1_3_4          = PI_1_1_2 + PI_1_4;
const PI_2              = PI * 2;


const quarterFingerPixelsForWidgets = ((2.75 - 2.25) / 2 + 2.25) / (Math.PI * 2) / 2 * ((window.devicePixelRatio || 1.0) * 96);

if (window.DISABLE_OVERLAY_WORKERS === undefined) {
    window.DISABLE_OVERLAY_WORKERS = false;
}

function Rect(...arg) {
    if (arg.length === 5) {
        this.x = arg[0];
        this.y = arg[1];
        this.w = arg[2];
        this.h = arg[3];
        this.id = arg[4];
    } else if (arg.length === 4) {
        this.x = arg[0];
        this.y = arg[1];
        this.w = arg[2];
        this.h = arg[3];
    } else if (arg.length === 1) {
        if (arg[0] instanceof Rect) {
            this.x = arg[0].x;
            this.y = arg[0].y;
            this.w = arg[0].w;
            this.h = arg[0].h;
            this.id = arg[0].id;
        } else if (arg[0] instanceof DOMRect) {
            this.x = arg[0].x;
            this.y = arg[0].y;
            this.w = arg[0].width;
            this.h = arg[0].height;
            this.id = undefined;
        } else {
            throw new Error("Rect invalid argument type", typeof arg[0]);
        }
    } else {
        throw new Error("Rect invalid argument count")
    }
}

function emptyFn() {};

function unify(e) { return e.changedTouches ? e.changedTouches[0] : e }

function assertIf(mustBeTrueToPass, message = "assert failing") { if (!mustBeTrueToPass) throw new Error(message); }

function boundingClientRect(dom) {
    const rect = dom.getBoundingClientRect();
    return Object.assign(Object.create(null), {
        x: rect.x, y: rect.y, width: rect.width, height: rect.height,
        left: rect.left, top: rect.top, bottom: rect.bottom, right: rect.right
    });
}

/*const requestDocumentFlushTime = (callback, ...args) => {
    requestAnimationFrame(() => {
        //requestAnimationFrame((() => {}).apply(undefined, args));
        setTimeout(callback, 0, ...args); //https://firefox-source-docs.mozilla.org/performance/bestpractices.html
    });
}*/


function PendingRequest() {
    return Object.assign(Object.create(null), { id: undefined, canceler: emptyFn, ctx: window, canceled: false, complete: false });
}

function cancelRequest(request) {
    //console.info("cancelRequest", request.id, request.canceler);
    request.canceler.call(request.ctx, request.id);
    request.canceler = emptyFn;
    request.canceled = true;
}

function requestDocumentFlushTime (callback, request = PendingRequest()) {
    return requestAnimationFrameTime(() => {
        //requestAnimationFrameTime(callback, request);
        requestImmediateTime(callback, request);
    }, request);
}

function requestAnimationFrameTime(callback, request = PendingRequest()) {
    request.id          = requestAnimationFrame(callback);
    request.canceler    = cancelAnimationFrame;
    request.ctx         = window;
    return request;
}

function requestImmediateTime(callback, request = PendingRequest()) {
    return requestTimeout(callback, 0, request);
}

function requestTimeout(callback, timeout, request = PendingRequest()) {
    request.id          = setTimeout(callback, timeout);
    request.canceler    = clearTimeout;
    request.ctx         = window;
    return request;
}

function requestSync(callback, request = PendingRequest()) {
    callback();
    return request;
}

function requestIdleTime(callback, options = undefined, failbackTimeout = 40, request = PendingRequest()) {
    if (window.requestIdleCallback) {
        request.id          = requestIdleCallback(callback, options);
        request.canceler    = cancelIdleCallback;
    } else {
        request.id          = setTimeout(callback, failbackTimeout);
        request.canceler    = clearTimeout;
    }
    request.ctx = window;
    return request;
}

//good for position, bad if scaling
function requestBoundingClientRect(dom, rect, request = PendingRequest()) {
    return requestDocumentFlushTime(() => {
        const domRect = dom.getBoundingClientRect();
        rect.x       = domRect.x;
        rect.y       = domRect.y;
        rect.width   = domRect.width;
        rect.height  = domRect.height;
        rect.left    = domRect.left;
        rect.top     = domRect.top;
        rect.bottom  = domRect.bottom;
        rect.right   = domRect.right;
    }, request);
}

//bad for position and empty size, good if scaling
function requestOffsetRect(dom, rect, request = PendingRequest()) {
    return requestDocumentFlushTime(() => {
        rect.x       = dom.offsetLeft;
        rect.y       = dom.offsetTop;
        rect.width   = dom.offsetWidth;
        rect.height  = dom.offsetHeight;
        rect.left    = rect.x;
        rect.top     = rect.y;
        rect.bottom  = undefined;
        rect.right   = undefined;
    }, request);
}

const boxStyleWidthModifier = (computedStyle) => {
    return parseFloat(computedStyle.paddingLeft) + parseFloat(computedStyle.paddingRight) + (parseFloat(computedStyle.borderLeft) || 0) + (parseFloat(computedStyle.borderRight) || 0);
}

const boxStyleHeightModifier = (computedStyle) => {
    return parseFloat(computedStyle.paddingTop) + parseFloat(computedStyle.paddingBottom) + (parseFloat(computedStyle.borderTop) || 0) + (parseFloat(computedStyle.borderBottom) || 0);
}



const identityMatrix = [
    1, 0, 0,
    1, 0, 0,
];


const protectedDecr = Symbol("protectedDecr");


function mixin(target, ...sources) {
    if (!sources.length) {
        return;
    }
    if (target[mixin.storage] === undefined) {
        target[mixin.storage] = new Set();
    }
    let mixTarget = target;
    if (target instanceof Function) {
        mixTarget = target.prototype;
    }
    for (let source of sources) {
        mixin.copyMissing(mixTarget, source instanceof Function ? source.prototype : source);
        //make shure that methods defined in mixTarget will override any methods in source
        //so mix working only for missing methods
        target[mixin.storage].add(source);
        if (source[mixin.storage] !== undefined) {
            source[mixin.storage].forEach(target[mixin.storage].add, target[mixin.storage]);
        }
    }

}

mixin.storage = Symbol("_mixin");

mixin.has = function(constructor, mixin) {
    if (constructor[mixin.storage] === undefined) {
        return false;
    } else {
        return constructor[mixin.storage].has(mixin);
    }
}

mixin.call = function(constructor, fn, that, ...args) {
    constructor.prototype[fn].call(that, ...args);
}

mixin.copyMissing = function(target, ...sources) {
    for (let source of sources) {
        for (let key of Object.keys(source)) {
            if (!(key in target)) {
                target[key] = source[key];
            }
        }
    }
    return target;
}

mixin.copy = function(target, ...sources) {
    return Object.assign(target, ...sources);
}


function OffscreenRendererTrait(width, height, rCtx = this.renderCtx) {
    if (!rCtx) {
        throw new Error("nor rCtx");
    }
    rCtx.offscreenCanvas        = document.createElement("canvas");
    rCtx.offscreenCanvas.width  = rCtx.offscreenCanvasWidth  = width;
    rCtx.offscreenCanvas.height = rCtx.offscreenCanvasHeight = height;
    rCtx.offscreenCtx           = rCtx.offscreenCanvas.getContext("2d");
    rCtx.offscreenInvalidate    = true;
}

const StaticRenderContextHelper = {
    wrapConstructor(Constructor, renderer) {
        return function() {
            const child = new Constructor(...arguments);
            renderer.attachChild(child);
            return renderer.renderProxy(child);
        }
    }
}

function StaticRenderContext(width, height, invalidateCb = emptyFn, childrens = []) {
    OffscreenRendererTrait.call(this, width, height, this.renderCtx = {});
    this.childrens              = [];
    this.timepass               = 0;
    this.partialUpdate          = true; //allow redraw child from internal buffer (wia ProxyChild.draw call) instead of redraw entire internal buffer
    this.invalidateProxy        = (eventId, context, eData, argument) => { argument.offscreenInvalidate = true; argument._invalidChild++; }; //in case of many to many relation with child
    this._invalidChild          = 0;
}

mixin(StaticRenderContext, OffscreenRendererTrait);

StaticRenderContext.prototype.attachChild = function(child) {
    this.childrens.push(child);
    child.attachEvent("invalidate", this.invalidateProxy, this);
    this._invalidChild++;
}

StaticRenderContext.prototype.detachChild = function(child) {
    const index = this.childrens.indexOf(child);
    if (index >= 0) {
        this.childrens.splice(index, 1);
        child.detachEvent("invalidate", this.invalidateProxy);
        if (child.invalid) {
            this._invalidChild--;
        }
    }
}

//StaticRenderContext.prototype.renderProxy = function(child, draw = emptyFn) {
/*StaticRenderContext.prototype.renderProxy = function(child, draw = this.drawChild) {
    let ctx = this;
    const drawProxy = (canvas, timepass) => {ctx.drawChild(child, canvas, timepass)};
    return new Proxy(child, {
        get(target, prop, rcv) {
            if (prop === "draw") {
                return drawProxy;
            }
            if (prop === "original") {
                return target;
            }
            return target[prop];
        },
    });
}*/

/*
    with invalidate there are no need for Proxy
 */
StaticRenderContext.prototype.renderProxy = function(child) {
    let ctx = this;
    return Object.create(child, {
        draw: {
            //semantically this is prefered
            //value:          function (canvas, timepass) { ctx.drawChild( Object.getPrototypeOf(this), canvas, timepass) },
            //but it is simpler, as we already capture the scope. NOTE: This one will not change behavior if "this" change
            value:          (canvas, timepass) => { ctx.drawChild(child, canvas, timepass) },
            enumerable:     true,
            configurable:   true,
            writable:       true
        },
        invalidate: {
            value(a, b) {
                return Object.getPrototypeOf(this).invalidate(a, b);
            }
        },
        invalid: {
            //mimic Proxy behavior allow set invalid prop directly
            get() {
                return Object.getPrototypeOf(this).invalid
            },
            set(value) {
                Object.getPrototypeOf(this).invalid = value;
            }
        }
    });
}

/**
* This method allow partialRedraw of StaticRenderContext (like original child draw) in order to provide method of redrawing only one widget
* child draw willbe first save in internal buffer and only then part of buffer (box pos and size) willbe draw on canvas
* if full background redraw needed in can be done like this:
* StaticRenderContext.draw(canvas, timepass);
* StaticRenderContext.partialUpdate = false; //make child draw noop
* widgetsCollection.draw(canvas, timepass);
* StaticRenderContext.partialUpdate = true;
 *
 * if only one widget needed to be updated:
 * StaticRenderContext.partialUpdate = true; //make child draw from StaticRenderContext.buffer
 * widgetsCollection.draw(canvas, timepass);
 * StaticRenderContext.partialUpdate = false;
**/
StaticRenderContext.prototype.drawChild = function(child, canvas, timepass) {
    if (!this.partialUpdate) {
        return;
    }
    if (child.invalid) {
        //console.warn("redraw", "StaticRenderContext.prototype.drawChild", "child.invalidate partial redraw(to internal buffer)", child.config.id);
        this.renderCtx.offscreenCtx.clearRect(child.box.x, child.box.y, child.box.w, child.box.h);
        this.renderCtx.offscreenCtx.lineWidth = canvas.lineWidth;
        this.renderCtx.offscreenCtx.font = canvas.font;
        child.draw(this.renderCtx.offscreenCtx, timepass);
        this._invalidChild--;
    }
    //console.warn("redraw", "StaticRenderContext.prototype.drawChild", "partial redraw(from internal buffer)", child,  child.box, child.config.id);
    canvas.drawImage(this.renderCtx.offscreenCanvas, child.box.x, child.box.y, child.box.w, child.box.h, child.box.x, child.box.y, child.box.w, child.box.h);
}

StaticRenderContext.prototype.draw = function(canvas, timepass) {
    if (this.renderCtx.offscreenInvalidate) {
        this.renderCtx.offscreenCtx.lineWidth   = canvas.lineWidth;
        this.renderCtx.offscreenCtx.font        = canvas.font;
        this.refresh();
    } else {
        this.timepass += timepass;
    }
    //console.warn("redraw", "StaticRenderContext.prototype.draw", "full static redraw(from buffer)");
    canvas.drawImage(this.renderCtx.offscreenCanvas, 0, 0);
}

StaticRenderContext.prototype.refresh = function() {

    //console.warn("redraw", "StaticRenderContext.prototype.refresh", "full static redraw(to internal buffer)");

    this.renderCtx.offscreenCtx.clearRect(0,0, this.renderCtx.offscreenCanvasWidth, this.renderCtx.offscreenCanvasHeight);
    //this._invalidateCb(this.renderCtx.offscreenCtx, this.timepass, this);

    for (const child of this.childrens) {
        child.draw(this.renderCtx.offscreenCtx, this.timepass);
    }

    this.renderCtx.offscreenInvalidate  = false;
    this.timepass = 0;
    this._invalidChild                  = 0;
}

Object.defineProperties(StaticRenderContext.prototype, {
    invalid: {
        get() {
            return this._invalidChild > 0;
        }
    }
});


function pointOnEllipseV1(radiusX, radiusY, angle) {

    let parametricAngle = Math.atan(Math.tan(angle)*(radiusX / radiusY));

    let vertexX=   (radiusX)*Math.cos(parametricAngle);
    let vertexY=   (radiusY)*Math.sin(parametricAngle);

    const anglePiChunk = Math.abs(angle % (PI_2));
    if (((PI_1_2 < anglePiChunk) && (anglePiChunk < PI_1_1_2))) {
        vertexX *= -1;
        vertexY *= -1;
    }

    return {
        vertexX,
        vertexY,
    }
}

function pointOnEllipseV2(radiusX, radiusY, angle) {

    let vertexX = (radiusX * radiusY) / Math.sqrt(Math.pow(radiusY, 2) + Math.pow(radiusX, 2) * Math.pow(Math.tan(angle), 2));
    let vertexY = (radiusX * radiusY * Math.tan(angle)) / Math.sqrt(Math.pow(radiusY, 2) + Math.pow(radiusX, 2) * Math.pow(Math.tan(angle), 2));

    const anglePiChunk = Math.abs(angle % (PI_2));
    if (((PI_1_2 < anglePiChunk) && (anglePiChunk < PI_1_1_2))) {
        vertexX *= -1;
        vertexY *= -1;
    }

    return {
        vertexX,
        vertexY,
    }
}

function pointOnEllipseGl(radiusX, radiusY, angle) {

    let vertexX=   (radiusX)*Math.cos(angle);
    let vertexY=   (radiusY)*Math.sin(angle);

    return {
        vertexX,
        vertexY,
    }
}

function distance(x1, y1, x2, y2) {
    return Math.sqrt(Math.pow(x2-x1, 2) + Math.pow(y2-y1, 2));
}

function distance0(x2, y2) {
    return distance(0, 0, x2, y2);
}

function directionRad(x, y) {
    let angle = Math.atan2(y, x);
    if (angle < 0) {
        angle = PI + (PI + angle);
    }
    return angle;
}

function crossingRect(x, y, rx, ry, rw, rh) {
    return rx <= x && x <= (rx + rw) && ry <= y && y <= (ry + rh);
}

function crossingBoundingBox(px, py, {x, y, w, h}) {
    return crossingRect(px, py, x, y, w, h);
}

function normalizeRad(rad) {
    if (rad < 0) {
        rad += PI_2;
    }
    return rad % (PI_2);
}

function resolveDimension(dimension, box = window) {
    return new Promise((resolve, reject) => {
        //todo make wrapper sizeOf box
        //todo make probe width = dimension;
        //todo applyChanges
        let probe;
        requestDocumentFlushTime(() => {
            resolve(getComputedStyle(probe).width);
        });
    });
}

function TextMetric(font, textAlign) {
    this.font       = font;
    this.textAlign  = textAlign;
    this.metric = new Map();
}

TextMetric.prototype.measureText = function(text) {
    if (this.metric.has(text)) {
        return this.metric.get(text);
    } else {
        //console.warn("TextMetric.prototype.measureText", text, "access not cached", this.font);
        const metric = TextMetric.measureAllMetrics(this.font, this.textAlign, text);
        this.metric.set(text, metric);
        return metric;
    }
}

TextMetric.prototype.cache = function(text, deep = true) {
    this.metric.set(text, TextMetric.measureAllMetrics(this.font, this.textAlign, text));
    if (deep && text.length > 1) {
        for (const symbol of text) {
            if (!this.metric.has(symbol)) {
                console.info("TextMetric", "cache", symbol);
                this.metric.set(symbol, TextMetric.measureAllMetrics(this.font, this.textAlign, symbol));
            }
        }
    }
    return this;
}

TextMetric.registryKey = function (font, textAlign) {
    return `${font} ${textAlign}`;
}

TextMetric.for = function(font, textAlign = "default") {
    if (textAlign === "default") {
        textAlign = TextMetric.ctx.textAlign;
    }
    const key = TextMetric.registryKey(font, textAlign);
    if (TextMetric.registry.has(key)) {
        return TextMetric.registry.get(key);
    } else {
        let metric = new TextMetric(font, textAlign);
        TextMetric.registry.set(key, metric);
        return metric;
    }
}

TextMetric.forDefault = function(textAlign = "default") {
    if (textAlign === "default") {
        textAlign = TextMetric.ctx.textAlign;
    }
    const key = TextMetric.registryKey(TextMetric.ctx.font, textAlign);
    if (TextMetric.registry.has(key)) {
        return TextMetric.registry.get(key);
    } else {
        let metric = new TextMetric(font, textAlign);
        TextMetric.registry.set(key, metric);
        return metric;
    }
}

TextMetric.fitInBox = function(font, caption, width, height, minFontSize = 8, maxFontSize = 60, textAlign = "default") {
    if (textAlign === "default") {
        textAlign = TextMetric.ctx.textAlign;
    }
    let fontSize, fontCandidate, box;
    for (fontSize = maxFontSize; fontSize >= minFontSize; fontSize--) {
        fontCandidate = TextMetric.normalize(font, {size: fontSize + "px"});
        box = TextMetric.for(fontCandidate, textAlign).measureText(caption);
        if (box.width <= width && box.actualBoundingBoxAscent <= height) {
            return {
                success:    true,
                font:       fontCandidate,
                fontSize,
                box,
            }
        }
    }
    return {
        success:    false,
        font:       fontCandidate,
        minFontSize,
        box,
    }
}

TextMetric.reset = function(font = "all") {
    if (font === "all") {
        TextMetric.registry.clear();
    } else {
        TextMetric.registry.delete(font);
    }
}

TextMetric.setup = function(config = {w: 100, h: 100}) {
    TextMetric.canvas = document.createElement("canvas");
    TextMetric.canvas.width  = config.w;
    TextMetric.canvas.height = config.h;
    TextMetric.ctx      = TextMetric.canvas.getContext("2d");
    const measure = TextMetric.ctx.measureText("TextMetric.setup");
    TextMetric.native = TextMetric.lookup.reduce((accumulator, key) => { return measure[key] === undefined ? accumulator : accumulator + 1 }, 0) > 1;
}

TextMetric.calculateActualBoundingBoxLeft = function(context) {
    if (context.isSpace) {
        return 0;
    }

    let i   = 0;
    let w4  = context.width * 4;
    let l            = context.data.length;

    // Find the min-x coordinate.
    for (; i < l && context.data[i] === 255;) {
        i += w4;
        if (i >= l) {
            i = (i - l) + 4;
        }
    }
    let minx = ((i % w4) / 4) | 0;

    return context.textPosX - minx;
}

TextMetric.calculateActualBoundingBoxRight = function(context) {
    if (context.isSpace) {
        return context.textWidth;
    }

    let i   = 0;
    let w4  = context.width * 4;
    let l   = context.data.length;
    let step = 1;

    // Find the max-x coordinate.
    for (i = l - 3; i >= 0 && context.data[i] === 255;) {
        i -= w4;
        if (i < 0) {
            i = (l - 3) - (step++) * 4;
        }
    }
    let maxx = ((i % w4) / 4) + 1 | 0;



    return maxx - context.textPosX;
}

TextMetric.calculateActualBoundingBoxAscent = function(context) {
    if (context.isSpace) {
        return 0;
    }

    let i = 0;
    let w4 = context.width * 4;
    let l = context.data.length;

    // Finding the ascent uses a normal, forward scanline.
    while (++i < l && context.data[i] === 255) {}
    let ascent = (i / w4) | 0;

    return context.textPosY - ascent;
}

TextMetric.calculateActualBoundingBoxDescent = function(context) {
    if (context.isSpace) {
        return 0;
    }

    let w4 = context.width * 4;
    let l = context.data.length;
    let i = l - 1;

    // Finding the descent uses a reverse scanline.
    while (--i > 0 && context.data[i] === 255) {}
    let descent = (i / w4) | 0;

    return  descent - context.textPosY;
}

TextMetric.parseFont = function(font) {
    let [fontStyle, fontSize, fontFamily] = font.split(" ");
    if (fontFamily === undefined) {
        fontFamily = fontSize;
        fontSize   = fontStyle;
        fontStyle  = "normal";
    }
    return {
        fontStyle,
        fontSize,
        fontFamily,
    }
}

TextMetric.joinFont = function(fontStyle, fontSize = undefined, fontFamily = undefined) {
    if (typeof fontStyle === "object") {
        ({ fontStyle, fontSize, fontFamily } = fontStyle);
    }
    return `${fontStyle} ${fontSize} ${fontFamily}`;
}

TextMetric.normalize = function(font, {style, size, family}) {
    let [fontStyle, fontSize, fontFamily] = font.split(" ");
    if (fontFamily === undefined) {
        fontFamily = fontSize;
        fontSize   = fontStyle;
        fontStyle  = "normal";
    }
    return  TextMetric.joinFont(style||fontStyle, size||fontSize, family||fontFamily);
}

TextMetric.measureAllMetrics = function(font, textAlign, text) {

    TextMetric.ctx.font = font;
    TextMetric.ctx.textAlign = textAlign;

    if (TextMetric.native) {
        const measure = TextMetric.ctx.measureText(text);
        return Object.fromEntries(TextMetric.lookup.map((key) => [key, measure[key]]));
    } else {

        const {fontStyle, fontSize, fontFamily} = TextMetric.parseFont(font);

        let context = {
            fontStyle,
            fontSize: parseInt(fontSize),
            fontFamily,
            textWidth: TextMetric.ctx.measureText(text).width,
            isSpace: /^\s$/.test(text),
        };

        if (TextMetric.ctx.reset) {
            TextMetric.ctx.reset();
        }

        context.padding             = context.fontSize / 2;
        TextMetric.canvas.width     = context.width     =  parseInt(context.textWidth + (context.padding * 2));
        TextMetric.canvas.height    = context.height    =  3 * context.fontSize;
        context.textPosX            = 0;
        context.textPosY            = context.height / 2;

        //canvas resize cause reset;
        TextMetric.ctx.font = font;
        TextMetric.ctx.textAlign = textAlign;

        // Set all canvas pixeldata values to 255, with all the content
        // data being 0. This lets us scan for data[i] != 255.
        TextMetric.ctx.fillStyle = 'white';
        TextMetric.ctx.fillRect(-1, -1, context.width + 2, context.height + 2);

        //console.log("TextMetric", "ctx", context)

        switch (textAlign) {
            case 'center':
                context.textPosX = context.width / 2;
                break;
            case 'right':
            case 'end':
                context.textPosX = context.width - context.padding;
                break;
            case 'left':
            case 'start':
            default:
                context.textPosX = context.padding;
                break;
        }
        TextMetric.ctx.fillStyle = 'black';
        TextMetric.ctx.fillText(text, context.textPosX, context.textPosY);
        context.data = TextMetric.ctx.getImageData(0, 0, context.width, context.height).data;

        let measure = Object.fromEntries(TextMetric.lookup.map((key) => [key, undefined]));

        //width
        measure.width = context.textWidth;

        //actualBoundingBoxLeft
        measure.actualBoundingBoxLeft = TextMetric.calculateActualBoundingBoxLeft(context);

        //actualBoundingBoxRight
        measure.actualBoundingBoxRight = TextMetric.calculateActualBoundingBoxRight(context);

        //fontBoundingBoxAscent
            //unable

        //fontBoundingBoxDescent
            //unable

        //actualBoundingBoxAscent
        measure.actualBoundingBoxAscent = TextMetric.calculateActualBoundingBoxAscent(context);

        //actualBoundingBoxAscent
        measure.actualBoundingBoxDescent = TextMetric.calculateActualBoundingBoxDescent(context);

        //hangingBaseline
            //unable

        return measure;

    }
}

TextMetric.registry = new Map();

TextMetric.native   = false;

TextMetric.lookup = [
    "width", "actualBoundingBoxLeft", "actualBoundingBoxRight", "fontBoundingBoxAscent", "fontBoundingBoxDescent",
    "actualBoundingBoxAscent", "actualBoundingBoxDescent", "emHeightAscent", "emHeightDescent", "hangingBaseline"
];

TextMetric.setup();

EventDispatcherTrait.isInited = function(instance, owner = instance) {
    return instance["eventDispatcherOwner"] === owner;
}
function EventDispatcherTrait(owner = this) {
    this.eventDispatcherAttachedEvents = Object.create(null);
    this.eventDispatcherOwner = this;
}

EventDispatcherTrait.prototype.attachEvent = function (eventId, callback, argument = undefined) {
    if (this.eventDispatcherAttachedEvents[eventId]) {
        this.eventDispatcherAttachedEvents[eventId].push({ callback, argument })
    } else {
        this.eventDispatcherAttachedEvents[eventId] = [{ callback, argument }];
    }
}

EventDispatcherTrait.prototype.detachEvent = function (eventId, callback) {
    let scope = this.eventDispatcherAttachedEvents[eventId];
    if (scope) {
        for (let indexOf = 0; indexOf < scope.length; indexOf++) {
            if (scope[indexOf].callback === callback) {
                scope.splice(indexOf, 1);
                break;
            }
        }
        if (scope.length === 0) {
            delete this.eventDispatcherAttachedEvents[eventId];
        }
    }
}

EventDispatcherTrait.prototype.dispatchEvent = function (eventId, context, eData) {
    if (this.eventDispatcherAttachedEvents[eventId]) {
        for (const callable of this.eventDispatcherAttachedEvents[eventId]) {
            callable.callback.call(this.eventDispatcherOwner, eventId, context, eData, callable.argument);
        }
    } else {
        console.warn("no event handler for", eventId);
    }
}

EventDispatcherTrait.prototype.hasAnyEvent = function (eventId) {
    return !!this.eventDispatcherAttachedEvents[eventId];
}

EventDispatcherTrait.prototype.freeEventDispatcher = function() {
    this.eventDispatcherAttachedEvents = {};
}

function WidgetInvalidateTrait(owner = this) {
    /*    if (!mixin.has(this, EventDispatcherTrait) && !mixin.has(this.constructor, EventDispatcherTrait)) {
            mixin(this, EventDispatcherTrait);
        }*/
    if (!EventDispatcherTrait.isInited(this, owner)) {
        EventDispatcherTrait.call(this, owner);
    }
    this.invalid = true;
}

mixin(WidgetInvalidateTrait, EventDispatcherTrait);

WidgetInvalidateTrait.isInited = function(instance, owner = instance) {
    return Object.hasOwn(instance,"invalid");
}

WidgetInvalidateTrait.prototype.invalidate = function (reason = undefined, eData = undefined) {
    if (this.invalid) {
        return;
    }
    this.invalid = true;
    //console.info("WidgetInvalidateTrait", "fire", this.config.id);
    this.dispatchEvent("invalidate", reason, eData);
}

WidgetInvalidateTrait.prototype.invalidateIf = function (ifTrue, resetIfFalse = true, reason = undefined, eData = undefined) {
    if (ifTrue) {
        this.invalidate(reason, eData);
    } else if (resetIfFalse) {
        this.invalid = false;
    }
}

function Transition(owner) {
    this.transitions    = new Map;
    this.owner = owner;
}

Transition.prototype.attach = function (name, callbacksLinkedList, globalCtx = this.owner) {
    this.transitions.set(name, { current: callbacksLinkedList, initial: Object.assign({}, this.owner), acc: 0, globalCtx });
    return this;
}

Transition.prototype.cancel = function (name) {
    this.transitions.delete(name);
    return this;
}

Transition.prototype.owner = function (name) {
    return this.owner;
}

Transition.prototype.update = function (timepass) {
    for (const [key, transition] of this.transitions) {
        if (transition.current.callback.call( transition.globalCtx, timepass, transition.current.params, transition.initial, this.owner, transition.acc += timepass)) {
            if (transition.current.next) {
                transition.current = transition.current.next;
                transition.initial = Object.assign({}, this.owner);
                transition.acc     = 0;
            } else {
                this.transitions.delete(key);
            }
        }
    }
}

Transition.prototype.hasAny = function () {
    return this.transitions.size > 0
}

function Combine(transitions = [], next = null) {

    let root = {
        next: null,
    }

    //let nextTransition = [];

    let node = root;
    for (const tr of transitions) {
        node.next = {
            data:       tr,
            next:       null,
        }
        node = node.next;
/*        if (tr.next) {
            nextTransition.push(tr.next);
        }*/
    }

/*    let nextChain = next;
    if (nextTransition.length) {  //todo op move it to CombineSteep() version of combine
                                  //currently this f mostly do by next param
        nextChain = Combine(nextTransition);
        let node = nextChain;
        while (node.next) {
            node = nextChain.next;
        }
        node.next = next;
    }*/

    return {
        callback: (timepass, params, initial, currentState, accumulator) => {
            for (let node = params.root.next, prev = params.root; node !== null; prev = node, node = node.next) {
                if (node.data.callback(timepass, node.data.params, initial, currentState, accumulator)) {
                    if (node.data.next) {
                        node.data = node.data.next; //move deeply in chain
                    } else {
                        prev.next = node.next;      //remove callback from call chain
                    }
                }
            }
            if (params.root.next === null) { //if no one left combile is complete
                return true;
            }
        },
        params: { root },
        next:   next,
    }
}


function invalidateProxy(evtId, instance, eData, argument) {
    //console.info("redraw", "invalidateProxy", this.config.id, argument.config.id);
    argument.invalidate(eData, argument);
}

function eventProxyDirect(evtId, instance, eData, argument) {
    argument.dispatchEvent(evtId, instance, eData, argument);
}

function eventProxyBubble(evtId, instance, eData, argument) {
    argument.dispatchEvent(evtId, argument, eData, argument);
}

function fillTextEllipse (canvas, text, radiusX, radiusY, startAngle, endAngle, direction = 'ltr', float = false) {
    let numRadsPerLetter = (endAngle - startAngle) / text.length;
    let reverse = direction === "rtl";
    canvas.save();

    const fontHeight = 15;
    const scale = radiusY / radiusX;

    for(let i= 0; i < text.length; i++){

        const trueIndex = reverse ? text.length - i - 1 : i;
        const angle = startAngle + i*numRadsPerLetter + numRadsPerLetter / 2;
        const point = pointOnEllipseGl(radiusX, radiusY, angle);
        const block = TextMetric.for(canvas.font).measureText(text[trueIndex]);
        const blockHeight = ~~(block.actualBoundingBoxAscent + 0.5);

        canvas.save();
        canvas.translate(point.vertexX, point.vertexY);

        if (0 < angle && angle < PI) {
            canvas.rotate(angle +  -PI_1_2 );
        } else {
            canvas.rotate(angle +  PI_1_2 );
        }

        canvas.translate(~~(-block.width / 2), ~~(blockHeight / 2));

        canvas.fillText(text[trueIndex], 0, 0);
        canvas.restore();
    }
    canvas.restore();
}

function ShieldVectorShape(ctx, radiusX, radiusY, startAngle, endAngle, spacingAngle, size, utilization = 1.0) {


    if (radiusX < size || radiusY < size) {
        throw new Error("invalid radius or size")
    }

    utilization = Math.max(Math.min(utilization, 1.0), 0.0);
    if (utilization === 0) {
        return;
    }

    const halfSize = size / 2;
    const utilHalfSize = halfSize * utilization;

    if (spacingAngle !== 0) {
        spacingAngle = spacingAngle + (((endAngle - spacingAngle) - (startAngle + spacingAngle)) * (1.0 - utilization)) / 2;
    }

    ctx.beginPath();
    ctx.ellipse(0, 0, radiusX - halfSize + utilHalfSize, radiusY - halfSize + utilHalfSize,  0, startAngle + spacingAngle, endAngle - spacingAngle, false);
    ctx.ellipse(0, 0, radiusX - halfSize - utilHalfSize, radiusY - halfSize - utilHalfSize,  0, endAngle - spacingAngle, startAngle + spacingAngle, true);

    ctx.closePath();
}

ShieldVectorShape.crossing = function (x, y, radiusX, radiusY, startAngle, endAngle, spacingAngle, size, utilization = 1.0) {

    let angle = directionRad(x, y);

    const halfSize = size / 2;
    const utilHalfSize = halfSize * utilization;

    if (this.inView(angle, startAngle, endAngle, spacingAngle, size, utilization)) {
        const pOuter = pointOnEllipseGl(radiusX - halfSize + utilHalfSize, radiusY - halfSize + utilHalfSize, angle);
        const pInner = pointOnEllipseGl(radiusX - halfSize - utilHalfSize, radiusY - halfSize - utilHalfSize, angle);
        const ourDist = distance0(x, y);
        return (distance0(pInner.vertexX, pInner.vertexY) <= ourDist) && (ourDist <= distance0(pOuter.vertexX, pOuter.vertexY));
    } else {
        return false;
    }
}

ShieldVectorShape.inView = function (angle, startAngle, endAngle, spacingAngle, size, utilization = 1.0) {

    if (spacingAngle !== 0) {
        spacingAngle = spacingAngle + (((endAngle - spacingAngle) - (startAngle + spacingAngle)) * (1.0 - utilization)) / 2;
    }

    const trueStart = startAngle + spacingAngle;
    const trueEnd   = endAngle - spacingAngle;

    if (trueEnd >= PI_2 && trueStart < PI_2) { //wrk tail crossing start
        return ((trueStart <= angle && angle < PI_2) || (angle + PI_2 <= trueEnd));
    }

    return ((trueStart <= angle) && (angle <= trueEnd));
}

ShieldVectorShape.distance = function (x, y, borderType, radiusX, radiusY, startAngle, endAngle, spacingAngle, size, utilization = 1.0) {

    const angle = directionRad(x, y);

    const halfSize = size / 2;
    const utilHalfSize = halfSize * utilization;

    let points;
    if (borderType === "inner") {
        points = pointOnEllipseGl(radiusX - halfSize - utilHalfSize, radiusY - halfSize - utilHalfSize, angle);
    } else if (borderType === "outer") {
        points = pointOnEllipseGl(radiusX - halfSize + utilHalfSize, radiusY - halfSize + utilHalfSize, angle);
    } else {
        throw new Error("not implemented: " + borderType);
    }


    return distance(x,y, points.vertexX, points.vertexY);
}

ShieldVectorShape.distance0 = function (angle, borderType, radiusX, radiusY, startAngle, endAngle, spacingAngle, size, utilization = 1.0) {

    const halfSize = size / 2;
    const utilHalfSize = halfSize * utilization;

    let points;
    if (borderType === "inner") {
        points = pointOnEllipseGl(radiusX - halfSize - utilHalfSize, radiusY - halfSize - utilHalfSize, angle);
    } else if (borderType === "outer") {
        points = pointOnEllipseGl(radiusX - halfSize + utilHalfSize, radiusY - halfSize + utilHalfSize, angle);
    } else {
        throw new Error("not implemented: " + borderType);
    }

    return distance0(points.vertexX, points.vertexY);
}


function ShieldVector(radiusX, radiusY, startAngle, endAngle, spacingAngle, size) {
    this.radiusX        = radiusX;
    this.radiusY        = radiusY;
    this.startAngle     = startAngle;
    this.endAngle       = endAngle;
    this.spacingAngle   = spacingAngle;
    this.size           = size;
    this.utilization    = 1.0;

    this.transition     = new Transition(this);

    this.fillStyleStart   = undefined;
    this.strokeStyleStart = undefined;

    this.fillStyleEnd     = undefined;
    this.strokeStyleEnd   = undefined;

    this.fillStrokeProgress  = 0.0;

    this.fillStrokeTransform = identityMatrix;

    this.sharedInvalidState = { invalid: true };

}

ShieldVector.prototype.draw = function (canvas, timepass) {

    this.transition.update(timepass);

    ShieldVectorShape(canvas, this.radiusX, this.radiusY, this.startAngle, this.endAngle, this.spacingAngle, this.size, this.utilization);
    canvas.save();
    canvas.transform(...this.fillStrokeTransform);
    this.stroke(canvas, timepass); //in exact direction or will-be line on buble configuration
    this.fill(canvas, timepass);   // may cause problems in multipixel size

    canvas.restore();

    this.sharedInvalidState.invalid |= this.transition.hasAny();
}

ShieldVector.prototype.fill = function (canvas, timepass) {
    if (this.fillStyleStart && this.fillStrokeProgress < 1.0) {
        canvas.fillStyle = this.fillStyleStart;
        canvas.fill(); ///"evenodd"
    }
    if (this.fillStyleEnd && this.fillStrokeProgress > 0.0) {
        canvas.globalAlpha = this.fillStrokeProgress;
        canvas.fillStyle = this.fillStyleEnd;
        canvas.fill(); ///"evenodd"
    }
}

ShieldVector.prototype.stroke = function (canvas, timepass) {
    if (this.strokeStyleStart && this.fillStrokeProgress < 1.0) {
        canvas.strokeStyle = this.strokeStyleStart;
    }
    if (this.strokeStyleEnd && this.fillStrokeProgress > 0.0) {
        canvas.globalAlpha = this.fillStrokeProgress;
        canvas.strokeStyle = this.strokeStyleEnd;
    }
}

ShieldVector.prototype.cross = function (x, y, ignoreUtilization = false) {
    return ShieldVectorShape.crossing(x, y, this.radiusX, this.radiusY, this.startAngle, this.endAngle, this.spacingAngle, this.size, ignoreUtilization ? 1.0 : this.utilization);
}

ShieldVector.prototype.inView = function (angle, ignoreUtilization = false) {
    return ShieldVectorShape.inView(angle, this.startAngle, this.endAngle, this.spacingAngle, this.size, ignoreUtilization ? 1.0 : this.utilization);
}

ShieldVector.prototype.distance = function (x, y, border = "inner", ignoreUtilization = false) {
    return ShieldVectorShape.distance(x, y, border, this.radiusX, this.radiusY, this.startAngle, this.endAngle, this.spacingAngle, this.size, ignoreUtilization ? 1.0 : this.utilization)
}

ShieldVector.prototype.distance0 = function (angle, border = "inner", ignoreUtilization = false) {
    return ShieldVectorShape.distance0(angle, border, this.radiusX, this.radiusY, this.startAngle, this.endAngle, this.spacingAngle, this.size, ignoreUtilization ? 1.0 : this.utilization)
}

Object.defineProperties(ShieldVector.prototype, {
/*    caption: {
        set(caption) {
            this.caption = new ShieldVectorCaption(caption, this.radiusX, this.radiusY, this.startAngle, this.endAngle, this.spacingAngle, this.size, this.utilization);
            this.caption.sharedInvalidState = this.sharedInvalidState;
            this.sharedInvalidState.invalid = true;
        },
        get() {
            return this.caption ? this.caption.caption : ""
        }
    },*/
})



function ShieldVectorCaption(caption, radiusX, radiusY, startAngle, endAngle, spacingAngle, size, utilization = 1.0, direction = "ltr") {

    this._captionBlock  = undefined;
    this._font          = "bold " + ~~(30 * window.devicePixelRatio) + "px serif";
    this._caption       = "";

    //it is tricky to scale font with window.devicePixelRatio as in some cases actual font size must be calculated
    //by API like TextMetric.fitInBox with already scaled geometry, so font is exception, dont self scale, it asume than fontSize already scaled
    //in that case we scale only "safe default" value

    utilization    = Math.max(Math.min(utilization, 1.0), 0.0);

    this.sharedInvalidState = { invalid: true };

    this.caption        = caption;
    this.radiusX        = radiusX;
    this.radiusY        = radiusY;
    this.startAngle     = startAngle;
    this.endAngle       = endAngle;
    this.spacingAngle   = spacingAngle;
    this.size           = size;
    this.utilization    = utilization;
    this.direction      = direction;



}

ShieldVectorCaption.prototype.draw = function (canvas, timepass) {

    let spacingAngle = this.spacingAngle;
    if (this.spacingAngle !== 0) {
        spacingAngle = this.spacingAngle + (((this.endAngle - this.spacingAngle) - (this.startAngle + this.spacingAngle)) * (1.0 - this.utilization)) / 2;
    }
    const radiusX = this.radiusX - (this.size + this._captionBlock.actualBoundingBoxAscent);
    const radiusY = this.radiusY - (this.size + this._captionBlock.actualBoundingBoxAscent);
    canvas.save();
    canvas.font = this._font;
    fillTextEllipse(canvas, this._caption, radiusX, radiusY, this.startAngle + spacingAngle, this.endAngle - spacingAngle, this.direction, true);
    canvas.restore();

}

Object.defineProperties(ShieldVectorCaption.prototype, {
    font: {
        set(font) {
            this._captionBlock = TextMetric.for(font).measureText(this.caption);
            this._font = font;
            this.sharedInvalidState.invalid = true;
        },
        get() {
            return this._font;
        }
    },
    caption: {
        set(caption) {
            this._captionBlock = TextMetric.for(this._font).measureText(caption);
            this._caption = caption;
            this.sharedInvalidState.invalid = true;
        },
        get() {
            return this._caption;
        }
    }
})

function ShieldWidgetIcon(width, height) {
    this.width  = width;
    this.height = height;
    //this.path = new Path2D("M36,84v4L11,72V68Zm2,6v4h6V90ZM23,63h3L49,26V22ZM64,84v4L89,72V68Zm-8,6v4h6V90ZM51,22v4L74,63h3ZM50,7,11.4,65.7,38,82.45V88h6V79.56L20,64,50,17,80,64,56,79.56V88h6V82.45L88.6,65.7Z");
    this.path = new Path2D("M26.9336,28.6411l-10-26a1,1,0,0,0-1.8672,0l-10,26A1,1,0,0,0,6.6,29.8L16,22.75l9.4,7.05a1,1,0,0,0,1.5332-1.1587Z");
}

ShieldWidgetIcon.prototype.draw = function (ctx, timepast) {
    ctx.fill(this.path);
}

ShieldWidget.defaults = {

    iconPagination: 5,
    iconOffsetX:    -0.045,
    iconOffsetY:    0.05,//0.10,//0.25
    iconScale:      0.75,

    count:          8,
    spacing:        PI_1_2 * .015,
    size:           50,
    ringSpacing:    5,
    shift:          PI_1_4,

    Vector:     ShieldVector,
    Caption:    ShieldVectorCaption,
    Icon:       ShieldWidgetIcon,

    noevents:       false,
    noanimation:    false,
}


ShieldWidget.vectors = {
    FRONT:      0,
    RIGHT:      1,
    BACK:       2,
    LEFT:       3,
    TOP:        4,
    SPECIAL:    5,
    BOTTOM:     6,
    RESERVED:   7,
    BUBLE:      0,
}

function ShieldWidget(dom, config = ShieldWidget.defaults) {

    const rect = boundingClientRect(dom); //for all avl size
    //const rect = { width: dom.offsetWidth, height: dom.offsetHeight };
    const computedStyle = getComputedStyle(dom); //for possible padding

    if (computedStyle.boxSizing !== "border-box") {
        throw new Error("other than 'border-box' sizing model not impl");
    }

    this.dom    = dom;
    this.canvas = dom.querySelector("canvas");
    //dom.offset* we must use actual size without scale effect(if it apply), to prevent canvas pixel decimation
    //remember that dom.offset* is mathematically rounded, so in the end 1px error may occur
    const width  = dom.offsetWidth  - boxStyleWidthModifier(computedStyle);
    const height = dom.offsetHeight - boxStyleHeightModifier(computedStyle);
    this.canvas.width  = ~~(width  * window.devicePixelRatio);
    this.canvas.height = ~~(height * window.devicePixelRatio);
    this.canvas.style.width  = (~~width ) + "px";  //or set it to 100% in css
    this.canvas.style.height = (~~height) + "px";

    this.ctx = this.canvas.getContext("2d");
    const defaultFont = TextMetric.parseFont(this.ctx.font);
    defaultFont.fontSize = ~~(parseInt(defaultFont.fontSize) * window.devicePixelRatio) + "px";
    this.ctx.font       = TextMetric.joinFont(defaultFont);
    this.ctx.lineWidth  = ~~Math.max(window.devicePixelRatio, 1);

    this.captionAlpha = 1.0;
    this.utilization = {
        front:  1.0,
        right:  1.0,
        back:   1.0,
        left:   1.0,
        top:    1.0,
        bottom: 1.0
    }

    requestBoundingClientRect(this.canvas, this.clientRect = rect); //temporal value, actual offset for events

    this.config = Object.assign({}, ShieldWidget.defaults, config);

    //this.icon = new this.config.Icon(100 * window.devicePixelRatio, 130 * window.devicePixelRatio);
    //this.icon is svg with path for 100*130 viewport do not scale it there it will-be scaled inside buildRenderCtx to fit in
    //this.icon = new this.config.Icon(100, 130);
    this.icon = new this.config.Icon(22, 32); //actual size is 32*32

    const rCtx = this.renderCtx = this.buildRenderCtx(this.config);

    this.sharedInvalidState = { invalid: true };

    this.parts = this.buildShieldVectorsList(this.config.count);

    for (let part of this.parts) {
        if (part) {
            part.sharedInvalidState = this.sharedInvalidState;
        }
    }

    this.captions = this.buildShieldCaptionsList(this.parts);

    this.initColorPalette();

    this.setupPartsColorPalette();

    this.updateFontSize();

    this.timeouts   = {};
    this.intervals  = {};
    this.refreshRequest = PendingRequest();
    this.beforeRefreshAnimateState = undefined;


    this.transition = new Transition(this);


    this.events = {};
    this.globalEvents = {};

    if (!config.noevents) {
        this.initEvents();
    }

    if (!EventDispatcherTrait.isInited(this)) {
        EventDispatcherTrait.call(this);
    }

    this.animate(!config.noanimation);

}

mixin(ShieldWidget, EventDispatcherTrait);

ShieldWidget.prototype.isAnimating = function () {
    return this.nextAnimationFrameId !== undefined;
}

ShieldWidget.prototype.animate = function (animate = true) {
    if (animate) {
        if (this.nextAnimationFrameId) {
            cancelAnimationFrame(this.nextAnimationFrameId);
            this.nextAnimationFrameId = undefined;
        }
        requestBoundingClientRect(this.canvas, this.clientRect);
        let prevTime = new Date();
        this.sharedInvalidState.invalid = true;
        const animation = function (timestamp){
            if (this.sharedInvalidState.invalid) {
                this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
                this.draw(this.ctx, timestamp - prevTime);
            }
            prevTime = timestamp;
            this.nextAnimationFrameId = requestAnimationFrame(animation);
        }.bind(this);
        this.nextAnimationFrameId = requestAnimationFrame(animation);
    } else {
        if (this.nextAnimationFrameId) {
            cancelAnimationFrame(this.nextAnimationFrameId);
            this.nextAnimationFrameId = undefined;
        }
    }
}

ShieldWidget.prototype.initEvents = function() {
    const handler = (e) => {

        const rCtx = this.renderCtx;
        const unified = unify(e);
        //const rect = e.target.getBoundingClientRect(); //not work is scale apply
        const rect = this.clientRect;
        const normX = ~~((unified.pageX - rect.left) * window.devicePixelRatio)  - this.canvas.width / 2;
        const normY = ~~((unified.pageY - rect.top)  * window.devicePixelRatio)  - this.canvas.height / 2;

        switch (e.type) {
            case "mouseleave":
            case "touchcancel":

                for (let part of this.parts) {
                    if (part && !this.completeTransition) {
                        part.transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusIn,
                            params: {duration: 500, target: 1.0},
                        });
                    }
                }

                if (this.beginSelectionTransition) {
                    clearTimeout(this.timeouts.beginSelection);
                }
                this.beginSelection = false;
                this.beginSelectionTransition = false;
                this.showCaption = false;
                e.preventDefault();
                this.sharedInvalidState.invalid = true;

                break;
            case "mouseup":
            case "touchend":

                this.showCaption = false;

                //console.log("mouseup", this.showCaption, this.beginSelectionTransition, this.beginSelection)

                if (this.beginSelectionTransition) {
                    for (let part of this.parts) {
                        if (part) {
                            part.transition.attach("focus", {
                                callback: ShieldWidget.transitions.focusIn,
                                params: {duration: 100, target: 1.0},
                            });
                        }
                    }
                    clearTimeout(this.timeouts.beginSelection);
                    this.beginSelectionTransition = false;

                } else if (this.beginSelection) {
                    let selected;
                    for (let part of this.parts) {
                        if (!part) {
                            continue;
                        }
                        let angle = directionRad(normX, normY);
                        if (!selected && part.inView(angle)) {
                            const dist      = part.distance(normX, normY, "outer", true);
                            const dist0     = part.distance0( angle, "outer", true);
                            const outDist0 = distance0(normX, normY);
                            if (outDist0 > dist0) {
                                //move out of figure, relax criteria
                                if (((dist0 - dist) / dist0) > 0.5) {
                                    selected = part;
                                }
                            } else {
                                if (((dist0 - dist) / dist0) > 0.7) {
                                    selected = part;
                                }
                            }
                        }
                        part.transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusIn,
                            params: {duration: 100, target: 1.0},
                        });
                    }
                    if (selected) {
                        this.chosed(this.parts.indexOf(selected), selected);
                    }
                    this.beginSelection = false;
                    this.showCaption = false;
                } else {

                    let active = null;
                    for (let part of this.parts) {
                        if (part && part.cross(normX, normY)) {
                            active = part;
                            break;
                        }
                    }
                    if (active && this.mousedown === active) {
                        for (let part of this.parts) {
                            if (!part) {
                                continue;
                            }
                            if (part === active) {
                                part.transition.attach("focus", {
                                    callback: ShieldWidget.transitions.focusIn,
                                    params: {duration: 500, target: 1.0},
                                    next: {
                                        callback: ShieldWidget.transitions.focusOut,
                                        params: {duration: 500, target: 1.0},
                                    }
                                });
                            } else {
                                part.transition.attach("focus", {
                                    callback: ShieldWidget.transitions.focusOut,
                                    params: {duration: 500, target: 0.8},
                                    next: {
                                        callback: ShieldWidget.transitions.focusIn,
                                        params: {duration: 500, target: 1.0},
                                    }
                                });
                            }
                        }
                        this.chosed(this.parts.indexOf(active), active);
                        this.beginSelection = false;
                        this.showCaption = false;
                    }

                }

                e.preventDefault();
                this.sharedInvalidState.invalid = true;

                break;

            case "mousedown":
            case "touchstart":

                this.sharedInvalidState.invalid = true;

                if (this.beginSelection) {
                    e.preventDefault();
                    return;
                }

                for (let part of this.parts) {
                    if (part && part.cross(normX, normY)) {
                        this.mousedown = part;
                        return;
                    }
                }
                for (let part of this.parts) {
                    if (!part) {
                        continue;
                    }
                    part.transition.attach("focus", {
                        callback: ShieldWidget.transitions.focusOut,
                        params: {duration: 500, target: .4},
                    });
                }

                this.dispatchEvent("activate", this, 0 );

                this.beginSelectionTransition = true;
                this.beginSelectionStartPos = { x: unified.pageX, y:unified.pageY }
                clearTimeout(this.timeouts.beginSelection);
                this.timeouts.beginSelection = setTimeout(() => {
                    //todo refactor to animation states
                    this.beginSelection = true;
                    this.beginSelectionTransition = false;
                    this.showCaption = true;
                    this.captionAlpha = 0.0;
                    this.transition.attach("fadein", {
                        callback: ShieldWidget.transitions.fade,
                        params: {duration: 400, target: 1.0, attribute: "captionAlpha"}
                    });
                    this.sharedInvalidState.invalid = true;
                    this.dispatchEvent("activate", this, 1);
                }, 500);


                e.preventDefault();

                break;
        }

    }

    this.canvas.addEventListener( "mouseleave",     this.events["mouseleave"]   = handler);
    this.canvas.addEventListener( "touchcancel",    this.events["touchcancel"]  = handler);
    this.canvas.addEventListener( "mouseup",        this.events["mouseup"]      = handler);
    this.canvas.addEventListener( "touchend",       this.events["touchend"]     = handler);
    this.canvas.addEventListener( "mousedown",      this.events["mousedown"]    = handler);
    this.canvas.addEventListener( "touchstart",     this.events["touchstart"]   = handler);


    let lastFocused = null;
    const moveHandler = (e) => {

        //console.log(e);

        const unified = unify(e);
        //const rect = e.target.getBoundingClientRect();
        const rect = this.clientRect;

        const normX = ~~((unified.pageX - rect.left) * window.devicePixelRatio)  - this.canvas.width / 2;
        const normY = ~~((unified.pageY - rect.top)  * window.devicePixelRatio)  - this.canvas.height / 2;

        console.log(normX, normY)

        if (this.completeTransition) {
            return;
        }

        //console.log("mousemove", this.showCaption, this.beginSelectionTransition, this.beginSelection)

        if (this.beginSelection) {
            e.stopPropagation();
            this.sharedInvalidState.invalid = true;

            let angle = directionRad(normX, normY);
            let selected = false;

            for (let i = 0; i < this.parts.length; i++) {
                if (this.parts[i]) {
                    if (!selected && this.parts[i].inView(angle)) {
                        const distance = this.parts[i].distance(normX, normY, "outer",true);
                        const distance0 = this.parts[i].distance0( angle,"outer", true);
                        this.parts[i].transition.cancel("focus").owner.utilization = Math.max(0.4 + (0.6 * ((distance0 - distance) / distance0) ), 0.2);
                        selected = true;
                    } else {
                        this.parts[i].transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusOut,
                            params: { duration: 100, target: 0.4 },
                        });
                    }
                }

            }

            this.beginSelectionTransition = false;
            return false;
        } else {
            if (
                this.beginSelectionTransition &&
                distance(this.beginSelectionStartPos.x, this.beginSelectionStartPos.y, unified.pageX, unified.pageY) > quarterFingerPixelsForWidgets
            ) {
                for (let part of this.parts) {
                    if (part) {
                        part.transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusIn,
                            params: {duration: 100, target: 1.0},
                        });
                    }
                }
                clearTimeout(this.timeouts.beginSelection);
                this.beginSelectionTransition = false;
            }

        }

        let focused = null;
        for (let part of this.parts) {
            if (part && part.cross(normX, normY)) {
                focused = part;
                break;
            }
        }

        if (focused !== lastFocused) {
            for (let part of this.parts) {
                if (!part) {
                    continue
                }
                if (focused) {
                    if (focused === part) {
                        part.transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusIn,
                            params: {duration: 250, target: 1.0},
                        });
                    } else {
                        part.transition.attach("focus", {
                            callback: ShieldWidget.transitions.focusOut,
                            params: {duration: 250, target: 0.9},
                        });
                    }
                } else {
                    part.transition.attach("focus", {
                        callback: ShieldWidget.transitions.focusOut,
                        params: {duration: 250, target: 1.0},
                    });
                }
                lastFocused = focused;
            }
        }

        e.preventDefault();
        this.sharedInvalidState.invalid = true;
    };

    this.canvas.addEventListener("mousemove", this.events["mousemove"] = moveHandler);
    this.canvas.addEventListener("touchmove", this.events["mousemove"] = moveHandler);

    this.canvas.addEventListener("contextmenu", this.events["contextmenu"] = (e) => e.preventDefault());

    removeEventListener("resize", this.globalEvents["resize"]);
    addEventListener( "resize", this.globalEvents["resize"] = (e) => {
        this.refresh();
    });
}

ShieldWidget.prototype.freeEvents = function(keepResize = false) {
    for (const [evtId, handler] of Object.entries(this.events)) {
        this.canvas.removeEventListener(evtId, handler);
    }

    for (const [evtId, handler] of Object.entries(this.globalEvents)) {
        if (keepResize && evtId === "resize") {
            continue;
        }
        removeEventListener(evtId, handler);
    }

}

ShieldWidget.prototype.free = function() {

    if (this.nextAnimationFrameId) {
        cancelAnimationFrame(this.nextAnimationFrameId);
    }

    this.freeEvents();

    for (const [,timerId] of Object.entries(this.timeouts)) {
        clearTimeout(timerId);
    }

    for (const [,intervalId] of Object.entries(this.intervals)) {
        clearInterval(intervalId);
    }

    this.events = {};
    this.globalEvents = {};
}

ShieldWidget.prototype.refresh = function () {

    if (this.beforeRefreshAnimateState === undefined) {
        this.beforeRefreshAnimateState = this.isAnimating();
        this.animate(false);
        //this.ctx.clearRect(0, 0, this.renderCtx.width, this.renderCtx.height);
    }

    cancelRequest(this.refreshRequest);

    const request = this.refreshRequest = PendingRequest();

    requestAnimationFrameTime(() => {

        this.canvas.width  = 0;
        this.canvas.height = 0;
        this.canvas.style.width  = '0px';
        this.canvas.style.height = '0px';
        this.ctx.font = TextMetric.defaultFont;

        requestDocumentFlushTime(() => {

            const rect          = boundingClientRect(this.dom); //for all avl size
            const computedStyle = getComputedStyle(this.dom);   //for possible padding
            //@see constructor
            const width     = this.dom.offsetWidth  - boxStyleWidthModifier(computedStyle);
            const height    = this.dom.offsetHeight - boxStyleHeightModifier(computedStyle);

            requestAnimationFrameTime(() => {

                this.canvas.width   = ~~(width  * window.devicePixelRatio);
                this.canvas.height  = ~~(height * window.devicePixelRatio);
                this.canvas.style.width  = ~~(width)  + "px";
                this.canvas.style.height = ~~(height) + "px";

                const defaultFont = TextMetric.parseFont(this.ctx.font);
                defaultFont.fontSize = ~~(parseInt(defaultFont.fontSize) * window.devicePixelRatio) + "px";
                this.ctx.font       = TextMetric.joinFont(defaultFont);
                this.ctx.lineWidth  = ~~Math.max(window.devicePixelRatio, 1);

                requestDocumentFlushTime(() => {

                    //requestBoundingClientRect(this.canvas, this.clientRect); //moved to animate

                    const rCtx = this.renderCtx = this.buildRenderCtx(this.config);

                    this.parts = this.buildShieldVectorsList(this.config.count);

                    for (let part of this.parts) {
                        if (part) {
                            part.sharedInvalidState = this.sharedInvalidState;
                        }
                    }

                    this.captions = this.buildShieldCaptionsList(this.parts);

                    this.initColorPalette();

                    this.setupPartsColorPalette();

                    this.updateFontSize();

                    this.animate(this.beforeRefreshAnimateState);

                    this.beforeRefreshAnimateState = undefined;

                }, request);

            }, request);

        }, request);

    }, request);

}

ShieldWidget.transitions = {
    focusIn : (timepass, params, initial, currentState, accumulator) => {
        const util = params.target - initial.utilization;
        currentState.utilization = initial.utilization + (accumulator / params.duration * util);
        if (accumulator >= params.duration) {
            currentState.utilization = params.target;
            return true;
        }
    },
    focusOut : (timepass, params, initial, currentState, accumulator) => {
        const util = initial.utilization - params.target;
        currentState.utilization = initial.utilization - (accumulator / params.duration * util);
        if (accumulator >= params.duration) {
            currentState.utilization = params.target;
            return true;
        }
    },
    orbit : (timepass, params, initial, currentState, accumulator) => {
        const range = initial.endAngle - initial.startAngle;
        currentState.startAngle = initial.startAngle + ((PI_2) * (accumulator / params.duration));
        currentState.endAngle   = currentState.startAngle + range;
        if (accumulator >= params.duration) {
            currentState.startAngle = initial.startAngle;
            currentState.endAngle   = initial.endAngle;
            return true;
        }
    },
    pulse : (timepass, params, initial, currentState, accumulator) => {
        const phaseDuration = params.phaseDuration ? params.phaseDuration : 2 * PI;
        const utilizationRange = params.range ? params.range : 0.3;
        let range = utilizationRange * Math.cos(accumulator / params.duration * phaseDuration);
        currentState.utilization = initial.utilization - utilizationRange + range;
        if (accumulator >= params.duration) {
            return true;
        }
    },
    idle : (timepass, params, initial, currentState, accumulator) => {
        if (accumulator >= params.duration) {
            return true;
        }
    },
    fade : (timepass, params, initial, currentState, accumulator) => {
        const range = params.target - initial[params.attribute];
        currentState[params.attribute] = initial[params.attribute] + (accumulator / params.duration * range);
        if (accumulator >= params.duration) {
            currentState[params.attribute] = params.target;
            return true;
        }
    }
}

ShieldWidget.prototype.chosed = function(index, direction, silent = false) {
    this.completeTransition = true;
    this.sharedInvalidState.invalid = true;
    if (index === ShieldWidget.vectors.SPECIAL) {
        for (let i = 0; i < this.parts.length; i++) {
            if (this.parts[i] && i < ShieldWidget.vectors.TOP) {
                this.parts[i].transition.attach("focus", {
                    callback: ShieldWidget.transitions.focusIn,
                    params: { duration: 1000, target: 1.0 },
                }).attach("reset", {
                    callback: ShieldWidget.transitions.orbit,
                    params: { duration: 2000 },
                    next: {
                        callback: () => {
                            this.completeTransition = false;
                            return true;
                        }
                    }
                });
            }
        }
    } else if (index === ShieldWidget.vectors.RESERVED) {
        this.completeTransition = false;
    } else {
        for (let i = 0; i < this.parts.length; i++) {

            if (i === ShieldWidget.vectors.RESERVED || i === ShieldWidget.vectors.SPECIAL || !this.parts[i]) {
                continue;
            }

            this.parts[i].fillStyleEnd = (i === index) ? this.parts[i].colorPalette.powered : this.parts[i].colorPalette.drained;
            this.parts[i].fillStrokeProgress = 0.0;

            this.parts[i].transition.cancel("focus").attach("chose", Combine([
                    {
                        callback: ShieldWidget.transitions.fade,
                        params: {duration: 200, target: 1.0, attribute: "fillStrokeProgress"}
                    },
                    {
                        callback: (index === i ? ShieldWidget.transitions.focusIn : ShieldWidget.transitions.focusOut),
                        params: { duration: 200, target: ( index === i ? 1.0 : 0.4) },
                    }
                ], {
                    callback: (index === i ? ShieldWidget.transitions.pulse : ShieldWidget.transitions.idle),
                    params: { duration: 1000, phaseDuration: PI * 4, range: 0.1 },
                    next: Combine([
                        {
                            callback: ShieldWidget.transitions.focusIn,
                            params: { duration: 500, target: 1.0 },
                        },
                        {
                            callback: ShieldWidget.transitions.fade,
                            params: { duration: 500, target: 0.0, attribute: "fillStrokeProgress" },
                        }
                    ], {
                        callback: (timepass, params, initial, currentState, accumulator) => {
                            this.completeTransition = false;
                            return true;
                        }
                    }),
                })
            );

        }
    }

    if (!silent) {
        this.dispatchEvent("chosed",  this,{ index: index, dictionary: ShieldWidget.vectors });
    }

}

ShieldWidget.prototype.initColorPalette = function() {

    let rCtx = this.renderCtx;
    let offset = rCtx.shieldSize;


    //this method of radialGradient not work for highly ellipsoid shape
    //because scaling technic using to fit gradient in to the shape will lead
    //to degrading border space size of two base circle in radialGradient
    //that make using fill(radialGradient) not precise enough

    this.colorPalette = {
        innerX: {
            normal:     'rgb(50, 92, 112)',
            powered:    'rgb(40, 167, 69)',
            drained:    'rgb(220, 53, 69)',
            transform:  identityMatrix
        },
        innerY: {
            normal:     'rgb(50, 92, 112)',
            powered:    'rgb(40, 167, 69)',
            drained:    'rgb(220, 53, 69)',
            transform:  identityMatrix
        },
        outer: {
            normal:     'rgb(50, 92, 112)',
            powered:    'rgb(40, 167, 69)',
            drained:    'rgb(220, 53, 69)',
            button:     'rgb(07, 22, 42)',
            transform:  identityMatrix
        },
        button: {
            normal:     'rgb(07, 22, 42)',
            powered:    'rgb(07, 22, 42)',
            drained:    'rgb(07, 22, 42)',
            transform:  identityMatrix
        },
    };

}

ShieldWidget.prototype.calculateFontSize = function(captions, rCtx) {
    const minFontSize = ~~(8 * window.devicePixelRatio);
    let currentFontSize = ~~(30 * window.devicePixelRatio);
    this.ctx.save();
    for (let i = 0; i < captions.length; i++) {
        if (captions[i]) {
            this.ctx.font = captions[i].font;

            const start = pointOnEllipseGl(captions[i].radiusX, captions[i].radiusY, captions[i].startAngle);
            const end   = pointOnEllipseGl(captions[i].radiusX, captions[i].radiusY, captions[i].endAngle);
            const space = distance(start.vertexX, start.vertexY, end.vertexX, end.vertexY) * 0.3;

            let box = TextMetric.for(this.ctx.font).measureText(captions[i].caption);
            let fontSize = Math.min(currentFontSize, parseInt(TextMetric.parseFont(this.ctx.font).fontSize));

            while (space < box.width && fontSize >= minFontSize) {
                fontSize--;
                this.ctx.font = this.ctx.font.replace(TextMetric.parseFont(this.ctx.font).fontSize, `${fontSize}px`);
                box = TextMetric.for(this.ctx.font).measureText(captions[i].caption);
            }
            currentFontSize = fontSize;
        }
    }
    this.ctx.restore();
    return currentFontSize;
}

ShieldWidget.prototype.setupPartsColorPalette = function(parts = this.parts, palette = this.colorPalette) {

    for (let i = 0; i < parts.length; i++) {
        if (!parts[i]) {
            continue;
        }
        switch (i) {
            case ShieldWidget.vectors.FRONT:
            case ShieldWidget.vectors.BACK:
                parts[i].colorPalette = palette.innerY;
                break;
            case ShieldWidget.vectors.LEFT:
            case ShieldWidget.vectors.RIGHT:
                parts[i].colorPalette = palette.innerX;
                break;
            case ShieldWidget.vectors.RESERVED:
            case ShieldWidget.vectors.SPECIAL:
                parts[i].colorPalette = palette.button;
                break;
            default:
                parts[i].colorPalette = palette.outer;
        }
        parts[i].fillStyleStart      = parts[i].colorPalette.normal;
        parts[i].strokeStyleStart    = "black";
        parts[i].fillStrokeTransform = parts[i].colorPalette.transform;
    }
}

ShieldWidget.prototype.buildShieldVectorsList = function(count) {

    let rCtx                = this.renderCtx;
    let colorPalette = this.colorPalette
    let Vector              = this.config.Vector;

    let vectors = [
        undefined,
        undefined,
        undefined,
        undefined,
        undefined,
        undefined,
        undefined,
        undefined,
    ];

    if (count === 1) {
        vectors[ShieldWidget.vectors.BUBLE] = new Vector(rCtx.radiusX, rCtx.radiusY, 0, PI_2, 0, rCtx.shieldSize);

    } else if (count === 2) {
        vectors[ShieldWidget.vectors.FRONT] =  new Vector(rCtx.radiusX, rCtx.radiusY, PI, PI_2, rCtx.spacingAngle * 4, rCtx.shieldSize);
        vectors[ShieldWidget.vectors.BACK]  =  new Vector(rCtx.radiusX, rCtx.radiusY,0, PI, rCtx.spacingAngle * 4, rCtx.shieldSize);
    } else if (count === 3) {
        vectors = this.buildShieldVectorsList(2);
        for (let v of vectors) {
            if (v) {
                v.size = v.size / 2 - rCtx.ringSpacing / 2;
                v.radiusX -= v.size + rCtx.ringSpacing;
                v.radiusY -= v.size + rCtx.ringSpacing;
            }
        }
        vectors[ShieldWidget.vectors.SPECIAL] = new Vector(rCtx.radiusX, rCtx.radiusY,0, PI_1_2, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
    }  else if (count === 4) {
        vectors[ShieldWidget.vectors.FRONT] =  new Vector(rCtx.radiusX, rCtx.radiusY, PI + rCtx.phaseShift, PI_1_1_2 + rCtx.phaseShift, rCtx.spacingAngle, rCtx.shieldSize);
        vectors[ShieldWidget.vectors.RIGHT] =  new Vector(rCtx.radiusX, rCtx.radiusY, PI_1_1_2 + rCtx.phaseShift, PI_2 + rCtx.phaseShift, rCtx.spacingAngle, rCtx.shieldSize);
        vectors[ShieldWidget.vectors.BACK]  =  new Vector(rCtx.radiusX, rCtx.radiusY,0 + rCtx.phaseShift, PI_1_2 + rCtx.phaseShift, rCtx.spacingAngle, rCtx.shieldSize);
        vectors[ShieldWidget.vectors.LEFT]  =  new Vector(rCtx.radiusX, rCtx.radiusY,PI_1_2 + rCtx.phaseShift, PI + rCtx.phaseShift, rCtx.spacingAngle, rCtx.shieldSize);
    } else if (count === 5) {
        vectors = this.buildShieldVectorsList(4);
        for (let v of vectors) {
            if (v) {
                v.size = v.size / 2 - rCtx.ringSpacing / 2;
                v.radiusX -= v.size + rCtx.ringSpacing;
                v.radiusY -= v.size + rCtx.ringSpacing;
            }
        }
        vectors[ShieldWidget.vectors.SPECIAL] = new Vector(rCtx.radiusX, rCtx.radiusY,0, PI_1_2, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
    } else if (count === 6) {
        vectors = this.buildShieldVectorsList(4);
        for (let v of vectors) {
            if (v) {
                v.size = v.size / 2 - rCtx.ringSpacing / 2;
                v.radiusX -= v.size + rCtx.ringSpacing;
                v.radiusY -= v.size + rCtx.ringSpacing;
            }
        }
        vectors[ShieldWidget.vectors.TOP]      =  new Vector(rCtx.radiusX, rCtx.radiusY, PI_1_1_2, PI_2, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
        vectors[ShieldWidget.vectors.BOTTOM]   =  new Vector(rCtx.radiusX, rCtx.radiusY,PI_1_2, PI, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
    }  else if (count === 7) {
        vectors = this.buildShieldVectorsList(6);
        vectors[ShieldWidget.vectors.SPECIAL] = new Vector(rCtx.radiusX, rCtx.radiusY,0, PI_1_2, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
    } else if (count === 8) {
        vectors = this.buildShieldVectorsList(7);
        vectors[ShieldWidget.vectors.RESERVED] = new Vector(rCtx.radiusX, rCtx.radiusY,PI, PI_1_1_2, rCtx.spacingAngle, rCtx.shieldSize / 2 - rCtx.ringSpacing / 2);
    }

    //console.log("vectors", vectors, count)

    return vectors;

}

ShieldWidget.prototype.updateFontSize = function() {
    //try to determinate adequate fontSize
    //do not true correct replace with distance on ellipse
    let rCtx = this.renderCtx;
    this.ctx.save();
    rCtx.fontSize = this.calculateFontSize(this.captions, rCtx);
    for (let i = 0; i < this.captions.length; i++) {
        if (this.captions[i]) {
            this.captions[i].font = this.captions[i].font.replace(TextMetric.parseFont(this.captions[i].font).fontSize, `${rCtx.fontSize}px`);
            TextMetric.for(this.captions[i].font).cache(this.captions[i].caption);
        }
    }
    this.ctx.restore();
}

ShieldWidget.prototype.buildShieldCaptionsList = function(parts) {
    let rCtx = this.renderCtx;
    let Caption = this.config.Caption;
    let captions = [];
    if (parts[ShieldWidget.vectors.TOP]) {
        captions[ShieldWidget.vectors.TOP]      = new Caption("Top",    rCtx.radiusX, rCtx.radiusY, PI_1_1_2, PI_2, rCtx.spacingAngle, parts[ShieldWidget.vectors.TOP].size, 0.4);
    }
    if (parts[ShieldWidget.vectors.BOTTOM]) {
        captions[ShieldWidget.vectors.BOTTOM]   = new Caption("Bottom", rCtx.radiusX, rCtx.radiusY, PI_1_2, PI, rCtx.spacingAngle, parts[ShieldWidget.vectors.BOTTOM].size, 0.4, "rtl");
    }
    if (parts[ShieldWidget.vectors.SPECIAL]) {
        captions[ShieldWidget.vectors.SPECIAL]  = new Caption("Reset",  rCtx.radiusX, rCtx.radiusY, parts[ShieldWidget.vectors.SPECIAL].startAngle, parts[ShieldWidget.vectors.SPECIAL].endAngle, rCtx.spacingAngle, parts[ShieldWidget.vectors.SPECIAL].size,  0.4, "rtl");
    }
    if (parts[ShieldWidget.vectors.RESERVED]) {
        captions[ShieldWidget.vectors.RESERVED] = new Caption("Rsv",    rCtx.radiusX, rCtx.radiusY, parts[ShieldWidget.vectors.RESERVED].startAngle, parts[ShieldWidget.vectors.RESERVED].endAngle, rCtx.spacingAngle, parts[ShieldWidget.vectors.RESERVED].size,  0.4);
    }
    return captions;
}

ShieldWidget.prototype.buildRenderCtx = function(config) {

    const pd = ~~Math.max(window.devicePixelRatio * .60, 1);

    let ctx = {
        spacingAngle:       PI_1_2 * .015,
        shieldSize:         config.size * pd,
        padding:            config.iconPagination * pd,
        phaseShift:         PI_1_4,
        width:              ~~this.canvas.width,
        height:             ~~this.canvas.height,
        iconFullSizeScale:  Math.min(this.canvas.width / this.icon.width, this.canvas.height / this.icon.height),
        iconScale:          0,
        utilization: {
            front:  1.0,
            right:  1.0,
            back:   1.0,
            left:   1.0,
            top:    1.0,
            bottom: 1.0
        },
    };
    if (!ctx.width || !ctx.height) {
        throw new Error("invalid canvas sizing");
    }
    ctx.iconScale = Math.min((ctx.width - (ctx.shieldSize+ctx.padding) * 2) / (this.icon.width), (this.canvas.height - (ctx.shieldSize+ctx.padding) * 2) / (this.icon.height));

    //console.log(ctx.iconScale, ctx.shieldSize);

    ctx.centerX =   ~~(ctx.width  / 2);
    ctx.centerY =   ~~(ctx.height / 2);
    ctx.radiusX =   ~~Math.max((this.icon.width  * ctx.iconScale) / 2  + ctx.padding + ctx.shieldSize, ctx.shieldSize + ctx.padding);
    ctx.radiusY =   ~~Math.max((this.icon.height * ctx.iconScale) / 2  + ctx.padding + ctx.shieldSize, ctx.shieldSize + ctx.padding);
    ctx.iconTranslateX = ~~((ctx.width / 2 -  this.icon.width / 2 * ctx.iconScale) + (this.icon.width * ctx.iconScale)   *  config.iconOffsetX); //config.iconOffsetX && config.iconOffsetX is scaller not px
    ctx.iconTranslateY = ~~((ctx.height / 2 - this.icon.height / 2 * ctx.iconScale) + (this.icon.height * ctx.iconScale) *  config.iconOffsetY);
    ctx.iconScale  *= config.iconScale;
    ctx.mainAxis    = Math.max(ctx.radiusX, ctx.radiusY);
    ctx.secondAxis  = Math.min(ctx.radiusX, ctx.radiusY);
    ctx.axisRatio   = ctx.secondAxis / ctx.mainAxis;
    ctx.ringSpacing = ~~(config.ringSpacing * pd);
    ctx.axisXScale  = ctx.radiusX === ctx.mainAxis ? 1 : ctx.axisRatio;
    ctx.axisYScale  = ctx.radiusY === ctx.mainAxis ? 1 : ctx.axisRatio;
    ctx.layersCount = config.count > 4 || config.count === 3 ? 2 : 1;

    //console.log("buildRenderCtx", ctx);

    return ctx;
}

ShieldWidget.prototype.draw = function(canvas, timepass) {

    this.sharedInvalidState.invalid = false;
    const rCtx = this.renderCtx;

    this.transition.update(timepass);

    canvas.save();
    canvas.translate(rCtx.iconTranslateX, rCtx.iconTranslateY);
    canvas.scale(rCtx.iconScale, rCtx.iconScale);
    canvas.fillStyle = "rgb(2, 12, 34)";
    this.icon.draw(canvas, timepass);
    canvas.restore();

    if (this.showCaption) {
        canvas.save();
        canvas.translate(rCtx.centerX, rCtx.centerY);
        canvas.globalAlpha = this.captionAlpha;
        //notify render text to texture, makes, currently, render slower so using direct render
        for (const caption of this.captions) {
            if (caption) {
                canvas.beginPath()
                caption.draw(canvas, timepass);
                canvas.closePath()
            }
        }
        canvas.restore();
    }

    canvas.save();
    canvas.translate(rCtx.centerX, rCtx.centerY);
    for (let i = 0; i < this.parts.length; i++) {
        if (this.parts[i]) {
            this.parts[i].draw(canvas, timepass);
        }
    }
    canvas.restore();

    this.sharedInvalidState.invalid |= this.transition.hasAny();
}

Object.defineProperties(ShieldWidget.prototype, {
    count: {
        get() {
            return this.config.count;
        },
        set(count) {
            count = +count;
            if (count < 0 || count > 8) {
                throw new Error("invalid count");
            }
            this.config.count = count;
            this.refresh();
        }
    },
    size: {
        get() {
            return this.config.size;
        },
        set(size) {
            size = +size;
            if (size < 0 || size >= (Math.min(this.renderCtx.width, this.renderCtx.height) / 2)) {
                throw new Error("invalid size");
            }
            this.config.size = size;
            this.refresh();
        }
    }
});


WidgetLayout.defaultLookupTable = {
    "box":   WidgetLayoutBox,
    "rows":  WidgetLayoutRows,
    "cols":  WidgetLayoutCols,
    "table": WidgetLayoutTable,
}

WidgetLayout.types = {
    BOX:    "box",
    ROWS:   "rows",
    COLS:   "cols",
    TABLE:  "table",
    AUTO:   "auto",
}

WidgetLayout.defaults = {
    type:    WidgetLayout.types.COLS,
    regions: [],
}

function WidgetLayout(layoutSchema = WidgetLayout.defaults, lookupTable = WidgetLayout.defaultLookupTable) {
    layoutSchema = Object.assign({}, WidgetLayout.defaults, layoutSchema);
    if (!WidgetLayout.defaultLookupTable[layoutSchema.type]) {
        throw new Error(`layout ${layoutSchema.type} doesn't exist`);
    }
    return new WidgetLayout.defaultLookupTable[layoutSchema.type](layoutSchema);
}

WidgetLayoutBoxTrait.defaults = {
    padding: 0,
    paddingLeft:    undefined,
    paddingRight:   undefined,
    paddingTop:     undefined,
    paddingBottom:  undefined
}

function WidgetLayoutBoxTrait(layoutSchema = WidgetLayoutBoxTrait.defaults) {
    this.scheme = layoutSchema = mixin.copyMissing(layoutSchema, WidgetLayoutBoxTrait.defaults);

    layoutSchema.paddingLeft    = layoutSchema.paddingLeft === undefined    ? layoutSchema.padding : layoutSchema.paddingLeft;
    layoutSchema.paddingRight   = layoutSchema.paddingRight === undefined   ? layoutSchema.padding : layoutSchema.paddingRight;
    layoutSchema.paddingTop     = layoutSchema.paddingTop === undefined     ? layoutSchema.padding : layoutSchema.paddingTop;
    layoutSchema.paddingBottom  = layoutSchema.paddingBottom === undefined  ? layoutSchema.padding : layoutSchema.paddingBottom;

}

WidgetLayoutBoxTrait.prototype.calculateInnerBox = function(box, layoutSchema = this.scheme) {
    //return { x: box.x + layoutSchema.paddingLeft, y: box.y + layoutSchema.paddingTop, w: box.w - (layoutSchema.paddingLeft + layoutSchema.paddingRight), h: box.h - (layoutSchema.paddingTop + layoutSchema.paddingBottom) };
    const pd =  ~~Math.max(window.devicePixelRatio, 1);
    return {
        x: box.x + ~~(layoutSchema.paddingLeft * pd),
        y: box.y + ~~(layoutSchema.paddingTop * pd),
        w: box.w - ~~((layoutSchema.paddingLeft + layoutSchema.paddingRight) * pd),
        h: box.h - ~~((layoutSchema.paddingTop + layoutSchema.paddingBottom) * pd),
    };
}

function WidgetLayoutBox(layoutSchema) {
    WidgetLayoutBoxTrait.call(this, layoutSchema);
    this.scheme = layoutSchema;

    this.innerBox = null;
}

mixin(WidgetLayoutBox, WidgetLayoutBoxTrait);

WidgetLayoutBox.prototype.calculate = function(box, result = []) {
    this.innerBox = this.calculateInnerBox(box, this.scheme);
    result.push(Object.assign({}, this.innerBox));
    return result;
}


WidgetLayoutRows.defaults = {
    regions:        [],
    regionDefault:  {},
}

WidgetLayoutRows.regionDefault = {
    type:       WidgetLayout.types.BOX,
    flex:       1,
}

function WidgetLayoutRows(layoutSchema = WidgetLayoutRows.defaults) {
    layoutSchema = Object.assign({}, WidgetLayoutRows.defaults, layoutSchema);

    WidgetLayoutBoxTrait.call(this, layoutSchema);

    //todo normalize repeat

    this.budget = 0;
    for (let region of layoutSchema.regions) {
        mixin.copyMissing(region, layoutSchema.regionDefault, WidgetLayoutCols.regionDefault);
        region._handler = new WidgetLayout(region);
        this.budget += region.flex;
    }

    this.scheme = layoutSchema;
    this.innerBox = null;
}

mixin(WidgetLayoutRows, WidgetLayoutBoxTrait);

WidgetLayoutRows.prototype.calculate = function(box, result = []) {

    this.innerBox = this.calculateInnerBox(box, this.scheme);

    let yOffset = this.innerBox.y;
    for (const region of this.scheme.regions) {
        const part = region.flex / this.budget;
        const h = part * this.innerBox.h;
        region._handler.calculate({ x: this.innerBox.x, y: ~~yOffset, w: this.innerBox.w, h: ~~h }, result);
        yOffset += h;
    }

    return result;
}

WidgetLayoutCols.defaults = {
    regions: [],
    regionDefault: {},
}

WidgetLayoutCols.regionDefault = {
    type:       WidgetLayout.types.BOX,
    flex:       1,
}

function WidgetLayoutCols(layoutSchema = WidgetLayoutCols.defaults) {
    layoutSchema = Object.assign({}, WidgetLayoutCols.defaults, layoutSchema);

    WidgetLayoutBoxTrait.call(this, layoutSchema);

    //todo normalize repeat

    this.budget = 0;
    for (let region of layoutSchema.regions) {
        mixin.copyMissing(region, layoutSchema.regionDefault, WidgetLayoutCols.regionDefault);
        region._handler = new WidgetLayout(region);
        this.budget += region.flex;
    }

    this.scheme = layoutSchema;
    this.innerBox = null;
}

mixin(WidgetLayoutCols, WidgetLayoutBoxTrait);

WidgetLayoutCols.prototype.calculate = function(box, result = []) {

    this.innerBox = this.calculateInnerBox(box, this.scheme);

    let xOffset = this.innerBox.x;
    let wBudget = this.innerBox.w;
    for (const region of this.scheme.regions) {
        const part = region.flex / this.budget;
        const w = part * this.innerBox.w;
        region._handler.calculate({ x: ~~xOffset, y: this.innerBox.y, w: ~~w, h: this.innerBox.h }, result);
        xOffset += w;
    }

    return result;
}

WidgetLayoutTable.defaults = {
    rowCount:           1,
    colCount:           1,
    regions:            [], //cells
    regionRow:          [],
    regionCol:          [],
    regionDefault:      {},
    regionDefaultRow:   {},
    regionDefaultCol:   {},
    cellPadding:        WidgetLayoutBox.defaults,
}

WidgetLayoutTable.regionDefaultRow = {
    type:       WidgetLayout.types.ROWS,
    flex:       1,
}

WidgetLayoutTable.regionDefaultCol = {
    type:       WidgetLayout.types.COLS,
    flex:       1,
}

function WidgetLayoutTable(layoutSchema = WidgetLayoutTable.defaults) {
    layoutSchema = Object.assign({}, WidgetLayoutTable.defaults, layoutSchema);

    WidgetLayoutBoxTrait.call(this, layoutSchema);

    let colConfig = Object.assign({}, WidgetLayoutTable.regionDefaultCol); //just handler
    colConfig.regions = layoutSchema.regionCol;
    colConfig.regions.length = layoutSchema.colCount;
    for (let index = 0; index < colConfig.regions.length; index++) {
        //there we unable to use array.forEach due "array elided" effect. added slots (if added) for new element via
        //colConfig.regions.length = layoutSchema.colCount; not yet "exist" in array and not iterable
        if (colConfig.regions[index] === undefined) {
            colConfig.regions[index] = {};
        }
        mixin.copyMissing(colConfig.regions[index], layoutSchema.regionDefaultCol, layoutSchema.regionDefault, { flex: 1, type: "box" });
    }


    let rowConfig = Object.assign({}, WidgetLayoutTable.regionDefaultRow);
    rowConfig.regions = layoutSchema.regionRow;
    rowConfig.regions.length = layoutSchema.rowCount;
    for (let index = 0; index < rowConfig.regions.length; index++) {
        //there we unable to use array.forEach due "array elided" effect. added slots (if added) for new element via
        //colConfig.regions.length = layoutSchema.colCount; not yet "exist" in array and not iterable
        if (rowConfig.regions[index] === undefined) {
            rowConfig.regions[index] = {};
        }
        Object.assign(rowConfig.regions[index], colConfig);
        mixin.copyMissing(rowConfig.regions[index], layoutSchema.regionDefaultRow, layoutSchema.regionDefault, { flex: 1 });
        rowConfig.regions[index].regions = colConfig.regions;
    }
    rowConfig._handler = new WidgetLayout(rowConfig);


    layoutSchema.regions = [rowConfig];

    this.scheme = layoutSchema;
    this.innerBox = null;
}

mixin(WidgetLayoutTable, WidgetLayoutBoxTrait);

WidgetLayoutTable.prototype.calculate = function(box, result = []) {

    this.innerBox = this.calculateInnerBox(box, this.scheme);

    for (const region of this.scheme.regions) {
        region._handler.calculate(this.innerBox, result);
    }

    return result;
}


PDSWidgetHeadLineralThemeTrait.defaults = {
    fillStyle:          "rgba(255, 255, 255, 0.2)",
    selectorFillStyle:  "silver",
    anchorPoint:         0.85,
    selectorScale:       0.1,
    fontFillStyle:      "silver",
    fontFillStyleHigh:  "silver",
    fontFillStyleLow:   "silver",
}

function PDSWidgetHeadLineralThemeTrait(boundingBox, config = PDSWidgetHeadLineralThemeTrait.defaults) {
    config = mixin.copyMissing(config, PDSWidgetHeadLineralThemeTrait.defaults);
}

PDSWidgetHeadLineralThemeTrait.prototype.buildRenderCtx = function (config, boundingBox, renderCtxChain = this.renderCtx) {

    const actualPB = renderCtxChain.boundingBox;
    const selectorScale = ~~(Math.min(actualPB.w, actualPB.h) * config.selectorScale );
    const zeroLevel = config.zeroAt === PDSWidgetHead.zeroAt.CENTER ? ~~(actualPB.h / 2 + actualPB.y) : actualPB.y + actualPB.h; //if there 4 direction?*/
    const valueLabelFontMetric =  TextMetric.fitInBox(config.font, "100", ~~(selectorScale * 2 * 1.5), ~~(selectorScale * 1.5), 8, 35); //fixme

    return {
        selectorScale,
        anchorStart:            ~~(actualPB.x + selectorScale * 0.6),
        anchorEnd:              ~~((actualPB.x + actualPB.w) - selectorScale * 0.6),
        zeroLevel,
        fontFillStyleLow:       config.fontFillStyleLow  || config.fontFillStyle,
        fontFillStyleHigh:      config.fontFillStyleHigh || config.fontFillStyle,
        highCornerFillStyle:    renderCtxChain.zeroAt === PDSWidgetHead.zeroAt.END ? config.fontFillStyleHigh : config.fontFillStyleLow,
        lowCornerFillStyle:     config.fontFillStyleLow,
        valueLabelFontMetric,
        valueLabelFontMetricActualBoundingBoxAscent: ~~valueLabelFontMetric.box.actualBoundingBoxAscent,
        collisionBody: {
            x: actualPB.x,
            y: ~~(zeroLevel - selectorScale * config.collisionBodyScale),
            w: actualPB.w,
            h: ~~(selectorScale * 2 * config.collisionBodyScale),
        },

    };

}

PDSWidgetHeadLineralThemeTrait.prototype.draw = function (canvas, timepass) {

    //todo move local var calculations to renderCtx

    const ctx            = this.renderCtx;
    const value =  -this._renderValue;
    const yValue=  ~~(value * this.renderCtx.range);
    const yPos           =  this.renderCtx.zeroLevel + yValue;

    //console.warn("redraw", "PDSWidgetHeadLineralThemeTrait.prototype.draw", this.config.id, this.renderCtx);

    canvas.save();

    canvas.fillStyle = this.config.fillStyle;
    if (this._renderValue > 0) { //high
        canvas.fillRect(this.renderCtx.boundingBox.x, yPos, this.renderCtx.boundingBox.w, -yValue);
    } else if (this._renderValue < 0) { //low
        canvas.fillRect(this.renderCtx.boundingBox.x, this.renderCtx.centerY, this.renderCtx.boundingBox.w, yValue);
    }

    canvas.fillStyle = this.config.selectorFillStyle;


    performance.mark("pathStartFast")
    //faster
    canvas.save();
    canvas.translate(this.renderCtx.anchorStart, yPos);
    this.drawTriangle(canvas, -this.renderCtx.selectorScale);
    canvas.restore();
    canvas.translate(this.renderCtx.anchorEnd,   yPos);
    this.drawTriangle(canvas, this.renderCtx.selectorScale);
    canvas.restore();
    performance.measure("pathFast", "pathStartFast")

/*    performance.mark("pathStartSlow")
    //SLOWER most of test show it 100% slower
    canvas.save();
    canvas.translate(0, yPos);
    canvas.fill(this.renderCtx.triangleEdgePath);
    canvas.restore();
    performance.measure("pathSlow", "pathStartSlow")*/

    if (this.config.drawValue) {
        canvas.save();
        //todo move params to rctx
        this.drawValue(canvas, timepass, yValue, yPos);
        canvas.restore();
    }


    if (this.config.DEBUG_DISPLAY_COLLISION_BODY) {
        canvas.fillRect(this.renderCtx.collisionBody.x, this.renderCtx.collisionBody.y, this.renderCtx.collisionBody.w, this.renderCtx.collisionBody.h);
    }
}

PDSWidgetHeadLineralThemeTrait.prototype.drawValue = function(canvas, timepass, yValue, yPos) {
    const rCtx = this.renderCtx;
    canvas.textAlign    = "center";
    canvas.textBaseline = 'middle';
    //canvas.globalCompositeOperation = "difference";
    canvas.font        = rCtx.valueLabelFontMetric.font;
    let value = (this._renderValue || 0) * 100;
    if (this.locked) {
        canvas.fillStyle    = rCtx.highCornerFillStyle;
        this.drawValueLabel(canvas, value, rCtx.centerX, rCtx.boundingBox.y + rCtx.boundingBox.h - rCtx.valueLabelFontMetricActualBoundingBoxAscent);
        canvas.fillStyle    = rCtx.lowCornerFillStyle;
        this.drawValueLabel(canvas, value, rCtx.centerX, rCtx.boundingBox.y + rCtx.valueLabelFontMetricActualBoundingBoxAscent);
    } else {
        let offset;
        if (rCtx.valueLabelFontMetric.box.actualBoundingBoxAscent * 2 < Math.abs(yValue)) {
            canvas.fillStyle    = this.config.fontFillStyleHigh;
            offset = rCtx.valueLabelFontMetricActualBoundingBoxAscent;
        } else {
            canvas.fillStyle    = this.config.fontFillStyleLow;
            offset = -rCtx.valueLabelFontMetricActualBoundingBoxAscent;
        }
        if (this._renderValue < 0) {
            offset = -offset;
        }
        this.drawValueLabel(canvas, value, rCtx.centerX, yPos + offset);
    }
}

PDSWidgetHeadLineralThemeTrait.prototype.drawValueLabel = function(canvas, value, centerX, centerY) {

    canvas.beginPath();
    canvas.fillStyle    = this.renderCtx.fillStyle;
    canvas.fillText(value.toFixed(0).toString(), centerX, centerY);

}

PDSWidgetHeadLineralThemeTrait.prototype.drawTriangle = function(canvas, triangleEdge) {
    const halfEdge = ~~Math.abs(triangleEdge/ 2);
    canvas.beginPath();
    canvas.moveTo(triangleEdge, 0);
    canvas.lineTo(triangleEdge, -halfEdge);
    canvas.lineTo(0, 0);
    canvas.moveTo(triangleEdge, 0);
    canvas.lineTo(triangleEdge, halfEdge);
    canvas.lineTo(0, 0);
    canvas.fill();
}

PDSWidgetHeadLineralThemeTrait.prototype.buildTriangleCursor = function() {

    const halfEdge = Math.abs(this.renderCtx.selectorScale / 2);
    let path = new Path2D();

    path.moveTo(this.renderCtx.anchorStart-this.renderCtx.selectorScale, 0);
    path.lineTo(this.renderCtx.anchorStart-this.renderCtx.selectorScale, -halfEdge);
    path.lineTo(this.renderCtx.anchorStart, 0);
    path.moveTo(this.renderCtx.anchorStart-this.renderCtx.selectorScale, 0);
    path.lineTo(this.renderCtx.anchorStart-this.renderCtx.selectorScale, halfEdge);
    path.lineTo(this.renderCtx.anchorStart, 0);

    path.moveTo(this.renderCtx.anchorEnd+this.renderCtx.selectorScale, 0);
    path.lineTo(this.renderCtx.anchorEnd+this.renderCtx.selectorScale, -halfEdge);
    path.lineTo(this.renderCtx.anchorEnd, 0);
    path.moveTo(this.renderCtx.anchorEnd+this.renderCtx.selectorScale, 0);
    path.lineTo(this.renderCtx.anchorEnd+this.renderCtx.selectorScale, halfEdge);
    path.lineTo(this.renderCtx.anchorEnd, 0);

    return path;
}

PDSWidgetHeadLineralThemeTrait.prototype.updateCollisionBodyByValue = function(value, body = this.renderCtx.collisionBody) {
    body.y = ~~(this.renderCtx.zeroLevel + (-value * this.renderCtx.range) - body.h / 2);
    if (body === this.renderCtx.collisionBody && this.lastEvent) {
        this.updateHover(crossingBoundingBox(this.lastEvent.x, this.lastEvent.y, this.renderCtx.collisionBody));
    }
    this.invalidate();
}

PDSWidgetHeadLineralThemeTrait.prototype.calculateValue = function(synt) {
    return Math.min(Math.max(-((synt.y - this.renderCtx.zeroLevel) / this.renderCtx.range), this.renderCtx.minValue), this.renderCtx.maxValue);
}

PDSWidgetHeadRadialThemeTrait.defaults = {
    startAngle:             PI,
    endAngle:               PI - PI / 32,
    drawValue:              true,
    drawSelector:           true,
    selectorScale:          0.15, //relative to radios
    selectorFillStyle:      "white",
    minFontSize:            8,
    maxFontSize:            100,
    fontSizeOffset:         0,
    fontFillColor:          "white",
    fillColor:              "rgb(135, 205, 250)",
    DEBUG_DISPLAY_TEXT_BOX: false
}

function PDSWidgetHeadRadialThemeTrait(boundingBox, config = PDSWidgetHeadRadialTheme.defaults) {
    mixin.copyMissing(config, PDSWidgetHeadRadialThemeTrait.defaults);
}

PDSWidgetHeadRadialThemeTrait.prototype.buildRenderCtx = function (config, boundingBox, renderCtxChain = this.renderCtx) {

    const radius        = ~~(Math.min(boundingBox.w, boundingBox.h) / 2);
    const radiusHalf    = ~~((Math.min(boundingBox.w, boundingBox.h) / 2) / 2);
    const radiusMiddle  = ~~(radiusHalf + (radius - radiusHalf) / 2);
    const textBoxEdge   = ~~(Math.sqrt(2 * Math.pow(radiusHalf, 2)));
    const rangeRaw      = config.endAngle - config.startAngle;

    const sinAngle = Math.sin(config.startAngle);
    const cosAngle = Math.cos(config.startAngle);

    const zeroAngle  = config.startAngle;
    const rangeAngle= config.startAngle === config.endAngle ? PI_2 : (rangeRaw < 0 ? PI_2 + rangeRaw : rangeRaw);

    let rCtx = {
        radius,
        radiusHalf,
        radiusMiddle,
        startAngle   :  config.startAngle,
        endAngle     :  config.endAngle,
        rangeAngle,
        zeroAngle,
        zeroAngleOffset :  0,
        textBoxEdge,
        valueLabelFontMetric         : TextMetric.fitInBox(config.font, "100", textBoxEdge, textBoxEdge, config.minFontSize, config.maxFontSize),
        collisionBody: {
            x: renderCtxChain.centerX + ~~(radiusMiddle*cosAngle - radius / 2),
            y: renderCtxChain.centerY + ~~(radiusMiddle*sinAngle - radius / 2),
            w: radius,
            h: radius,
        },
        cursorTriangleEdge: ~~(radiusHalf * config.selectorScale),

        //render values
        angle: zeroAngle,
        sinAngle,
        cosAngle
    };

    rCtx.valueLabelFontMetric.font      = TextMetric.normalize(rCtx.valueLabelFontMetric.font, { size: Math.max(parseInt(TextMetric.parseFont(rCtx.valueLabelFontMetric.font).fontSize) -  config.fontSizeOffset, 8) + 'px'    });
    rCtx.valueLabelFontMetric.box.width = rCtx.valueLabelFontMetric.box.width - (rCtx.valueLabelFontMetric.box.width - TextMetric.for(rCtx.valueLabelFontMetric.font).measureText("100").width) / 2;

    if (config.zeroAt === PDSWidgetHead.zeroAt.CENTER) {
        rCtx.rangeAngle = rCtx.rangeAngle / 2;
        let newStartAngle = rCtx.startAngle + rCtx.rangeAngle;
        newStartAngle = newStartAngle > PI_2 ? newStartAngle - PI_2 : newStartAngle;
        rCtx.cosAngle = Math.cos(newStartAngle);
        rCtx.sinAngle = Math.sin(newStartAngle);
        rCtx.zeroAngle = rCtx.angle = newStartAngle;
        rCtx.zeroAngleOffset =  rCtx.rangeAngle;
        rCtx.collisionBody.x = ~~(renderCtxChain.centerX + radiusMiddle*rCtx.cosAngle - radius / 2);
        rCtx.collisionBody.y = ~~(renderCtxChain.centerY + radiusMiddle*rCtx.sinAngle - radius / 2);
    }

    return rCtx;
}

PDSWidgetHeadRadialThemeTrait.prototype.draw = function (canvas, timepass) {

    const rCtx = this.renderCtx;
    const endAngle = rCtx.zeroAngle + rCtx.rangeAngle * this._renderValue;


    //console.warn("redraw", "PDSWidgetHeadRadialThemeTrait.prototype.draw", this.config.id, rCtx);

    canvas.save();
    canvas.beginPath();
    if (this._renderValue > 0) {
        canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radius, rCtx.zeroAngle, endAngle);
        canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radiusHalf, endAngle, rCtx.zeroAngle, true);
    } else {
        canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radius, rCtx.zeroAngle, endAngle, true);
        canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radiusHalf, endAngle, rCtx.zeroAngle);
    }
    canvas.fillStyle = this.config.fillColor;
    canvas.fill("evenodd");
    canvas.restore();

    if (this.config.DEBUG_DISPLAY_TEXT_BOX) {
        canvas.save();
        canvas.fillStyle = "gray";
        canvas.fillRect(rCtx.centerX - ~~(rCtx.textBoxEdge / 2), rCtx.centerY - ~~(rCtx.textBoxEdge / 2), rCtx.textBoxEdge, rCtx.textBoxEdge);
        canvas.restore();
    }

    if (this.config.drawSelector) {
        canvas.save();
        const fitPoint = ~~(rCtx.radiusHalf + rCtx.cursorTriangleEdge * 0.5);
        canvas.translate(rCtx.centerX + ~~(fitPoint*rCtx.cosAngle), rCtx.centerY + ~~(fitPoint*rCtx.sinAngle));
        canvas.rotate(rCtx.angle + PI);
        this.drawTriangle(canvas, rCtx.cursorTriangleEdge);
        canvas.fillStyle = this.config.selectorFillStyle;
        canvas.fill();
        canvas.restore();
    }

    if (this.config.drawValue) {
        canvas.save();
        this.drawValueLabel(canvas, this._renderValue * 100, rCtx.centerX, rCtx.centerY);
        canvas.restore();
    }

    if (this.config.DEBUG_DISPLAY_COLLISION_BODY) {
        canvas.save();
        canvas.fillStyle = "white";
        canvas.fillRect(rCtx.collisionBody.x, rCtx.collisionBody.y, rCtx.collisionBody.w, rCtx.collisionBody.h);
        canvas.restore();
    }
}

PDSWidgetHeadRadialThemeTrait.prototype.drawTriangle = function(canvas, triangleEdge) {
    const halfEdge = ~~Math.abs(triangleEdge/ 2);
    canvas.beginPath();
    canvas.moveTo(triangleEdge, 0);
    canvas.lineTo(triangleEdge, -halfEdge);
    canvas.lineTo(0, 0);
    canvas.moveTo(triangleEdge, 0);
    canvas.lineTo(triangleEdge, halfEdge);
    canvas.lineTo(0, 0);
    canvas.fill();
}

PDSWidgetHeadRadialThemeTrait.prototype.updateCollisionBodyByValue = function(value, body = this.renderCtx.collisionBody) {
    const rCtx = this.renderCtx;
    const angle = rCtx.angle =  rCtx.zeroAngle + rCtx.rangeAngle * value;

    const cos = rCtx.cosAngle = Math.cos(angle);
    const sin = rCtx.sinAngle = Math.sin(angle);

    body.x = ~~(rCtx.centerX + rCtx.radiusMiddle*cos - body.w / 2);
    body.y = ~~(rCtx.centerY + rCtx.radiusMiddle*sin - body.h / 2);
    //console.log("updateCollisionBodyByValue", body, angle, value)
    if (value === undefined) {
        throw new Error("undefined");
    }
    if (body === rCtx.collisionBody && this.lastEvent) {
        this.updateHover(crossingBoundingBox(this.lastEvent.x, this.lastEvent.y, rCtx.collisionBody));
    }
    this.invalidate();
}

//todo fix nonlinearity of value range if clipAngle is set
PDSWidgetHeadRadialThemeTrait.prototype.calculateValue = function(synt) {
    const rCtx = this.renderCtx;
    let direction = directionRad(synt.x - rCtx.centerX, synt.y - rCtx.centerY); // 0 - PI_2
    if (direction < rCtx.startAngle) {
        direction += PI_2 - rCtx.startAngle;
    } else {
        direction -= rCtx.startAngle;
    }
    direction -= rCtx.zeroAngleOffset;
    let value = Math.min(Math.max(direction / rCtx.rangeAngle, rCtx.minValue), rCtx.maxValue);
    //fixme this check not exact correct if zeroAt === CENTER but is seems its work as is
    if (value < 0.25 && this._renderValue > 0.75) {
        return rCtx.maxValue;
    } else if (this._renderValue < 0.25 && value > 0.75) {
        return rCtx.minValue;
    }
    return value;
}

PDSWidgetHeadRadialThemeTrait.prototype.drawValueLabel = function(canvas, value, centerX, centerY) {

    canvas.fillStyle    = this.config.fontFillColor;
    canvas.textAlign    = "center";
    canvas.textBaseline = "middle";
    canvas.font         = this.renderCtx.valueLabelFontMetric.font;
    canvas.fillText((~~value).toString(), centerX, centerY);

}

PDSWidgetHead.transitions = {
    update: (timepass, params, initial, currentState, accumulator) => {
        const range = params.target - initial._renderValue;
        currentState._renderValue = initial._renderValue + (accumulator / params.duration * range);
        if (accumulator >= params.duration) {
            currentState._renderValue = params.target;
            return true;
        }
    }
}

PDSWidgetHead.changeEventPolicy = {
    ON_RELEASE: 1,
    ON_CHANGE:  2,
}

PDSWidgetHead.zeroAt = {
    CENTER:    1,
    END:       2,
}

PDSWidgetHead.defaults = {
    padding:                        10,
    collisionBodyScale:             2.0,
    font:                           "bold 25px sans-serif",
    changeEventPolicy:              PDSWidgetHead.changeEventPolicy.ON_RELEASE,
    zeroAt:                         PDSWidgetHead.zeroAt.CENTER,
    drawValue:                      false,
    themeConstructor:               PDSWidgetHeadLineralThemeTrait,
    DEBUG_DISPLAY_COLLISION_BODY:   false,
}

function PDSWidgetHead(boundingBox, config = PDSWidgetHead.defaults) {

    this.box = boundingBox;

    config = Object.assign({}, PDSWidgetHead.defaults, config);

    if (!EventDispatcherTrait.isInited(this)) { //in case of multiple inheritance
        EventDispatcherTrait.call(this);
    }

    if (!WidgetInvalidateTrait.isInited(this)) {
        WidgetInvalidateTrait.call(this);
    }

    //event must be inited before or invalidate will fail
    mixin(this, config.themeConstructor);                       //mix theme methods
    config.themeConstructor.call(this, boundingBox, config);    //init theme

    this.renderCtx = this.buildRenderCtx(config, boundingBox, this.renderCtx = {});

    //console.info("this.renderCtx", this.renderCtx, config)

    this.config         = config;
    this._value         = undefined;
    this._renderValue   = undefined;
    this.locked = this.hover = false;
    this.lockIdentifyer = undefined; //mouse or touchId

    this.transition = new Transition(this);

    //preset this._renderValue first to prevent glitching animation
    this._value = this._renderValue = this.config.defaultValue === undefined ? 0 : this.config.defaultValue;

}

mixin(PDSWidgetHead, EventDispatcherTrait, WidgetInvalidateTrait);


PDSWidgetHead.prototype.buildRenderCtx = function(config, boundingBox, renderCtxChain = this.renderCtx) {

    let padding = config.padding;
    if (padding * 2 >= boundingBox.w || padding * 2 >= boundingBox.h) {
        padding = 0;
    }

    const actualPB = {
        x: boundingBox.x + padding,
        y: boundingBox.y + padding,
        w: ~~(boundingBox.w - padding * 2),
        h: ~~(boundingBox.h - padding * 2)
    };

    renderCtxChain = {
        boundingBox:    actualPB,
        centerX:        ~~(actualPB.w / 2 + actualPB.x),
        centerY:        ~~(actualPB.h / 2 + actualPB.y),
        halfY:          ~~(actualPB.h / 2),
        range:          config.zeroAt === PDSWidgetHead.zeroAt.CENTER ? ~~(actualPB.h / 2) : actualPB.h,
        minValue:       config.zeroAt === PDSWidgetHead.zeroAt.CENTER ? -1.0 : 0,
        maxValue:       config.zeroAt === PDSWidgetHead.zeroAt.CENTER ?  1.0 : 1.0,
    };

    return mixin.copyMissing(renderCtxChain, config.themeConstructor.prototype.buildRenderCtx.call(this, config, boundingBox, renderCtxChain));
}

PDSWidgetHead.prototype.draw = function(canvas, timepass) {

    this.invalid = false;

    this.transition.update(timepass);

    this.config.themeConstructor.prototype.draw.call(this, canvas, timepass);

    this.invalidateIf(this.transition.hasAny(), this);

}

PDSWidgetHead.prototype.updateHover = function(value) {
    if (value === this.hover) {
        return;
    }
    if (value) {
        //document.body.style.cursor = "pointer";
        this.hover = true;
        this.dispatchEvent("hover", this, true);
    } else {
        //document.body.style.cursor = "auto";
        this.hover = false;
        this.dispatchEvent("hover", this, false);
    }
    this.invalidate();
}

PDSWidgetHead.prototype.updateLock = function(value, identifier) {
    if (value === this.locked) {
        if (identifier !== this.lockIdentifyer) {
            console.warn("PDSWidgetHead.prototype.updateLock", "transfer to", value, "ignored but have different identifier");
        }
        return;
    }
    this.lockIdentifyer = !!value ? identifier : undefined;
    this.dispatchEvent("activate", this, { active: this.locked = !!value });
    this.invalidate(); //keep it or one last frame may be lost
}

PDSWidgetHead.prototype.updateValue = function(value, silent = false, async = false) {
    /*value = Math.min(Math.max(value, -1), 1);*/
    if (this._value === value) {
        return;
    }
    const prevValue = this._value;
    this._value = value;
    this.updateCollisionBodyByValue(value);
    if (!silent) {
        if (async) {
            //in case changeEventPolicy === PDSWidgetHead.changeEventPolicy.ON_CHANGE
            //that is main reason for async === true
            //we try prevent performance stall causing by possibility of slow change handler (in real apl it will be socket.send)
            //to do that we move event handler out of flow and ensure that only one handler will be executed in one system tick
            //with lastest available "value" and initial "prevValue"
            //we do not expect any draw by change listener so its "setTimeout" rather that "requestAnimationFrame"
            //this is not silver bullet so other problems may appear
            if (this._pendingUpdateValue === undefined) {
                let _pendingCtx = this;
                this._pendingUpdateValue = setTimeout(() => {
                    this.dispatchEvent("change", this, { prevValue, value: _pendingCtx._value });
                    this._pendingUpdateValue = undefined;
                }, 0);
            }
        } else {
            if (this._pendingUpdateValue !== undefined) {
                clearTimeout(this._pendingUpdateValue);
                this._pendingUpdateValue = undefined;
            }
            this.dispatchEvent("change", this, { prevValue, value: value });
        }
    }
}

PDSWidgetHead.prototype.setupRenderer = function(canvas) {

}

PDSWidgetHead.prototype.scaleCollisionBody = function(scale = 1) {
    return;
    this.renderCtx.collisionBody.w *= scale;
    this.renderCtx.collisionBody.h *= scale;
}

PDSWidgetHead.prototype.applyState = function(newState) {
    if (this.locked) {
        throw new Error("widget is locked");
    }
    if (newState.value !== undefined) {
        const value = (+newState.value);
        if (this._value !== value) {
            if (!Number.isFinite(value) || value < this.renderCtx.minValue || value > this.renderCtx.maxValue) {
                throw new Error("invalid PDSWidgetHead value");
            }
            this.updateValue(value, true);
            this.transition.attach("update", {
                callback: PDSWidgetHead.transitions.update,
                params:   { duration: 500, target: this._value },
            });
            this.invalidate();
        }
    }
}

PDSWidgetHead.prototype.applyEvent = function(synt, unified, original) {
    if (this.locked && synt.identifier !== this.lockIdentifyer) {
        //ignore different finger event but suppress it for better performance
        return true;
    }
    if (!crossingBoundingBox(synt.x, synt.y, this.renderCtx.collisionBody)) {
        //in some case if head move faster and screen refresh rate is not enough
        //pointer may jump off from head collisionBody so if widget is locked
        //allow some offset until pointer in range of boundingRect
        if (this.locked && crossingBoundingBox(synt.x, synt.y, this.box)) {
            //valid, move on
        } else {
            if (this.locked) {
                console.log("directions", "!crossingBoundingBox");
                this.updateLock(false, synt.identifier);
                this.updateValue(this._renderValue);
            }
            this.updateHover(false);
            this.lastEvent = synt;
            return false;
        }
    }
    this.updateHover(true);
    this.lastEvent = synt;
    let reset = false;
    switch (synt.type) {
        case "leave":
        case "touchcancel":
            if (this.locked) {
                this.updateLock(false, synt.identifier);
                this.updateValue(this._renderValue);
                reset = true;
            }
            this.updateHover(false);
            break;
        case "mouseup":
        case "touchend":
            if (this.locked) {
                this.updateLock(false, synt.identifier);
                this.updateValue(this._renderValue = this.calculateValue(synt));
                reset = true;
            }
            break;
        case "mousedown":
        case "touchstart":
            this.updateLock(true, synt.identifier);
            this.transition.cancel("update");
            this.scaleCollisionBody(2);
            this.updateCollisionBodyByValue(this._renderValue = this.calculateValue(synt));
            break;
        case "mousemove":
        case "touchmove":
            if (this.locked) {
                this._renderValue = this.calculateValue(synt);
                if (this.config.changeEventPolicy === PDSWidgetHead.changeEventPolicy.ON_CHANGE) {
                    this.updateValue(this._renderValue, false, true);
                } else {
                    this.updateCollisionBodyByValue(this._renderValue);
                }
            }
            break;
    }
    if (reset) {
        setTimeout(() => {
            this.scaleCollisionBody(0.5);
            this.updateCollisionBodyByValue(this._renderValue);
            this.invalidate();
        }, 500)
    }
    return true;
}

mixin(PDSWidgetHead, EventDispatcherTrait);

Object.defineProperties(PDSWidgetHead.prototype, {
    value: {
        set(value) {
            if (this.locked) {
                throw new Error("widget is locked");
            }
            value = +value;
            if (this._value !== value) {
                if (!Number.isFinite(value) || value < this.renderCtx.minValue || value > this.renderCtx.maxValue) {
                    console.warn("invalid value", value);
                    throw new Error("invalid PDSWidgetHead value");
                }
                this.updateValue(value);
                this.transition.attach("update", {
                    callback: PDSWidgetHead.transitions.update,
                    params:   { duration: 500, target: this._value },
                });
                this.invalidate();
            }
        },
        get() {
            return this._value;
        }
    }
});

PDSWidgetBackgroundLineralThemeTrait.defaults = {
    padding:                    10,

    steps:              10,
    middleNotchRate:    5,
    notchFillStyle:    "silver",

    borderArcRadios:            .1,
    defaultBorderStrokeStyle:  "rgb(135, 205, 250)",
    fillStyle :                "rgba(20, 20, 20, .4)",
}

function PDSWidgetBackgroundLineralThemeTrait(boundingBox, config = PDSWidgetBackgroundLineralThemeTrait.defaults) {
    config = mixin.copyMissing(config, PDSWidgetBackgroundLineralThemeTrait.defaults);
}

PDSWidgetBackgroundLineralThemeTrait.prototype.buildRenderCtx = function(boundingBox, config) {
    let padding = config.padding;
    if (padding * 2 >= boundingBox.w || padding * 2 >= boundingBox.h) {
        padding = 0;
    }

    const actualPB = { x: boundingBox.x + padding, y: boundingBox.y + padding, w: boundingBox.w - padding * 2, h: boundingBox.h - padding * 2 };
    const borderArcRadios = Math.max(~~(Math.min(actualPB.w, actualPB.h)  * config.borderArcRadios), 0);
    let renderCtx = {
        boundingBox:    actualPB,
        centerX:        ~~(actualPB.w / 2 + actualPB.x),
        centerY:        ~~(actualPB.h / 2 + actualPB.y),
        borderArcRadios,

        notchFullW:     ~~(actualPB.w * 0.4),
        notchHalfW:     ~~(actualPB.w * 0.2),
        notchQuarterW:  ~~(actualPB.w * 0.1),
        stepH:          (actualPB.h / (config.steps * 2)),
        steps:          config.steps,

        leftTopArcX:        boundingBox.x + borderArcRadios,
        leftTopArcY:        boundingBox.y + borderArcRadios,
        rightTopArcX:       boundingBox.x + boundingBox.w - borderArcRadios,
        rightTopArcY:       boundingBox.y + borderArcRadios,
        rightBottomArcX:    boundingBox.x + boundingBox.w - borderArcRadios,
        rightBottomArcY:    boundingBox.y + boundingBox.h - borderArcRadios,
        leftBottomArcX:     boundingBox.x + borderArcRadios,
        leftBottomArcY:     boundingBox.y + boundingBox.h - borderArcRadios,

        scaleRenderer:      undefined,
    }
    if (config.scaleStyle === PDSWidgetBackground.scaleStyle.zeroCenteredScale) {
        renderCtx.scaleRenderer = undefined;
    } else if (config.scaleStyle === PDSWidgetBackground.scaleStyle.zeroCenteredSwitch) {
        renderCtx.scaleRenderer = this.drawZeroCentredNotchSwitch;
        const plusFitting= TextMetric.fitInBox(config.font, "+", renderCtx.notchHalfW * 2, renderCtx.notchHalfW * 2);
        const minusFitting  = TextMetric.for(plusFitting.font).measureText("-");
        renderCtx.endSteep      = ~~((renderCtx.steps) / 2);
        renderCtx.endOfZone     = ~~(renderCtx.steps * renderCtx.stepH);
        renderCtx.centerOfZone  = ~~((renderCtx.endOfZone - renderCtx.endSteep * renderCtx.stepH) / 2 + renderCtx.endSteep * renderCtx.stepH);
        renderCtx.plusBox       = plusFitting.box;
        renderCtx.minusBox      = minusFitting;
        renderCtx.font          = plusFitting.font;
    }
    //console.warn("buildRenderCtx", renderCtx, config)
    return renderCtx;
}

PDSWidgetBackgroundLineralThemeTrait.prototype.setupRenderer = function(canvas){}

PDSWidgetBackgroundLineralThemeTrait.prototype.draw = function(canvas, timepass) {

    this.drawBorder(canvas, timepass);

    canvas.save();
    canvas.fillStyle = this.config.fillStyle;
    canvas.fillRect(this.renderCtx.boundingBox.x, this.renderCtx.boundingBox.y, this.renderCtx.boundingBox.w, this.renderCtx.boundingBox.h);
    canvas.restore();

    if (this.renderCtx.scaleRenderer) {
        canvas.save();
        this.renderCtx.scaleRenderer.call(this, canvas, timepass);
        canvas.restore();
    }
}

/*PDSWidgetBackgroundLineralThemeTrait.prototype.drawZeroCentredNotchScale = function(canvas, halfSize) {
    canvas.translate(this.renderCtx.centerX, this.renderCtx.centerY);
    canvas.shadowColor = this.config.notchShadowColor;
    canvas.shadowBlur  = this.config.notchShadowBlur; //64
    this.drawNotch(canvas, this.renderCtx.notchFullW);
    for (let i = 1; i < this.renderCtx.steps; i++) {
        const yOffset = ~~(i * this.renderCtx.stepH);
        if (i % this.config.middleNotchRate === 0) {
            this.drawMirrorNotch(canvas, this.renderCtx.notchHalfW,     yOffset);
        } else {
            this.drawMirrorNotch(canvas, this.renderCtx.notchQuarterW,  yOffset);
        }
    }
    this.drawMirrorNotch(canvas, this.renderCtx.notchFullW, ~~(this.renderCtx.steps * this.renderCtx.stepH));
}*/

PDSWidgetBackgroundLineralThemeTrait.prototype.drawZeroCentredNotchSwitch = function(canvas, halfSize) {
    canvas.translate(this.renderCtx.centerX, this.renderCtx.centerY);
    canvas.strokeStyle  =  this.config.notchFillStyle;
    canvas.fillStyle    = this.config.notchFillStyle;
    for (let i = 1; i <= this.renderCtx.endSteep; i++) {
        const yOffset = ~~(i * this.renderCtx.stepH);
        if (i % this.config.middleNotchRate === 0) {
            this.drawMirrorNotch(canvas, this.renderCtx.notchHalfW,     yOffset);
        } else {
            this.drawMirrorNotch(canvas, this.renderCtx.notchQuarterW,  yOffset);
        }
    }
    canvas.textAlign = "center";
    canvas.font      = this.renderCtx.font;
    canvas.fillText("+", 0, ~~(-this.renderCtx.centerOfZone + this.renderCtx.plusBox.actualBoundingBoxAscent  / 2));
    canvas.fillText("-", 0, ~~(this.renderCtx.centerOfZone + this.renderCtx.minusBox.actualBoundingBoxAscent));
}

PDSWidgetBackgroundLineralThemeTrait.prototype.drawNotch = function(canvas, halfSize) {
    canvas.save();
    canvas.beginPath();
    canvas.moveTo(-halfSize, 0);
    canvas.lineTo(halfSize, 0);
    canvas.closePath();
    canvas.stroke();
    canvas.restore();
}

PDSWidgetBackgroundLineralThemeTrait.prototype.drawMirrorNotch = function(canvas, halfSize, yOffset) {
    canvas.save();
    canvas.translate(0, yOffset);
    this.drawNotch(canvas, halfSize);
    canvas.restore();
    canvas.save();
    canvas.translate(0, -yOffset);
    this.drawNotch(canvas, halfSize);
    canvas.restore();
}

PDSWidgetBackgroundLineralThemeTrait.prototype.drawBorder = function(canvas, timepass) {

    const rCtx = this.renderCtx;

    let strokeStyle = this.config.defaultBorderStrokeStyle
    if (this.config.borderStrokeStyle) {
        strokeStyle = this.config.borderStrokeStyle;
    }

    if (!strokeStyle) {
        return;
    }

    canvas.save();

    canvas.lineWidth = ~~(3 * window.devicePixelRatio);
    canvas.strokeStyle = strokeStyle;

    canvas.beginPath();
    canvas.arc(rCtx.leftTopArcX, rCtx.leftTopArcY, rCtx.borderArcRadios, PI, PI_1_1_2);
    canvas.lineTo(this.box.x + this.box.w - rCtx.borderArcRadios, this.box.y);
    canvas.arc(rCtx.rightTopArcX, rCtx.rightTopArcY, rCtx.borderArcRadios, PI_1_1_2, 0);
    canvas.stroke();

    canvas.beginPath();
    canvas.arc(rCtx.rightBottomArcX, rCtx.rightBottomArcY, rCtx.borderArcRadios, 0, PI_1_2, 0);
    canvas.lineTo(this.box.x + rCtx.borderArcRadios, this.box.y + this.box.h);
    canvas.arc(rCtx.leftBottomArcX, rCtx.leftBottomArcY, rCtx.borderArcRadios, PI_1_2, PI);
    canvas.stroke();
    canvas.restore();

}


PDSWidgetBackgroundRadialThemeTrait.defaults = {
    padding:                    10,
    startAngle:                 PI,
    endAngle:                   PI - PI / 32,
    fillColor:                  "rgba(20, 20, 20, .4)",
    defaultBorderStrokeStyle:   undefined,
    borderStrokeStyle:          undefined,
    borderArcRadios:            .1,
}

function PDSWidgetBackgroundRadialThemeTrait(boundingBox, config = PDSWidgetBackgroundRadialThemeTrait.defaults) {
    config = mixin.copyMissing(config, PDSWidgetBackgroundRadialThemeTrait.defaults);
}

PDSWidgetBackgroundRadialThemeTrait.prototype.buildRenderCtx = function(boundingBox, config) {

    const borderArcRadios = ~~(Math.min(boundingBox.w, boundingBox.h)  * config.borderArcRadios);

    let renderCtx = {
        boundingBox:    boundingBox,
        centerX:        ~~(boundingBox.w / 2 + boundingBox.x),
        centerY:        ~~(boundingBox.h / 2 + boundingBox.y),
        radius:         ~~( Math.min(boundingBox.w, boundingBox.h) / 2),
        radiusHalf:     ~~((Math.min(boundingBox.w, boundingBox.h) / 2) / 2),
        startAngle:     config.startAngle,
        endAngle:       config.endAngle,

        borderArcRadios,


        //there some px offset is safety margin for partial update renderer
        leftTopArcX:        boundingBox.x + 2 + borderArcRadios,
        leftTopArcY:        boundingBox.y + 2 + borderArcRadios,
        rightTopArcX:       boundingBox.x - 2 + boundingBox.w - borderArcRadios,
        rightTopArcY:       boundingBox.y + 2 + borderArcRadios,
        rightBottomArcX:    boundingBox.x - 2 + boundingBox.w - borderArcRadios,
        rightBottomArcY:    boundingBox.y - 2 + boundingBox.h - borderArcRadios,
        leftBottomArcX:     boundingBox.x + 2 + borderArcRadios,
        leftBottomArcY:     boundingBox.y - 2 + boundingBox.h - borderArcRadios,
    }


    if (config.scaleStyle === PDSWidgetBackground.scaleStyle.zeroCenteredSwitch) {

    }
    return renderCtx;
}

PDSWidgetBackgroundRadialThemeTrait.prototype.setupRenderer = function(canvas) {

}

PDSWidgetBackgroundRadialThemeTrait.prototype.draw = function(canvas, timepass) {

    const rCtx = this.renderCtx;

    //console.warn("redraw", "PDSWidgetBackgroundRadialThemeTrait.prototype.draw", this.config.id, rCtx);

    this.drawBorder(canvas, timepass);

    canvas.save();
    canvas.beginPath();
    canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radius, rCtx.startAngle, rCtx.endAngle);
    canvas.arc(rCtx.centerX, rCtx.centerY, rCtx.radiusHalf, rCtx.endAngle, rCtx.startAngle, true);
    canvas.fillStyle = this.config.fillColor;
    canvas.fill("evenodd");
    canvas.restore();
}

PDSWidgetBackgroundRadialThemeTrait.prototype.drawBorder = function(canvas, timepass) {

    const rCtx = this.renderCtx;

    let strokeStyle = this.config.defaultBorderStrokeStyle
    if (this.config.borderStrokeStyle) {
        strokeStyle = this.config.borderStrokeStyle;
    }

    if (!strokeStyle) {
        return;
    }

    canvas.save();

    canvas.lineWidth = ~~(3 * window.devicePixelRatio);

    canvas.strokeStyle = strokeStyle;

    canvas.beginPath();
    canvas.arc(rCtx.leftTopArcX, rCtx.leftTopArcY, rCtx.borderArcRadios, PI, PI_1_1_2);
    canvas.stroke();

    canvas.beginPath();
    canvas.arc(rCtx.rightTopArcX, rCtx.rightTopArcY, rCtx.borderArcRadios, PI_1_1_2, 0);
    canvas.stroke();

    canvas.beginPath();
    canvas.arc(rCtx.rightBottomArcX, rCtx.rightBottomArcY, rCtx.borderArcRadios, 0, PI_1_2, 0);
    canvas.stroke();

    canvas.beginPath();
    canvas.arc(rCtx.leftBottomArcX, rCtx.leftBottomArcY, rCtx.borderArcRadios, PI_1_2, PI);
    canvas.stroke();

    canvas.restore();

}

PDSWidgetBackground.scaleStyle = {
    zeroCenteredScale:  1,
    zeroCenteredSwitch: 2,
    grovingScale:       3,
}

PDSWidgetBackground.defaults = {
    borderStrokeStyle:  undefined,
    font:               "normal 25px sans-serif",
    themeConstructor:   PDSWidgetBackgroundLineralThemeTrait,
    scaleStyle:         PDSWidgetBackground.scaleStyle.zeroCenteredScale,
}

function PDSWidgetBackground(boundingBox, config = PDSWidgetBackground.defaults) {
    this.box = boundingBox;
    config = Object.assign({}, PDSWidgetBackground.defaults, config);
    mixin(this, config.themeConstructor); //dynamic mixing with our theme
    config.themeConstructor.call(this, boundingBox, config); //init our theme
    if (!EventDispatcherTrait.isInited(this)) {
        EventDispatcherTrait.call(this);
    }
    if (!WidgetInvalidateTrait.isInited(this)) {
        WidgetInvalidateTrait.call(this);
    }
    this.renderCtx      = this.buildRenderCtx(boundingBox, config);
    this.config = config;
}

mixin(PDSWidgetBackground, EventDispatcherTrait, WidgetInvalidateTrait);

PDSWidgetBackground.prototype.buildRenderCtx = function(boundingBox, config) {
    let padding = config.padding;
    if (padding * 2 >= boundingBox.w || padding * 2 >= boundingBox.h) {
        padding = 0;
    }
    const actualPB = { x: boundingBox.x + padding, y: boundingBox.y + padding, w: boundingBox.w - padding * 2, h: boundingBox.h - padding * 2 };
    let renderCtx = {
        boundingBox:        actualPB,
        centerX:            ~~(actualPB.w / 2 + actualPB.x),
        centerY:            ~~(actualPB.h / 2 + actualPB.y),
        backgroundOverflow: ~~(padding / 2),
    }
    //example of calling trait method from overrided function, note local variable renderCtx must have priority over trait data
    return Object.assign(config.themeConstructor.prototype.buildRenderCtx.call(this, boundingBox, config), renderCtx);
}


PDSWidgetBackground.prototype.draw = function(canvas, timepass) {
    //console.warn("redraw", "PDSWidgetBackground.prototype.draw", this.config.id, this.renderCtx);
    this.invalid = false;
    this.config.themeConstructor.prototype.draw.call(this, canvas, timepass);
}

Object.defineProperties(PDSWidgetBackground.prototype, {
    borderStrokeStyle: {
        set(value) {
            if (this.config.borderStrokeStyle !== value) {
                this.config.borderStrokeStyle = value;
                this.invalidate(this);
            }
        },
        get()            { return this.config.borderStrokeStyle; },
    }
});

PDSWidgetLabel.defaults = {
    font:       undefined,
    fillStyle:  undefined,
}
function PDSWidgetLabel (text, boundingBox, config = PDSWidgetLabel.defaults) {
    this.text = text;

    this.config = Object.assign({}, config, boundingBox);

    this.box = boundingBox;

    this.renderCtx = {
        //it is tricky to scale font with window.devicePixelRatio as in some cases actual font size must be calculated
        //by API like TextMetric.fitInBox with already scaled geometry, so font is exception, dont self scale, it asume than fontSize already scaled
        centerX: ~~(boundingBox.x + boundingBox.w / 2),
        centerY: ~~(boundingBox.y + boundingBox.h / 2),
    }

    if (!EventDispatcherTrait.isInited(this)) {
        EventDispatcherTrait.call(this);
    }

    if (!WidgetInvalidateTrait.isInited(this)) {
        WidgetInvalidateTrait.call(this);
    }

}

mixin(PDSWidgetLabel, EventDispatcherTrait, WidgetInvalidateTrait);

PDSWidgetLabel.prototype.draw = function(canvas, timepass) {
    //console.warn("redraw", "PDSWidgetLabel.prototype.draw", this.config.id);

    this.invalid = false; //rearm invalidate

    canvas.save();
    if (this.config.font) {
        canvas.font = this.config.font;
    }
    if (this.config.fillStyle) {
        canvas.fillStyle = this.config.fillStyle;
    }
    canvas.textAlign = "center";
    canvas.fillText(this.text, this.renderCtx.centerX, this.renderCtx.centerY, this.box.w);
    canvas.restore();
}

PDSWidget.defaults = {

    head: {
        Constructor:    PDSWidgetHead,
    },
    background:     {
        Constructor:    PDSWidgetBackground,
    },
    label:  {
        Constructor:    PDSWidgetLabel,
        text:           undefined,
        fillStyle:      "white",
        font:           TextMetric.normalize(TextMetric.ctx.font, { style: "bold" })
    },

    noevents:           false,
}

PDSWidget.transitions = {

}

function PDSWidget(boundingRect, config = PDSWidget.defaults) {

    this.captionAlpha   = 1.0;

    this.config             = Object.assign({}, PDSWidget.defaults, config, boundingRect);
    this.config.label       = Object.assign({}, PDSWidget.defaults.label, config.label);
    this.config.background  = Object.assign({}, PDSWidget.defaults.background, config.background);
    this.config.head        = Object.assign({}, PDSWidget.defaults.head, config.head);

    this.box            = boundingRect;

    this.renderCtx      = this.buildRenderCtx(this.config);

    this.background     = new this.config.background.Constructor(this.renderCtx.bgBoundingBox, this.config.background);

    this.head           = new this.config.head.Constructor(this.renderCtx.bgBoundingBox, this.config.head);

    if (this.config.label.text !== undefined) {
        this.label = new this.config.label.Constructor(this.config.label.text, this.renderCtx.labelBoundingBox, this.renderCtx.labelConfig);
    }

    this.transition     = new Transition(this);

    if (!EventDispatcherTrait.isInited(this)) {
        EventDispatcherTrait.call(this);
    }

    if (!WidgetInvalidateTrait.isInited(this)) {
        WidgetInvalidateTrait.call(this);
    }

    this.events = {};
    this.globalEvents = {};

    this.head.attachEvent("change",     eventProxyBubble, this);
    this.head.attachEvent("activate",   eventProxyBubble, this);
    this.head.attachEvent("hover",      eventProxyBubble, this);

    this.head.attachEvent("invalidate", invalidateProxy, this);
    this.background.attachEvent("invalidate", invalidateProxy, this);
    if (this.label) {
        this.label.attachEvent("invalidate", invalidateProxy, this);
    }

}

mixin(PDSWidget, EventDispatcherTrait, WidgetInvalidateTrait);

PDSWidget.prototype.buildRenderCtx = function(config) {

    const mainAxis = Math.max(config.w, config.h);
    const secondAxis=   Math.min(config.w, config.h);

    let ctx = {
        bgBoundingBox:  this.box,
        width:        config.w,
        height:       config.h,
        centerX:      ~~(config.w  / 2),
        centerY:      ~~(config.h  / 2),
        mainAxis,
        secondAxis,
        axisRatio:    secondAxis / mainAxis,
        labelConfig:  config.label,
    };

    if (config.label.text) {
        const font= TextMetric.parseFont(config.label.font);
        let font2h = parseInt(font.fontSize) * window.devicePixelRatio;
        font.fontSize = font2h + "px";
        ctx.labelBoundingBox = { x: config.x, y: config.y + config.h - font2h,  w: config.w, h: font2h };
        ctx.bgBoundingBox =    { x: config.x, y: config.y,  w: config.w, h: config.h - ctx.labelBoundingBox.h - (ctx.labelBoundingBox.h ? (10 * window.devicePixelRatio) : 0) };
        ctx.labelConfig.font = TextMetric.joinFont(font);
    }

    if (!ctx.width || !ctx.height) {
        throw new Error("invalid widget sizing");
    }

    return ctx;
}

PDSWidget.prototype.setupRenderer = function(canvas) {
    if (this.background.setupRenderer) {
        this.background.setupRenderer(canvas);
    }
    if (this.head.setupRenderer) {
        this.head.setupRenderer(canvas);
    }
    if (this.label) {
        if (this.label.setupRenderer) {
            this.label.setupRenderer(canvas);
        }
    }

}

PDSWidget.prototype.applyState = function(newState) {
    this.head.applyState(newState);
}

PDSWidget.prototype.applyEvent = function(synt, unified, original) {
    if (this.head.applyEvent) {
        if (this.head.applyEvent(synt, unified, original)) {
            //head is main event rcv in widget so if it can handle it we don't need for other handler
            //also it can be in state of flux causing by animation or other interaction so it better
            //to delegate him event handling other than check it collisionBody there and decide who be a handler
            return true;
        }
    }
    if (this.background.applyEvent) {
        this.background.applyEvent(synt, unified, original);
        //failback handler
    }
    return true;
}

PDSWidget.prototype.draw = function(canvas, timepass) {

    //console.warn("redraw", "PDSWidget.prototype.draw", this.config.id);

    this.invalid = false; //rearm invalidate

    this.transition.update(timepass);

    this.background.draw(canvas, timepass);

    if (this.label) {
        this.label.draw(canvas, timepass);
    }

    this.head.draw(canvas, timepass);
}

Object.defineProperties(PDSWidget.prototype, {
    value: {
        set(value) { this.head.value = value; },
        get()            { return this.head.value;  }
    },
    locked: {
        get()            { return this.head.locked; }
    },
    borderStrokeStyle: {
        set(value) { this.background.borderStrokeStyle = value;/* this.label ? this.label.config.fillStyle = value : null;*/ },
        get()            { return this.background.borderStrokeStyle;  }
    },
    labeled: {
        get()            { return !!this.label }
    },
    labelText: {
        set(value) {
            const labeled = this.labeled;
            if (this.labeled && value) {
                this.label.text = value;
            } else if (!this.labeled && !value) {
                //nope
            } else {
                throw new Error("not implemented");
            }
        },
        get() { return this.label ? this.label.text : undefined ;  }
    },
})

PDSWidgetsPad.defaults = {

    widgetDefaults: {
        Constructor:    PDSWidget,
        head: {
            Constructor: PDSWidget.defaults.head.Constructor,
        },
        background: {
            Constructor: PDSWidget.defaults.background.Constructor,
        },
        label: {
            Constructor: PDSWidget.defaults.label.Constructor,
        }
    },

    layout: {
        type: WidgetLayout.types.AUTO
    },

    widgets:    [
        {
            id: "widget",
        }
    ],

    noevents:       false,
    noanimation:    false,

}
function PDSWidgetsPad(dom, config = PDSWidgetsPad.defaults) {

    this.dom            = dom;

    const rect = boundingClientRect(dom); //for scaled size
    //const rect = { width: dom.offsetWidth, height: dom.offsetHeight };
    const computedStyle = getComputedStyle(dom); //for possible padding
    //caution if box-sizing != border-box in may fail

    if (computedStyle.boxSizing !== "border-box") {
        throw new Error("other than 'border-box' sizing model not impl");
    }


    this.canvas         = dom.querySelector("canvas");
    //dom.offset* we must use actual size without scale effect (if it apply), to prevent canvas pixel decimation
    //remember that dom.offset* is mathematically rounded, so in the end 1px error may occur
    const width  = dom.offsetWidth  - boxStyleWidthModifier(computedStyle);
    const height = dom.offsetHeight - boxStyleHeightModifier(computedStyle);
    this.canvas.width   = ~~(width  * window.devicePixelRatio);
    this.canvas.height  = ~~(height * window.devicePixelRatio);
    this.ctx            = this.canvas.getContext("2d");

    const defaultFont = TextMetric.parseFont(this.ctx.font);
    defaultFont.fontSize = ~~(parseInt(defaultFont.fontSize) * window.devicePixelRatio) + "px";
    this.ctx.font        = TextMetric.joinFont(defaultFont);
    this.ctx.lineWidth   = ~~Math.max(window.devicePixelRatio, 1);

    this.canvas.style.width  = ~~width + "px";  //or set it to 100% in css
    this.canvas.style.height = ~~height + "px"; //or set it to 100% in css

    requestBoundingClientRect(this.canvas, this.clientRect = rect); //inital rect for temporal use, offset for event

    this.config    = Object.assign({}, PDSWidgetsPad.defaults, config);
    this.config.widgetDefaults = this.mergeWidgetCfg(PDSWidgetsPad.defaults.widgetDefaults, config.widgetDefaults ? config.widgetDefaults : {});
    this.config.layout = Object.assign({}, PDSWidgetsPad.defaults.layout, this.config.layout);

    if (this.config.layout.type === WidgetLayout.types.AUTO) {
        this.config.layout.type     = WidgetLayout.types.COLS;
        this.config.layout.regions.length   = this.config.widgets.length;
        for (let index = 0; index < this.config.layout.regions.length; index++) {
            if (this.config.layout.regions[index] === undefined) {
                this.config.layout.regions[index] = {};
            }
            mixin.copyMissing(this.config.layout.regions[index], { flex: 1 });
        }
        this.layout = new WidgetLayout(this.config.layout);
    } else {
        this.layout = new WidgetLayout(this.config.layout);
    }

    this.renderCtx = this.buildRenderCtx(this.config);

    this.widgets   = this.buildWidgets();

    this.events = {};
    this.globalEvents = {};
    this.timeouts   = {};
    this.intervals  = {};
    this.activataions = 0;
    this.hovers = 0;
    this.beforeRefreshAnimateState = undefined;
    this.refreshRequest = PendingRequest();

    if (!config.noevents) {
        this.initEvents();
    }

    if (!EventDispatcherTrait.isInited(this)) {
        EventDispatcherTrait.call(this);
    }

    if (!WidgetInvalidateTrait.isInited(this)) {
        WidgetInvalidateTrait.call(this);
    }

    this.setupWidgets();

    this.animate(!config.noanimation);
}

mixin(PDSWidgetsPad, EventDispatcherTrait, WidgetInvalidateTrait);

PDSWidgetsPad.prototype.buildRenderCtx = function(config) {

    let ctx = {
        layout:       this.layout.calculate({ x: 0, y: 0, w: this.canvas.width, h: this.canvas.height }),
        width:        this.canvas.width,
        height:       this.canvas.height,
        centerX:      this.canvas.width  / 2,
        centerY:      this.canvas.height / 2,
        mainAxis:     Math.max(this.canvas.width, this.canvas.height),
        secondAxis:   Math.max(this.canvas.width, this.canvas.height),
        widgetsCount: config.widgets.length,
        staticRenderer: new StaticRenderContext(this.canvas.width, this.canvas.height)
        /** todo interesting way to do, instead of invalidate StaticRenderContext when border is changed try
            render widgetsBg without border (ie transparent) and after that replace ordinary bg render with only border render */
    };

    if (!ctx.width || !ctx.height) {
        throw new Error("invalid canvas sizing");
    }

    ctx.axisRatio = ctx.secondAxis / ctx.mainAxis;

    return ctx;
}

PDSWidgetsPad.prototype.animate = function(animate = true) {
    if (animate) {

        if (this.nextAnimationFrameId) {
            cancelAnimationFrame(this.nextAnimationFrameId);
            this.nextAnimationFrameId = undefined;
        }

        requestBoundingClientRect(this.canvas, this.clientRect); //dangerous

        let initialDraw = true;
        let prevTime = 0;
        //WORKAROUND:
        //there is a problem: for some reason if app initialy render(loaded) with page to which the widget belongs
        //then pageX ~= screenX and offsetLeft is relative to the screen not to the document
        //but if app loaded with different page then pageX is document related and offsetLeft too
        //it is not problem if offsetLeft and pageX are hot but make problem is some of them are cached
        //so as we cache boundingClientRect(offsetLeft) - (in order to prevent layout flush) we must do it as later as possible
        //best is after swipe complete. As we not have direct control over swipe we rely on the assumption that
        //the animation will be stopped as soon as the page leaves the screen and resumed as soon as the page occupies
        //the screen. It is at this point that we cache the boundingClientRect. Possible problem belong to the css grid
        const animation = function (timestamp){
            if (initialDraw) {
                //console.warn("redraw", "PDSWidgetsPad.prototype.animate", "fullredraw",  this.config.id);
                this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
                this.renderCtx.staticRenderer.partialUpdate = false;
                this.draw(this.ctx, timestamp - prevTime);
                this.renderCtx.staticRenderer.partialUpdate = true;
                initialDraw = false;
            } else if (this.invalid) {
                //console.warn("redraw", "PDSWidgetsPad.prototype.animate", "partialredraw",  this.config.id, timestamp - prevTime);
                this.invalid = false;
                for (const widget of this.widgets) {
                    if (widget.invalid) {
                        this.ctx.clearRect(widget.box.x, widget.box.y, widget.box.w, widget.box.h);
                        widget.draw(this.ctx, timestamp - prevTime);
                    }
                }
            }
            prevTime = timestamp;
            this.nextAnimationFrameId = requestAnimationFrame(animation);
        }.bind(this);
        this.nextAnimationFrameId = requestAnimationFrame(animation);
    } else {
        if (this.nextAnimationFrameId) {
            cancelAnimationFrame(this.nextAnimationFrameId);
            this.nextAnimationFrameId = undefined;
        }
    }
}

PDSWidgetsPad.prototype.isAnimating = function() {
    return this.nextAnimationFrameId !== undefined;
}

PDSWidgetsPad.prototype.buildWidgets = function(config = this.config) {
    const rCtx = this.renderCtx;
    const count = rCtx.widgetsCount;

    let widgets = [];
    let x = rCtx.paddingHor, y = rCtx.paddingVer;
    let i = 0;
    for (let wIndex = 0; wIndex < config.widgets.length; wIndex++) {
        let actualConfig = this.mergeWidgetCfg(config.widgetDefaults, config.widgets[wIndex]);
        actualConfig.background.Constructor = StaticRenderContextHelper.wrapConstructor(actualConfig.background.Constructor, this.renderCtx.staticRenderer);
        //console.warn("PDSWidgetsPad.prototype.buildWidgets", actualConfig, rCtx.width, rCtx.height)
/*        const w = this.parseSizeCfg(actualConfig.w, rCtx.width - rCtx.paddingHor) / rCtx.widgetsCount - rCtx.paddingHor;
        const h = this.parseSizeCfg(actualConfig.h, rCtx.height - rCtx.paddingVer) - rCtx.paddingVer;*/
        let widget = new actualConfig.Constructor(this.renderCtx.layout[wIndex], actualConfig);
        widget.id = actualConfig.id;
        widgets.push(widget);
        //x += w + rCtx.paddingHor;
    }

    return widgets;
}

const PDSWidgetsPadWidgetsEventListener = {
    activate: (evtId, widget, eData, that) => {
        that.activataions += eData.active ? 1 : -1;
        that.dispatchEvent("activate", widget, eData);
    },
    hover: (evtId, widget, eData, that) => {
        that.hovers += eData ? 1 : -1;
        if (that.hovers > 0) {
            that.canvas.style.cursor = "pointer"; //requestAnimationFrame() //??
        } else {
            that.canvas.style.cursor = "auto";
        }
    }
}

PDSWidgetsPad.prototype.setupWidgets = function(widgets = this.widgets) {
    for (let widget of widgets) {
        if (widget.setupRenderer) {
            widget.setupRenderer(this.ctx);
        }
        widget.attachEvent("activate",      PDSWidgetsPadWidgetsEventListener.activate, this );
        widget.attachEvent("change",        eventProxyDirect,                           this );
        widget.attachEvent("hover",         PDSWidgetsPadWidgetsEventListener.hover,    this );
        widget.attachEvent("invalidate",    invalidateProxy,                            this );
    }
}


PDSWidgetsPad.prototype.mergeWidgetCfg = function(widgetDefaults, widgetCfg) {
    let actualConfig        = Object.assign({}, widgetDefaults, widgetCfg);
    actualConfig.label          = Object.assign({}, widgetDefaults.label, widgetCfg.label ? widgetCfg.label : {});
    actualConfig.background     = Object.assign({}, widgetDefaults.background, widgetCfg.background ? widgetCfg.background : {});
    actualConfig.head           = Object.assign({}, widgetDefaults.head, widgetCfg.head ? widgetCfg.head : {});
    return actualConfig;
}

PDSWidgetsPad.prototype.refresh = function() {
    console.warn("PDSWidgetsPad.refresh");

    if (this.beforeRefreshAnimateState === undefined) {
        this.beforeRefreshAnimateState = this.isAnimating();
        this.animate(false);
        //this.ctx.clearRect(0, 0, this.renderCtx.width, this.renderCtx.height);
    }

    //Test approach for layout computation value decrease
    //zero frame (x)

    cancelRequest(this.refreshRequest);
    const request = this.refreshRequest = PendingRequest();

    requestAnimationFrameTime(() => {  // frame x+1 before layout flush

        if (request.canceled) {
            return;
        }

        this.canvas.width   = 0;
        this.canvas.height  = 0;
        this.canvas.style.width  = '0px';
        this.canvas.style.height = '0px';
        this.ctx.font = TextMetric.defaultFont;

        requestDocumentFlushTime(() => { // frame x+2 after layout flush

            if (request.canceled) {
                return;
            }

            const rect = boundingClientRect(this.dom);
            const computedStyle = getComputedStyle(this.dom);
            //@see constructor
            const width  = this.dom.offsetWidth  - boxStyleWidthModifier(computedStyle);
            const height = this.dom.offsetHeight - boxStyleHeightModifier(computedStyle);

            requestAnimationFrameTime(() => { // frame x+3 before layout flush

                if (request.canceled) {
                    return;
                }

                this.canvas.width   = ~~(width  * window.devicePixelRatio);
                this.canvas.height  = ~~(height * window.devicePixelRatio);
                this.canvas.style.width  = (~~width)  + "px";
                this.canvas.style.height = (~~height) + "px";

                const defaultFont = TextMetric.parseFont(this.ctx.font);
                defaultFont.fontSize = ~~(parseInt(defaultFont.fontSize) * window.devicePixelRatio) + "px";
                this.ctx.font       = TextMetric.joinFont(defaultFont);
                this.ctx.lineWidth  = ~~Math.max(window.devicePixelRatio, 1);

                requestDocumentFlushTime(() => {  // frame x+4 after layout flush (because we may need and
                                                                // actualy do) query size information inside buildRenderCtx

                    if (request.canceled) {
                        return;
                    }

                    const old = this.renderCtx;
                    this.renderCtx      = this.buildRenderCtx(this.config);

                    const oldWidgets = this.widgets;
                    this.widgets = this.buildWidgets(this.config);
                    for (let index = 0; index < this.widgets.length; index++) {
                        this.widgets[index].value = oldWidgets[index].value;
                    }

                    this.setupWidgets(this.widgets);

                    this.animate(this.beforeRefreshAnimateState);  //so we introduce at least 5 frame delay before animation resume
                                                             //but in theory we remove most of the style computation
                                                             //we also add 4 more junk object to GC

                    this.beforeRefreshAnimateState = undefined;

                }, request);

            }, request);

        }, request);

    }, request);

}

/*PDSWidgetsPad.prototype.refresh = function() {
    console.warn("PDSWidgetsPad.refresh");

    const animating = this.isAnimating();
    this.animate(false);


    this.canvas.width = 0;
    this.canvas.height = 0;


    const old = this.renderCtx;

    const rect = boundingClientRect(this.dom);
    const computedStyle = getComputedStyle(this.dom);

    this.canvas.width   = rect.width  - ~~(boxStyleWidthModifier(computedStyle));
    this.canvas.height  = rect.height - ~~(boxStyleHeightModifier(computedStyle));

    //requestBoundingClientRect(this.canvas, this.clientRect); //moved to animate

    this.renderCtx      = this.buildRenderCtx(this.config);

    const oldWidgets = this.widgets;
    this.widgets = this.buildWidgets(this.config);
    for (let index = 0; index < this.widgets.length; index++) {
        this.widgets[index].value = oldWidgets[index].value;
    }

    this.setupWidgets(this.widgets);
    this.animate(animating);
}*/

PDSWidgetsPad.prototype.initEvents = function() {

    let touchToWidgetIndex = new Map();

    let widgetActivationCount= [];
    widgetActivationCount.length = this.widgets.length;
    widgetActivationCount.fill(0);

    const singleTouchEventHandler = (unified, e, rect) => {


        const rCtx = this.renderCtx;

        const normX = (unified.pageX - rect.left) * window.devicePixelRatio;
        const normY = (unified.pageY - rect.top)  * window.devicePixelRatio;

        //console.log(unified.pageX, unified.pageY, rect.left, rect.top, unified, e.target.getBoundingClientRect())

        let touchIdentifier;
        if (unified !== e) {
            touchIdentifier = unified.identifier;
        } else {
            touchIdentifier = "mouse";
        }
        let activeWidgetIndex = touchToWidgetIndex.get(touchIdentifier);

        let crossingAny = false;
        for (let i = 0; i < this.widgets.length; i++) {
            let widget = this.widgets[i];
            if (crossingBoundingBox(normX, normY, widget.box)) {

                if (activeWidgetIndex !== i) {

                    //console.log("initEvents", "send enter", i);
                    widget.applyEvent({ type: "enter", x: normX, y: normY, target: widget, identifier: touchIdentifier }, unified, e);
                    widgetActivationCount[i]++;

                    if (activeWidgetIndex !== undefined) {
                        //console.log("initEvents", "send leave", activeWidgetIndex);
                        widget.applyEvent({ type: "leave", x: 0, y: 0, target: widget, identifier: touchIdentifier }, unified, e);
                        widgetActivationCount[activeWidgetIndex]--;
                    }

                    touchToWidgetIndex.set(touchIdentifier, activeWidgetIndex = i);
                }

                widget.applyEvent({ type: e.type, x: normX, y: normY, target: widget, identifier: touchIdentifier }, unified, e);
                crossingAny = true;

                break;
            }
        }

        if (activeWidgetIndex !== undefined) {
            if (!crossingAny || (e.type === "mouseup" || e.type === "mouseleave" || e.type === "touchend" || e.type === "touchcancel")) {
                //console.log("initEvents", "send leave (not crossing or cancel)", activeWidgetIndex);
                let widget = this.widgets[activeWidgetIndex];
                widget.applyEvent({
                    type: "leave",
                    x: normX,
                    y: normY,
                    target: widget,
                    identifier: touchIdentifier
                }, unified, e);
                widgetActivationCount[activeWidgetIndex]--;
                touchToWidgetIndex.set(touchIdentifier, undefined);
            }
        }

    }

    const handler = (e) => {

        if (!this.config.widgetDefaults.Constructor.prototype.applyEvent) {
            return;
        }

        const rect = this.clientRect;
        //const rect = e.target.getBoundingClientRect();

        if (e.changedTouches) {
            for (const event of e.changedTouches) {
                singleTouchEventHandler(event, e, rect);
            }
        } else {
            singleTouchEventHandler(e, e, rect);
        }

        for (let i = 0; i < this.widgets.length; i++) {
            if (widgetActivationCount[i] !== 0 && widgetActivationCount[i] !== 1) {
                //console.log("initEvents", "widgetActivationCount[i]", widgetActivationCount[i])
            }
            if (widgetActivationCount[i]) {
                if (!this.widgets[i].borderStrokeStyle) {
                    this.widgets[i].borderStrokeStyle = "white";
                }
            } else {
                if (this.widgets[i].borderStrokeStyle) {
                    this.widgets[i].borderStrokeStyle = undefined;
                }
            }
        }

        if (this.activataions) {
            //console.log("activations left", this.activataions);
            e.preventDefault();
            e.stopPropagation();
            return false;
        } else {
            e.preventDefault();
            //console.log("activations 0");
        }

    }

    this.canvas.addEventListener( "mouseleave",     this.events["mouseleave"]   = handler);
    this.canvas.addEventListener( "touchcancel",    this.events["touchcancel"]  = handler);
    this.canvas.addEventListener( "touchend",       this.events["touchend"]     = handler);
    this.canvas.addEventListener( "mouseup",        this.events["mouseup"]      = handler);
    this.canvas.addEventListener( "mousedown",      this.events["mousedown"]    = handler);
    this.canvas.addEventListener( "touchstart",     this.events["touchstart"]   = handler);

    this.canvas.addEventListener("mousemove",       this.events["mousemove"]    = handler);
    this.canvas.addEventListener("touchmove",       this.events["touchmove"]    = handler);

    this.canvas.addEventListener("contextmenu", this.events["contextmenu"] = (e) => e.preventDefault());

    removeEventListener("resize", this.globalEvents["resize"]);
    addEventListener( "resize", this.globalEvents["resize"] = (e) => {
        console.log("PDSWidget resize")
        this.refresh();
    });
}

PDSWidgetsPad.prototype.freeEvents = function(keepResize = false) {

    for (const [evtId, handler] of Object.entries(this.events)) {
        this.canvas.removeEventListener(evtId, handler);
    }

    for (const [evtId, handler] of Object.entries(this.globalEvents)) {
        if (keepResize && evtId === "resize") {
            continue;
        }
        removeEventListener(evtId, handler);
    }

    for (let widget of this.widgets) {
        widget.applyEvent({ type: "mouseleave", x: widget.renderCtx.centerX, y: widget.renderCtx.centerY, target: widget }, null, null);
        //widget.value = 0; //why?
        widget.borderStrokeStyle   = undefined;
    }

}

PDSWidgetsPad.prototype.free = function() {

    if (this.nextAnimationFrameId) {
        cancelAnimationFrame(this.nextAnimationFrameId);
    }

    this.freeEvents();

    for (const [,timerId] of Object.entries(this.timeouts)) {
        clearTimeout(timerId);
    }

    for (const [,intervalId] of Object.entries(this.intervals)) {
        clearInterval(intervalId);
    }

    this.events = {};
    this.globalEvents = {};
    this.attachedEvents = {};

}

PDSWidgetsPad.prototype.draw = function(canvas, timepass) {
    //console.warn("redraw", "PDSWidgetsPad.prototype.draw", this.config.id);
    this.invalid = false; //rearm invalidate
    this.renderCtx.staticRenderer.draw(canvas, timepass);
    for (const widget of this.widgets) {
        widget.draw(canvas, timepass);
    }
}

Object.defineProperties(PDSWidgetsPad.prototype, {
/*    invalid: {
        get() {
            for (const widget of this.widgets) {
                if (widget.invalid) {
                    return true;
                }
            }
            return false;
        }
    }*/
});

Overlay.anyGroups = [];

Overlay.default = {
    fill:   "",
    groups: Overlay.anyGroups,
}
function Overlay(canvas, config) {

    canvas.onclick = function (e) {
        console.log(e.layerX, e.layerY, e)
    }

    if (config !== Overlay.default) {
        config = Object.assign({}, Overlay.default, config);
    }

    let privateCtx = {
        canvas,
        config,
        failback: !!DISABLE_OVERLAY_WORKERS || !window.Worker,
        failbackLoadingPromise: null,
        context: canvas.getContext('2d', { alpha: false }),
        doubleBuffer: document.createElement('canvas').getContext('2d'/*, { alpha: false }*/),
        cache:      new Map(),
        pending:    new Map(),
        prefetched: new Map(),
        pair:           null,
        page:           null,
        selectPending:  null,
        requestCounter: 0,
        extract(collection) {
            return Array.prototype.reduce.call(collection, (accumulator, element) => {
                //element coord must rel to page
                //offset seems do not respect scroll
                const parentBox = element.closest(".page").getBoundingClientRect();
                const box       = element.getBoundingClientRect();
                accumulator.push({
                    /*dom: element,*/
                    id:    element.id,
                    rect:  new Rect(~~(box.left - parentBox.left), ~~(box.top - parentBox.top), ~~Math.ceil(box.width), ~~Math.ceil(box.height), element.id), //element.getBoundingClientRect(),
                    groups: Array.prototype.reduce.call(element.classList, (accumulator, classstr) => {
                        if (classstr.startsWith("group-")) {
                            const gName = classstr.replace("group-", "").trim();
                            if (privateCtx.config.groups === Overlay.anyGroups || privateCtx.config.groups.find((g) => g.id === gName )) {
                                accumulator.push(gName);
                            }
                            return accumulator;
                        } else {
                            return accumulator;
                        }
                    }, [])
                });
                return accumulator;
            }, []);
        },
        requestId() {
            return privateCtx.requestCounter++;
        }
    }

    if (privateCtx.failback) {
        if (!document.querySelector('script[src="overlay.js"]')) {
            privateCtx.failbackLoadingPromise = new Promise((resolve, reject) => {
                const script = document.createElement('script');
                script.src      = 'overlay.js';
                script.type     = "text/javascript";
                script.onload   = resolve;
                script.onerror  = reject;
                document.head.appendChild(script);
            });
        } else {
            privateCtx.failbackLoadingPromise = Promise.resolve();
        }
    } else {
        privateCtx.port = new Worker("overlay.js");
        privateCtx.port.onmessage = privateCtx.port.onerror = (e) => {
            if (e instanceof ErrorEvent || e.type === "error") {
                console.error("cancel", e);
            } else {
                const [data, options] = e.data;
                if (privateCtx.pending.has(options.requestId)) {
                    let pending = privateCtx.pending.get(options.requestId);
                    if (data instanceof Error) {
                        pending.reject(e.data);
                    } else {
                        pending.resolve(e.data);
                    }
                } else {
                    console.error("cancel not found", e);
                }
            }
        }
    }

    return {
        clear() {
            privateCtx.context.fillStyle = privateCtx.config.fill;
            privateCtx.context.fillRect(0,0, privateCtx.canvas.width, privateCtx.canvas.height);
        },
        free() {
            this.invalidateAll();
            if (!privateCtx.failback) {
                privateCtx.port.onmessage = privateCtx.port.onerror = null;
                if (privateCtx.port.terminate) {
                    privateCtx.port.terminate();
                }
            }
        },
        forked() {
            return !privateCtx.failback;
        },
        invalidateAll() {
            console.log("invalidate", "all");
            privateCtx.cache.clear();
            for (const [,pending] of privateCtx.pending) {
                pending.reject([new Error("invalidate"), pending.config]);
            }
            privateCtx.pending.clear();
            if (privateCtx.selectPending) {
                privateCtx.selectPending.reject([new Error("invalidate"), []]);
            }
        },
        invalidate(page, feeder = emptyFn) {
            privateCtx.cache.delete(page.id);
            if (privateCtx.pending.has(page.id)) {
                const pending = privateCtx.pending.get(page.id);
                privateCtx.pending.delete(page.id);
                pending.reject([new Error("invalidate"), pending.config]);
            }
            if (privateCtx.selectPending && privateCtx.page && privateCtx.page.id === page.id) {
                privateCtx.selectPending.reject([new Error("invalidate"), []]);
            }
            if (feeder !== emptyFn) {
                this.prefetch(page, feeder);
            }
        },
        cancel() {
            if (privateCtx.selectPending) {
                privateCtx.selectPending.reject([new Error("canceled"), []]);
                privateCtx.selectPending = privateCtx.page = null;
            }
        },
        select(page, feeder = emptyFn) {

            console.info("select", page.id)

            privateCtx.page = page;

            if (privateCtx.selectPending) {
                privateCtx.selectPending.reject([new Error("canceled"), []]);
                privateCtx.selectPending = null;
            }

            if (privateCtx.cache.has(page.id)) {
                console.log("select cached", page.id)
                privateCtx.pair = privateCtx.cache.get(page.id);
                return Promise.resolve(privateCtx.pair);
            } else {
                console.log("select new", page.id)
                let closure;
                if (feeder === emptyFn) {
                    throw new Error("non empty feeder must be provided");
                }
                privateCtx.selectPending = closure = Promise.withResolvers();
                this.prefetch(page, feeder).then(closure.resolve, (pair) => {
                    closure.reject(pair);
                    return Promise.reject(pair);
                });

                return privateCtx.selectPending.promise.then((pair) => {
                    return privateCtx.pair = pair;
                });
            }

        },
        prefetch(page, feeder) {

            if (privateCtx.cache.has(page.id)) {
                return Promise.resolve(privateCtx.cache.get(page.id));
            } else if (privateCtx.prefetched.has(page.id)) {
                console.info("prefetch", "reuse", page.id);
                return privateCtx.prefetched.get(page.id);
            } else {
                console.info("prefetch", "a new one", page.id);
                let pending = this.evaluate.apply(this, feeder(page)).then((pair) => {
                    console.log("prefetch", "set", page.id)
                    privateCtx.cache.set(page.id, pair);
                    privateCtx.prefetched.delete(page.id);
                    return pair;
                }, (reason) => {
                    privateCtx.prefetched.delete(page.id);
                    return Promise.reject(reason);
                });
                privateCtx.prefetched.set(page.id, pending);
                return pending;
            }

        },
        evaluate(collection, config) {
            let pending = Promise.withResolvers();
            pending.config = config;
            privateCtx.pending.set(config.requestId = privateCtx.requestId(), pending);

            if (privateCtx.failback) {
                //must respect invalidateAll (it must have way to call reject)
                privateCtx.failbackLoadingPromise.then(() => {
                    try {
                        const impl = new OverlayPageCalculator(privateCtx.extract(collection), config);
                        return impl.build().then((pair) => {
                            privateCtx.pending.delete(config.requestId);
                            pending.resolve(pair);
                        }, (pair) => {
                            privateCtx.pending.delete(config.requestId);
                            pending.reject(pair);
                        });
                    } catch (e) {
                        privateCtx.pending.delete(config.requestId);
                        pending.reject([e, config]);
                    }
                });

                return pending.promise;
            } else {
                privateCtx.port.postMessage([privateCtx.extract(collection), config]);
                pending.timeout = setTimeout(() => pending.reject([new Error("timeout"), config]), 35000);
                return pending.promise.then((pair) => {
                    clearTimeout(pending.timeout);
                    privateCtx.pending.delete(config.requestId);
                    console.info("evaluate", "resolve")
                    return pair;
                }, (pair) => {
                    clearTimeout(pending.timeout);
                    privateCtx.pending.delete(config.requestId);
                    console.info("evaluate", "reject", pair[0])
                    return Promise.reject(pair);
                });
            }
        },
        draw(timeleft) {

            if (!privateCtx.pair && privateCtx.pair[0].length) {
                console.warn("no to draw")
                return;
            }

            const [clusters, config] = privateCtx.pair;

            privateCtx.context.fillStyle = privateCtx.config.fill;
            privateCtx.context.fillRect(0, 0, privateCtx.context.canvas.width, privateCtx.context.canvas.height);

            privateCtx.doubleBuffer.canvas.width    = privateCtx.canvas.width;
            privateCtx.doubleBuffer.canvas.height   = privateCtx.canvas.height;

            privateCtx.doubleBuffer.clearRect(0, 0, privateCtx.doubleBuffer.canvas.width, privateCtx.doubleBuffer.canvas.height);

            let defaultColorIndex = 0;
            clusters.forEach((cluster, i)=>{

                let pol = [...cluster.boundingPolygon.sort((a,b) => b.i - a.i)];

                let clusterRect = cluster.boundingRect;
                let first  = pol.pop();
                let current, color;

                const group  = privateCtx.config.groups.find((g) => g.id === cluster.label );
                if (group) {
                    color = group.color = group.color || Overlay.colors[(defaultColorIndex++) % Overlay.colors.length];
                } else {
                    //keep in mind that group may not exist if overlay.config.groups === Overlay.anyGroup
                    color = Overlay.colors[(defaultColorIndex++) % Overlay.colors.length];
                }

                if (!first) {
                    console.log("no items in cluster", cluster);
                    return
                }

                //console.log("cloud", pol.length, ...pol);

                privateCtx.doubleBuffer.save();

                privateCtx.doubleBuffer.beginPath()

                privateCtx.doubleBuffer.fillStyle   = color;
                privateCtx.doubleBuffer.strokeStyle = color;
                privateCtx.doubleBuffer.lineWidth   = config.cluster.probe.radius.collision * 2 - 2; //do not scale overlay its just color fill
                privateCtx.doubleBuffer.lineJoin    = "round";

                /**
                 * round effect will-be unseen until some value of config.cluster.probe.radius.collision
                 */

                privateCtx.doubleBuffer.moveTo(first.x, first.y);
                while (current = pol.pop()) {
                    privateCtx.doubleBuffer.lineTo(current.x, current.y );
                }
                privateCtx.doubleBuffer.closePath();
                privateCtx.doubleBuffer.fill();
                privateCtx.doubleBuffer.stroke();

                privateCtx.doubleBuffer.restore();

                //if we use alpha when using fill and stroke with lineWidth > 1 when on edge fill alpha will multiply buy stroke alpha
                //to prevent this we draw in offscreen buffer without alpha and then redraw with global alpha
/*                let idata = privateCtx.doubleBuffer.getImageData(0,0, clusterRect.x + clusterRect.w + 50, clusterRect.y + clusterRect.h + 50);
                for (let idx = 3; idx < idata.data.length; idx+=4) {
                    console.info(idata.data[idx]);
                    if (idata.data[idx] !== 0) idata.data[idx] = 50;
                }
                privateCtx.doubleBuffer.putImageData(idata, 0,0);*/

            });

            privateCtx.context.save();
            privateCtx.context.globalAlpha = 0.19; // 50/255
            privateCtx.context.drawImage(privateCtx.doubleBuffer.canvas, 0, 0);
            privateCtx.context.restore();

        }
    }
}


Overlay.colors = [
    //"rgba(200, 0, 0, 1)",
    "rgba(200, 180, 200, 1)",
    "rgba(0, 0, 200, 1)",
    "rgba(100, 80, 0, 1)",
    "rgba(0, 100, 80, 1)",
    "rgba(80, 0, 100, 1)",
    "rgba(100, 0, 80, 1)",
    "rgba(80, 100, 0, 1)",
    "rgba(0, 80, 100, 1)",
    "rgba(180, 0, 200, 1)",
];

Overlay.colors = [
    "#d53c7a",
    "#ac40e7",
    "#c4e141",
    //"#533ac9",
    "#d244d2",
    //"#5fda9e",
    "#9447cb",
    //"#ddc445",
    //"#412a89",
    //"#bfdf86",
    "#dc3fac",
    "#d279dd",
    "#808429",
    //"#6a70de",
    //"#e37b2c",
    //"#5588cc",
    "#e1452b",
    //"#66d5d6",
    //"#d94352",
    "#4c8a79",
    "#8b2f89",
    "#ca9233",
    "#361950",
    //"#cfb578",
    //"#43487e",
    //"#a74722",
    //"#afc9e7",
    //"#862a2d",
    "#b0d5b6",
    "#2f1b2e",
    //"#dec3af",
    "#7e3059",
    //"#5f98b3",
    //"#de8a67",
    "#8d6bb7",
    //"#7f5624",
    //"#b6a4e0",
    "#344120",
    "#e79ad2",
    "#2c4b54",
    "#d67384",
    "#7d805d",
    "#b46298",
    //"#542925",
    "#d0a4b2",
    //"#7b647c",
    //"#9d6c5f"
]
