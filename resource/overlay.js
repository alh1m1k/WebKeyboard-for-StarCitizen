
"use strict";

(() => {

    //console.log = console.warn = console.info = () => {}

    if (self.postMessage) {
        self.onmessage = (e) => {
            try {
                const [collection, options] = e.data;
                const impl = new OverlayPageCalculator(collection, options);
                impl.build().then(self.postMessage, self.postMessage);
            } catch (e) {
                self.postMessage([e, (e.data instanceof Array && e.data[1]) ? e.data[1] : { requestId: undefined }] );
            }
        }
        debugPoint = () => {}; //console.log;
    }

    const _protectedDescriptor = Symbol("protected");
    self.OverlayPageCalculator = OverlayPageCalculator;

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
            if (
                arg[0] instanceof Rect ||
                (arg[0] instanceof Object && (arg[0].x !== undefined && arg[0].y !== undefined && arg[0].w !== undefined && arg[0].h !== undefined))
            ) {
                this.x = arg[0].x;
                this.y = arg[0].y;
                this.w = arg[0].w;
                this.h = arg[0].h;
                this.id = arg[0].id;
            } else {
                throw new Error("Rect invalid argument type " + typeof arg[0]);
            }
        } else {
            throw new Error("Rect invalid argument count")
        }
    }

    function arrayIntersect(array, array2) {
        return array.filter(value => array2.includes(value));
    }

    function equal(p1, p2) {
        return p1.x === p2.x && p1.y === p2.y;
    }

    function distance(x1, y1, x2, y2) {
        return Math.sqrt(Math.pow(x2-x1, 2) + Math.pow(y2-y1, 2));
    }

    Probe.default = { radius: { collision: 8, detection: 25 }}
    function Probe(figures, options = Probe.default) {

        //tobig value detection cause problems with poligon hop and rouded shape of bounding poligon
        //to small of it broke collision detection in way then it unable to detect direction of collision
        options         = Object.assign({}, Probe.default, options);
        options.radius  = Object.assign({}, Probe.default.radius, options.radius);

        options.radius.collision += 1; //+1px for better detection of edges is gap === radios

        this.body = {
            point: { x: 0, y: 0 },
            collisionBody: { x: 0, y: 0, radius: options.radius },
        }

        this.figures = figures;
        this.i = 0;
        this.direction = Probe.directionLR; //mb better is angle
        this.dimensions = this.figures.reduce((acc, f, i) => {
            //console.warn(acc);
            acc.minW = Math.min(acc.minW, f.w);
            acc.minH = Math.min(acc.minH, f.h);
            return acc;
        }, {minW: Number.MAX_SAFE_INTEGER, minH: Number.MAX_SAFE_INTEGER});

        this.state = Object.assign(Object.create(null), {
            visited:    [],
            spChecked:  false, //startingPoint checked for this state
            atThisOpt:  0,
            point:      null,
            direction:  null,
            options:    [],
            decision:   null,
            hoping:     [],
        });

    }

    Probe.directionLR = new Direction( 1, 0);
    Probe.directionRL = new Direction(-1, 0);
    Probe.directionTB = new Direction( 0, 1);
    Probe.directionBT = new Direction( 0,-1);

    Probe.floatTop    = new Direction( 0,-1);
    Probe.floatBottom = new Direction( 0, 1);
    Probe.floatLeft   = new Direction(-1, 0);
    Probe.floatRight  = new Direction( 1, 0);

    Probe.leftTop          = 0;
    Probe.rightTop         = 1;
    Probe.rightBottom      = 2;
    Probe.leftBottom       = 3;

    Probe.prototype.emitPoint = function(x,y, tag = "") {
        this.state.visited.push({x,y, i:this.i++, tag});
        this.state.atThisOpt++;
        debugPoint({x, y}, "", {fill: "green", radius: 10});
    }

    Probe.prototype.pushState = function() {
        this.state = Object.create(this.state);
        this.state.atThisOpt    = 0;
        this.state.spChecked    = false;
    }

    Probe.prototype.popState = function() {
        while (this.state.atThisOpt) {
            const point = this.state.visited.pop();
            this.state.atThisOpt--;
            debugPoint(point, "", {fill: "black", radius: this.body.collisionBody.radius.collision});
        }
        const proto = Object.getPrototypeOf(this.state);
        if (proto) {
            this.state = proto;
            return true;
        }
        return false;
    }

    Probe.prototype.begin = function(startingFigure) {

        this.figure         = startingFigure;
        this.startingPoint  = {  x: startingFigure.x, y: startingFigure.y, id: startingFigure.id + "-lt" };

        this.state.point    = this.startingPoint;
        this.state.options  = this.opportunity(this.startingPoint, this.figure);
        this.state.decision = this.decide(this.startingPoint, Probe.directionLR, this.state.options);
        this.state.other    = { direction: Probe.directionLR }

        if (!this.state.decision) {
            return Promise.resolve();
        }

/*        for (let i = 0; i < 10000000; i++) {
            console.log("ddd")
        }*/

        this.precision = Math.min(this.dimensions.minW / 2, this.dimensions.minH / 2, this.body.collisionBody.radius.collision);

        this.state.options  = this.state.options.filter((opt) => opt !== this.state.decision.option);

        this.emitPoint(this.startingPoint.x, this.startingPoint.y, startingFigure.id + "-startingPoint");

        let retry = 250;
        let i = 0;

        return new Promise((resolve, reject) => {
            let executor;
            let state = {
                point:      this.startingPoint,
                figure:     startingFigure,
                nocollide:  [],
                continue:   false,
            }
            setTimeout(executor = () => {
                if (retry--) {
                    state = this.nextSteep(state, i++);
                    if (state.continue) {
                        setTimeout(executor, 0);
                    } else {
                        resolve();
                    }
                }
            }, 0);
        });
    }

    Probe.prototype.nextSteep = function(prevState, index) {

        let moveState = this.move(this.state.decision, prevState.figure, this.figures, prevState.nocollide, index++);

        if (!moveState.continue) {
            return moveState;
        }

        if (equal(moveState.point, this.startingPoint)) {
            //this equation probably must be improved
            if (this.state.visited.length >= 4) {
                moveState.continue = false;
                return moveState;
            }
        }

        if (!equal(prevState.point,  moveState.point)) {
            this.pushState();
            this.state.options  = this.opportunity(this.state.point = moveState.point, moveState.figure);
            this.state.other    = { direction: moveState.direction }
            this.state.hoping   = [];

            //hungry move, try to hop to nearest figure
            //at this moment we save are state in pushState so we able to return
            if (moveState.finish && moveState.collision.detection) {
                this.state.hoping = this.hoping(moveState);
            }
        }

        moveState = this.ifExistExecuteNextHop(moveState);
        this.state.decision = this.decide(this.state.point, this.state.other.direction, this.state.options);

        while (!this.state.decision && this.popState()) {
            moveState = this.ifExistExecuteNextHop(moveState);
            this.state.decision = this.decide(this.state.point, this.state.other.direction, this.state.options);
        }

        if (this.state.decision) {
            this.state.options = this.state.options.filter((opt) => opt !== this.state.decision.option);
        } else {
            //console.log("no point to trace");
            moveState.continue = false;
        }

        return moveState;
    }

    Probe.prototype.decide = function(point, direction, options) {

        let unusedOptions = options.filter((ops) => {
            if (this.state.visited.length > 2) {
                //after 3 and move visited point startingPoint is always valid end point
                return !this.isPassed(ops.point) || equal(ops.point, this.startingPoint);
            } else {
                return !this.isPassed(ops.point);
            }
        });

        if (unusedOptions.length === 0) {
            return null;
        } else if (unusedOptions.length > 1) {
            for (const option of unusedOptions) {
                if (equal(option.direction, direction)) {
                    return { from: point, option };
                }
            }
        }

        return { from: point, option: unusedOptions[0] };
    }

    Probe.prototype.ifExistExecuteNextHop = function(moveState) {
        let decision;
        if ((decision = this.state.hoping.pop())) {
            this.pushState();
            this.state.options  = this.opportunity(this.state.point = decision.to, decision.toFigure);
            this.state.other    = { direction: decision.direction }
            this.emitPoint(decision.from.x, decision.from.y,    Probe.floatToText(decision.float));
            this.emitPoint(decision.to.x,   decision.to.y,      Probe.floatToText(decision.float));
            return {
                point:      decision.to,
                figure:     decision.toFigure,
                direction:  decision.direction,
                float:      decision.float,
                nocollide:  [decision.fromFigure],
                collision:  [],
                continue:   true,
                finish:     true,
            };
        }
        return moveState;
    }

    Probe.prototype.move = function(decision, figure, figures, nocollide = [], index) {

        const { from:point, option: { point:to , direction, float } } = decision;

        debugPoint(point, `sp-${index}[${point.x}:${point.y}]`);

        //do not work in ff52
        //debugPoint({...to, y: to.y - 25}, `tp-${index}[${to.x}:${to.y}]`);

        let delta = {x: 0, y: 0};
        let collisionBody = {x:0, y:0};
        let endPoint = {x:0, y:0};
        if (direction === Probe.directionRL || direction === Probe.directionLR) {
            delta.x = this.precision * direction.x;
        } else if (direction === Probe.directionTB || direction === Probe.directionBT) {
            delta.y = this.precision * direction.y;
        }
        collisionBody.x = point.x + (-direction.x * this.body.collisionBody.radius.collision) + (float.x * this.body.collisionBody.radius.collision);
        collisionBody.y = point.y + (-direction.y * this.body.collisionBody.radius.collision) + (float.y * this.body.collisionBody.radius.collision);
        endPoint.x = to.x + (direction.x * this.body.collisionBody.radius.collision) + (float.x * this.body.collisionBody.radius.collision);
        endPoint.y = to.y + (direction.y * this.body.collisionBody.radius.collision) + (float.y * this.body.collisionBody.radius.collision);

        figures = figures.filter(f => (f !== figure));                               //remove self from collision check
        const graceTimeFigures = figures.filter(f => (nocollide.indexOf(f) === -1)); //remove prev self (nocollide) from check for few frame
        const graceTimeFrames  = Math.max(~~Math.ceil(this.precision / (this.body.collisionBody.radius.collision) * 2), 1); //how much tick needed to leave corner

        let colliderProduct;
        let tickCounter = 0;
        let collide = false;
        while (delta.x || delta.y) {
            //console.log("tickCounter start: ", tickCounter);
            debugPoint(collisionBody, "", { fill: "transparent", stroke: "red", radius: this.body.collisionBody.radius.detection });
            debugPoint(collisionBody, "", { fill: "red", radius: this.body.collisionBody.radius.collision });
            colliderProduct = this.cross(collisionBody, this.body.collisionBody.radius, tickCounter < graceTimeFrames ? graceTimeFigures : figures);
            if (colliderProduct.collision) {
                //console.log("tickCounter: has collision at", collisionBody, "collide with", colliderProduct.product);
                collide = true;
                for (const product of colliderProduct.product) {
                    for (const collisionPoint of product.points) {
                        if (collisionPoint.distance > this.body.collisionBody.radius.collision+2) {
                            //todo fix me increase slightly this.body.collisionBody.radius.collision based on space between elm
                            //to allow proper collide
                            break;
                        }
                        const colPoint = collisionPoint.point;
                        const isVertex = this.isPointBelong(colPoint, this.vertexes(product.figure));
                        if (!isVertex) {
                            colPoint.x -= float.x * this.body.collisionBody.radius.collision;
                            colPoint.y -= float.y * this.body.collisionBody.radius.collision;
                        }
                        if (!this.isPassed(colPoint) && this.hasOp2direction(this.opportunity(colPoint, product.figure), this.floatToDirection(float))) {

                            //starting point
                            if (distance(collisionBody.x, collisionBody.y, endPoint.x, endPoint.y) < this.body.collisionBody.radius.collision) {
                                //crossing "to" point
                                this.emitPoint(to.x, to.y); //save starting point
                            } else {
                                this.emitPoint(direction.x ? colPoint.x : point.x , direction.y ? colPoint.y : point.y, Probe.floatToText(float));
                            }

                            this.emitPoint(colPoint.x, colPoint.y); //next hop point

                            //console.info("collide with transfer to", product.figure);
                            return {
                                point:      colPoint,
                                figure:     product.figure,
                                direction:  this.floatToDirection(float),
                                float:      undefined, //float is changed and undefined until decided (but it probably can be calculated if need)
                                nocollide:  [figure],
                                collision:  colliderProduct,
                                continue:   true,
                                finish:     false,
                            };

                        }
                    }
                }

                return {
                    point:      point,
                    figure:     figure,
                    direction:  direction,
                    float,
                    nocollide:  nocollide,
                    collision:  colliderProduct,
                    continue:   true,
                    finish:     false,
                };

            }
            collisionBody.x += (delta.x = Math.sign(delta.x) * Math.min(Math.abs(delta.x), Math.abs(endPoint.x - collisionBody.x)));
            collisionBody.y += (delta.y = Math.sign(delta.y) * Math.min(Math.abs(delta.y), Math.abs(endPoint.y - collisionBody.y)));
            tickCounter++;
        }

        if (!collide) {
            this.emitPoint(to.x, to.y, "endOf figure");
            //console.log("not collide");
        }

        return {
            point: to,
            figure,
            direction,
            float,
            nocollide:  [],
            collision:  colliderProduct,
            continue:   true,
            finish:     true,
        }

    }

    Probe.prototype.floatToDirection = function(float) {
        switch (float) {
            case Probe.floatLeft:
                return Probe.directionRL;
            case Probe.floatTop:
                return Probe.directionBT;
            case Probe.floatRight:
                return Probe.directionLR;
            case Probe.floatBottom:
                return Probe.directionTB;
            default:
                throw new Error("undefined float to direction conv");
        }

    }

    Probe.prototype.directionTo = function(point, to) {
        if (point.x === to.x && point.y === to.y) {
            return {x: 0, y: 0};
        } else if (point.x === to.x) {
            return {x: 0, y: Math.sign(point.y - to.y)};
        } else {
            return {x: Math.sign(point.x - to.x), y: 0};
        }
    }

    Probe.prototype.vertexes = function(figure) {
        return [
            { point:{x:figure.x,            y:figure.y},           type: Probe.leftTop     , isVertex: true },
            { point:{x:figure.x+figure.w,   y:figure.y},           type: Probe.rightTop    , isVertex: true },
            { point:{x:figure.x+figure.w,   y:figure.y+figure.h},  type: Probe.rightBottom , isVertex: true },
            { point:{x:figure.x,            y:figure.y+figure.h},  type: Probe.leftBottom  , isVertex: true },
        ];
    }

    Probe.prototype.isPointBelong = function (point, points) {
        for (const pointContainer of points) {
            if (equal(point, pointContainer.point)) {
                return true;
            }
        }
        return false;
    }

    Probe.prototype.hasOp2direction = function(ops, direction) {
        if (ops.length === 0) {
            return false;
        } else if (ops.length === 1) {
            return ops[0].direction === direction;
        } else {
            return ops[0].direction === direction || ops[1].direction === direction;
        }
    }

    Probe.prototype.closestVertex = function(point, figure) {
        let vertexes = this.vertexes(figure);
        vertexes.forEach(v => v.distance = this.distance(point, v.point));
        vertexes.sort((a,b) => a.distance - b.distance);
        return vertexes[0];
    }

    Probe.prototype.isValidHop = function(vertexType, floatType, direction) {
        if (floatType === Probe.floatTop && direction === Probe.directionLR) {
            return vertexType === Probe.leftTop;
        }
        if (floatType === Probe.floatTop && direction === Probe.directionRL) {
            return vertexType === Probe.rightTop;
        }
        if (floatType === Probe.floatRight && direction === Probe.directionTB) {
            return vertexType === Probe.rightTop;
        }
        if (floatType === Probe.floatRight && direction === Probe.directionBT) {
            return vertexType === Probe.rightBottom;
        }
        if (floatType === Probe.floatBottom && direction === Probe.directionRL) {
            return vertexType === Probe.rightBottom;
        }
        if (floatType === Probe.floatBottom && direction === Probe.directionLR) {
            return vertexType === Probe.leftBottom;
        }
        if (floatType === Probe.floatLeft && direction === Probe.directionBT) {
            return vertexType === Probe.leftBottom;
        }
        if (floatType === Probe.floatLeft && direction === Probe.directionTB) {
            return vertexType === Probe.leftTop;
        }
        return false;
    }

    Probe.floatToText = function(float) {
        switch (float) {
            case Probe.floatTop:
                return "top";
            case Probe.floatRight:
                return "right";
            case Probe.floatBottom:
                return "bottom";
            case Probe.floatLeft:
                return "left";
            default:
                return "";
        }
    }

    Probe.prototype.pointsOfInterest = function(point, figure) {
        if (point.x >= figure.x && point.x <= figure.x + figure.w) {
            return [
                { point: {x:point.x,   y:figure.y         }, distance: 0 },
                { point: {x:point.x,   y:figure.y+figure.h}, distance: 0 },

                { point:{x:figure.x,            y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y+figure.h},    distance: 0 },
                { point:{x:figure.x,            y:figure.y+figure.h},    distance: 0 },
            ]
        } else if (point.y >= figure.y && point.y <= figure.y + figure.h) {
            return [
                { point: {x:figure.x,            y:point.y}, distance: 0 },
                { point: {x:figure.x+figure.w,   y:point.y}, distance: 0 },

                { point:{x:figure.x,            y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y+figure.h},    distance: 0 },
                { point:{x:figure.x,            y:figure.y+figure.h},    distance: 0 },
            ]
        } else {
            return [
                { point:{x:figure.x,            y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y},             distance: 0 },
                { point:{x:figure.x+figure.w,   y:figure.y+figure.h},    distance: 0 },
                { point:{x:figure.x,            y:figure.y+figure.h},    distance: 0 },
            ]
        }
    }

    Probe.prototype.cross = function(point, radiuses, figures) {
        let result = {
            product: [],
            detection: false,
            collision: false,
        };
        for (const figure of figures) {
            let points = this.pointsOfInterest(point, figure);
            points.forEach((p, i) => p.distance = this.distance(point, p.point));
            points.sort((a,b) => a.distance - b.distance);
            if (points[0].distance <= radiuses.collision || points[0].distance <= radiuses.detection) {
                result.collision |= points[0].distance <= radiuses.collision; //result is cumulative
                result.detection |= points[0].distance <= radiuses.detection;
                result.product.push({
                    distance: points[0].distance,
                    figure,
                    points,
                });
            }
        }

        return result;
    }

    Probe.prototype.isPassed = function(point) {
        for (const passedPoint of this.state.visited) {
            if (point.x === passedPoint.x && point.y === passedPoint.y) {
                return true;
            }
        }
        return false;
    }

    Probe.prototype.distance = function(point, target) {
        return Math.sqrt(Math.pow(target.x - point.x, 2) + Math.pow(target.y - point.y, 2));
    }

    Probe.prototype.hoping = function(movingState) {
        let result = [];
        for (const collision of movingState.collision.product) {
            const vertex = this.closestVertex(collision.points[0].point, collision.figure);
            if (vertex.distance <= this.body.collisionBody.radius.detection) {
                if (this.isPassed(vertex.point)) {
                    continue;
                }
                if (this.hasOp2direction(this.opportunity(vertex.point, collision.figure), this.floatToDirection(movingState.float))) {
                    result.push(
                        { from: movingState.point, to: vertex.point, float: undefined,
                            fromFigure: movingState.figure, toFigure: collision.figure,
                            direction: this.floatToDirection(movingState.float), priority: 0,
                        }
                    );
                    continue;
                }
                if (this.isValidHop(vertex.type, movingState.float, movingState.direction)) {
                    result.push({
                        from: movingState.point, to: vertex.point, float: movingState.float,
                        fromFigure: movingState.figure, toFigure: collision.figure,
                        direction: movingState.direction, priority: 1,
                    });
                    continue;
                }
            }
        }
        //less priority is better, less priority at end of array to allow pop()
        return result.sort((d1, d2) => d2.priority - d1.priority );
    }

    Probe.prototype.opportunity = function(body, figure) {
        if (body.x === figure.x && body.y === figure.y) {
            //lefttop
            return [
                { direction: Probe.directionLR, point: {x: figure.x + figure.w, y: figure.y, id: figure.id + "-rt" }, float: Probe.floatTop,    isVertex: true },
                { direction: Probe.directionTB, point: {x: figure.x, y: figure.y + figure.h, id: figure.id + "-lb" }, float: Probe.floatLeft,   isVertex: true  },
            ];
        } else if (body.x === figure.x+figure.w && body.y === figure.y) {
            //righttop
            return [
                { direction: Probe.directionRL, point: {x: figure.x,            y: figure.y, id: figure.id + "-lt" },    float: Probe.floatTop,              isVertex: true    },
                { direction: Probe.directionTB, point: {x: figure.x + figure.w, y: figure.y + figure.h, id: figure.id + "-rb" },    float: Probe.floatRight, isVertex: true  },
            ];
        } else if (body.x === figure.x+figure.w && body.y === figure.y+figure.h) {
            //rightbottom
            return [
                { direction: Probe.directionBT, point: {x: figure.x+figure.w,   y: figure.y}, float: Probe.floatRight,  isVertex: true  },
                { direction: Probe.directionRL, point: {x: figure.x, y: figure.y + figure.h}, float: Probe.floatBottom, isVertex: true  },
            ];
        } else if (body.x === figure.x && body.y === figure.y+figure.h) {
            //leftbottom
            return [
                /*            { direction: Probe.directionBT, point: {x: figure.x+figure.w, y: figure.y}, float: Probe.floatLeft },
                            { direction: Probe.directionLR, point: {x: figure.x, y: figure.y + figure.h}, float: Probe.floatBottom },*/
                { direction: Probe.directionBT, point: {x: figure.x, y: figure.y, id: figure.id + "-lt"}, float: Probe.floatLeft, isVertex: true  },
                { direction: Probe.directionLR, point: {x: figure.x+figure.w, y: figure.y + figure.h, id: figure.id + "-rb"}, float: Probe.floatBottom, isVertex: true  },
            ];
        } else if (body.x >= figure.x && body.x <= figure.x+figure.w && body.y === figure.y) {
            return [
                { direction: Probe.directionLR, point: {x: figure.x+figure.w, y: figure.y, id: figure.id + "-rx"},     float: Probe.floatTop, isVertex: false },
                { direction: Probe.directionRL, point: {x: figure.x,          y: figure.y, id: figure.id + "-lx"},     float: Probe.floatTop, isVertex: false },
            ];
        } else if (body.x >= figure.x && body.x <= figure.x+figure.w && body.y === figure.y+figure.h) {
            return [
                { direction: Probe.directionLR, point: {x: figure.x+figure.w, y: figure.y+figure.h, id: figure.id + "-rx2"},     float: Probe.floatBottom, isVertex: false },
                { direction: Probe.directionRL, point: {x: figure.x,          y: figure.y+figure.h, id: figure.id + "-lx2"},     float: Probe.floatBottom, isVertex: false },
            ];
        } else if (body.y >= figure.y && body.y <= figure.y+figure.h && body.x === figure.x) {
            return [
                { direction: Probe.directionTB, point: {x: figure.x, y: figure.y+figure.h, id: figure.id + "-bx"}, float: Probe.floatLeft, isVertex: false },
                { direction: Probe.directionBT, point: {x: figure.x,          y: figure.y, id: figure.id + "-tx"}, float: Probe.floatLeft, isVertex: false },
            ];
        } else if (body.y >= figure.y && body.y <= figure.y+figure.h && body.x === figure.x+figure.w) {
            return [
                { direction: Probe.directionTB, point: {x: figure.x+figure.w, y: figure.y+figure.h, id: figure.id + "-bx2"}, float: Probe.floatRight, isVertex: false },
                { direction: Probe.directionBT, point: {x: figure.x+figure.w,          y: figure.y, id: figure.id + "-tx2"}, float: Probe.floatRight, isVertex: false },
            ];
        } else {
            return [];
        }
    }

    Cluster.default = {
        probe: Probe.default
    }
    function Cluster(options = Cluster.default) {

        if (options !== Cluster.default) {
            options = Object.assign({}, Cluster.default, options);
            if (options.probe !== Probe.default) {
                options.probe = Object.assign({}, Probe.default, options.probe);
            }
        }

        this.boundingPolygon = [];
        this.label           = undefined;

        this[_protectedDescriptor] = {
            options,
            groups:  [],
            items:   [],
            count: 0,
            minX: Number.MAX_SAFE_INTEGER,
            minY: Number.MAX_SAFE_INTEGER,
            maxX: 0,
            maxY: 0,
            _rect: new Rect(0,0,0,0),
        }

    }

    Cluster.prototype.addItems = function(...items) {
        let protectedCtx = this[_protectedDescriptor];
        if (items.length === 1) {
            protectedCtx.items.push(...items);
            protectedCtx.count += items.length;
            protectedCtx.minX = Math.min(protectedCtx.minX, items[0].x);
            protectedCtx.minY = Math.min(protectedCtx.minY, items[0].y);
            protectedCtx.maxX = Math.max(protectedCtx.maxX, items[0].x + items[0].w);
            protectedCtx.maxY = Math.max(protectedCtx.maxY, items[0].y + items[0].h);
        } else {
            protectedCtx.items.push(...items);
            protectedCtx.count += items.length;
            [ protectedCtx.minX, protectedCtx.minY, protectedCtx.maxX, protectedCtx.maxY] = items.reduce((accumulator, value) => {
                accumulator[0] = Math.min(accumulator[0], value.x);
                accumulator[1] = Math.min(accumulator[1], value.y);
                accumulator[2] = Math.max(accumulator[2], value.x + value.w);
                accumulator[3] = Math.max(accumulator[3], value.y + value.h);
                return accumulator;
            }, ([protectedCtx.minX, protectedCtx.minY, protectedCtx.maxX, protectedCtx.maxY]));
        }
    }

    Cluster.prototype.merge = function(cluster) {
        this.addItems(...cluster[_protectedDescriptor].items);
    }

    Cluster.prototype.addGroups = function(...groups) {
        let protectedCtx = this[_protectedDescriptor];
        protectedCtx.groups.push(...groups);
    }

    Object.defineProperties(Cluster.prototype, {
        itemCount: {
            get: function () { return this[_protectedDescriptor].count }
        },
        boundingRect: {
            get: function() {
                let protectedCtx = this[_protectedDescriptor];
                protectedCtx._rect.x = protectedCtx.minX;
                protectedCtx._rect.y = protectedCtx.minY;
                protectedCtx._rect.w = protectedCtx.maxX - protectedCtx.minX;
                protectedCtx._rect.h = protectedCtx.maxY - protectedCtx.minY;
                return protectedCtx._rect;
            }
        },
        boundingPolygonLegacy: {
            get: function() {
                let protectedCtx = this[_protectedDescriptor];
                const sortedX = [...protectedCtx.items].sort((a,b) => { return a.x - b.x });
                const sortedY = [...protectedCtx.items].sort((a,b) => { return a.y - b.y });

                let xyminmax = {};
                sortedX.forEach((e) => {
                    if (xyminmax[e.x] === undefined) {
                        xyminmax[e.x] = {yMin: Number.MAX_SAFE_INTEGER, yMax: 0};
                    }
                    if (xyminmax[e.x+e.w] === undefined) {
                        xyminmax[e.x+e.w] = {yMin: Number.MAX_SAFE_INTEGER, yMax: 0};
                    }
                    xyminmax[e.x].yMin = Math.min(xyminmax[e.x].yMin, e.y);
                    xyminmax[e.x].yMax = Math.max(xyminmax[e.x].yMin, e.y + e.h);

                    xyminmax[e.x+e.w].yMin = Math.min(xyminmax[e.x+e.w].yMin, e.y);
                    xyminmax[e.x+e.w].yMax = Math.max(xyminmax[e.x+e.w].yMin, e.y + e.h);
                });

                let yxminmax = {};
                sortedY.forEach((e) => {
                    if (yxminmax[e.y] === undefined) {
                        yxminmax[e.y] = {xMin: Number.MAX_SAFE_INTEGER, xMax: 0};
                    }
                    if (yxminmax[e.y+e.h] === undefined) {
                        yxminmax[e.y+e.h] = {xMin: Number.MAX_SAFE_INTEGER, xMax: 0};
                    }
                    yxminmax[e.y].xMin = Math.min(yxminmax[e.y].xMin, e.x);
                    yxminmax[e.y].xMax = Math.max(yxminmax[e.y].xMax, e.x + e.w);
                    yxminmax[e.y+e.h].xMin = Math.min(yxminmax[e.y+e.h].xMin, e.x);
                    yxminmax[e.y+e.h].xMax = Math.max(yxminmax[e.y+e.h].xMax, e.x + e.w);
                });

                let points = [];
                for (const [x, yv] of Object.entries(xyminmax)) {
                    points.push({x: +x, y: yv.yMin, left: false, top: true, bottom: false, right: false});
                    points.push({x: +x, y: yv.yMax, left: false, top: false, bottom: true, right: false });
                }
                for (const [y, xv] of Object.entries(yxminmax)) {
                    points.push({x: xv.xMin, y: +y, left: true, top: false, bottom: false, right: false});
                    points.push({x: xv.xMax, y: +y, left: false, top: false, bottom: false, right: true});
                }

                let prevFilter = {x: Number.MAX_SAFE_INTEGER, y: Number.MAX_SAFE_INTEGER, left: false, top: false, bottom: false, right: false};
                return points.sort((a,b) => a.y !== b.y ? a.y - b.y : a.x - b.x).filter((e) => {
                    if (prevFilter.x === e.x && prevFilter.y === e.y) {
                        prevFilter.left     = prevFilter.left   || e.left;
                        prevFilter.top      = prevFilter.top    || e.top;
                        prevFilter.bottom   = prevFilter.bottom || e.bottom;
                        prevFilter.right    = prevFilter.right  || e.right;
                        return false;
                    }
                    prevFilter = e;
                    return true;
                })
            },
        },
        boundingPolygonPromise: {
            get: function() {
                let protectedCtx = this[_protectedDescriptor];
                let leftTopCorner = protectedCtx.items.sort((a,b) => { return a.y === b.y ? b.x - a.x : b.y - a.y })[protectedCtx.items.length-1];
                //console.info("probe items", protectedCtx.items);
                let p = new Probe(protectedCtx.items, protectedCtx.options.probe);
                if (!protectedCtx.boundingPolygonPromise) {
                    protectedCtx.boundingPolygonPromise = p.begin(leftTopCorner).then(() => {
                        this.boundingPolygon = p.state.visited; //override boundingPolygon
                        return p.state.visited;
                    })
                }
                return protectedCtx.boundingPolygonPromise;
            }
        }
    });


    function Direction(x,y) {
        this.x = x;
        this.y = y;
    }

    Direction.prototype.eq = function(direction) {
        return this.x === direction.x && this.y === direction.y;
    }

    function debugPoint(point, text, { stroke = undefined, fill = "white", color = "white", radius = 3, } = {  }) {
        if (!debugPoint.target) {
            debugPoint.target   = document.querySelector("canvas#overlay");
            debugPoint.ctx      = debugPoint.target.getContext("2d");
        }
        if (debugPoint.ctx) {
            if (fill || stroke) {
                debugPoint.ctx.beginPath();
                debugPoint.ctx.ellipse(point.x, point.y, radius, radius, 0, 0, Math.PI * 2);
            }
            if (fill) {
                debugPoint.ctx.fillStyle = fill;
                debugPoint.ctx.fill();
            }
            if (stroke) {
                debugPoint.ctx.strokeStyle = stroke;
                debugPoint.ctx.stroke();
            }
            if (text) {
                debugPoint.ctx.fillStyle = color;
                debugPoint.ctx.fillText(text, point.x + 2,  point.y - 3);
            }
        } else {
            //console.warn(point, text);
        }
    }

    OverlayPageCalculator.default = {
        groups:     [],
        cluster:    Cluster.default,
    }

    OverlayPageCalculator.inRange  = function(value, min, max) {
        return (value >= min) && (value <= max);
    }

    OverlayPageCalculator.cross = function(recta, rectb) {
        let xOverlap = OverlayPageCalculator.inRange(recta.x, rectb.x, rectb.x + rectb.w)  || OverlayPageCalculator.inRange(rectb.x, recta.x, recta.x + recta.w);
        let yOverlap = OverlayPageCalculator.inRange(recta.y, rectb.y, rectb.y + rectb.h)  || OverlayPageCalculator.inRange(rectb.y, recta.y, recta.y + recta.h);
        return xOverlap && yOverlap;
    }

    function OverlayPageCalculator(collection, config = OverlayPageCalculator.default) {
        if (config !== OverlayPageCalculator.default) {
            config = Object.assign({}, OverlayPageCalculator.default, config);
            if (config.cluster !== Cluster.default) {
                config.cluster = Object.assign({}, Cluster.default, config.cluster);
            }
        }
        this.config     = config;
        this.collection = this.prepare(collection);
        this.clusters   = [];
    }

    OverlayPageCalculator.prototype.processIntersection = function(that, product, thatBody, productBody ) {
        let intersected = arrayIntersect(that.e.groups, product.e.groups);
        if (!intersected.length) {
            return false
        } else {
            intersected.forEach((group) => {
                if (that.clusters[group] === undefined && product.clusters[group] === undefined) {
                    //edge case both hasent cluster group
                    let cluster = new Cluster(this.config.cluster);
                    cluster.label = group;
                    cluster.addGroups(group);
                    cluster.addItems(thatBody, productBody);
                    that.clusters[group] = cluster;
                    product.clusters[group] = cluster;
                } else if (that.clusters[group] !== undefined && product.clusters[group] !== undefined) {
                    if (that.clusters[group] !== product.clusters[group]) {
                        that.clusters[group].merge(product.clusters[group]);
                        product.clusters[group] = that.clusters[group];
                    } else {
                        //if cluster is same we both already in the cluster
                    }
                } else {
                    if (that.clusters[group] !== undefined) {
                        that.clusters[group].addItems(productBody);
                        product.clusters[group] = that.clusters[group];
                    } else {
                        product.clusters[group].addItems(thatBody);
                        that.clusters[group] = product.clusters[group];
                    }
                }
            });
        }
        return true;
    }

    OverlayPageCalculator.prototype.prepare = function (extract) {
        let hash = new Map();
        let unordered = [];

        let minW = Number.MAX_SAFE_INTEGER;
        let minH = Number.MAX_SAFE_INTEGER;
        let minX = Number.MAX_SAFE_INTEGER;
        let minY = Number.MAX_SAFE_INTEGER;
        let maxX = 0;
        let maxY = 0;

        //maxX, maxY;

        for (const e of extract) {
            let rect = new Rect(e.rect);
            hash.set(rect, {
                e,
                clusters: Object.create(null),
            });
            unordered.push(rect);
            minW = Math.min(minW, rect.w);
            minH = Math.min(minH, rect.h);
            minX = Math.min(minX, rect.x);
            minY = Math.min(minY, rect.y);
            maxX = Math.max(maxX, rect.x + rect.w);
            maxY = Math.max(maxY, rect.y + rect.h);
        }

        const sortedX = unordered.sort((a,b) => { return a.x - b.x });
        const sortedY = [...unordered].sort((a,b) => { return a.y - b.y });

        return {
            hash,
            sortedX,
            sortedY,
            minW,
            minH,
            minX,
            minY,
            maxX,
            maxY,
        }
    }

    OverlayPageCalculator.prototype.clustering = function(rect, data) {

        const that            = data.hash.get(rect);
        const detection = this.config.cluster.probe.radius.collision * 2 + 1; //+1 to pass floating error
        let   indexX = data.sortedX.indexOf(rect);
        let   indexY = data.sortedY.indexOf(rect);

        if (indexX === -1 || indexY === -1) {
            throw new Error("invalid index");
        }

        let rectCopy = Object.assign({}, rect);

        rectCopy.x -= detection;
        for (let i = indexX-1; i >= 0; i--) {
            if (OverlayPageCalculator.cross(rectCopy, data.sortedX[i])) {
                let product = data.hash.get(data.sortedX[i]);
                this.processIntersection(that, product, rect, data.sortedX[i]);
            }
        }

        Object.assign(rectCopy, rect);
        rectCopy.x += detection;
        for (let i = indexX+1; i < data.sortedX.length; i++) {
            if (OverlayPageCalculator.cross(rectCopy, data.sortedX[i])) {
                let product = data.hash.get(data.sortedX[i]);
                this.processIntersection(that, product, rect, data.sortedX[i]);
            }
        }

        Object.assign(rectCopy, rect);
        rectCopy.y -= detection;
        for (let i = indexY-1; i >= 0; i--) {
            if (OverlayPageCalculator.cross(rectCopy, data.sortedY[i])) {
                let product = data.hash.get(data.sortedY[i]);
                this.processIntersection(that, product, rect, data.sortedY[i]);
            }
        }

        Object.assign(rectCopy, rect);
        rectCopy.y += detection;
        for (let i = indexY+1; i < data.sortedY.length; i++) {
            if (OverlayPageCalculator.cross(rectCopy, data.sortedY[i])) {
                let product = data.hash.get(data.sortedY[i]);
                this.processIntersection(that, product, rect, data.sortedY[i]);
            }
        }

    }

    OverlayPageCalculator.prototype.build = function() {

        const c = this.collection;
        this.clusters = [];

        return c.sortedY.reduce((header, e) => {
            return header.then(() => {
                return new Promise((resolve) => {
                    this.clustering(e, c); //will-be async anyway, so not need for setTimeout
                    resolve();
                })
            })
        }, Promise.resolve()).then(() => {
            for (const [key, {e, clusters}] of c.hash) {
                for (const [index, cluster] of Object.entries(clusters)) {
                    if (this.clusters.indexOf(cluster) === -1) {
                        this.clusters.push(cluster);
                    }
                }
            }
        }).then(() => {
            return this.clusters.reduce((acc, e) => acc.then(() => (
                e.itemCount > 1 ? e.boundingPolygonPromise : Promise.resolve())
            ), Promise.resolve());
        }).then(() => [ this.clusters.map((c) => {
            return { label: c.label, boundingPolygon: c.boundingPolygon, boundingRect: c.boundingRect, }
        } ), this.config ]);
    }

})();