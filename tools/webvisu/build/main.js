/**
 * A backend to draw graphics on a HTML 5 canvas using the library PixiJS.
 * The backend includes functions to draw primitive shapes either as graphics objects or as sprites with a specified resolution.
 * Functions to store multiple primitive shapes of one type as one PixiJS graphics object are also included.
 *
 */
var pixiBackend;
(function (pixiBackend) {
    /**
     * The PixiJS containers are objects to hold graphics elements that are rendered on the canvas.
     * They can be nested to build something like a folderstructure. The substructure is accessible via the "childrens" of a container.
     */
    pixiBackend.container = {
        stage: new PIXI.Container(),
        backgrounds: new PIXI.Container(),
        inputs: new PIXI.Container(),
        overviewBusesLeft: new PIXI.Container(),
        overviewBusesRight: new PIXI.Container(),
        overviewBusesHorizontal: new PIXI.Container(),
        hicannImages: new PIXI.Container(),
        logos: new PIXI.Container(),
        numberAll: new PIXI.Container(),
        numberHover: new PIXI.Container(),
        border: new PIXI.Container(),
        routes: new PIXI.Container(),
        switches: new PIXI.Container(),
        selectedRoutes: new PIXI.Container(),
        selectedSwitches: new PIXI.Container(),
        hicannSelection: new PIXI.Container(),
        reticles: new PIXI.Container(),
        detailView: new PIXI.Container(),
        busesLeft: new PIXI.Container(),
        busesRight: new PIXI.Container(),
        busesHorizontal: new PIXI.Container(),
        synGrid: new PIXI.Container(),
        synapses: new PIXI.Container(),
        synapseCross: new PIXI.Container(),
        synDrivers: new PIXI.Container(),
        neurons: new PIXI.Container(),
        repeaters: new PIXI.Container(),
        repeaterBusConnections: new PIXI.Container(),
        busesLeftSprite: new PIXI.Container(),
        busesRightSprite: new PIXI.Container(),
        busesHorizontalSprite: new PIXI.Container(),
        synGridSprite: new PIXI.Container(),
        synapsesSprite: new PIXI.Container(),
        neuronsSprite: new PIXI.Container(),
        repeaterBusConnectionsSprite: new PIXI.Container(),
        /**
         * Build the container (folder) structure together. The top-level container is stage.
         * All graphics elements that belong to the detailView are nested in the detailView container.
         */
        setup: function () {
            this.stage.addChild(this.backgrounds);
            this.stage.addChild(this.hicannImages);
            this.stage.addChild(this.logos);
            this.stage.addChild(this.inputs);
            this.stage.addChild(this.overviewBusesLeft);
            this.stage.addChild(this.overviewBusesRight);
            this.stage.addChild(this.overviewBusesHorizontal);
            this.stage.addChild(this.detailView);
            this.stage.addChild(this.routes);
            this.stage.addChild(this.switches);
            this.stage.addChild(this.selectedRoutes);
            this.stage.addChild(this.selectedSwitches);
            this.stage.addChild(this.numberAll);
            this.stage.addChild(this.numberHover);
            this.stage.addChild(this.border);
            this.stage.addChild(this.hicannSelection);
            this.stage.addChild(this.reticles);
            this.detailView.addChild(this.busesLeft);
            this.detailView.addChild(this.busesRight);
            this.detailView.addChild(this.busesHorizontal);
            this.detailView.addChild(this.synGrid);
            this.detailView.addChild(this.synapses);
            this.detailView.addChild(this.synapseCross);
            this.detailView.addChild(this.synDrivers);
            this.detailView.addChild(this.neurons);
            this.detailView.addChild(this.repeaters);
            this.detailView.addChild(this.repeaterBusConnections);
            this.detailView.addChild(this.busesLeftSprite);
            this.detailView.addChild(this.busesRightSprite);
            this.detailView.addChild(this.busesHorizontalSprite);
            this.detailView.addChild(this.synGridSprite);
            this.detailView.addChild(this.synapsesSprite);
            this.detailView.addChild(this.neuronsSprite);
            this.detailView.addChild(this.repeaterBusConnectionsSprite);
        },
    };
    /**
     * A renderer instance is needed to render graphics on the canvas
     */
    var Renderer = /** @class */ (function () {
        function Renderer(domParent, backgroundColor, canvasWidth, canvasHeight, forceCanvas, resolution) {
            if (forceCanvas === void 0) { forceCanvas = false; }
            if (resolution === void 0) { resolution = 1; }
            this.backgroundColor = backgroundColor;
            if (!forceCanvas) {
                this.renderer = PIXI.autoDetectRenderer({
                    width: canvasWidth,
                    height: canvasHeight,
                    backgroundColor: backgroundColor,
                    autoResize: true,
                    resolution: resolution
                });
            }
            else {
                this.renderer = new PIXI.CanvasRenderer({
                    width: canvasWidth,
                    height: canvasHeight,
                    backgroundColor: backgroundColor,
                    autoResize: true,
                    resolution: resolution
                });
            }
            this.renderer.view.style.position = 'absolute';
            this.renderer.view.style.display = 'block';
            this.renderer.view.id = "pixiJSCanvas";
            domParent.append(this.renderer.view);
        }
        /**
         * Render the stage (including all the substructure) on the canvas.
         * Call this method every time changes are made to the graphics elements that should be displayed.
         */
        Renderer.prototype.render = function () {
            this.renderer.render(pixiBackend.container.stage);
        };
        return Renderer;
    }());
    pixiBackend.Renderer = Renderer;
    /**
     * Draw a rectangle as a graphics object.
     * @param container PixiJS container to hold the graphics object
     * @param x x-coordinate of the top left corner of the rectangle.
     * @param y y-coordinate of the top left corner of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color of the rectangle. Requires hex-color in the form "0xffffff".
     * @param interactive Set to true to allow mouse interactivity with the object.
     * @param mouseoverFunction Any callback function for the mouseover event.
     * @param mouseoutFunction Any callback function for the mouseout event.
     * @param clickFunction Any callback function for the mouseclick event.
     */
    function drawRectangle(container, x, y, width, height, color, interactive, mouseoverFunction, mouseoutFunction, clickFunction) {
        if (interactive === void 0) { interactive = false; }
        if (mouseoverFunction === void 0) { mouseoverFunction = undefined; }
        if (mouseoutFunction === void 0) { mouseoutFunction = undefined; }
        if (clickFunction === void 0) { clickFunction = undefined; }
        var rectangle = new PIXI.Graphics();
        rectangle.beginFill(color);
        rectangle.drawRect(x, y, width, height);
        rectangle.endFill();
        container.addChild(rectangle);
        if (interactive) {
            rectangle.interactive = true;
            rectangle.hitArea = new PIXI.Rectangle(x, y, width, height);
            rectangle.mouseover = mouseoverFunction;
            rectangle.mouseout = mouseoutFunction;
            rectangle.click = clickFunction;
        }
        ;
    }
    pixiBackend.drawRectangle = drawRectangle;
    /**
     * Draw a circle as graphics object.
     * @param container PixiJS container to hold the graphics object.
     * @param x x-coordinate of the top left corner.
     * @param y y-coordinate of the top left corner.
     * @param radius Radius of the circle.
     * @param color Fill color for the circle. Requires hex-color in the form "0xffffff".
     */
    function drawCircle(container, x, y, radius, color) {
        var circle = new PIXI.Graphics();
        circle.beginFill(color);
        circle.drawCircle(x, y, radius);
        circle.endFill();
        container.addChild(circle);
    }
    pixiBackend.drawCircle = drawCircle;
    /**
     * Draw a triangle as graphics object. The triangles base is on the bottom
     * @param container PixiJS container to hold the graphics object.
     * @param x x-coordinate of the top left corner of the triangle.
     * @param y y-coordinate of the top left corner of the triangle.
     * @param width Width of the triangle.
     * @param height Height of the triangle.
     * @param color Fill color of the triangle. Requires hex-color in the form "0xffffff".
     */
    function drawTriangle(container, x, y, width, height, color) {
        var path = [x, y, x + width, y, x + width / 2, y - height, x, y];
        var triangle = new PIXI.Graphics();
        triangle.beginFill(color);
        triangle.drawPolygon(path);
        triangle.endFill();
        container.addChild(triangle);
    }
    pixiBackend.drawTriangle = drawTriangle;
    /**
     * Calculates the graphics data for triangles from the specified input parameters.
     * @param cornerOne X and Y coordinates of the first corners.
     * @param cornerTwo X and Y coordinates of the second corners.
     * @param cornerThree X and Y coordinates of the third corners.
     * @param color fill color.
     * @param triangles Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    function drawTriangles(cornerOne, cornerTwo, cornerThree, color, triangles) {
        if (triangles === void 0) { triangles = new PIXI.Graphics(); }
        if ((cornerOne.length !== cornerTwo.length) || (cornerOne.length !== cornerThree.length)) {
            throw (new Error("Lengths of cornerOne, cornerTwo, and cornerThree values are not matching"));
        }
        for (var i = 0; i < cornerOne.length; i++) {
            var path = [
                cornerOne[i].x, cornerOne[i].y,
                cornerTwo[i].x, cornerTwo[i].y,
                cornerThree[i].x, cornerThree[i].y,
                cornerOne[i].x, cornerOne[i].y
            ];
            triangles.beginFill(color);
            triangles.drawPolygon(path);
            triangles.endFill();
        }
        return triangles;
    }
    pixiBackend.drawTriangles = drawTriangles;
    /**
     * Calculates the graphics data for rectangles from the specified input parameters.
     * @param xValues x-coordinates of the top left corners of the rectangles.
     * @param yValues y-coordinates of the top left corners of the rectangles.
     * @param widthValues Widths of the rectangles.
     * @param heightValues Heights of the rectangles.
     * @param color Colors of the rectangles. Requires hex-colors in the form 0xffffff.
     * @param rectangles Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    function drawRectangles(x, y, width, height, color, rectangles, alpha, lineWidth, lineColor) {
        if (rectangles === void 0) { rectangles = new PIXI.Graphics(); }
        if (alpha === void 0) { alpha = 1; }
        if (lineWidth === void 0) { lineWidth = 0; }
        if (lineColor === void 0) { lineColor = 0; }
        if ((x.length !== y.length) || (x.length !== width.length) || (x.length !== height.length)) {
            throw (new Error("Lengths of xValues, yValues, widthValues and heightValues do not match"));
        }
        rectangles.lineStyle(lineWidth, lineColor);
        for (var i = 0; i < x.length; i++) {
            rectangles.beginFill(color, alpha);
            rectangles.drawRect(x[i], y[i], width[i], height[i]);
            rectangles.endFill();
        }
        rectangles.lineStyle(0, 0);
        return rectangles;
    }
    pixiBackend.drawRectangles = drawRectangles;
    /**
     * Calculates the graphics data for lines from the specified input parameters.
     * @param xOne The X coordinates of the first endpoints.
     * @param yOne The Y coordinates of the first endpoints.
     * @param xTwo The X coordinates of the second enpoints.
     * @param yTwo The Y coordinates of the second endpoints.
     * @param width The width values of the lines.
     * @param color The fill color.
     * @param lines Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    function drawStraightLines(xOne, yOne, xTwo, yTwo, width, color, lines) {
        if (lines === void 0) { lines = new PIXI.Graphics(); }
        if ((xOne.length !== yOne.length) || (xOne.length !== xTwo.length) || (xOne.length !== width.length)) {
            throw (new Error("Lengths of xOne, yOne, xTwo, yTwo and width values do not match"));
        }
        for (var i = 0; i < xOne.length; i++) {
            lines.lineStyle(width[i], color);
            lines.moveTo(xOne[i], yOne[i]);
            lines.lineTo(xTwo[i], yTwo[i]);
        }
        lines.lineStyle(0, 0);
        return lines;
    }
    pixiBackend.drawStraightLines = drawStraightLines;
    function drawLines(x, y, width, color, lines) {
        if (lines === void 0) { lines = new PIXI.Graphics(); }
        if ((x.length !== y.length)) {
            throw (new Error("Lengths of x and y do not match"));
        }
        if (x.length < 2) {
            return;
        }
        lines.lineStyle(width, color);
        lines.moveTo(x[0], y[0]);
        for (var i = 1; i < x.length; i++) {
            lines.lineTo(x[i], y[i]);
        }
        lines.lineStyle(0, 0);
        return lines;
    }
    pixiBackend.drawLines = drawLines;
    /**
     * Calculates the graphics data for circles from the specified input parameters
     * @param x The X coordinates of the centers of the circles.
     * @param y The Y coordinates of the centers of the circles.
     * @param radius The radii of the circles.
     * @param color The fill color
     * @param circles Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    function drawCircles(x, y, radius, color, circles) {
        if (circles === void 0) { circles = new PIXI.Graphics(); }
        if ((x.length !== y.length) || (x.length !== radius.length)) {
            throw (new Error("Lengths of x, y and radius Values do not match"));
        }
        for (var i = 0; i < x.length; i++) {
            circles.beginFill(color);
            circles.drawCircle(x[i], y[i], radius[i]);
            circles.endFill();
        }
        return circles;
    }
    pixiBackend.drawCircles = drawCircles;
    /**
     * Store graphics data as graphics object.
     * @param graphicsObject graphics data to store.
     * @param container container to store the sprite in.
     */
    function storeGraphics(graphicsObject, container) {
        container.addChild(graphicsObject);
    }
    pixiBackend.storeGraphics = storeGraphics;
    /**
     * Store graphics data as a sprite. The graphics data is renderered as a sprite with fixed resolution.
     * @param graphicsObject graphics data to store.
     * @param container container to store the sprite in.
     * @param resolution resolution of the sprite.
     */
    function storeSprite(graphicsObject, container, resolution) {
        if (resolution === void 0) { resolution = 10; }
        var sprite = new PIXI.Sprite(graphicsObject.generateCanvasTexture(0, resolution));
        sprite.position = new PIXI.Point(graphicsObject.getBounds().x, graphicsObject.getBounds().y);
        container.addChild(sprite);
    }
    pixiBackend.storeSprite = storeSprite;
    /**
     * Draws text in the boundaries of the rectangle. The text is sized to fit exactly in, either by width or by height.
     * @param container PixiJS container to hold the text object.
     * @param x x-coordinate of the top left corner of the text.
     * @param y y-coordinate of the top left corner of the text.
     * @param rectWidth Width of the rectangle, the text should fit in.
     * @param rectHeight Height of the rectangle, the text should fit in.
     * @param style style-object for the text.
     */
    function drawTextInRectangle(container, x, y, rectWidth, rectHeight, textContent, style) {
        var text = new PIXI.Text(textContent, style);
        text.x = x;
        text.y = y;
        // set width & height
        var originalWidth = text.width;
        var originalHeight = text.height;
        text.width = rectWidth;
        text.height *= text.width / originalWidth;
        if (text.height > rectHeight) {
            originalHeight = text.height;
            text.height = rectHeight;
            text.width *= text.height / originalHeight;
        }
        container.addChild(text);
    }
    pixiBackend.drawTextInRectangle = drawTextInRectangle;
    /**
     * Draw just the border, not the fill of a rectangle.
     * @param container PixiJS container to hold the graphics object.
     * @param x x-coordinate of the top left corner of the rectangle.
     * @param y y-coordinate of the top left corner of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param lineWidth Width of the border-line.
     * @param color Color of the border-line. Requires hex-color in the form "0xffffff".
     * @param alpha Transparency of the border-line. Requires value between 0 and 1.
     */
    function drawRectangleBorder(container, x, y, width, height, lineWidth, color, alpha) {
        var rectangle = new PIXI.Graphics();
        rectangle.lineStyle(lineWidth, color, alpha);
        rectangle.drawRect(x, y, width, height);
        container.addChild(rectangle);
    }
    pixiBackend.drawRectangleBorder = drawRectangleBorder;
    /**
     * Draw an image from a local url.
     * @param container PixiJS container to hold the sprite.
     * @param url Image file-path.
     * @param x x-coordinate of the top left corner of the image.
     * @param y y-coordinate of the top left corner of the image.
     * @param width Width of the image.
     * @param height Height of the image.
     */
    function drawImage(container, url, x, y, width, height, alpha) {
        if (alpha === void 0) { alpha = 1; }
        var image = PIXI.Sprite.fromImage(url);
        image.position = new PIXI.Point(x, y);
        image.width = width;
        image.height = height;
        image.alpha = alpha;
        container.addChild(image);
    }
    pixiBackend.drawImage = drawImage;
    /**
     * Remove a child of any type from a PixiJS container.
     * @param container PixiJS container to remove the child from.
     * @param childIndex Index of the child to be removed (starts at 0).
     */
    function removeChild(container, childIndex) {
        if (childIndex !== -1) {
            container.removeChild(container.children[childIndex]);
        }
        else {
            throw (new Error("Cannot delete child " + childIndex + " in container " + container + "."));
        }
    }
    pixiBackend.removeChild = removeChild;
    /**
     * Remove all children of any type from a PixiJS container.
     * @param container PixiJS container to remove children from.
     */
    function removeAllChildren(container) {
        var numChildren = container.children.length;
        for (var i = 0; i < numChildren; i++) {
            container.removeChild(container.children[0]);
        }
    }
    pixiBackend.removeAllChildren = removeAllChildren;
    /**
     * Zoom the whole stage in.
     * @param factor zoom-factor.
     * @param x x-coordinate of the target position. Typically the x-coordinate of the mouse position.
     * @param y y-coordinate of the target position. Typically the y-coordinate of the mouse position.
     */
    function zoomIn(factor, x, y) {
        var scale = this.container.stage.transform.scale;
        var position = this.container.stage.transform.position;
        var oldScale = scale.x;
        scale.x *= factor;
        scale.y *= factor;
        position.x -= (x - position.x) * Math.abs(scale.x / oldScale - 1);
        position.y -= (y - position.y) * Math.abs(scale.x / oldScale - 1);
    }
    pixiBackend.zoomIn = zoomIn;
    /**
     * Zoom the whole stage out.
     * @param factor zoom-factor.
     * @param x x-coordinate of the target position. Typically the x-coordinate of the mouse position.
     * @param y y-coordinate of the target position. Typically the y-coordinate of the mouse position.
     */
    function zoomOut(factor, x, y) {
        var scale = this.container.stage.transform.scale;
        var position = this.container.stage.transform.position;
        var oldScale = scale.x;
        scale.x /= factor;
        scale.y /= factor;
        position.x += (x - position.x) * Math.abs(scale.x / oldScale - 1);
        position.y += (y - position.y) * Math.abs(scale.x / oldScale - 1);
    }
    pixiBackend.zoomOut = zoomOut;
    /**
     * Move the whole stage (panning).
     * @param deltaX Shift in x-direction (positive or negative value).
     * @param deltaY Shift in y-direction (positive or negative value).
     */
    function moveStage(deltaX, deltaY) {
        this.container.stage.position.x += deltaX;
        this.container.stage.position.y += deltaY;
    }
    pixiBackend.moveStage = moveStage;
    /**
     * Animate panning of the whole stage.
     * @param x1 Start x-coordinate
     * @param y1 Start y-coordinate
     * @param x2 Target x-coordinate
     * @param y2 Target y-coordinate
     * @param duration Animation duration in milliseconds
     */
    function animateStagePosition(x1, y1, x2, y2, duration) {
        var numberSteps = Math.floor(duration / 20);
        var step = {
            x: (x2 - x1) / numberSteps,
            y: (y2 - y1) / numberSteps,
        };
        var stepTracker = 0;
        var timer = setInterval(function () {
            pixiBackend.moveStage(step.x, step.y);
            pixiBackend.renderer.render();
            stepTracker++;
            if (stepTracker === numberSteps) {
                clearInterval(timer);
            }
        }, 20);
    }
    pixiBackend.animateStagePosition = animateStagePosition;
    /**
     * Checks if the mouse is within the boundaries of a rectangle.
     * @param mousePosition x-y-position of the mouse.
     * @param x x-coordinate of the rectangle.
     * @param y y-coordinate of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    function mouseInRectangle(mousePosition, x, y, width, height) {
        var scale = this.container.stage.transform.scale.x;
        var stagePosition = this.container.stage.transform.position;
        if ((mousePosition.x >= x * scale + stagePosition.x)
            && (mousePosition.x <= (x + width) * scale + stagePosition.x)
            && (mousePosition.y >= y * scale + stagePosition.y)
            && (mousePosition.y <= (y + height) * scale + stagePosition.y)) {
            return (true);
        }
        ;
    }
    pixiBackend.mouseInRectangle = mouseInRectangle;
    /**
     * Checks if the mouse is within the boundaries of a line.
     * @param mouseX x-coordinate of the mouse.
     * @param mouseY y-coordinate of the mouse.
     * @param lineX1 x-coordinate of the first line point.
     * @param lineY1 y-coordinate of the first line point.
     * @param lineX2 x-coordinate of the second line point.
     * @param lineY2 y-coordinate of the second line point.
     * @param width line-width.
     */
    function mouseInLine(mouseX, mouseY, lineX1, lineY1, lineX2, lineY2, width) {
        var scale = this.container.stage.transform.scale.x;
        var stagePosition = this.container.stage.transform.position;
        lineX1 = lineX1 * scale + stagePosition.x;
        lineY1 = lineY1 * scale + stagePosition.y;
        lineX2 = lineX2 * scale + stagePosition.x;
        lineY2 = lineY2 * scale + stagePosition.y;
        width *= scale;
        return tools.pointInLine(mouseX, mouseY, lineX1, lineY1, lineX2, lineY2, width);
    }
    pixiBackend.mouseInLine = mouseInLine;
    /**
     * Checks if a rectangle is at least partially within in the boundaries of the window.
     * @param x x-coordinate of the rectangle.
     * @param y y-coordinate of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    function rectanglePartiallyInWindow(x, y, width, height) {
        var view = {
            width: $(window).width(),
            height: $(window).height(),
        };
        var scale = this.container.stage.transform.scale.x;
        var stagePosition = this.container.stage.transform.position;
        x *= scale;
        y *= scale;
        width *= scale;
        height *= scale;
        if ((x + stagePosition.x < view.width)
            && (x + width + stagePosition.x > 0)
            && (y + stagePosition.y < view.height)
            && (y + height + stagePosition.y > 0)) {
            return true;
        }
        else {
            return false;
        }
        ;
    }
    pixiBackend.rectanglePartiallyInWindow = rectanglePartiallyInWindow;
})(pixiBackend || (pixiBackend = {}));
/// <reference path="module.d.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * Representation of a HICANN. Position is the position in the visualization and is added when the HICANN is drawn the first time.
     */
    var HICANN = /** @class */ (function () {
        function HICANN(index, x, y, hasInputs, hasNeurons, isAvailable, numBusesHorizontal, numBusesLeft, numBusesRight, numBusesVertical, numInputs, numNeurons) {
            this.index = index;
            this.x = x;
            this.y = y;
            this.position = {
                x: undefined,
                y: undefined
            };
            this.hasInputs = hasInputs;
            this.hasNeurons = hasNeurons;
            this.isAvailable = isAvailable;
            this.numBusesHorizontal = numBusesHorizontal;
            this.numBusesLeft = numBusesLeft;
            this.numBusesRight = numBusesRight;
            this.numBusesVertical = numBusesVertical;
            this.numInputs = numInputs;
            this.numNeurons = numNeurons;
        }
        return HICANN;
    }());
    internalModule.HICANN = HICANN;
    /**
     * Representation of a HICANN wafer. The data is parsed from a configuration xml-file using the Marocco JavaScript API.
     */
    var Wafer = /** @class */ (function () {
        function Wafer() {
            this.marocco = undefined;
            // hardcode some wafer properties, because they are not yet wrapped.
            this.hicanns = [];
            this.enumMin = 0;
            this.enumMax = 383;
            this.xMin = 0;
            this.xMax = 35;
            this.yMin = 0;
            this.yMax = 15;
            // settings for HICANN proportions, try to fit graphics to HICANN image
            this.hicannMargin = 0;
            this.hicannWidth = 100;
            this.hicannHeight = 200;
            this.inputTriangleHeight = 30;
            this.busesLeftPosition = {
                x: 4 / 100 * this.hicannWidth,
                y: 14.5 / 200 * this.hicannHeight,
                width: 12 / 100 * this.hicannWidth,
                height: (this.hicannHeight - 2 * 14.5) / 200 * this.hicannHeight
            };
            this.busesRightPosition = {
                x: (this.hicannWidth - 4 - 12) / 100 * this.hicannWidth,
                y: 14.5 / 200 * this.hicannHeight,
                width: 12 / 100 * this.hicannWidth,
                height: (this.hicannHeight - 2 * 14.5) / 200 * this.hicannHeight
            };
            this.busesHorizontalPosition = {
                original: {
                    x: 19.8 / 100 * this.hicannWidth,
                    y: 97 / 200 * this.hicannHeight,
                    width: 60.6 / 100 * this.hicannWidth,
                    height: 6.2 / 200 * this.hicannHeight
                },
                current: {
                    x: 19.8 / 100 * this.hicannWidth,
                    y: 97 / 200 * this.hicannHeight,
                    width: 60.6 / 100 * this.hicannWidth,
                    height: 6.2 / 200 * this.hicannHeight
                }
            };
            this.synapseArraysTopPosition = {
                x: 23.5 / 100 * this.hicannWidth,
                y: this.busesLeftPosition.y,
                width: this.hicannWidth - (2 * 23.5 / 100 * this.hicannWidth),
                height: 71.5 / 200 * this.hicannHeight,
            };
            this.synapseArraysBottomPosition = {
                x: 23.5 / 100 * this.hicannWidth,
                y: this.busesLeftPosition.y + this.busesLeftPosition.height - this.synapseArraysTopPosition.height,
                width: this.synapseArraysTopPosition.width,
                height: this.synapseArraysTopPosition.height,
            };
            this.synDriverPosition = {
                topLeft: {
                    x: this.synapseArraysTopPosition.x - 1 / 100 * this.hicannWidth,
                    y: this.synapseArraysTopPosition.y,
                },
                topRight: {
                    x: this.synapseArraysTopPosition.x + this.synapseArraysBottomPosition.width,
                    y: this.synapseArraysTopPosition.y,
                },
                bottomLeft: {
                    x: this.synapseArraysBottomPosition.x - 1 / 100 * this.hicannWidth,
                    y: this.synapseArraysBottomPosition.y,
                },
                bottomRight: {
                    x: this.synapseArraysBottomPosition.x + this.synapseArraysBottomPosition.width,
                    y: this.synapseArraysBottomPosition.y,
                },
                width: 1 / 100 * this.hicannWidth,
                height: this.synapseArraysTopPosition.height,
            };
            this.neuronArrayPosition = {
                top: {
                    x: this.synapseArraysTopPosition.x,
                    y: this.synapseArraysTopPosition.y + this.synapseArraysTopPosition.height + 0.5 / 200 * this.hicannHeight,
                },
                bottom: {
                    x: this.synapseArraysBottomPosition.x,
                    y: this.synapseArraysBottomPosition.y - (3 / 200 + 0.5 / 200) * this.hicannHeight,
                },
                width: this.synapseArraysTopPosition.width,
                height: 3 / 200 * this.hicannHeight
            };
            this.repeaterBlockPosition = {
                horizontal: {
                    left: {
                        original: {
                            x: 1 / 100 * this.hicannWidth,
                            y: 87.5 / 200 * this.hicannHeight
                        },
                        current: {
                            x: 1 / 100 * this.hicannWidth,
                            y: 87.5 / 200 * this.hicannHeight
                        }
                    },
                    right: {
                        original: {
                            x: (1 - 4 / 100) * this.hicannWidth,
                            y: 87.5 / 200 * this.hicannHeight
                        },
                        current: {
                            x: (1 - 4 / 100) * this.hicannWidth,
                            y: 87.5 / 200 * this.hicannHeight
                        }
                    },
                    width: 3 / 100 * this.hicannWidth,
                    height: {
                        original: 25 / 200 * this.hicannHeight,
                        current: 25 / 200 * this.hicannHeight
                    }
                },
                vertical: {
                    top: {
                        left: {
                            original: {
                                x: 0.8 / 100 * this.hicannWidth,
                                y: 1 / 200 * this.hicannHeight
                            },
                            current: {
                                x: 0.8 / 100 * this.hicannWidth,
                                y: 1 / 200 * this.hicannHeight
                            }
                        },
                        right: {
                            original: {
                                x: 50 / 100 * this.hicannWidth,
                                y: 1 / 200 * this.hicannHeight
                            },
                            current: {
                                x: 50 / 100 * this.hicannWidth,
                                y: 1 / 200 * this.hicannHeight
                            }
                        }
                    },
                    bottom: {
                        left: {
                            original: {
                                x: 0.8 / 100 * this.hicannWidth,
                                y: (1 - 3 / 200) * this.hicannHeight
                            },
                            current: {
                                x: 0.8 / 100 * this.hicannWidth,
                                y: (1 - 3 / 200) * this.hicannHeight
                            }
                        },
                        right: {
                            original: {
                                x: 50 / 100 * this.hicannWidth,
                                y: (1 - 3 / 200) * this.hicannHeight
                            },
                            current: {
                                x: 50 / 100 * this.hicannWidth,
                                y: (1 - 3 / 200) * this.hicannHeight
                            }
                        }
                    },
                    width: {
                        original: ((50 - 0.8) / 100) * this.hicannWidth,
                        current: ((50 - 0.8) / 100) * this.hicannWidth
                    },
                    height: 2 / 200 * this.hicannHeight
                }
            };
            this.numNeuronsMax = 0;
            this.numInputsMax = 0;
            this.numBusesLeftMax = 0;
            this.numBusesRightMax = 0;
            this.numBusesHorizontalMax = 0;
        }
        /**
         * process the marocco results file using the Marocco JavaScript API.
         * - new instances of HICANN are created
         * @param networkFilePath path to the results file, located in the virtual emscripten filesystem
         */
        Wafer.prototype.loadOverviewData = function (networkFilePath) {
            if (networkFilePath) {
                try {
                    this.marocco = Module.Marocco.from_file(networkFilePath);
                }
                catch (error) {
                    this.marocco = new Module.Marocco();
                }
            }
            else {
                this.marocco = new Module.Marocco();
            }
            $("#setupScreen").fadeTo(1500, 0, function () { $("#setupScreen").css("display", "none"); });
            // reading properties from marocco
            for (var i = this.enumMin; i <= this.enumMax; i++) {
                var enumRanged = new Module.HICANNOnWafer_EnumRanged_type(i);
                var hicann = new Module.HICANNOnWafer(enumRanged);
                var properties_1 = this.marocco.properties(hicann);
                this.hicanns.push(new HICANN(i, hicann.x().value(), hicann.y().value(), properties_1.has_inputs(), properties_1.has_neurons(), properties_1.is_available(), properties_1.num_buses_horizontal(), properties_1.num_buses_left(), properties_1.num_buses_right(), properties_1.num_buses_vertical(), properties_1.num_inputs(), properties_1.num_neurons()));
            }
            this.maxPropertyValues();
        };
        /**
         * Find out the maximum values for HICANN properties
         */
        Wafer.prototype.maxPropertyValues = function () {
            for (var _i = 0, _a = this.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                if (hicann.numNeurons > this.numNeuronsMax) {
                    this.numNeuronsMax = hicann.numNeurons;
                }
                if (hicann.numInputs > this.numInputsMax) {
                    this.numInputsMax = hicann.numInputs;
                }
                if (hicann.numBusesLeft > this.numBusesLeftMax) {
                    this.numBusesLeftMax = hicann.numBusesLeft;
                }
                if (hicann.numBusesRight > this.numBusesRightMax) {
                    this.numBusesRightMax = hicann.numBusesRight;
                }
                if (hicann.numBusesHorizontal > this.numBusesHorizontalMax) {
                    this.numBusesHorizontalMax = hicann.numBusesHorizontal;
                }
            }
        };
        /**
         * Calculate the index/enum-coordinate of the northern HICANN, if existent.
         */
        Wafer.prototype.northernHicann = function (hicannIndex) {
            var northernHicann;
            for (var _i = 0, _a = this.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                if ((hicann.y == this.hicanns[hicannIndex].y - 1)
                    && (hicann.x == this.hicanns[hicannIndex].x)) {
                    northernHicann = hicann;
                    break;
                }
                ;
            }
            ;
            return (northernHicann ? northernHicann.index : undefined);
        };
        /**
         * Calculate the index/enum-coordinate of the southern HICANN, if existent.
         */
        Wafer.prototype.southernHicann = function (hicannIndex) {
            var southernHicann;
            for (var _i = 0, _a = this.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                if ((hicann.y == this.hicanns[hicannIndex].y + 1)
                    && (hicann.x == this.hicanns[hicannIndex].x)) {
                    southernHicann = hicann;
                    break;
                }
                ;
            }
            ;
            return (southernHicann ? southernHicann.index : undefined);
        };
        /**
         * Calculate the index/enum-coordinate of the eastern HICANN, if existent.
         */
        Wafer.prototype.easternHicann = function (hicannIndex) {
            var easternHicann;
            for (var _i = 0, _a = this.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                if ((hicann.y == this.hicanns[hicannIndex].y)
                    && (hicann.x == this.hicanns[hicannIndex].x + 1)) {
                    easternHicann = hicann;
                    break;
                }
                ;
            }
            ;
            return (easternHicann ? easternHicann.index : undefined);
        };
        /**
         * Calculate the index/enum-coordinate of the western HICANN, if existent.
         */
        Wafer.prototype.westernHicann = function (hicannIndex) {
            var westernHicann;
            for (var _i = 0, _a = this.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                if ((hicann.y == this.hicanns[hicannIndex].y)
                    && (hicann.x == this.hicanns[hicannIndex].x - 1)) {
                    westernHicann = hicann;
                    break;
                }
                ;
            }
            ;
            return (westernHicann ? westernHicann.index : undefined);
        };
        return Wafer;
    }());
    internalModule.Wafer = Wafer;
    ;
})(internalModule || (internalModule = {}));
/**
 * A collection of functions useful functions, that are not specific to the visualization.
 */
var tools;
(function (tools) {
    /**
     * For a color gradient, where "colorOne" corresponds to "zero" and "colorTwo" corresponds to "max",
     * the color corresponding to "value" is calculated.
     */
    function colorInGradient(colorOne, colorTwo, max, value) {
        var frac = max ? value / max : 0;
        var c1 = {
            r: parseInt(colorOne.slice(0, 2), 16),
            g: parseInt(colorOne.slice(2, 4), 16),
            b: parseInt(colorOne.slice(4, 6), 16)
        };
        var c2 = {
            r: parseInt(colorTwo.slice(0, 2), 16),
            g: parseInt(colorTwo.slice(2, 4), 16),
            b: parseInt(colorTwo.slice(4, 6), 16)
        };
        var diff = {
            r: c2.r - c1.r,
            g: c2.g - c1.g,
            b: c2.b - c1.b
        };
        var cnew = {
            r: Math.floor(diff.r * frac + c1.r),
            g: Math.floor(diff.g * frac + c1.g),
            b: Math.floor(diff.b * frac + c1.b)
        };
        var cnew_hex = {
            r: ((cnew.r).toString(16).length === 2) ?
                (cnew.r).toString(16) : (0).toString(16) + (cnew.r).toString(16),
            g: ((cnew.g).toString(16).length === 2) ?
                (cnew.g).toString(16) : (0).toString(16) + (cnew.g).toString(16),
            b: ((cnew.b).toString(16).length === 2) ?
                (cnew.b).toString(16) : (0).toString(16) + (cnew.b).toString(16),
        };
        var result = "0x" + cnew_hex.r + cnew_hex.g + cnew_hex.b;
        return result;
    }
    tools.colorInGradient = colorInGradient;
    /**
     * returns all teh digits in a string, concatenated.
     * e.g. "Hello13Visu08" -> 1308
     */
    function numberInString(string) {
        var number = "";
        for (var letter in string) {
            number += (!isNaN(parseInt(string[letter]))) ? string[letter] : "";
        }
        return parseInt(number);
    }
    tools.numberInString = numberInString;
    /**
     * returns a random number between "bottom" and "top".
     */
    function randomNumber(bottom, top) {
        return (Math.floor(Math.random() * (top - bottom + 1) + bottom));
    }
    ;
    /**
     * returns a semi-random color in hexadecimal form (e.g. 0xffffff).
     */
    function randomHexColor() {
        // semi-random color
        var goldenRatioConjugate = 0.618033988749895;
        var hue = (Math.random() + goldenRatioConjugate) % 1;
        var hsv = hsvToRGB(hue, 0.6, 0.95);
        var color = {
            r: hsv[0],
            g: hsv[1],
            b: hsv[2]
        };
        // convert to Hex color
        var colorHex = {
            r: ((color.r).toString(16).length === 2) ?
                (color.r).toString(16) : (0).toString(16) + (color.r).toString(16),
            g: ((color.g).toString(16).length === 2) ?
                (color.g).toString(16) : (0).toString(16) + (color.g).toString(16),
            b: ((color.b).toString(16).length === 2) ?
                (color.b).toString(16) : (0).toString(16) + (color.b).toString(16),
        };
        // concatenate and return
        return "0x" + colorHex.r + colorHex.g + colorHex.b;
    }
    tools.randomHexColor = randomHexColor;
    ;
    /**
     * HSV values in [0..1[
     * returns [r, g, b] values from 0 to 255
     * @param h hue
     * @param s saturation
     * @param v value
     */
    function hsvToRGB(h, s, v) {
        var h_i = Math.floor(h * 6);
        var f = h * 6 - h_i;
        var p = v * (1 - s);
        var q = v * (1 - f * s);
        var t = v * (1 - (1 - f) * s);
        var r;
        var g;
        var b;
        switch (h_i) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            default:// 5
                r = v;
                g = p;
                b = q;
        }
        return [Math.floor(r * 256), Math.floor(g * 256), Math.floor(b * 256)];
    }
    /**
     * Calculate the intersection of two rectangles.
     * caveat: works only in the special case of two intersecting bus segments
     */
    function intersectionRectangle(rect1, rect2) {
        var position = {};
        // intersection x & width
        if (rect1.width < rect2.width) {
            position.x = rect1.x;
            position.width = rect1.width;
        }
        else {
            position.x = rect2.x;
            position.width = rect2.width;
        }
        ;
        // intersection y & height
        if (rect1.height < rect2.height) {
            position.y = rect1.y;
            position.height = rect1.height;
        }
        else {
            position.y = rect2.y;
            position.height = rect2.height;
        }
        ;
        return (position);
    }
    tools.intersectionRectangle = intersectionRectangle;
    ;
    function intersectionPoint(line1, line2) {
        var position = {};
        if (line1.y1 === line1.y2) {
            position = {
                x: line2.x1,
                y: line1.y1,
            };
        }
        else {
            position = {
                x: line1.x1,
                y: line2.y1,
            };
        }
        return position;
    }
    tools.intersectionPoint = intersectionPoint;
    /**
     * calculates center position and radius of a circle that fits exactly into the square
     */
    function circleInSquare(square) {
        // circle x
        var x = square.x + square.width / 2;
        // circle y
        var y = square.y + square.height / 2;
        // circle radius
        var radius = square.width / 2;
        return ({
            x: x,
            y: y,
            radius: radius,
        });
    }
    tools.circleInSquare = circleInSquare;
    /**
     * check if point is inside boundaries of rectangle
     */
    function pointInRectangle(point, rectangle) {
        if ((point.x >= rectangle.x && point.x <= (rectangle.x + rectangle.width))
            && (point.y >= rectangle.y && point.y <= (rectangle.y + rectangle.height))) {
            return true;
        }
        else {
            return false;
        }
    }
    tools.pointInRectangle = pointInRectangle;
    /**
     * check if mouse is inside boundaries of HTML div
     * @param mouse position of the mouse
     * @param id ID of the div (must include the #!)
     */
    function mouseInDiv(mouse, id) {
        return (tools.pointInRectangle({
            x: mouse.x,
            y: mouse.y
        }, {
            x: $(id).offset().left,
            y: $(id).offset().top,
            width: $(id).outerWidth(),
            height: $(id).outerHeight(),
        }));
    }
    tools.mouseInDiv = mouseInDiv;
    /**
     * Gaußsche Summenformel "kleiner Gauss"
     */
    function kleinerGauss(n) {
        return (Math.pow(n, 2) + n) / 2;
    }
    tools.kleinerGauss = kleinerGauss;
    /**
     * Calculate the distance between two two-dimensional points
     */
    function distanceBetweenPoints(point1, point2) {
        var distance = Math.sqrt(Math.pow((point1.x - point2.x), 2)
            + Math.pow((point1.y - point2.y), 2));
        return (distance);
    }
    tools.distanceBetweenPoints = distanceBetweenPoints;
    function shiftToZero(x, y, x2, y2) {
        if (y < y2) {
            x2 -= x;
            y2 -= y;
            x -= x;
            y -= y;
        }
        else {
            x -= x2;
            y -= y2;
            x2 -= x2;
            y2 -= y2;
        }
        return [x, y, x2, y2];
    }
    function calcShift(x, y, x2, y2) {
        return [-x, -y];
    }
    function calcAlpha(x, y, x2, y2) {
        return (Math.atan((x2 - x) / (y2 - y)));
    }
    function rotate(x, y, alpha) {
        var x1 = Math.cos(alpha) * x - Math.sin(alpha) * y;
        var y1 = Math.sin(alpha) * x + Math.cos(alpha) * y;
        return [x1, y1];
    }
    function pointInLine(pointX, pointY, lineX1, lineY1, lineX2, lineY2, width) {
        if ((lineY1 > lineY2) || ((lineY1 === lineY2) && (lineX1 > lineX2))) {
            _a = [lineX2, lineX1], lineX1 = _a[0], lineX2 = _a[1];
            _b = [lineY2, lineY1], lineY1 = _b[0], lineY2 = _b[1];
        }
        var shift = calcShift(lineX1, lineY1, lineX2, lineY2);
        pointX += shift[0];
        pointY += shift[1];
        lineX1 += shift[0];
        lineY1 += shift[1];
        lineX2 += shift[0];
        lineY2 += shift[1];
        var alpha = calcAlpha(lineX1, lineY1, lineX2, lineY2);
        _c = rotate(pointX, pointY, alpha), pointX = _c[0], pointY = _c[1];
        _d = rotate(lineX1, lineY1, alpha), lineX1 = _d[0], lineY1 = _d[1];
        _e = rotate(lineX2, lineY2, alpha), lineX2 = _e[0], lineY2 = _e[1];
        if ((pointX >= (lineX1 - width / 2)) && (pointX <= (lineX1 + width / 2)) && (pointY >= lineY1) && (pointY <= lineY2)) {
            return true;
        }
        else {
            return false;
        }
        ;
        var _a, _b, _c, _d, _e;
    }
    tools.pointInLine = pointInLine;
})(tools || (tools = {}));
/// <reference path="tools.ts" />
/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * The Detailview includes detailed representations of HICANN elements.
     * Buses are drawn as thin rectangles and the synapse arrays as arrays of small squares.
     * The detailview is divided into two sub-levels. The first one (detailview) drawing elements as Sprites,
     * the second one (detialviewLevelTwo) drawing real graphics elements.
     */
    var Detailview = /** @class */ (function () {
        function Detailview(wafer) {
            /**
             * set this property when detailview is entered or left
             */
            this.enabled = false;
            /**
             * set this property when detailviewLevelTwo is entered or left.
             */
            this.levelTwoEnabled = false;
            /**
             * Index of the HICANN that is currently in detailview. Used for auto mode.
             */
            this.currentHicann = undefined;
            /**
             * Index of northern HICANN of the one currently in detailview. Used for auto mode.
             */
            this.northernHicann = undefined;
            /**
             * Index of southern HICANN of the one currently in detailview. Used for auto mode.
             */
            this.southernHicann = undefined;
            /**
             * Index of eastern HICANN of the one currently in detailview. Used for auto mode.
             */
            this.easternHicann = undefined;
            /**
             * Index of western HICANN of the one currently in detailview. Used for auto mode.
             */
            this.westernHicann = undefined;
            /**
             * zoom-scale threshold to start detailview
             */
            this.threshold = NaN;
            /**
             * zoom-scale threshold to start detailviewLevelTwo
             */
            this.threshold2 = NaN;
            /**
             * Hardcoded number of neurons on a HICANN.
             */
            this.numNeurons = 256;
            /**
             * Hardcoded number of synapse rows in a synapse array on a HICANN.
             */
            this.numSynapsesVertical = 224;
            /**
             * Number of synapse drivers on one HICANN.
             */
            this.numSynapseDrivers = 224;
            /**
             * Hardcoded number of vertical buses on a HICANN.
             */
            this.numBusesVertical = 128;
            /**
             * Hardcoded number of horizontal buses on a HICANN.
             */
            this.numBusesHorizontal = 64;
            /**
             * Unit distances between synapse array and Buses.
             */
            this.gap = 4;
            /**
             * Number of repeaters in the left and right block together.
             */
            this.numRepeatersHorizontal = 64;
            /**
             * Number of repeaters in the top and bottom blocks together.
             */
            this.numRepeatersVertical = 256;
            this.wafer = wafer;
            this.determineThreshold($(window).height());
        }
        Object.defineProperty(Detailview.prototype, "vRepeaterDistance", {
            /**
             * Distance between vertical repeaters (top and bottom)
             */
            get: function () {
                return this.wafer.repeaterBlockPosition.vertical.width.current * 2 / this.numRepeatersVertical;
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "hRepeaterDistance", {
            /**
             * Distance between horizontal repeaters (left and right)
             */
            get: function () {
                return this.wafer.repeaterBlockPosition.horizontal.height.current / (this.numRepeatersHorizontal);
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "vBusDistance", {
            /**
             * Distance between vertical buses (left and right)
             */
            get: function () {
                return this.wafer.busesLeftPosition.width / this.numBusesVertical;
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "hBusDistance", {
            /**
             * Distance between horizontal buses
             */
            get: function () {
                return this.wafer.busesHorizontalPosition.current.height / this.numBusesHorizontal;
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "vBusWidth", {
            /**
             * Width of a vertical bus
             */
            get: function () {
                return this.vBusDistance / 2;
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "hBusWidth", {
            /**
             * Width of a horizontal bus
             */
            get: function () {
                return this.vBusDistance / 2;
            },
            enumerable: true,
            configurable: true
        });
        Object.defineProperty(Detailview.prototype, "edge", {
            /**
             * Margin around HICANN. used to calculate when to start the detailview.
             */
            get: function () {
                return (this.wafer.hicannWidth / 4);
            },
            enumerable: true,
            configurable: true
        });
        /**
         * Calculate the position of the center of a HICANN
         */
        Detailview.prototype.hicannCenter = function (hicannIndex) {
            var transform = pixiBackend.container.stage.transform;
            var scale = transform.scale.x;
            var stagePosition = transform.position;
            var hicannCenter = {
                x: (this.wafer.hicanns[hicannIndex].x * (this.wafer.hicannWidth + this.wafer.hicannMargin) + this.wafer.hicannWidth / 2)
                    * scale + stagePosition.x,
                y: (this.wafer.hicanns[hicannIndex].y * (this.wafer.hicannHeight + this.wafer.hicannMargin) + this.wafer.hicannHeight / 2)
                    * scale + stagePosition.y,
            };
            return (hicannCenter);
        };
        /**
         * Find the HICANN that is closest to the center of the canvas.
         */
        Detailview.prototype.hicannClosestToCenter = function (canvasCenter) {
            var minDistance = Infinity;
            var closestHicann = undefined;
            for (var hicannIndex = this.wafer.enumMin; hicannIndex <= this.wafer.enumMax; hicannIndex++) {
                var hicannCenter = this.hicannCenter(hicannIndex);
                var distance = tools.distanceBetweenPoints(hicannCenter, canvasCenter);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestHicann = hicannIndex;
                }
                ;
            }
            return (closestHicann);
        };
        /**
         * Find indices of neighboring HICANNS and update class properties.
         */
        Detailview.prototype.updateSurroundingHicanns = function () {
            this.northernHicann = this.wafer.northernHicann(this.currentHicann);
            this.southernHicann = this.wafer.southernHicann(this.currentHicann);
            this.easternHicann = this.wafer.easternHicann(this.currentHicann);
            this.westernHicann = this.wafer.westernHicann(this.currentHicann);
        };
        /**
         * Draw all detailed elements of a HICANN
         * @param options Information about which parts of the HICANNs to draw in detail according to checkboxes.
         */
        Detailview.prototype.drawHicann = function (newHicann, options) {
            var hicannPosition = this.wafer.hicanns[newHicann].position;
            // draw hicann details
            if (options.synapses) {
                // TODO??: This option is completely useless, the synapse grid is simple filled out with a rectangle at every synapse position
                // -> very computationally costly, no new information..
                this.drawAllSynapses(hicannPosition);
            }
            if (options.synDrivers) {
                this.drawSynDrivers(hicannPosition);
            }
            if (options.neurons) {
                this.drawNeurons(hicannPosition);
            }
            if (options.leftBuses) {
                this.drawBusesLeft(hicannPosition);
            }
            if (options.rightBuses) {
                this.drawBusesRight(hicannPosition);
            }
            if (options.horizontalBuses) {
                this.drawBusesHorizontal(hicannPosition);
            }
            if (options.repeaters) {
                this.drawRepeaters(hicannPosition);
            }
            if (options.synGrids) {
                this.drawSynGrid(hicannPosition);
            }
        };
        /**
         * Determine the zoom-scale where the detailview should begin.
         * The HICANN is fully in taking up almost the whole window at that point.
         */
        Detailview.prototype.determineThreshold = function (canvasHeight) {
            var fullHicannScale = canvasHeight / (this.wafer.hicannHeight + 2 * this.edge);
            this.threshold = fullHicannScale;
            this.threshold2 = fullHicannScale * 8;
        };
        Detailview.prototype.drawNeurons = function (hicannPosition) {
            var positions = {
                xOne: [],
                yOne: [],
                xTwo: [],
                yTwo: [],
                width: [],
            };
            var segmentWidth = this.wafer.neuronArrayPosition.width / this.numNeurons;
            for (var i = 0; i < this.numNeurons; i++) {
                // top
                for (var _i = 0, _a = [this.wafer.neuronArrayPosition.top, this.wafer.neuronArrayPosition.bottom]; _i < _a.length; _i++) {
                    var basePosition = _a[_i];
                    positions.xOne.push(basePosition.x + hicannPosition.x + (i + 0.5) * segmentWidth);
                    positions.yOne.push(basePosition.y + hicannPosition.y);
                    positions.xTwo.push(basePosition.x + hicannPosition.x + (i + 0.5) * segmentWidth);
                    positions.yTwo.push(basePosition.y + hicannPosition.y + this.wafer.neuronArrayPosition.height);
                    positions.width.push(segmentWidth / 2);
                }
            }
            var neuronColor = 0x33aabb;
            var graphicsObject = pixiBackend.drawStraightLines(positions.xOne, positions.yOne, positions.xTwo, positions.yTwo, positions.width, neuronColor);
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.neuronsSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.neurons);
        };
        /**
         * draw both synapse arrays of a HICANN, both as sprites and graphics objects.
         * @param hicannPosition top left corner of the HICANN.
         */
        Detailview.prototype.drawAllSynapses = function (hicannPosition) {
            var positions = {
                x: [],
                y: [],
                width: [],
                height: []
            };
            for (var x = 0; x < this.numNeurons; x++) {
                for (var y = 0; y < 2 * this.numSynapsesVertical; y++) {
                    var position = this.calcSynapse(hicannPosition, x, y);
                    positions.x.push(position[0]);
                    positions.y.push(position[1]);
                    positions.width.push(position[2]);
                    positions.height.push(position[3]);
                }
            }
            var graphicsObject = pixiBackend.drawRectangles(positions.x, positions.y, positions.width, positions.height, 0xfbcb3f);
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.synapsesSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synapses);
        };
        Detailview.prototype.drawSynGrid = function (hicannPosition) {
            var positions = {
                xOne: [],
                yOne: [],
                xTwo: [],
                yTwo: [],
                width: []
            };
            var segmentHeight = this.wafer.synapseArraysTopPosition.height / this.numSynapsesVertical;
            var segmentWidth = this.wafer.synapseArraysTopPosition.width / this.numNeurons;
            for (var i = 0; i <= this.numSynapsesVertical; i++) {
                //const synDriverLeft = (Math.floor(i/2))%2;
                var synDriverLeft = false;
                positions.xOne.push(hicannPosition.x + (synDriverLeft ? this.wafer.synDriverPosition.topLeft.x : this.wafer.synapseArraysTopPosition.x));
                positions.yOne.push(hicannPosition.y + this.wafer.synapseArraysTopPosition.y + i * segmentHeight);
                positions.xTwo.push(hicannPosition.x + this.wafer.synDriverPosition.topRight.x + (synDriverLeft ? 0 : this.wafer.synDriverPosition.width));
                positions.yTwo.push(hicannPosition.y + this.wafer.synapseArraysTopPosition.y + i * segmentHeight);
                positions.xOne.push(hicannPosition.x + (!synDriverLeft ? this.wafer.synDriverPosition.bottomLeft.x : this.wafer.synapseArraysBottomPosition.x));
                positions.yOne.push(hicannPosition.y + this.wafer.synapseArraysBottomPosition.y + i * segmentHeight);
                positions.xTwo.push(hicannPosition.x + this.wafer.synDriverPosition.bottomRight.x + (!synDriverLeft ? 0 : this.wafer.synDriverPosition.width));
                positions.yTwo.push(hicannPosition.y + this.wafer.synapseArraysBottomPosition.y + i * segmentHeight);
            }
            for (var i = 0; i <= this.numNeurons; i++) {
                for (var _i = 0, _a = [this.wafer.synapseArraysTopPosition, this.wafer.synapseArraysBottomPosition]; _i < _a.length; _i++) {
                    var basePosition = _a[_i];
                    positions.xOne.push(hicannPosition.x + basePosition.x + i * segmentWidth);
                    positions.yOne.push(hicannPosition.y + basePosition.y);
                    positions.xTwo.push(hicannPosition.x + basePosition.x + i * segmentWidth);
                    positions.yTwo.push(hicannPosition.y + basePosition.y + basePosition.height);
                }
            }
            for (var _ = 0; _ < positions.xOne.length; _++) {
                positions.width.push(this.vBusWidth);
            }
            var graphicsObject = pixiBackend.drawStraightLines(positions.xOne, positions.yOne, positions.xTwo, positions.yTwo, positions.width, 0xffffff);
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.synGridSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synGrid);
        };
        /**
         * draw synapse all the synapse drivers of a HICANN, as graphics objects.
         * @param hicannPosition top left corner of the HICANN.
         */
        Detailview.prototype.drawSynDrivers = function (hicannPosition) {
            var cornerOne = [];
            var cornerTwo = [];
            var cornerThree = [];
            var synDriverColor = 0xffffff;
            for (var i = 0; i < this.numSynapseDrivers; i++) {
                var synDriverPosition = this.calcSynDriver(hicannPosition, i);
                cornerOne = cornerOne.concat(synDriverPosition[0]);
                cornerTwo = cornerTwo.concat(synDriverPosition[1]);
                cornerThree = cornerThree.concat(synDriverPosition[2]);
            }
            // Graphics Objects
            var graphicsObject = pixiBackend.drawTriangles(cornerOne, cornerTwo, cornerThree, synDriverColor);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synDrivers);
        };
        Detailview.prototype.calcSynDriver = function (hicannPosition, index) {
            var top = index < (this.numSynapseDrivers / 2);
            var left = top ? (index % 2 === 1) : (index % 2 === 0);
            index = index % (this.numSynapseDrivers / 2);
            var cornerOne = undefined;
            var cornerTwo = undefined;
            var cornerThree = undefined;
            var segmentHeight = this.wafer.synDriverPosition.height / (this.numSynapsesVertical / 2);
            if (top) {
                if (left) {
                    cornerOne = ({
                        x: this.wafer.synDriverPosition.topLeft.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.topLeft.y + hicannPosition.y + (index) * segmentHeight
                    });
                    cornerTwo = ({
                        x: this.wafer.synDriverPosition.topLeft.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.topLeft.y + hicannPosition.y + (index + 1) * segmentHeight
                    });
                    cornerThree = ({
                        x: this.wafer.synDriverPosition.topLeft.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.topLeft.y + hicannPosition.y + (index + 0.5) * segmentHeight
                    });
                }
                else {
                    cornerOne = ({
                        x: this.wafer.synDriverPosition.topRight.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.topRight.y + hicannPosition.y + (index + 0.5) * segmentHeight
                    });
                    cornerTwo = ({
                        x: this.wafer.synDriverPosition.topRight.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.topRight.y + hicannPosition.y + (index) * segmentHeight
                    });
                    cornerThree = ({
                        x: this.wafer.synDriverPosition.topRight.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.topRight.y + hicannPosition.y + (index + 1) * segmentHeight
                    });
                }
            }
            else {
                if (left) {
                    cornerOne = ({
                        x: this.wafer.synDriverPosition.bottomLeft.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.bottomLeft.y + hicannPosition.y + (index) * segmentHeight
                    });
                    cornerTwo = ({
                        x: this.wafer.synDriverPosition.bottomLeft.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.bottomLeft.y + hicannPosition.y + (index + 1) * segmentHeight
                    });
                    cornerThree = ({
                        x: this.wafer.synDriverPosition.bottomLeft.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.bottomLeft.y + hicannPosition.y + (index + 0.5) * segmentHeight
                    });
                }
                else {
                    cornerOne = ({
                        x: this.wafer.synDriverPosition.bottomRight.x + hicannPosition.x,
                        y: this.wafer.synDriverPosition.bottomRight.y + hicannPosition.y + (index + 0.5) * segmentHeight
                    });
                    cornerTwo = ({
                        x: this.wafer.synDriverPosition.bottomRight.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.bottomRight.y + hicannPosition.y + (index) * segmentHeight
                    });
                    cornerThree = ({
                        x: this.wafer.synDriverPosition.bottomRight.x + hicannPosition.x + this.wafer.synDriverPosition.width,
                        y: this.wafer.synDriverPosition.bottomRight.y + hicannPosition.y + (index + 1) * segmentHeight
                    });
                }
            }
            return [cornerOne, cornerTwo, cornerThree];
        };
        Detailview.prototype.calcSynapse = function (hicannPosition, xIndex, yIndex) {
            var top = yIndex < this.numSynapsesVertical;
            yIndex = top ? yIndex : (yIndex - this.numSynapsesVertical);
            var x = undefined;
            var y = undefined;
            var segmentWidth = (top ? this.wafer.synapseArraysTopPosition.width : this.wafer.synapseArraysBottomPosition.width) / this.numNeurons;
            var segmentHeight = (top ? this.wafer.synapseArraysTopPosition.height : this.wafer.synapseArraysBottomPosition.height) / this.numSynapsesVertical;
            var baseX = hicannPosition.x + (top ? this.wafer.synapseArraysTopPosition.x : this.wafer.synapseArraysBottomPosition.x);
            var baseY = hicannPosition.y + (top ? this.wafer.synapseArraysTopPosition.y : this.wafer.synapseArraysBottomPosition.y);
            x = baseX + xIndex * segmentWidth;
            y = baseY + yIndex * segmentHeight;
            return [x, y, segmentWidth, segmentHeight, this.vBusWidth];
        };
        /**
         * Draw all vertical left routes of a HICANN as graphics objects and sprite.
         * @param hicannPosition Top left corner of the HICANN.
         */
        Detailview.prototype.drawBusesLeft = function (hicannPosition) {
            var graphicsObject = new PIXI.Graphics;
            ;
            for (var i = 0; i < this.numBusesVertical; i++) {
                var positions = this.calcBusLeft(hicannPosition, i);
                graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.vBusWidth, 0xffffff, graphicsObject);
            }
            ;
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesLeftSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesLeft);
        };
        /**
         * Calculate the x and y values of the two points for all the lines, that make up a vertical left bus.
         * This includes the connecting lines between repeaters and buses.
         * @param hicannPosition Top left corner of the HICANN.
         * @param index Index of the vertical line.
         */
        Detailview.prototype.calcBusLeft = function (hicannPosition, index) {
            var x = [];
            var y = [];
            var repeaterBaseX = hicannPosition.x + this.wafer.repeaterBlockPosition.vertical.top.left.current.x + (index + 0.5) * this.vRepeaterDistance;
            var busBaseX = hicannPosition.x + this.wafer.busesLeftPosition.x + index * this.vBusDistance;
            var busShiftTop = this.vRepeaterDistance * (index < (this.numBusesVertical - 2) ? 1 : -(this.numBusesVertical - 2) / 2);
            var busShiftBottom = this.vRepeaterDistance * (index > 1 ? -1 : (this.numBusesVertical - 2) / 2);
            // top repeater bus connections
            x.push(repeaterBaseX + busShiftTop);
            y.push(hicannPosition.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.top.left.current.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.top.left.current.y + this.wafer.repeaterBlockPosition.vertical.height);
            // main line
            x.push(busBaseX);
            y.push(hicannPosition.y + this.wafer.busesLeftPosition.y);
            x.push(busBaseX);
            y.push(hicannPosition.y + this.wafer.busesLeftPosition.y + this.wafer.busesLeftPosition.height);
            // bottom repeater bus connections
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.bottom.left.current.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.bottom.left.current.y + this.wafer.repeaterBlockPosition.vertical.height);
            x.push(repeaterBaseX + busShiftBottom);
            y.push(hicannPosition.y + this.wafer.hicannHeight);
            return [x, y];
        };
        /**
         * Draw all vertical right routes of a HICANN as graphics objects and sprite.
         * @param hicannPosition Top left corner of the HICANN.
         */
        Detailview.prototype.drawBusesRight = function (hicannPosition) {
            var graphicsObject = new PIXI.Graphics;
            for (var i = 0; i < this.numBusesVertical; i++) {
                var positions = this.calcBusRight(hicannPosition, i);
                graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.vBusWidth, 0xffffff, graphicsObject);
            }
            ;
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesRightSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesRight);
        };
        /**
         * Calculate the x and y values of the two points for all the lines, that make up a vertical right bus.
         * This includes the connecting lines between repeaters and buses.
         * @param hicannPosition Top left corner of the HICANN.
         * @param index Index of the vertical line. Caveat: the indices start at 0 again for the right bus block.
         */
        Detailview.prototype.calcBusRight = function (hicannPosition, index) {
            var x = [];
            var y = [];
            var repeaterBaseX = hicannPosition.x + this.wafer.repeaterBlockPosition.vertical.top.right.current.x + (index + 0.5) * this.vRepeaterDistance;
            var busBaseX = hicannPosition.x + this.wafer.busesRightPosition.x + index * this.vBusDistance;
            var busShiftTop = this.vRepeaterDistance * (index > (1) ? -1 : (this.numBusesVertical - 2) / 2);
            var busShiftBottom = this.vRepeaterDistance * (index < (this.numBusesVertical - 2) ? 1 : -(this.numBusesVertical - 2) / 2);
            // top repeater bus connections
            x.push(repeaterBaseX + busShiftTop);
            y.push(hicannPosition.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.top.right.current.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.top.right.current.y + this.wafer.repeaterBlockPosition.vertical.height);
            // main line
            x.push(busBaseX);
            y.push(hicannPosition.y + this.wafer.busesRightPosition.y);
            x.push(busBaseX);
            y.push(hicannPosition.y + this.wafer.busesRightPosition.y + this.wafer.busesRightPosition.height);
            // bottom repeater bus connections
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.bottom.right.current.y);
            x.push(repeaterBaseX);
            y.push(hicannPosition.y + this.wafer.repeaterBlockPosition.vertical.bottom.right.current.y + this.wafer.repeaterBlockPosition.vertical.height);
            x.push(repeaterBaseX + busShiftBottom);
            y.push(hicannPosition.y + this.wafer.hicannHeight);
            return [x, y];
        };
        /**
         * Draw all horizontal buses of a HICANN as graphics objects and sprite.
         * @param hicannPosition Top left corner of the HICANN.
         */
        Detailview.prototype.drawBusesHorizontal = function (hicannPosition) {
            var graphicsObject = new PIXI.Graphics;
            for (var i = 0; i < this.numBusesHorizontal; i++) {
                var positions = this.calcBusHorizontal(hicannPosition, i);
                graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.hBusWidth, 0xffffff, graphicsObject);
            }
            ;
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesHorizontalSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesHorizontal);
            this.drawHVBusSwitches(hicannPosition);
        };
        /**
         * Calculate the x and y values of the two points for all the lines, that make up a horizontal bus.
         * This includes the connecting lines between repeaters and buses.
         * @param hicannPosition Top left corner of the HICANN.
         * @param index Index of the horizontal line.
         */
        Detailview.prototype.calcBusHorizontal = function (hicannPosition, index) {
            var x = [];
            var y = [];
            var repeaterLevelBaseY = hicannPosition.y + this.wafer.repeaterBlockPosition.horizontal.left.current.y + (index + 0.5) * this.hRepeaterDistance;
            var busBaseY = hicannPosition.y + this.wafer.busesHorizontalPosition.current.y + index * this.hBusDistance;
            // left repeater bus connections
            x.push(hicannPosition.x);
            y.push(repeaterLevelBaseY + this.hRepeaterDistance * (index > 1 ? -1 : (this.numRepeatersHorizontal - 2) / 2));
            x.push(hicannPosition.x + this.wafer.repeaterBlockPosition.horizontal.left.current.x);
            y.push(repeaterLevelBaseY);
            x.push(hicannPosition.x + this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width);
            y.push(repeaterLevelBaseY);
            // main line
            x.push(hicannPosition.x + this.wafer.busesHorizontalPosition.current.x);
            y.push(busBaseY);
            x.push(hicannPosition.x + this.wafer.busesHorizontalPosition.current.x + this.wafer.busesHorizontalPosition.current.width);
            y.push(busBaseY);
            // right repeater bus connections
            x.push(hicannPosition.x + this.wafer.busesRightPosition.x);
            y.push(repeaterLevelBaseY);
            x.push(hicannPosition.x + this.wafer.repeaterBlockPosition.horizontal.right.current.x + this.wafer.repeaterBlockPosition.horizontal.width);
            y.push(repeaterLevelBaseY);
            x.push(hicannPosition.x + this.wafer.hicannWidth);
            y.push(repeaterLevelBaseY + this.hRepeaterDistance * (index < (this.numRepeatersHorizontal - 2) ? 1 : -(this.numRepeatersHorizontal - 2) / 2));
            return [x, y];
        };
        /**
         * Draw all repeaters of a HICANN as graphics objects.
         * @param hicannPosition Top left corner of the HICANN.
         */
        Detailview.prototype.drawRepeaters = function (hicannPosition) {
            var horizontal = {
                leftTriangle: {
                    left: [],
                    top: [],
                    bottom: []
                },
                middleBar: {
                    x: [],
                    y: [],
                    width: [],
                    height: []
                },
                rightTriangle: {
                    right: [],
                    top: [],
                    bottom: []
                }
            };
            var vertical = {
                topTriangle: {
                    top: [],
                    left: [],
                    right: []
                },
                middleBar: {
                    x: [],
                    y: [],
                    width: [],
                    height: []
                },
                bottomTriangle: {
                    bottom: [],
                    left: [],
                    right: []
                }
            };
            // vertical
            var vSegmentWidth = wafer.repeaterBlockPosition.vertical.width.current * 2 / (this.numRepeatersVertical);
            for (var i = 0; i < this.numRepeatersVertical; i++) {
                var top_1 = (i < this.numRepeatersVertical / 2) ? (i % 2 === 1) : (i % 2 === 0);
                var left = (i < this.numRepeatersVertical / 2);
                var baseX = hicannPosition.x + (top_1 ?
                    (left ? wafer.repeaterBlockPosition.vertical.top.left.current.x : wafer.repeaterBlockPosition.vertical.top.right.current.x) :
                    (left ? wafer.repeaterBlockPosition.vertical.bottom.left.current.x : wafer.repeaterBlockPosition.vertical.bottom.right.current.x)) +
                    (i % (this.numRepeatersVertical / 2)) * vSegmentWidth;
                var baseY = hicannPosition.y + (top_1 ?
                    (left ? wafer.repeaterBlockPosition.vertical.top.left.current.y : wafer.repeaterBlockPosition.vertical.top.right.current.y) :
                    (left ? wafer.repeaterBlockPosition.vertical.bottom.left.current.y : wafer.repeaterBlockPosition.vertical.bottom.right.current.y));
                // top triangle
                vertical.topTriangle.top.push({
                    x: baseX + vSegmentWidth / 2,
                    y: baseY
                });
                vertical.topTriangle.left.push({
                    x: baseX,
                    y: baseY + wafer.repeaterBlockPosition.vertical.height / 3
                });
                vertical.topTriangle.right.push({
                    x: baseX + vSegmentWidth,
                    y: baseY + wafer.repeaterBlockPosition.vertical.height / 3
                });
                // middle bar, connecting the triangles
                vertical.middleBar.x.push(baseX + vSegmentWidth / 4);
                vertical.middleBar.y.push(baseY + wafer.repeaterBlockPosition.vertical.height / 3);
                vertical.middleBar.width.push(vSegmentWidth / 2);
                vertical.middleBar.height.push(wafer.repeaterBlockPosition.vertical.height / 3);
                // bottom triangle
                vertical.bottomTriangle.bottom.push({
                    x: baseX + vSegmentWidth / 2,
                    y: baseY + this.wafer.repeaterBlockPosition.vertical.height
                });
                vertical.bottomTriangle.left.push({
                    x: baseX,
                    y: baseY + 2 / 3 * this.wafer.repeaterBlockPosition.vertical.height
                });
                vertical.bottomTriangle.right.push({
                    x: baseX + vSegmentWidth,
                    y: baseY + 2 / 3 * this.wafer.repeaterBlockPosition.vertical.height
                });
            }
            // horizontal
            var hSegmentHeight = this.wafer.repeaterBlockPosition.horizontal.height.current / (this.numRepeatersHorizontal);
            for (var i = 0; i < this.numRepeatersHorizontal; i++) {
                var left = (i % 2 === 0);
                var baseX = hicannPosition.x + (left ? this.wafer.repeaterBlockPosition.horizontal.left.current.x : this.wafer.repeaterBlockPosition.horizontal.right.current.x);
                var baseY = hicannPosition.y + wafer.repeaterBlockPosition.horizontal.left.current.y + i * hSegmentHeight;
                // left triangle
                horizontal.leftTriangle.top.push({
                    x: baseX + wafer.repeaterBlockPosition.horizontal.width / 3,
                    y: baseY
                });
                horizontal.leftTriangle.left.push({
                    x: baseX,
                    y: baseY + hSegmentHeight / 2
                });
                horizontal.leftTriangle.bottom.push({
                    x: baseX + wafer.repeaterBlockPosition.horizontal.width / 3,
                    y: baseY + hSegmentHeight
                });
                // middle bar, connecting the triangles
                horizontal.middleBar.x.push(baseX + wafer.repeaterBlockPosition.horizontal.width / 3);
                horizontal.middleBar.y.push(baseY + hSegmentHeight / 4);
                horizontal.middleBar.width.push(wafer.repeaterBlockPosition.horizontal.width / 3);
                horizontal.middleBar.height.push(hSegmentHeight / 2);
                // bottom triangle
                horizontal.rightTriangle.top.push({
                    x: baseX + wafer.repeaterBlockPosition.horizontal.width * 2 / 3,
                    y: baseY
                });
                horizontal.rightTriangle.right.push({
                    x: baseX + wafer.repeaterBlockPosition.horizontal.width,
                    y: baseY + hSegmentHeight / 2
                });
                horizontal.rightTriangle.bottom.push({
                    x: baseX + wafer.repeaterBlockPosition.horizontal.width * 2 / 3,
                    y: baseY + hSegmentHeight
                });
            }
            // draw top triangle
            var repeaterGraphics = pixiBackend.drawTriangles(vertical.topTriangle.left, vertical.topTriangle.right, vertical.topTriangle.top, 0xffffff);
            // add the middle bar
            repeaterGraphics = pixiBackend.drawRectangles(vertical.middleBar.x, vertical.middleBar.y, vertical.middleBar.width, vertical.middleBar.height, 0xffffff, repeaterGraphics);
            // add the bottom triangle
            repeaterGraphics = pixiBackend.drawTriangles(vertical.bottomTriangle.left, vertical.bottomTriangle.right, vertical.bottomTriangle.bottom, 0xffffff, repeaterGraphics);
            // add the left triangle
            repeaterGraphics = pixiBackend.drawTriangles(horizontal.leftTriangle.top, horizontal.leftTriangle.left, horizontal.leftTriangle.bottom, 0xffffff, repeaterGraphics);
            // add the middle bar
            repeaterGraphics = pixiBackend.drawRectangles(horizontal.middleBar.x, horizontal.middleBar.y, horizontal.middleBar.width, horizontal.middleBar.height, 0xffffff, repeaterGraphics);
            // add the right triangle
            repeaterGraphics = pixiBackend.drawTriangles(horizontal.rightTriangle.top, horizontal.rightTriangle.right, horizontal.rightTriangle.bottom, 0xffffff, repeaterGraphics);
            // store the graphics element in the container
            pixiBackend.storeGraphics(repeaterGraphics, pixiBackend.container.repeaters);
        };
        Detailview.prototype.drawHVBusSwitches = function (hicannPosition) {
            var x = [];
            var y = [];
            var radius = [];
            var color = 0x000000;
            for (var i = 0; i < this.numBusesVertical * 2; i++) {
                x = x.concat(this.calcHVBusSwitch(hicannPosition, i)[0]);
                y = y.concat(this.calcHVBusSwitch(hicannPosition, i)[1]);
            }
            for (var _ = 0; _ < x.length; _++) {
                radius.push(this.vBusWidth);
            }
            var graphicsObject = pixiBackend.drawCircles(x, y, radius, color);
            pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesHorizontalSprite);
            pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesHorizontal);
        };
        Detailview.prototype.calcHVBusSwitch = function (hicannPosition, vBusCoord) {
            var _this = this;
            var xCoords = [];
            var yCoords = [];
            var left = vBusCoord < 128;
            var xCoord = function (vBusCoord) {
                return hicannPosition.x
                    + (left ? _this.wafer.busesLeftPosition.x : _this.wafer.busesRightPosition.x)
                    + (vBusCoord % 128) * _this.vBusDistance;
            };
            var yCoord = function (hBusCoord) {
                return hicannPosition.y
                    + _this.wafer.repeaterBlockPosition.horizontal.left.current.y
                    + (hBusCoord + 0.5) * _this.hRepeaterDistance;
            };
            xCoords.push(xCoord(vBusCoord));
            xCoords.push(xCoord(vBusCoord));
            if (left) {
                yCoords.push(yCoord(63 - 2 * (vBusCoord % 32)));
                yCoords.push(yCoord(63 - 2 * (vBusCoord % 32) - 1));
            }
            else {
                yCoords.push(yCoord(2 * (vBusCoord % 32)));
                yCoords.push(yCoord(2 * (vBusCoord % 32) + 1));
            }
            return [xCoords, yCoords];
        };
        /**
         * remove all the detailed elements
         */
        Detailview.prototype.resetDetailview = function () {
            // remove detailview level two elements
            pixiBackend.removeAllChildren(pixiBackend.container.synapses);
            pixiBackend.removeAllChildren(pixiBackend.container.synDrivers);
            pixiBackend.removeAllChildren(pixiBackend.container.neurons);
            pixiBackend.removeAllChildren(pixiBackend.container.busesLeft);
            pixiBackend.removeAllChildren(pixiBackend.container.busesRight);
            pixiBackend.removeAllChildren(pixiBackend.container.busesHorizontal);
            pixiBackend.removeAllChildren(pixiBackend.container.synapseCross);
            pixiBackend.removeAllChildren(pixiBackend.container.repeaters);
            pixiBackend.removeAllChildren(pixiBackend.container.repeaterBusConnections);
            pixiBackend.removeAllChildren(pixiBackend.container.synGrid);
            // remove detailview level One synapse arrays
            pixiBackend.removeAllChildren(pixiBackend.container.synapsesSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.neuronsSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.repeaterBusConnectionsSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.busesLeftSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.busesRightSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.busesHorizontalSprite);
            pixiBackend.removeAllChildren(pixiBackend.container.synGridSprite);
        };
        /**
         * Check if the northern HICANN is closer to the canvas center than the one currently in detailview.
         * Needed for auto mode.
         */
        Detailview.prototype.northernHicannCloser = function (canvasCenter) {
            if (this.northernHicann) {
                var distanceCurrentHicann = tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
                var distanceNorthernHicann = tools.distanceBetweenPoints(this.hicannCenter(this.northernHicann), canvasCenter);
                if (distanceNorthernHicann + 2 * this.edge < distanceCurrentHicann) {
                    return true;
                }
                else {
                    return false;
                }
                ;
            }
            ;
        };
        /**
         * Check if the southern HICANN is closer to the canvas center than the one currently in detailview.
         * Needed for auto mode.
         */
        Detailview.prototype.southernHicannCloser = function (canvasCenter) {
            if (this.southernHicann) {
                var distanceCurrentHicann = tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
                var distanceSouthernHicann = tools.distanceBetweenPoints(this.hicannCenter(this.southernHicann), canvasCenter);
                if (distanceSouthernHicann + 2 * this.edge < distanceCurrentHicann) {
                    return true;
                }
                else {
                    return false;
                }
                ;
            }
            ;
        };
        /**
         * Check if the eastern HICANN is closer to the canvas center than the one currently in detailview.
         * Needed for auto mode.
         */
        Detailview.prototype.easternHicannCloser = function (canvasCenter) {
            if (this.easternHicann) {
                var distanceCurrentHicann = tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
                var distanceEasternHicann = tools.distanceBetweenPoints(this.hicannCenter(this.easternHicann), canvasCenter);
                if (distanceEasternHicann + 4 * this.edge < distanceCurrentHicann) {
                    return true;
                }
                else {
                    return false;
                }
                ;
            }
            ;
        };
        /**
         * Check if the western HICANN is closer to the canvas center than the one currently in detailview.
         * Needed for auto mode.
         */
        Detailview.prototype.westernHicannCloser = function (canvasCenter) {
            if (this.westernHicann) {
                var distanceCurrentHicann = tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
                var distanceWesternHicann = tools.distanceBetweenPoints(this.hicannCenter(this.westernHicann), canvasCenter);
                if (distanceWesternHicann + 4 * this.edge < distanceCurrentHicann) {
                    return true;
                }
                else {
                    return false;
                }
                ;
            }
            ;
        };
        return Detailview;
    }());
    internalModule.Detailview = Detailview;
})(internalModule || (internalModule = {}));
/// <reference path="tools.ts" />
/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * The overview does not show the HICANN elements such as buses in detail but provides cumulative information about a HICANN.
     * All bus segments (vertical left, vertical right, horizontal) are drawn as one rectangle each in a color that represents the number of routes running over that segment.
     * The number of inputs on a HICANN are represented by a colored triangle at the bottom of the HICANN.
     * The number of neurons on a HICANN are represented by a colored rectangle in the background.
     */
    var Overview = /** @class */ (function () {
        function Overview(wafer, hicannColors) {
            this.wafer = wafer;
            this.numNeuronsColorOne = hicannColors.numNeuronsColorOne;
            this.numNeuronsColorTwo = hicannColors.numNeuronsColorTwo;
            this.numInputsColorOne = hicannColors.numInputsColorOne;
            this.numInputsColorTwo = hicannColors.numInputsColorTwo;
            this.numRoutesLeftColorOne = hicannColors.numRoutesLeftColorOne;
            this.numRoutesLeftColorTwo = hicannColors.numRoutesLeftColorTwo;
            this.numRoutesRightColorOne = hicannColors.numRoutesRightColorOne;
            this.numRoutesRightColorTwo = hicannColors.numRoutesRightColorTwo;
            this.numRoutesHorizontalColorOne = hicannColors.numRoutesHorizontalColorOne;
            this.numRoutesHorizontalColorTwo = hicannColors.numRoutesHorizontalColorTwo;
        }
        /**
         * Draw HICANN background inputs and vertical left, right and horizontal buses for all HICANNs.
         */
        Overview.prototype.drawWafer = function () {
            // loop through hicanns on wafer
            for (var hicannIndex = this.wafer.enumMin; hicannIndex <= this.wafer.enumMax; hicannIndex++) {
                // calculate Hicann position in pixels
                var hicannX = this.wafer.hicanns[hicannIndex].x * (this.wafer.hicannWidth + this.wafer.hicannMargin);
                var hicannY = this.wafer.hicanns[hicannIndex].y * (this.wafer.hicannHeight + this.wafer.hicannMargin);
                this.wafer.hicanns[hicannIndex].position = { x: hicannX, y: hicannY };
                // draw rectangle as hicann representation, color scale for number of neurons
                this.drawHicannBackground(hicannIndex, hicannX, hicannY);
                // draw triangle in color scale for number of hicann inputs
                this.drawInputs(hicannIndex, hicannX, hicannY);
                // draw "H" ín color scale for number of route-segment
                this.drawBusH(hicannIndex, hicannX, hicannY);
            }
            ;
            // render stage
            pixiBackend.renderer.render();
        };
        /**
         * Draw a rectangle in the HICANN background in a color representing the number of neurons on that HICANN.
         */
        Overview.prototype.drawHicannBackground = function (hicannIndex, x, y) {
            // calculate color on number of neurons color gradient
            var colorNumNeurons = tools.colorInGradient(this.numNeuronsColorOne, this.numNeuronsColorTwo, this.wafer.numNeuronsMax, this.wafer.hicanns[hicannIndex].numNeurons);
            // draw rectangle as hicann representation
            pixiBackend.drawRectangle(pixiBackend.container.backgrounds, x, y, this.wafer.hicannWidth, this.wafer.hicannHeight, colorNumNeurons);
        };
        /**
         * Draw a rectangle on the bottom of a HICANN in a color representing the number of inputs on that HICANN.
         */
        Overview.prototype.drawInputs = function (hicannIndex, x, y) {
            // calculate color on number of inputs color gradient
            var colorNumInputs = tools.colorInGradient(this.numInputsColorOne, this.numInputsColorTwo, this.wafer.numInputsMax, this.wafer.hicanns[hicannIndex].numInputs);
            // draw triangle in color scale as number of hicann inputs representation
            pixiBackend.drawTriangle(pixiBackend.container.inputs, x + this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width, y + this.wafer.busesLeftPosition.y + this.wafer.busesLeftPosition.height, this.wafer.busesRightPosition.x - (this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width), this.wafer.inputTriangleHeight, colorNumInputs);
        };
        /**
         * Draw vertical left, vertical right and horizontal buses of a HICANN
         */
        Overview.prototype.drawBusH = function (hicannIndex, x, y) {
            // draw three segments of "H" seperately
            this.drawLeftBusSegment(hicannIndex, x, y);
            this.drawRightBusSegment(hicannIndex, x, y);
            this.drawHorizontalBusSegment(hicannIndex, x, y);
        };
        /**
         * Draw all vertical left buses as one colored rectangle for a HICANN (as graphics object).
         */
        Overview.prototype.drawLeftBusSegment = function (hicannIndex, x, y) {
            var colorNumBuses = tools.colorInGradient(this.numRoutesLeftColorOne, this.numRoutesLeftColorTwo, this.wafer.numBusesLeftMax, this.wafer.hicanns[hicannIndex].numBusesLeft);
            pixiBackend.drawRectangle(pixiBackend.container.overviewBusesLeft, x + this.wafer.busesLeftPosition.x, y + this.wafer.busesLeftPosition.y, this.wafer.busesLeftPosition.width, this.wafer.busesLeftPosition.height, colorNumBuses);
        };
        /**
         * Draw all vertical right buses as one colored rectangle for a HICANN (as graphics object).
         */
        Overview.prototype.drawRightBusSegment = function (hicannIndex, x, y) {
            var colorNumBuses = tools.colorInGradient(this.numRoutesRightColorOne, this.numRoutesRightColorTwo, this.wafer.numBusesRightMax, this.wafer.hicanns[hicannIndex].numBusesRight);
            pixiBackend.drawRectangle(pixiBackend.container.overviewBusesRight, x + this.wafer.busesRightPosition.x, y + this.wafer.busesRightPosition.y, this.wafer.busesRightPosition.width, this.wafer.busesRightPosition.height, colorNumBuses);
        };
        /**
         * Draw all horizontal buses as one colored rectangle for a HICANN (as graphics object).
         */
        Overview.prototype.drawHorizontalBusSegment = function (hicannIndex, x, y) {
            var colorNumBuses = tools.colorInGradient(this.numRoutesHorizontalColorOne, this.numRoutesHorizontalColorTwo, this.wafer.numBusesHorizontalMax, this.wafer.hicanns[hicannIndex].numBusesHorizontal);
            pixiBackend.drawRectangle(pixiBackend.container.overviewBusesHorizontal, x + this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width, y + this.wafer.busesHorizontalPosition.current.y, this.wafer.busesRightPosition.x - (this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width), this.wafer.busesHorizontalPosition.current.height, colorNumBuses);
        };
        Overview.prototype.resetOverview = function () {
            pixiBackend.removeAllChildren(pixiBackend.container.backgrounds);
            pixiBackend.removeAllChildren(pixiBackend.container.inputs);
            pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesLeft);
            pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesRight);
            pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesHorizontal);
            pixiBackend.removeAllChildren(pixiBackend.container.routes);
            pixiBackend.removeAllChildren(pixiBackend.container.switches);
            pixiBackend.removeAllChildren(pixiBackend.container.selectedRoutes);
            pixiBackend.removeAllChildren(pixiBackend.container.selectedSwitches);
        };
        return Overview;
    }());
    internalModule.Overview = Overview;
})(internalModule || (internalModule = {}));
/// <reference path="overview.ts" />
/// <reference path="detailview.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * The AutoMode controls automatically what details of which HICANN are displayed,
     * depending on the zoom level as well as the part of the wafer that is currently within in the canvas boundaries.
     */
    var Automode = /** @class */ (function () {
        function Automode(overview, detailview) {
            var _this = this;
            this.enabled = undefined;
            this.overview = overview;
            this.detailview = detailview;
            this.wafer = overview.wafer;
            this.options = function () {
                var object = {};
                for (var prop in _this.options) {
                    object[prop] = _this.options[prop]();
                }
                return object;
            };
            this.options.synapses = function () { return $("#autoSynapsesCheckbox").prop("checked"); },
                this.options.synDrivers = function () { return $("#autoSynDriversCheckbox").prop("checked"); },
                this.options.neurons = function () { return $("#autoNeuronsCheckbox").prop("checked"); },
                this.options.leftBuses = function () { return $("#autoLeftCheckbox").prop("checked"); },
                this.options.rightBuses = function () { return $("#autoRightCheckbox").prop("checked"); },
                this.options.horizontalBuses = function () { return $("#autoHorizontalCheckbox").prop("checked"); },
                this.options.repeaters = function () { return $("#autoRepeatersCheckbox").prop("checked"); },
                this.options.synGrids = function () { return $("#autoSynGridCheckbox").prop("checked"); },
                this.detailedHicanns = [];
        }
        /**
         * Initialization of the auto mode. Call this method when entering auto mode.
         * @param hicannIndex The index/coordinate of the HICANN, whose details should be shown.
         * @param levelOneEnabled Set to true if the automode should start in detailview.
         * @param levelTwoEnabled Set to true if the auomode should start in detailviewLevelTwo.
         */
        Automode.prototype.init = function (hicannIndex, levelOneEnabled, levelTwoEnabled) {
            this.enabled = true;
            // Auto/Manual switch design
            $("#automode")[0].classList.add("selected");
            $("#automode")[0].classList.remove("nonSelected");
            $("#manualmode")[0].classList.add("nonSelected");
            $("#manualmode")[0].classList.remove("selected");
            // display option checkboxes for auto mode
            $("#automodeCheckboxes").css("display", "block");
            $("#manualmodeCheckboxes").css("display", "none");
            // disable wafer elements list
            // otherwise automatic mode would get messed up
            $("#waferList").css("display", "none");
            $("#hicanns_0").prop("checked", true);
            $("#hicanns_0").prop("disabled", true);
            // start detailLevel depending on zoom level
            if (levelOneEnabled) {
                this.startDetailview(hicannIndex, true);
            }
            ;
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
            }
            ;
            // render stage
            pixiBackend.renderer.render();
        };
        /**
         * Potentially leave a detailview and start the overview.
         * @param hicannIndex Index of the HICANN, whose detailview is left.
         */
        Automode.prototype.startOverview = function () {
            // reset detailview
            this.detailview.resetDetailview();
            // set parameters in detailview
            this.detailview.enabled = false;
            this.detailview.levelTwoEnabled = false;
            this.detailview.currentHicann = undefined;
            // display overview
            for (var _i = 0, _a = this.detailedHicanns; _i < _a.length; _i++) {
                var hicannIndex = _a[_i];
                this.setOverview(hicannIndex, true);
            }
            // display HICANN numbers
            hicannNumber.recover();
            // hide level one details
            this.setDetailview(false);
            // hide level two details
            this.setDetailviewLevelTwo(false);
        };
        /**
         * Start the detailview for a specified HICANN.
         * The Detailview can be entered coming either form the overview or the detailviewLevelTwo.
         */
        Automode.prototype.startDetailview = function (hicannIndex, drawElements) {
            // check if coming from detailview level two
            if (drawElements) {
                this.getDetailedHicanns(hicannIndex);
                // draw detail objects i.e.
                //   synapse array level one and level two
                //	 buses level two
                for (var _i = 0, _a = this.detailedHicanns; _i < _a.length; _i++) {
                    var hicannIndex_1 = _a[_i];
                    this.detailview.drawHicann(hicannIndex_1, this.options());
                }
                ;
            }
            // hide overview containers
            for (var _b = 0, _c = this.detailedHicanns; _b < _c.length; _b++) {
                var hicannIndex_2 = _c[_b];
                this.setOverview(hicannIndex_2, false);
            }
            // hide HICANN numbers
            hicannNumber.disable();
            // display level one detailview
            this.setDetailview(true);
            // hide level two details
            this.setDetailviewLevelTwo(false);
            // set parameters in detailview
            this.detailview.enabled = true;
            this.detailview.levelTwoEnabled = false;
            this.detailview.currentHicann = hicannIndex;
            this.detailview.updateSurroundingHicanns();
        };
        /**
         * Start the detailview level two (graphics objects instead of sprites).
         * Call this function only if currently in detailview.
         */
        Automode.prototype.startDetailviewLevelTwo = function () {
            // set parameter in detailview
            this.detailview.levelTwoEnabled = true;
            // hide the sprites from detailview level one
            this.setDetailview(false);
            // display graphicsobject details from detailview level two
            this.setDetailviewLevelTwo(true);
        };
        /**
         * Switch to detailview of the western HICANN.
         */
        Automode.prototype.startWesternHicann = function (hicannIndex) {
            var levelTwoEnabled = this.detailview.levelTwoEnabled;
            // end detailview of old hicann
            this.startOverview();
            // start detailview of new hicann
            this.startDetailview(this.detailview.westernHicann, true);
            // if level two was enabled before, start level two on new hicann
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
            }
            ;
        };
        /**
         * Switch to detailview of the eastern HICANN.
         */
        Automode.prototype.startEasternHicann = function (hicannIndex) {
            var levelTwoEnabled = this.detailview.levelTwoEnabled;
            // end detailview of old hicann
            this.startOverview();
            // start detailview of new hicann
            this.startDetailview(this.detailview.easternHicann, true);
            // if level two was enabled before, start level two on new hicann
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
            }
            ;
        };
        /**
         * Switch to detailview of the northern HICANN.
         */
        Automode.prototype.startNorthernHicann = function (hicannIndex) {
            var levelTwoEnabled = this.detailview.levelTwoEnabled;
            // end detailview of old hicann
            this.startOverview();
            this.startDetailview(this.detailview.northernHicann, true);
            // if level two was enabled before, start level two on new hicann
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
                // start detailview of new hicann
            }
            ;
        };
        /**
         * Switch to detailview of the sourthern HICANN.
         */
        Automode.prototype.startSouthernHicann = function (hicannIndex) {
            var levelTwoEnabled = this.detailview.levelTwoEnabled;
            // end detailview of old hicann
            this.startOverview();
            // start detailview of new hicann
            this.startDetailview(this.detailview.southernHicann, true);
            // if level two was enabled before, start level two on new hicann
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
            }
            ;
        };
        /**
         * Set visible properties for the overview elements of a HICANN.
         * @param hicannIndex HICANN to be set.
         * @param enabled pass true for visible and false for hidden.
         */
        Automode.prototype.setOverview = function (hicannIndex, enabled) {
            pixiBackend.container.backgrounds.children[hicannIndex].visible = enabled;
            pixiBackend.container.inputs.children[hicannIndex].visible = enabled;
            pixiBackend.container.overviewBusesLeft.children[hicannIndex].visible = enabled;
            pixiBackend.container.overviewBusesRight.children[hicannIndex].visible = enabled;
            pixiBackend.container.overviewBusesHorizontal.children[hicannIndex].visible = enabled;
        };
        /**
         * Set visible properties for all detailview elements.
         * @param hicannIndex HICANN to be set.
         * @param enabled pass true for visible and false for hidden.
         */
        Automode.prototype.setDetailview = function (enabled) {
            pixiBackend.container.synapsesSprite.visible = this.options.synapses() ? enabled : false;
            pixiBackend.container.neuronsSprite.visible = this.options.neurons() ? enabled : false;
            pixiBackend.container.repeaterBusConnectionsSprite.visible = this.options.repeaters() ? enabled : false;
            pixiBackend.container.busesLeftSprite.visible = this.options.leftBuses() ? enabled : false;
            pixiBackend.container.busesRightSprite.visible = this.options.rightBuses() ? enabled : false;
            pixiBackend.container.busesHorizontalSprite.visible = this.options.horizontalBuses() ? enabled : false;
            pixiBackend.container.synGridSprite.visible = this.options.synGrids() ? enabled : false;
        };
        /**
         * Set visible properties for all detailviewLevelTwo elements.
         * @param enabled pass true for visible and false for hidden.
         */
        Automode.prototype.setDetailviewLevelTwo = function (enabled) {
            pixiBackend.container.synapses.visible = this.options.synapses() ? enabled : false;
            pixiBackend.container.neurons.visible = this.options.neurons() ? enabled : false;
            pixiBackend.container.repeaterBusConnections.visible = this.options.repeaters() ? enabled : false;
            pixiBackend.container.busesLeft.visible = this.options.leftBuses() ? enabled : false;
            pixiBackend.container.busesRight.visible = this.options.rightBuses() ? enabled : false;
            pixiBackend.container.busesHorizontal.visible = this.options.horizontalBuses() ? enabled : false;
            pixiBackend.container.synGrid.visible = this.options.synGrids() ? enabled : false;
        };
        /**
         * Find the eigth surrounding HICANNs of a HICANN (if existing).
         */
        Automode.prototype.getDetailedHicanns = function (hicannIndex) {
            // reset array
            this.detailedHicanns = [];
            // center HICANN
            this.detailedHicanns.push(hicannIndex);
            // northern HICANN
            if (this.wafer.northernHicann(hicannIndex) !== undefined) {
                this.detailedHicanns.push(this.wafer.northernHicann(hicannIndex));
                // north-western HICANN
                if (this.wafer.westernHicann(this.wafer.northernHicann(hicannIndex)) !== undefined) {
                    this.detailedHicanns.push(this.wafer.westernHicann(this.wafer.northernHicann(hicannIndex)));
                }
                // north-eastern HICANN
                if (this.wafer.easternHicann(this.wafer.northernHicann(hicannIndex)) !== undefined) {
                    this.detailedHicanns.push(this.wafer.easternHicann(this.wafer.northernHicann(hicannIndex)));
                }
            }
            // eastern HICANN
            if (this.wafer.easternHicann(hicannIndex) !== undefined) {
                this.detailedHicanns.push(this.wafer.easternHicann(hicannIndex));
            }
            // southern HICANN
            if (this.wafer.southernHicann(hicannIndex) !== undefined) {
                this.detailedHicanns.push(this.wafer.southernHicann(hicannIndex));
                // south-western HICANN
                if (this.wafer.westernHicann(this.wafer.southernHicann(hicannIndex)) !== undefined) {
                    this.detailedHicanns.push(this.wafer.westernHicann(this.wafer.southernHicann(hicannIndex)));
                }
                // south-eastern HICANN
                if (this.wafer.easternHicann(this.wafer.southernHicann(hicannIndex)) !== undefined) {
                    this.detailedHicanns.push(this.wafer.easternHicann(this.wafer.southernHicann(hicannIndex)));
                }
            }
            // western HICANN
            if (this.wafer.westernHicann(hicannIndex) !== undefined) {
                this.detailedHicanns.push(this.wafer.westernHicann(hicannIndex));
            }
        };
        return Automode;
    }());
    internalModule.Automode = Automode;
})(internalModule || (internalModule = {}));
/// <reference path="detailview.ts" />
/// <reference path="overview.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * The manual mode aims at giving the user full control over what details to show for which HICANN.
     * Clicking checkboxes in the UI sets the visible property for the respective pixiJS containers.
     * Switching between detailview and detailviewLevelTwo (sprites vs. graphics objects) is still done automatically.
     */
    var Manualmode = /** @class */ (function () {
        function Manualmode(overview, detailview) {
            /**
             * set this property when entering or leaving the manual mode
             */
            this.enabled = false;
            this.detailview = detailview;
            this.overview = overview;
            this.wafer = overview.wafer;
            /**
             *
             */
            this.selectedElements = {
                overview: {
                    numNeurons: [],
                    numInputs: [],
                    left: [],
                    right: [],
                    horizontal: []
                },
                detailview: {
                    left: [],
                    right: [],
                    horizontal: [],
                    synDriver: [],
                    neurons: [],
                    repeaters: [],
                    synGrids: []
                }
            };
            this.containerIndices = {
                left: [],
                right: [],
                horizontal: [],
                synDriver: [],
                neurons: [],
                repeaters: [],
                synGrids: []
            };
            this.initSelectedElements();
        }
        ;
        /**
         * Initialize the manual mode. Call this method when starting the manual mode.
         * @param levelOneEnabled Set to true if the manual mode is started in detailviewLevelOne.
         * @param levelTwoEnabled Set to true if the manual mode is started in detailviewLevelTwo.
         */
        Manualmode.prototype.init = function (levelOneEnabled, levelTwoEnabled) {
            this.enabled = true;
            // Auto/Manual switch design
            $("#manualmode")[0].classList.add("selected");
            $("#manualmode")[0].classList.remove("nonSelected");
            $("#automode")[0].classList.add("nonSelected");
            $("#automode")[0].classList.remove("selected");
            // display option checkboxes for manual mode
            $("#manualmodeCheckboxes").css("display", "block");
            $("#automodeCheckboxes").css("display", "none");
            // enable Wafer elements list
            // user can now select each element to be shown by hand
            $("#waferList").css("display", "flex");
            $("#hicanns_0").prop("disabled", false);
            // display / draw the checked elements
            this.recoverView(levelOneEnabled, levelTwoEnabled);
            // render stage
            pixiBackend.renderer.render();
        };
        /**
         * initialize selected Elements with standard overview values.
         */
        Manualmode.prototype.initSelectedElements = function () {
            for (var i = this.wafer.enumMin; i <= this.wafer.enumMax; i++) {
                this.selectedElements.overview.numNeurons.push(true);
                this.selectedElements.overview.numInputs.push(true);
                this.selectedElements.overview.left.push(true);
                this.selectedElements.overview.right.push(true);
                this.selectedElements.overview.horizontal.push(true);
                this.selectedElements.detailview.left.push(false);
                this.selectedElements.detailview.right.push(false);
                this.selectedElements.detailview.horizontal.push(false);
                this.selectedElements.detailview.synDriver.push(false);
                this.selectedElements.detailview.neurons.push(false);
                this.selectedElements.detailview.repeaters.push(false);
                this.selectedElements.detailview.synGrids.push(false);
            }
        };
        /**
         * Reset all elements to a plain "overview".
         * Call this method when switching to automode.
         */
        Manualmode.prototype.resetView = function () {
            // remove detailview elements
            this.detailview.resetDetailview();
            // empty container-indices arrays
            for (var type in this.containerIndices) {
                this.containerIndices[type] = [];
            }
            // show overview
            this.setOverview(true, true);
        };
        Manualmode.prototype.recoverView = function (levelOneEnabled, levelTwoEnabled) {
            for (var hicannIndex in this.selectedElements.detailview.left) {
                this.busesLeft(parseInt(hicannIndex), this.selectedElements.detailview.left[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.right) {
                this.busesRight(parseInt(hicannIndex), this.selectedElements.detailview.right[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.horizontal) {
                this.busesHorizontal(parseInt(hicannIndex), this.selectedElements.detailview.horizontal[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.synDriver) {
                this.synDriver(parseInt(hicannIndex), this.selectedElements.detailview.synDriver[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.neurons) {
                this.neurons(parseInt(hicannIndex), this.selectedElements.detailview.neurons[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.repeaters) {
                this.repeaters(parseInt(hicannIndex), this.selectedElements.detailview.repeaters[hicannIndex], true);
            }
            for (var hicannIndex in this.selectedElements.detailview.synGrids) {
                this.synGrid(parseInt(hicannIndex), this.selectedElements.detailview.synGrids[hicannIndex], true);
            }
            // show checked elements of overview
            this.setOverview(true, false);
            // start detailLevel depending on zoom level
            if (levelOneEnabled) {
                this.startDetailview();
            }
            if (levelTwoEnabled) {
                this.startDetailviewLevelTwo();
            }
            else {
                this.leaveDetailviewLevelTwo();
            }
            ;
        };
        /**
         * Start the detailview. Only hides the HICANN number, because the rest is managed manually.
         */
        Manualmode.prototype.startDetailview = function () {
            hicannNumber.disable();
            this.detailview.enabled = true;
        };
        /**
         * Leave the detailview.
         */
        Manualmode.prototype.leaveDetailview = function () {
            hicannNumber.recover();
            this.detailview.enabled = false;
        };
        /**
         * Start the DetailviewLevelTwo to switch from sprites to graphics objects.
         */
        Manualmode.prototype.startDetailviewLevelTwo = function () {
            // set detailview property
            this.detailview.levelTwoEnabled = true;
            // detailed buses left
            pixiBackend.container.busesLeft.visible = true;
            pixiBackend.container.busesLeftSprite.visible = false;
            // detailed buses right
            pixiBackend.container.busesRight.visible = true;
            pixiBackend.container.busesRightSprite.visible = false;
            // detailed buses horizontal
            pixiBackend.container.busesHorizontal.visible = true;
            pixiBackend.container.busesHorizontalSprite.visible = false;
            // neurons
            pixiBackend.container.neurons.visible = true;
            pixiBackend.container.neuronsSprite.visible = false;
            // repeater-bus connections
            pixiBackend.container.repeaterBusConnections.visible = true;
            pixiBackend.container.repeaterBusConnectionsSprite.visible = false;
            // synapse grid
            pixiBackend.container.synGrid.visible = true;
            pixiBackend.container.synGridSprite.visible = false;
        };
        /**
         * Leave the detailviewLevelTwo to switch from graphics objects back to sprites.
         */
        Manualmode.prototype.leaveDetailviewLevelTwo = function () {
            // set detailview property
            this.detailview.levelTwoEnabled = false;
            // detailed buses left
            pixiBackend.container.busesLeft.visible = false;
            pixiBackend.container.busesLeftSprite.visible = true;
            // detailed buses right
            pixiBackend.container.busesRight.visible = false;
            pixiBackend.container.busesRightSprite.visible = true;
            // detailed buses horizontal
            pixiBackend.container.busesHorizontal.visible = false;
            pixiBackend.container.busesHorizontalSprite.visible = true;
            // neurons
            pixiBackend.container.neurons.visible = false;
            pixiBackend.container.neuronsSprite.visible = true;
            // repeater-bus connections
            pixiBackend.container.repeaterBusConnections.visible = false;
            pixiBackend.container.repeaterBusConnectionsSprite.visible = true;
            // synapse grid
            pixiBackend.container.synGrid.visible = false;
            pixiBackend.container.synGridSprite.visible = true;
        };
        /**
         * Set the visible properties for all elements of the overview.
         * @param viewChecked Set to true if checked (in UI checkbox) elements should be set.
         * @param viewUnchecked Set to true if unchecked (in UI checkbox) elements should be set.
         */
        Manualmode.prototype.setOverview = function (viewChecked, viewUnchecked) {
            // loop through detailed Buses and hide/display checked ones
            for (var i = this.wafer.enumMin; i <= this.wafer.enumMax; i++) {
                // number neurons
                pixiBackend.container.backgrounds.children[i].visible =
                    (this.selectedElements.overview.numNeurons[i] == true) ? viewChecked : viewUnchecked;
                // number inputs
                pixiBackend.container.inputs.children[i].visible =
                    (this.selectedElements.overview.numInputs[i] == true) ? viewChecked : viewUnchecked;
                // left buses
                pixiBackend.container.overviewBusesLeft.children[i].visible =
                    (this.selectedElements.overview.left[i] == true) ? viewChecked : viewUnchecked;
                // right buses
                pixiBackend.container.overviewBusesRight.children[i].visible =
                    (this.selectedElements.overview.right[i] == true) ? viewChecked : viewUnchecked;
                // horizontal buses
                pixiBackend.container.overviewBusesHorizontal.children[i].visible =
                    (this.selectedElements.overview.horizontal[i] == true) ? viewChecked : viewUnchecked;
            }
        };
        /**
         * handle clicking the checkbox for overview elements of a HICANN in the HICANN list.
         * @param hicannIndex Index of the selected HICANN
         * @param checked state of the checkbox
         */
        Manualmode.prototype.overviewCheckbox = function (hicannIndex, checked) {
            // display or hide elements
            pixiBackend.container.backgrounds.children[hicannIndex].visible = checked;
            pixiBackend.container.inputs.children[hicannIndex].visible = checked;
            pixiBackend.container.overviewBusesLeft.children[hicannIndex].visible = checked;
            pixiBackend.container.overviewBusesRight.children[hicannIndex].visible = checked;
            pixiBackend.container.overviewBusesHorizontal.children[hicannIndex].visible = checked;
            // render stage
            pixiBackend.renderer.render();
            // update selected elements lists
            this.selectedElements.overview.numNeurons[hicannIndex] = checked;
            this.selectedElements.overview.numInputs[hicannIndex] = checked;
            this.selectedElements.overview.left[hicannIndex] = checked;
            this.selectedElements.overview.right[hicannIndex] = checked;
            this.selectedElements.overview.horizontal[hicannIndex] = checked;
            // set checkboxes
            $("#hicanns_0_" + hicannIndex + "_OV_neu").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_OV_inp").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_OV_bsl").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_OV_bsr").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_OV_bsh").prop("checked", checked);
            // make sure checkboxes are consistent
            manualmode.checkAllCheckboxes("numNeurons");
            manualmode.checkAllCheckboxes("numInputs");
            manualmode.checkAllCheckboxes("left");
            manualmode.checkAllCheckboxes("right");
            manualmode.checkAllCheckboxes("horizontal");
        };
        /**
         * handle clicking the checkbox for detailed elements of a HICANN in the HICANN list.
         * @param hicannIndex Index of the selected HICANN
         * @param checked state of the checkbox
         */
        Manualmode.prototype.detailviewCheckbox = function (hicannIndex, checked) {
            // draw or remove elements
            this.busesLeft(hicannIndex, checked);
            this.busesRight(hicannIndex, checked);
            this.busesHorizontal(hicannIndex, checked);
            this.synDriver(hicannIndex, checked);
            this.neurons(hicannIndex, checked);
            this.repeaters(hicannIndex, checked);
            this.synGrid(hicannIndex, checked);
            // render stage
            pixiBackend.renderer.render();
            // update checkboxes
            $("#hicanns_0_" + hicannIndex + "_DV_bsl").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_bsr").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_bsh").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_snd").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_neu").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_rep").prop("checked", checked);
            $("#hicanns_0_" + hicannIndex + "_DV_sng").prop("checked", checked);
            // make sure, checkboxes are consistent
            this.checkAllCheckboxes("detailLeft");
            this.checkAllCheckboxes("detailRight");
            this.checkAllCheckboxes("detailHorizontal");
            this.checkAllCheckboxes("synDriver");
            this.checkAllCheckboxes("neurons");
            this.checkAllCheckboxes("repeaters");
            this.checkAllCheckboxes("synGrid");
        };
        /**
         * Handle clicking the checkbox for a vertical left buses of a HICANN in the HICANN list.
         * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
         */
        Manualmode.prototype.busesLeft = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.left[hicannIndex] !== checked) || skipDoubleCheck) {
                if (checked) {
                    this.detailview.drawBusesLeft(this.wafer.hicanns[hicannIndex].position);
                    this.containerIndices.left.push(hicannIndex);
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.left.indexOf(hicannIndex);
                    pixiBackend.removeChild(pixiBackend.container.busesLeft, containerIndex);
                    pixiBackend.removeChild(pixiBackend.container.busesLeftSprite, containerIndex);
                    this.containerIndices.left.splice(containerIndex, 1);
                }
                this.selectedElements.detailview.left[hicannIndex] = checked;
            }
        };
        /**
         * Handle clicking the checkbox for a vertical right buses of a HICANN in the HICANN list.
         * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
         */
        Manualmode.prototype.busesRight = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.right[hicannIndex] !== checked) || skipDoubleCheck) {
                if (checked) {
                    this.detailview.drawBusesRight(this.wafer.hicanns[hicannIndex].position);
                    this.containerIndices.right.push(hicannIndex);
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.right.indexOf(hicannIndex);
                    pixiBackend.removeChild(pixiBackend.container.busesRight, containerIndex);
                    pixiBackend.removeChild(pixiBackend.container.busesRightSprite, containerIndex);
                    this.containerIndices.right.splice(containerIndex, 1);
                }
                this.selectedElements.detailview.right[hicannIndex] = checked;
            }
        };
        /**
         * Handle clicking the checkbox for a horizontal buses of a HICANN in the HICANN list.
         * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
         */
        Manualmode.prototype.busesHorizontal = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.horizontal[hicannIndex] !== checked) || skipDoubleCheck) {
                // number of children stored in the container when drawing horizontal buses for one HICANN
                var childrenPerHicann = 2;
                if (checked) {
                    this.detailview.drawBusesHorizontal(this.wafer.hicanns[hicannIndex].position);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        this.containerIndices.horizontal.push(hicannIndex);
                    }
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.horizontal.indexOf(hicannIndex);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        pixiBackend.removeChild(pixiBackend.container.busesHorizontal, containerIndex);
                        pixiBackend.removeChild(pixiBackend.container.busesHorizontalSprite, containerIndex);
                    }
                    this.containerIndices.horizontal.splice(containerIndex, childrenPerHicann);
                }
                this.selectedElements.detailview.horizontal[hicannIndex] = checked;
            }
        };
        /**
         * Handle clicking the checkbox for a synapse driver of a HICANN in the HICANN list.
         * If checked, the graphics elements for these synapse drivers are drawn, if unchecked, they are removed.
         */
        Manualmode.prototype.synDriver = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.synDriver[hicannIndex] !== checked) || skipDoubleCheck) {
                // number of children stored in the container when drawing synDrivers for one HICANN
                var childrenPerHicann = 4;
                if (checked) {
                    this.detailview.drawSynDrivers(this.wafer.hicanns[hicannIndex].position);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        this.containerIndices.synDriver.push(hicannIndex);
                    }
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.synDriver.indexOf(hicannIndex);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        pixiBackend.removeChild(pixiBackend.container.synDrivers, containerIndex);
                    }
                    this.containerIndices.synDriver.splice(containerIndex, childrenPerHicann);
                }
                this.selectedElements.detailview.synDriver[hicannIndex] = checked;
            }
        };
        /**
         * Handle clicking the checkbox for neurons of a HICANN in the HICANN list.
         * If checked, the graphics elements and sprites for these neurons are drawn, if unchecked, they are removed.
         */
        Manualmode.prototype.neurons = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.neurons[hicannIndex] !== checked) || skipDoubleCheck) {
                // number of children stored in the container when drawing neurons for one HICANN
                var childrenPerHicann = 1;
                if (checked) {
                    this.detailview.drawNeurons(this.wafer.hicanns[hicannIndex].position);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        this.containerIndices.neurons.push(hicannIndex);
                    }
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.neurons.indexOf(hicannIndex);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        pixiBackend.removeChild(pixiBackend.container.neurons, containerIndex);
                        pixiBackend.removeChild(pixiBackend.container.neuronsSprite, containerIndex);
                    }
                    this.containerIndices.neurons.splice(containerIndex, childrenPerHicann);
                }
                this.selectedElements.detailview.neurons[hicannIndex] = checked;
            }
        };
        /**
         * Handle clicking the checkbox for repeaters of a HICANN in the HICANN list.
         * If checked, the graphics elements for these repeaters are drawn, if unchecked, they are removed.
         */
        Manualmode.prototype.repeaters = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.repeaters[hicannIndex] !== checked) || skipDoubleCheck) {
                // number of children stored in the container when drawing synDrivers for one HICANN
                var childrenPerHicann = 1;
                if (checked) {
                    this.detailview.drawRepeaters(this.wafer.hicanns[hicannIndex].position);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        this.containerIndices.repeaters.push(hicannIndex);
                    }
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.repeaters.indexOf(hicannIndex);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        pixiBackend.removeChild(pixiBackend.container.repeaters, containerIndex);
                        pixiBackend.removeChild(pixiBackend.container.repeaterBusConnections, containerIndex);
                        pixiBackend.removeChild(pixiBackend.container.repeaterBusConnectionsSprite, containerIndex);
                    }
                    this.containerIndices.repeaters.splice(containerIndex, childrenPerHicann);
                }
                this.selectedElements.detailview.repeaters[hicannIndex] = checked;
            }
        };
        Manualmode.prototype.synGrid = function (hicannIndex, checked, skipDoubleCheck) {
            if (skipDoubleCheck === void 0) { skipDoubleCheck = false; }
            if ((this.selectedElements.detailview.synGrids[hicannIndex] !== checked) || skipDoubleCheck) {
                var childrenPerHicann = 1;
                if (checked) {
                    this.detailview.drawSynGrid(this.wafer.hicanns[hicannIndex].position);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        this.containerIndices.synGrids.push(hicannIndex);
                    }
                }
                else if (!skipDoubleCheck) {
                    var containerIndex = this.containerIndices.synGrids.indexOf(hicannIndex);
                    for (var _ = 0; _ < childrenPerHicann; _++) {
                        pixiBackend.removeChild(pixiBackend.container.synGrid, containerIndex);
                        pixiBackend.removeChild(pixiBackend.container.synGridSprite, containerIndex);
                    }
                    this.containerIndices.synGrids.splice(containerIndex, childrenPerHicann);
                }
                this.selectedElements.detailview.synGrids[hicannIndex] = checked;
            }
        };
        /**
         * synchronize the checkboxes in the HICANN list when the all elements of one type are drawn at once.
         * @param element type of the element. "numNeurons" for example are are the colored HICANN backgrounds of the overview.
         */
        Manualmode.prototype.checkAllCheckboxes = function (element) {
            var allElementsSelected = true;
            switch (element) {
                case "numNeurons":
                    for (var i = 0; i < this.selectedElements.overview.numNeurons.length; i++) {
                        if (this.selectedElements.overview.numNeurons[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#numNeuronsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "numInputs":
                    for (var i = 0; i < this.selectedElements.overview.numInputs.length; i++) {
                        if (this.selectedElements.overview.numInputs[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#numInputsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "left":
                    for (var i = 0; i < this.selectedElements.overview.left.length; i++) {
                        if (this.selectedElements.overview.left[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#verticalLeftCheckbox").prop("checked", allElementsSelected);
                    break;
                case "right":
                    for (var i = 0; i < this.selectedElements.overview.right.length; i++) {
                        if (this.selectedElements.overview.right[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#verticalRightCheckbox").prop("checked", allElementsSelected);
                    break;
                case "horizontal":
                    for (var i = 0; i < this.selectedElements.overview.horizontal.length; i++) {
                        if (this.selectedElements.overview.horizontal[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#horizontalCheckbox").prop("checked", allElementsSelected);
                    break;
                case "detailLeft":
                    for (var i = 0; i < this.selectedElements.detailview.left.length; i++) {
                        if (this.selectedElements.detailview.left[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#verticalLeftDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "detailRight":
                    for (var i = 0; i < this.selectedElements.detailview.right.length; i++) {
                        if (this.selectedElements.detailview.right[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#verticalRightDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "detailHorizontal":
                    for (var i = 0; i < this.selectedElements.detailview.horizontal.length; i++) {
                        if (this.selectedElements.detailview.horizontal[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#horizontalDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "synDriver":
                    for (var i = 0; i < this.selectedElements.detailview.synDriver.length; i++) {
                        if (this.selectedElements.detailview.synDriver[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#synDriverDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "synGrid":
                    for (var _i = 0, _a = this.selectedElements.detailview.synGrids; _i < _a.length; _i++) {
                        var synGrid = _a[_i];
                        if (synGrid === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#synGridDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "neurons":
                    for (var i = 0; i < this.selectedElements.detailview.neurons.length; i++) {
                        if (this.selectedElements.detailview.neurons[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#neuronsDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
                case "repeaters":
                    for (var i = 0; i < this.selectedElements.detailview.repeaters.length; i++) {
                        if (this.selectedElements.detailview.repeaters[i] === false) {
                            allElementsSelected = false;
                            break;
                        }
                        ;
                    }
                    ;
                    $("#repeatersDetailsCheckbox").prop("checked", allElementsSelected);
                    break;
            }
            ;
        };
        /**
         * Update the selectedElements list and the UI checkboxes in the HICANN list when all elements of one type are set at once.
         * @param element type of the element. "numNeurons" for example are are the colored HICANN backgrounds of the overview.
         */
        Manualmode.prototype.setAllCheckboxes = function (element, checked) {
            switch (element) {
                case "numNeurons":
                    for (var i = 0; i < this.selectedElements.overview.numNeurons.length; i++) {
                        this.selectedElements.overview.numNeurons[i] = checked;
                        $("#hicanns_0_" + i + "_OV_neu").prop("checked", checked);
                    }
                    ;
                    break;
                case "numInputs":
                    for (var i = 0; i < this.selectedElements.overview.numInputs.length; i++) {
                        this.selectedElements.overview.numInputs[i] = checked;
                        $("#hicanns_0_" + i + "_OV_inp").prop("checked", checked);
                    }
                    ;
                    break;
                case "left":
                    for (var i = 0; i < this.selectedElements.overview.left.length; i++) {
                        this.selectedElements.overview.left[i] = checked;
                        $("#hicanns_0_" + i + "_OV_bsl").prop("checked", checked);
                    }
                    ;
                    break;
                case "right":
                    for (var i = 0; i < this.selectedElements.overview.right.length; i++) {
                        this.selectedElements.overview.right[i] = checked;
                        $("#hicanns_0_" + i + "_OV_bsr").prop("checked", checked);
                    }
                    ;
                    break;
                case "horizontal":
                    for (var i = 0; i < this.selectedElements.overview.horizontal.length; i++) {
                        this.selectedElements.overview.horizontal[i] = checked;
                        $("#hicanns_0_" + i + "_OV_bsh").prop("checked", checked);
                    }
                    ;
                    break;
                case "detailLeft":
                    for (var i = 0; i < this.selectedElements.detailview.left.length; i++) {
                        this.selectedElements.detailview.left[i] = checked;
                        $("#hicanns_0_" + i + "_DV_bsl").prop("checked", checked);
                    }
                    ;
                    break;
                case "detailRight":
                    for (var i = 0; i < this.selectedElements.detailview.right.length; i++) {
                        this.selectedElements.detailview.right[i] = checked;
                        $("#hicanns_0_" + i + "_DV_bsr").prop("checked", checked);
                    }
                    ;
                    break;
                case "detailHorizontal":
                    for (var i = 0; i < this.selectedElements.detailview.horizontal.length; i++) {
                        this.selectedElements.detailview.horizontal[i] = checked;
                        $("#hicanns_0_" + i + "_DV_bsh").prop("checked", checked);
                    }
                    ;
                    break;
                case "synDriver":
                    for (var i = 0; i < this.selectedElements.detailview.synDriver.length; i++) {
                        this.selectedElements.detailview.synDriver[i] = checked;
                        $("#hicanns_0_" + i + "_DV_snd").prop("checked", checked);
                    }
                    ;
                    break;
                case "neurons":
                    for (var i = 0; i < this.selectedElements.detailview.neurons.length; i++) {
                        this.selectedElements.detailview.neurons[i] = checked;
                        $("#hicanns_0_" + i + "_DV_neu").prop("checked", checked);
                    }
                    ;
                    break;
                case "repeaters":
                    for (var i = 0; i < this.selectedElements.detailview.repeaters.length; i++) {
                        this.selectedElements.detailview.repeaters[i] = checked;
                        $("#hicanns_0_" + i + "_DV_rep").prop("checked", checked);
                    }
                    ;
                    break;
                case "synGrid":
                    for (var i = 0; i < this.selectedElements.detailview.synGrids.length; i++) {
                        this.selectedElements.detailview.synGrids[i] = checked;
                        $("#hicanns_0_" + i + "_DV_sng").prop("checked", checked);
                    }
            }
            ;
        };
        Manualmode.prototype.checkAllDetailedElements = function (hicannIndex) {
            var allPropertiesSelected = true;
            for (var key in this.selectedElements.detailview) {
                if (this.selectedElements.detailview[key][hicannIndex] === false) {
                    allPropertiesSelected = false;
                    break;
                }
            }
            $("#hicanns_0_" + hicannIndex + "_DV_checkbox").prop("checked", allPropertiesSelected);
        };
        Manualmode.prototype.checkAllOverviewElements = function (hicannIndex) {
            var allPropertiesSelected = true;
            for (var key in this.selectedElements.overview) {
                if (this.selectedElements.overview[key][hicannIndex] === false) {
                    allPropertiesSelected = false;
                    break;
                }
            }
            $("#hicanns_0_" + hicannIndex + "_OV_checkbox").prop("checked", allPropertiesSelected);
        };
        return Manualmode;
    }());
    internalModule.Manualmode = Manualmode;
})(internalModule || (internalModule = {}));
/// <reference path="routes_json.d.ts" />
/// <reference path="detailview.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    function loadRouteData(wafer) {
        var l1Properties = wafer.marocco.l1_properties();
        var routes = [];
        var types = [
            "hicann", "merger0", "merger1", "merger2", "merger3", "gbitLink", "DNCMerger", "repeaterBlock", "HLine", "VLine", "synapseDriver", "synapse"
        ];
        for (var routeIndex = 0; routeIndex < l1Properties.size(); routeIndex++) {
            var route = l1Properties.get(routeIndex).route();
            var projections = l1Properties.get(routeIndex).projection_ids();
            var routeElements = [];
            var projectionIDs = [];
            var hicann = undefined;
            var index = undefined;
            for (var segmentIndex = 0; segmentIndex < route.size(); segmentIndex++) {
                // match for any sequence of number followed by closing paranthesis ")"
                index = parseInt(route.get(segmentIndex).to_string().match(/\d+(?=\))/)[0]);
                var type = route.get(segmentIndex).which();
                if (type === 0) {
                    hicann = index;
                }
                else {
                    routeElements.push(new L1RouteSegment(hicann, index, types[type]));
                }
            }
            for (var i = 0; i < projections.size(); i++) {
                projectionIDs.push(projections.get(i));
            }
            // store route in routes array
            routes.push(new Route(routeElements, projectionIDs, routes.length));
        }
        return routes;
    }
    /**
     * An element of a route, either a vertical or a horizontal bus.
     */
    var L1RouteSegment = /** @class */ (function () {
        function L1RouteSegment(hicann, index, type, position) {
            this.hicann = hicann;
            this.index = index;
            this.type = type;
            this.position = position;
        }
        return L1RouteSegment;
    }());
    /**
     * A complete route with source and target HICANN.
     */
    var Route = /** @class */ (function () {
        function Route(routeElements, projectionIDs, ID) {
            /**
             * Indicates whether the route will be rendered or not.
             */
            this.visible = true;
            this.routeSegments = routeElements;
            this.projectionIDs = projectionIDs;
            this.switchPositions = [];
            this.sourceHicann = this.routeSegments[0].hicann;
            this.targetHicann = this.routeSegments[this.routeSegments.length - 1].hicann;
            this.ID = ID;
            this.color = tools.randomHexColor();
            this.greyedOut = false;
        }
        return Route;
    }());
    /**
     * Control the route information in the UI route-info box.
     */
    var RouteInfo = /** @class */ (function () {
        function RouteInfo() {
            this.details = false;
            $("#routeInfo").click(function () {
                if ($("#routeInfoBox").css("display") === "none") {
                    $("#routeInfoBox").css("display", "initial");
                    $("#routeInfo").addClass("infoBoxSelected");
                }
                else {
                    $("#routeInfoBox").css("display", "none");
                    $("#routeInfo").removeClass("infoBoxSelected");
                }
            });
        }
        /**
         * Display information about a list of routes.
         */
        RouteInfo.prototype.displayRouteInfo = function (routes, routeButtonClickHandler, that) {
            //remove old info
            this.reset();
            if (routes.length !== 1) {
                // display numbers of all selected routes
                var routeNumberParent = $("#routeNumber");
                routeNumberParent.html("Routes: ");
                var _loop_1 = function (route) {
                    var routeNumber = $("<button></button>").text("" + (route.ID + 1));
                    routeNumber.click(function () { routeButtonClickHandler.call(that, [route]); });
                    routeNumber.css("color", "#" + route.color.slice(2));
                    routeNumberParent.append(routeNumber);
                };
                for (var _i = 0, routes_1 = routes; _i < routes_1.length; _i++) {
                    var route = routes_1[_i];
                    _loop_1(route);
                }
            }
            else {
                // display info about selected route
                var IDs = [];
                for (var _a = 0, _b = routes[0].projectionIDs; _a < _b.length; _a++) {
                    var ID = _b[_a];
                    IDs.push(ID);
                }
                var IDString = IDs.sort(function (a, b) { return a - b; }).toString().split(",").join(", ");
                $("#routeInfoBox").append("<p id=\"projectionIDs\" class=\"routeInfoItem\">\n\t\t\t\t\t\tProjection IDs: <span>" + IDString + "</span></p>");
                $("#routeNumber").html("Route " + (routes[0].ID + 1));
                var sourceHicann = $("<p id=\"sourceHicann\" class=\"routeInfoItem\">\n\t\t\t\t\t\tSource HICANN: <button>" + routes[0].sourceHicann + "</button></p>")
                    .click(function () {
                    $("#hicanns_0_" + routes[0].sourceHicann).siblings("button").click();
                });
                var targetHicann = $("<p id=\"targetHicann\" class=\"routeInfoItem\">\n\t\t\t\t\t\tTarget HICANN: <button>" + routes[0].targetHicann + "</button></p>")
                    .click(function () {
                    $("#hicanns_0_" + routes[0].targetHicann).siblings("button").click();
                });
                $("#routeInfoBox").append(sourceHicann);
                $("#routeInfoBox").append(targetHicann);
                // expand list to show all route segments
                var routeDetails = $("<button id=\"routeDetails\">details</button>");
                routeDetails.click(function () {
                    if (this.details) {
                        this.removeDetailsList();
                        this.details = false;
                    }
                    else {
                        this.buildRouteDetailsList(routes[0]);
                        this.details = true;
                    }
                }.bind(this));
                $("#targetHicann").after(routeDetails);
                if (this.details) {
                    this.buildRouteDetailsList(routes[0]);
                }
            }
        };
        /**
         * Build an HTML list of route segments.
         */
        RouteInfo.prototype.buildRouteDetailsList = function (route) {
            var html = "";
            // open containing div
            html += "<div id='routeElementsBox'>";
            // build route elements list
            for (var _i = 0, _a = route.routeSegments; _i < _a.length; _i++) {
                var element = _a[_i];
                html += "<p class='routeElementItem'>HICANN <span>" + element.hicann + "</span>: " + element.type + " <span>" + element.index + "</span></p>";
            }
            // close containing div
            html += "</div>";
            // append
            $("#sourceHicann").after(html);
        };
        /**
         * Remove the HTML list of route segments
         */
        RouteInfo.prototype.removeDetailsList = function () {
            $("#routeElementsBox").remove();
            $(".routeElementItem").remove();
        };
        /**
         * reset the UI route-info box.
         */
        RouteInfo.prototype.reset = function () {
            $("#routeNumber").html("Route Info");
            $("#routeDetails").remove();
            $(".routeInfoItem").remove();
            this.removeDetailsList();
        };
        return RouteInfo;
    }());
    internalModule.RouteInfo = RouteInfo;
    /**
     * Controls all the routes that are visualized. All routes are stored as a new instance of the class Route.
     * When a route is selected, an additional route is drawn on top of all the other routes, so selected route are not hidden by other routes.
     * When the class is constructed with a routes array, the number of positions (first index) have to match the number of routes.
     */
    var RoutesOnStage = /** @class */ (function () {
        function RoutesOnStage(detailview, routes) {
            /**
             * Different zoom-levels for route width adjustment.
             */
            this.zoomLevels = {
                level0: {
                    scale: 0.2,
                    originalWidth: 100,
                    width: 100 // changes when width-slider changes
                },
                level1: {
                    scale: 0.4,
                    originalWidth: 35,
                    width: 35
                },
                level2: {
                    scale: 1.0,
                    originalWidth: 15,
                    width: 15
                },
                level3: {
                    scale: 3.0,
                    originalWidth: 5,
                    width: 5
                },
                level4: {
                    scale: 8.0,
                    originalWidth: 1.5,
                    width: 1.5
                },
                current: undefined
            };
            this.detailview = detailview;
            this.wafer = detailview.wafer;
            // determine current zoomlevel
            var transform = pixiBackend.container.stage.transform;
            this.zoomLevels.current = this.currentZoomLevel(transform.scale.x);
            this.routes = loadRouteData(this.wafer);
            this.selectedRoutes = [];
        }
        Object.defineProperty(RoutesOnStage.prototype, "numRoutes", {
            /**
             * total number of routes in the visualization.
             */
            get: function () {
                return this.routes.length;
            },
            enumerable: true,
            configurable: true
        });
        ;
        /**
         * Number of segments of a route.
         * @param routeID
         */
        RoutesOnStage.prototype.routeLength = function (routeID) {
            return this.routes[routeID].routeSegments.length;
        };
        ;
        /**
         * Calculate all route positions and draw all routes
         */
        RoutesOnStage.prototype.drawRoutes = function () {
            // draw already stored routes, recalculate positions
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                this.calcRoutePosition(route);
                this.drawRoute(route);
            }
            // ... and the selected routes
            for (var _b = 0, _c = this.selectedRoutes; _b < _c.length; _b++) {
                var route = _c[_b];
                this.calcRoutePosition(route);
                this.drawRoute(route, true);
            }
        };
        ;
        /**
         * draw a single route
         * @param selected Set to true, to store the route in the PixiJS container for selected routes.
         */
        RoutesOnStage.prototype.drawRoute = function (route, selected) {
            if (selected === void 0) { selected = false; }
            var graphicsObject = new PIXI.Graphics();
            var graphicsObjectSynapse = new PIXI.Graphics();
            for (var _i = 0, _a = route.routeSegments; _i < _a.length; _i++) {
                var routeSegment = _a[_i];
                switch (routeSegment.type) {
                    case "VLine":
                    case "HLine":
                        graphicsObject = pixiBackend.drawLines(routeSegment.position.x, routeSegment.position.y, routeSegment.position.width, route.greyedOut ? 0x8c8c8c : route.color, graphicsObject);
                        break;
                    case "synapseDriver":
                        graphicsObject = pixiBackend.drawTriangles([routeSegment.position.cornerOne], [routeSegment.position.cornerTwo], [routeSegment.position.cornerThree], route.greyedOut ? 0x8c8c8c : route.color, graphicsObject);
                        break;
                    case "synapse":
                        // TODO:	store synaptic weight in route and use this information here
                        // 				Will need to divide the weight by the maximum weight of all synapses to get value between 0 and 1 as required for alpha
                        // 				better calculate maximum weight before and store in routesOnStage
                        var weight = Math.random();
                        // TODO: store synaptic input (excitatory || inhibitory) in route and use this information here
                        var excitatory = Math.round(Math.random());
                        graphicsObjectSynapse = pixiBackend.drawRectangles(routeSegment.position.x, routeSegment.position.y, [routeSegment.position.width], [routeSegment.position.height], route.greyedOut ? 0x8c8c8c : (excitatory ? 0x00ff00 : 0xff0000), graphicsObjectSynapse, weight, this.detailview.vBusWidth, route.greyedOut ? 0x8c8c8c : route.color);
                        break;
                    default:
                        break;
                }
            }
            // switches
            var switchX = [];
            var switchY = [];
            var switchRadius = [];
            for (var _b = 0, _c = route.switchPositions; _b < _c.length; _b++) {
                var switchPosition = _c[_b];
                switchX.push(switchPosition.x);
                switchY.push(switchPosition.y);
                switchRadius.push(switchPosition.radius);
            }
            var graphicsObjectSwitches = pixiBackend.drawCircles(switchX, switchY, switchRadius, route.greyedOut ? 0x8c8c8c : 0xffffff);
            // container to store routes in
            var routesContainer;
            var switchesContainer;
            if (selected) {
                routesContainer = pixiBackend.container.selectedRoutes;
                switchesContainer = pixiBackend.container.selectedSwitches;
            }
            else {
                routesContainer = pixiBackend.container.routes;
                switchesContainer = pixiBackend.container.switches;
            }
            ;
            // store as graphics objects
            pixiBackend.storeGraphics(graphicsObject, routesContainer);
            pixiBackend.storeSprite(graphicsObjectSynapse, routesContainer, 100);
            pixiBackend.storeGraphics(graphicsObjectSwitches, switchesContainer);
        };
        RoutesOnStage.prototype.calcRoutePosition = function (route) {
            var previousType = "undefined";
            route.switchPositions = [];
            for (var i in route.routeSegments) {
                var routeSegment = route.routeSegments[i];
                switch (routeSegment.type) {
                    case "VLine":
                        routeSegment.position = this.calcSegmentVertical(routeSegment);
                        if (previousType === "HLine") {
                            route.switchPositions.push(this.calcSwitches(routeSegment, route.routeSegments[parseInt(i) - 1]));
                        }
                        break;
                    case "HLine":
                        routeSegment.position = this.calcSegmentHorizontal(routeSegment);
                        if (previousType === "VLine") {
                            route.switchPositions.push(this.calcSwitches(routeSegment, route.routeSegments[parseInt(i) - 1]));
                        }
                        break;
                    case "synapseDriver":
                        routeSegment.position = this.calcSegmentSynDriver(routeSegment);
                        break;
                    case "synapse":
                        routeSegment.position = this.calcSegmentSynapse(routeSegment);
                        break;
                    default:
                        break;
                }
                previousType = routeSegment.type;
            }
            return route;
        };
        RoutesOnStage.prototype.calcSwitches = function (segment1, segment2) {
            var position1;
            var position2;
            var seg1Index = 3;
            var seg2Index = 3;
            if (segment1.type === "VLine") {
                seg2Index = (segment1.index < 128) ? 1 : 5;
            }
            else {
                seg1Index = (segment2.index < 128) ? 1 : 5;
            }
            position1 = {
                x1: segment1.position.x[seg1Index],
                y1: segment1.position.y[seg1Index],
                x2: segment1.position.x[seg1Index + 1],
                y2: segment1.position.y[seg1Index + 1],
            };
            position2 = {
                x1: segment2.position.x[seg2Index],
                y1: segment2.position.y[seg2Index],
                x2: segment2.position.x[seg2Index + 1],
                y2: segment2.position.y[seg2Index + 1],
            };
            var intersectionPoint = tools.intersectionPoint(position1, position2);
            var routeWidth = this.detailview.vBusWidth * this.zoomLevels.current.width;
            return { x: intersectionPoint.x, y: intersectionPoint.y, radius: routeWidth / 2 };
        };
        /**
         * Calculate line that represents a vertical route segment.
         */
        RoutesOnStage.prototype.calcSegmentVerticalOld = function (segment) {
            var hicannPosition = {
                x: this.wafer.hicanns[segment.hicann].position.x,
                y: this.wafer.hicanns[segment.hicann].position.y,
            };
            var routePositions = (segment.index < 128) ?
                this.detailview.calcBusLeft(hicannPosition, segment.index) :
                this.detailview.calcBusRight(hicannPosition, segment.index - 128);
            var width = this.detailview.vBusWidth * this.zoomLevels.current.width;
            return { x1: routePositions[0], y1: routePositions[1], x2: routePositions[2], y2: routePositions[3], width: width };
        };
        ;
        RoutesOnStage.prototype.calcSegmentVertical = function (segment) {
            var hicannPosition = {
                x: this.wafer.hicanns[segment.hicann].position.x,
                y: this.wafer.hicanns[segment.hicann].position.y,
            };
            var routePositions = (segment.index < 128) ?
                this.detailview.calcBusLeft(hicannPosition, segment.index) :
                this.detailview.calcBusRight(hicannPosition, segment.index - 128);
            var width = this.detailview.vBusWidth * this.zoomLevels.current.width;
            return { x: routePositions[0], y: routePositions[1], width: width };
        };
        /**
         * Calculate line that represents a horizontal route segment.
         */
        RoutesOnStage.prototype.calcSegmentHorizontal = function (segment) {
            var hicannPosition = {
                x: this.wafer.hicanns[segment.hicann].position.x,
                y: this.wafer.hicanns[segment.hicann].position.y,
            };
            var routePositions = this.detailview.calcBusHorizontal(hicannPosition, segment.index);
            var width = this.detailview.hBusWidth * this.zoomLevels.current.width;
            return { x: routePositions[0], y: routePositions[1], width: width };
        };
        ;
        /**
         * Calculate triangle that represents a synapse driver.
         */
        RoutesOnStage.prototype.calcSegmentSynDriver = function (segment) {
            var hicannPosition = {
                x: this.wafer.hicanns[segment.hicann].position.x,
                y: this.wafer.hicanns[segment.hicann].position.y,
            };
            var synDriverPosition = this.detailview.calcSynDriver(hicannPosition, segment.index);
            return { cornerOne: synDriverPosition[0], cornerTwo: synDriverPosition[1], cornerThree: synDriverPosition[2] };
        };
        /**
         * Calculate rectangle that represents a synapse.
         */
        RoutesOnStage.prototype.calcSegmentSynapse = function (segment) {
            var hicannPosition = {
                x: this.wafer.hicanns[segment.hicann].position.x,
                y: this.wafer.hicanns[segment.hicann].position.y
            };
            // TODO: convert enum coordinate into x, y coordinates
            //			replace with marocco function
            var _a = this.convertSynapseEnumToXY(segment.index), xIndex = _a[0], yIndex = _a[1];
            var synapsePosition = this.detailview.calcSynapse(hicannPosition, xIndex, yIndex);
            return { x: [synapsePosition[0]], y: [synapsePosition[1]], width: synapsePosition[2], height: synapsePosition[3], lineWidth: synapsePosition[4] };
        };
        /**
         * TODO: replace with marocco function for that
         */
        RoutesOnStage.prototype.convertSynapseEnumToXY = function (synEnum) {
            return [synEnum % 256, Math.floor(synEnum / 256)];
        };
        /**
         * Removes the graphics objects for all routes (and selected routes) from the PixiJS containers.
         */
        RoutesOnStage.prototype.removeRoutesFromContainer = function () {
            // remove all routes (and switch circles) from pixiJS container
            var numRoutes = pixiBackend.container.routes.children.length;
            for (var i = 0; i < numRoutes; i++) {
                pixiBackend.removeChild(pixiBackend.container.routes, 0);
                pixiBackend.removeChild(pixiBackend.container.switches, 0);
            }
            ;
            // ... and from selected Route pixiJS container
            var numSelectedRoutes = pixiBackend.container.selectedRoutes.children.length;
            for (var i = 0; i < numSelectedRoutes; i++) {
                pixiBackend.removeChild(pixiBackend.container.selectedRoutes, 0);
                pixiBackend.removeChild(pixiBackend.container.selectedSwitches, 0);
            }
        };
        /**
         * Set a route in the visualization visible or hide it.
         * @param visible Set to true to make the route visible.
         */
        RoutesOnStage.prototype.setRoute = function (route, visible) {
            // set pixiJS route according to input
            pixiBackend.container.routes.children[route.ID * 2].visible = visible;
            pixiBackend.container.routes.children[route.ID * 2 + 1].visible = visible;
            // set pixiJS switch circle according to input
            pixiBackend.container.switches.children[route.ID].visible = visible;
            // set pixiJS route and switches for selected Route according to input
            if (this.selectedRoutes.indexOf(route) !== -1) {
                pixiBackend.container.selectedRoutes.children[this.selectedRoutes.indexOf(route) * 2].visible = visible;
                pixiBackend.container.selectedRoutes.children[this.selectedRoutes.indexOf(route) * 2 + 1].visible = visible;
                pixiBackend.container.selectedSwitches.children[this.selectedRoutes.indexOf(route)].visible = visible;
            }
            ;
            // update Route.visible property
            route.visible = visible;
        };
        ;
        /**
         * Set all routes in the visualization visible or hide them according to their "visible" property.
         */
        RoutesOnStage.prototype.setAllRoutes = function () {
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                // set pixiJS route according to the Route.visible property
                pixiBackend.container.routes.children[route.ID * 2].visible = route.visible;
                pixiBackend.container.routes.children[route.ID * 2 + 1].visible = route.visible;
                // set pixiJS switch circles according to the Route.visible property
                pixiBackend.container.switches.children[route.ID].visible = route.visible;
                // set pixiJS route and switch for selected Route according to the Route.visible property
                var indexSelectedRoute = this.selectedRoutes.indexOf(route);
                if (indexSelectedRoute !== -1) {
                    pixiBackend.container.selectedRoutes.children[indexSelectedRoute * 2].visible = route.visible;
                    pixiBackend.container.selectedRoutes.children[indexSelectedRoute * 2 + 1].visible = route.visible;
                    pixiBackend.container.selectedSwitches.children[indexSelectedRoute].visible = route.visible;
                }
                ;
            }
        };
        ;
        /**
         * Calculate the current zoom-level (for route widths) from the current zoom-scale.
         * @param pixiScale zoom-scale of the "stage" PixiJS container.
         */
        RoutesOnStage.prototype.currentZoomLevel = function (pixiScale) {
            // determine the zoomlevel from current zoom
            if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level4.scale) {
                return this.zoomLevels.level4;
            }
            else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level3.scale) {
                return this.zoomLevels.level3;
            }
            else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level2.scale) {
                return this.zoomLevels.level2;
            }
            else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level1.scale) {
                return this.zoomLevels.level1;
            }
            else {
                return this.zoomLevels.level0;
            }
            ;
        };
        ;
        /**
         * Adjust the widths of all routes if a new zoom-level is reached.
         * @param pixiScale Zoom-scale of the "stage" PixiJS container.
         */
        RoutesOnStage.prototype.adjustRouteWidth = function (pixiScale) {
            // check if width has to be adjusted
            if ((this.currentZoomLevel(pixiScale)).width !== this.zoomLevels.current.width) {
                // store new zoom level
                this.zoomLevels.current = this.currentZoomLevel(pixiScale);
                // remove old routes from pixiJS container
                this.removeRoutesFromContainer();
                // draw new Routes, but store only new positions
                this.drawRoutes();
                // set pixiJS containers visible/non-visible like before
                this.setAllRoutes();
            }
        };
        ;
        /**
         * If all routes are visible (UI checkboxes checked), the Checkbox for all routes is set to checked as well.
         */
        RoutesOnStage.prototype.checkAllRoutes = function () {
            var allElementsSelected = true;
            // go through all routes and check if visible
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                if (!route.visible) {
                    allElementsSelected = false;
                    break;
                }
                ;
            }
            ;
            // set Routes checkbox
            $("#routes_0_check").prop("checked", allElementsSelected);
        };
        ;
        /**
         * set the checkbox for a route.
         */
        RoutesOnStage.prototype.setCheckbox = function (route, checked) {
            // set checkbox of route
            $("#routes_0_" + route.ID).prop("checked", checked);
        };
        ;
        /**
         * Check if a route (or multiple routes) was clicked in the visualization
         */
        RoutesOnStage.prototype.handleVisuClick = function (mouseX, mouseY) {
            var selectedRoutes = [];
            // check what routes the mouse is over
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                if (this.mouseOverRoute(mouseX, mouseY, route)) {
                    selectedRoutes.push(route);
                }
            }
            if (selectedRoutes.length !== 0) {
                this.handleRouteClick(selectedRoutes);
            }
        };
        ;
        /**
         * Handle selected routes.
         * - update route info box
         * - highlight selected routes
         * @param routes
         */
        RoutesOnStage.prototype.handleRouteClick = function (routes) {
            // update selected routes array
            this.selectedRoutes = routes;
            // display route information in box
            routeInfo.displayRouteInfo(this.selectedRoutes, this.handleRouteClick, this);
            // grey out all routes excep the selected ones
            this.highlightRoutes(this.selectedRoutes);
            // remove all routes from pixiJS container
            this.removeRoutesFromContainer();
            // draw Routes, but store only new positions
            this.drawRoutes();
            // set pixiJS containers visible/non-visible like before
            this.setAllRoutes();
            // render stage
            pixiBackend.renderer.render();
        };
        ;
        /**
         * Check if a route (or multiple routes) was doubleclicked in the visualization
         */
        RoutesOnStage.prototype.handleVisuDoubleClick = function (mouseX, mouseY) {
            var clickedRoute = undefined;
            // check if mouse is over route
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                if (this.mouseOverRoute(mouseX, mouseY, route)) {
                    clickedRoute = route;
                    break;
                }
            }
            if (clickedRoute !== undefined) {
                this.handleRouteDoubleClick();
            }
        };
        /**
         * Double clicking a route resets the routes.
         * - reset the route info box
         * - remove highlighting and draw all routes in color
         */
        RoutesOnStage.prototype.handleRouteDoubleClick = function () {
            // reset selected routes array
            this.selectedRoutes = [];
            // remove routes from pixiJS container
            this.removeRoutesFromContainer();
            // remove route info in box
            routeInfo.reset();
            // redraw all routes in their original color
            this.unhighlightRoutes();
            // draw new Routes, but store only new positions
            this.drawRoutes();
            // set pixiJS containers visible/non-visible like before
            this.setAllRoutes();
            // render stage
            pixiBackend.renderer.render();
        };
        /**
         * check is within the boundaries of the segments of a route.
         */
        RoutesOnStage.prototype.mouseOverRoute = function (mouseX, mouseY, route) {
            // check if route is visible
            if (route.visible) {
                // check if mouse is over route
                for (var _i = 0, _a = route.routeSegments; _i < _a.length; _i++) {
                    var routeSegment = _a[_i];
                    switch (routeSegment.type) {
                        case "VLine":
                        case "HLine":
                            for (var i = 1; i < routeSegment.position.x.length; i++) {
                                if (pixiBackend.mouseInLine(mouseX, mouseY, routeSegment.position.x[i - 1], routeSegment.position.y[i - 1], routeSegment.position.x[i], routeSegment.position.y[i], routeSegment.position.width)) {
                                    return true;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        };
        ;
        /**
         * Set the greyout property for non-selected routes.
         */
        RoutesOnStage.prototype.highlightRoutes = function (selectedRoutes) {
            // set greyedOut property
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                if (selectedRoutes.includes(route)) {
                    route.greyedOut = false;
                }
                else {
                    route.greyedOut = true;
                }
            }
        };
        /**
         * set the greyed out property to false for all routes.
         */
        RoutesOnStage.prototype.unhighlightRoutes = function () {
            // set greyedOut property
            for (var _i = 0, _a = this.routes; _i < _a.length; _i++) {
                var route = _a[_i];
                route.greyedOut = false;
            }
        };
        /**
         * handle the route-width slider in the UI route info box
         * - remove old routes
         * - draw routes in new width (store only positions)
         */
        RoutesOnStage.prototype.handleRouteWidthSlider = function (sliderValue) {
            // change the route widths for all zoom levels
            for (var zoomLevel in this.zoomLevels) {
                this.zoomLevels[zoomLevel].width = this.zoomLevels[zoomLevel].originalWidth * sliderValue / 2;
            }
            // remove old routes from pixiJS container
            this.removeRoutesFromContainer();
            // draw new Routes, but store only new positions
            this.drawRoutes();
            // set pixiJS containers visible/non-visible like before
            this.setAllRoutes();
            // render stage
            pixiBackend.renderer.render();
        };
        return RoutesOnStage;
    }());
    internalModule.RoutesOnStage = RoutesOnStage;
})(internalModule || (internalModule = {}));
/// <reference path="overview.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * Calculate the coordinate of the top left HICANN of a reticle.
     */
    function topLeftHicannInReticle(reticleCoord) {
        return (reticleCoord + reticlesInFullRows(row(reticleCoord + 1) - 1)) * 4;
    }
    /**
     * Calculate the row of a reticle. (row is not the HICANN y coordinate!)
     */
    function row(reticleCoord) {
        var row = undefined;
        if (reticleCoord <= 3) {
            row = 1;
        }
        else if (reticleCoord <= 8) {
            row = 2;
        }
        else if (reticleCoord <= 15) {
            row = 3;
        }
        else if (reticleCoord <= 24) {
            row = 4;
        }
        else if (reticleCoord <= 33) {
            row = 5;
        }
        else if (reticleCoord <= 40) {
            row = 6;
        }
        else if (reticleCoord <= 45) {
            row = 7;
        }
        else if (reticleCoord <= 48) {
            row = 8;
        }
        else {
            throw (new Error("reticle coordinate out of range"));
        }
        ;
        return row;
    }
    /**
     * Calculate the number of reticles in all the rows up to the one passed as argument.
     */
    function reticlesInFullRows(row) {
        if (row <= 4) {
            return 2 * tools.kleinerGauss(row - 1) + 3 * row;
        }
        else if (row <= 8) {
            return 2 * (2 * tools.kleinerGauss(3) + 12) - (2 * tools.kleinerGauss(7 - row) + 3 * (8 - row));
        }
        else {
            throw (new Error("row out of range"));
        }
    }
    /**
     * Convert a reticle coordinate into the respective fpga coordinate.
     */
    function fpgaCoords(reticleCoord) {
        var fpgaCoords = [12, 13, 11, 16, 14, 15, 10, 9, 20, 17, 19, 7, 6, 8, 3, 22, 21, 23, 18, 5, 4,
            0, 2, 1, 25, 26, 24, 28, 43, 42, 47, 45, 46, 27, 29, 30, 41, 40, 38, 44, 31, 32, 39, 37, 36, 33, 34, 35];
        return fpgaCoords[reticleCoord];
    }
    /**
     * The reticle class stores information about the graphical representation of the reticle.
     */
    var Reticle = /** @class */ (function () {
        function Reticle(reticleCoord, fpgaCoord, x, y, width, height, color) {
            this.reticleCoord = reticleCoord;
            this.fpgaCoord = fpgaCoords(reticleCoord);
            this.position = {
                x: x,
                y: y,
                width: width,
                height: width,
            };
            this.color = color;
        }
        return Reticle;
    }());
    /**
     * ReticlesOnStage controls the visualization of the lookup plot including reticle and fpga coordinates.
     */
    var ReticlesOnStage = /** @class */ (function () {
        function ReticlesOnStage(overview, container) {
            this.overview = overview;
            this.wafer = overview.wafer;
            this.container = container;
            this.reticles = [];
            this.numReticles = 48;
            this.threshold = undefined;
            this.buildReticles();
            //this.enabled = this.container.visible;
            //console.log(this.enabled);
        }
        Object.defineProperty(ReticlesOnStage.prototype, "enabled", {
            /**
             * Set this property to make the graphics visible or hide them.
             */
            get: function () {
                return this.container.visible;
            },
            set: function (enabled) {
                this.container.visible = enabled;
            },
            enumerable: true,
            configurable: true
        });
        /**
         * Calculate the positions for all reticles on the wafer and instantiate new reticle classes
         */
        ReticlesOnStage.prototype.buildReticles = function () {
            for (var reticleCoord = 0; reticleCoord < this.numReticles; reticleCoord++) {
                // hicann in top left corner of reticle
                var hicannTopLeft = topLeftHicannInReticle(reticleCoord);
                // reticle position x
                var x = this.wafer.hicanns[hicannTopLeft].position.x - this.wafer.hicannMargin / 2;
                // reticle position y
                var y = this.wafer.hicanns[hicannTopLeft].position.y - this.wafer.hicannMargin / 2;
                // reticle width
                var width = 4 * (this.wafer.hicannWidth + this.wafer.hicannMargin);
                // reticle height
                var height = 2 * (this.wafer.hicannHeight + this.wafer.hicannMargin);
                // reticle color
                var color = "0xffffff";
                // store reticle
                this.reticles.push(new Reticle(reticleCoord, 3, x, y, width, height, color));
            }
        };
        /**
         * Draw reticle boundaries as well as reticle and fpga coordinates as text in the visualization
         */
        ReticlesOnStage.prototype.drawReticles = function () {
            // style for reticle and fpga coordinate
            var reticleStyle = {
                font: 'normal 100px Arial',
                fill: "0x177076"
            };
            var fpgaStyle = {
                font: 'normal 100px Arial',
                fill: "0xff581e"
            };
            // draw reticles
            for (var _i = 0, _a = this.reticles; _i < _a.length; _i++) {
                var reticle = _a[_i];
                // draw background rectangle
                var graphicsObject = pixiBackend.drawRectangles([reticle.position.x], [reticle.position.y], [reticle.position.width], [reticle.position.height], reticle.color, new PIXI.Graphics(), 1, 10, "0x000000");
                pixiBackend.storeGraphics(graphicsObject, this.container);
                // draw reticle (=DNC) coordinate
                pixiBackend.drawTextInRectangle(this.container, reticle.position.x, reticle.position.y, reticle.position.width, reticle.position.height / 2, "F " + reticle.fpgaCoord.toString(), reticleStyle);
                // draw FPGA coordinate
                pixiBackend.drawTextInRectangle(this.container, reticle.position.x, reticle.position.y + reticle.position.height / 2, reticle.position.width, reticle.position.height / 2, "D " + reticle.reticleCoord.toString(), fpgaStyle);
            }
            // render stage
            pixiBackend.renderer.render();
        };
        /**
         * Pass "true" to make the reticles visible and "false" to hide them
         */
        ReticlesOnStage.prototype.setReticles = function (visible) {
            // set pixiJS container visibility
            this.enabled = visible;
            // set UI reticle checkbox
            this.setCheckbox(visible);
        };
        /**
         * Set the UI checkbox
         */
        ReticlesOnStage.prototype.setCheckbox = function (checked) {
            // set UI reticle checkbox
            $("#reticlesCheckbox").prop("checked", checked);
        };
        return ReticlesOnStage;
    }());
    internalModule.ReticlesOnStage = ReticlesOnStage;
})(internalModule || (internalModule = {}));
/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    /**
     * An image of the HICANN can be display in the background.
     * Those images can be set visible or hidden via a checkbox in the UI.
     */
    var WaferImage = /** @class */ (function () {
        function WaferImage(wafer, hicannImagePath, container, width, height) {
            this.wafer = wafer;
            this.hicannImagePath = hicannImagePath;
            this.container = container;
            this.width = width;
            this.height = height;
            // preload the hicann Image
            if (!PIXI.loader.resources.hasOwnProperty("img/hicann.png")) {
                PIXI.loader.add(hicannImagePath);
            }
        }
        /**
         * Draw the images for all HICANNs.
         */
        WaferImage.prototype.draw = function () {
            for (var _i = 0, _a = this.wafer.hicanns; _i < _a.length; _i++) {
                var hicann = _a[_i];
                // draw png-image of hicann
                pixiBackend.drawImage(this.container, this.hicannImagePath, hicann.position.x, hicann.position.y, this.width, this.height);
            }
            // render stage when hicann image finished loading
            PIXI.loader.load(function () {
                pixiBackend.renderer.render();
            });
        };
        /**
         * Set the images to visible or hide them.
         */
        WaferImage.prototype.setVisible = function (visible) {
            // set pixiJS container property
            this.container.visible = visible;
        };
        return WaferImage;
    }());
    internalModule.WaferImage = WaferImage;
})(internalModule || (internalModule = {}));
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    var Settings = /** @class */ (function () {
        function Settings() {
            var _this = this;
            this.resultsFile = undefined;
            this.resetDescription = "Reset the visualization to the initial state.";
            this.saveDescription = "Save the current state of the visualization as an external file.";
            this.uploadDescription = "Upload an external configuration file for the visualization.";
            this.reloadFileDescription = "Upload a different results file. The visu configuration will be automatically restored.";
            this.longestDescription = function () {
                var length = 0;
                var longest = undefined;
                for (var _i = 0, _a = [_this.resetDescription, _this.saveDescription, _this.uploadDescription, _this.reloadFileDescription]; _i < _a.length; _i++) {
                    var description = _a[_i];
                    if (description.length > length) {
                        length = description.length;
                        longest = description;
                    }
                }
                return longest;
            };
            this.open = function () {
                $("#settingsBackground").css("display", "initial");
            };
            this.close = function () {
                $("#settingsBackground").css("display", "none");
            };
            this.checkSymbol = function () {
                $(".settingDescription").children().eq(1).css("display", "none");
                $("#checkSymbol").css("display", "block");
                setTimeout(function () {
                    $("#checkSymbol").fadeOut(500);
                }, 2000);
            };
            this.reloadFile = function () {
                // create hidden file browser
                var fileBrowser = $("<input type=\"file\"/>").prop("display", "none");
                $("body").append(fileBrowser);
                fileBrowser.change(function () {
                    // uploaded configuration file
                    var configfile = fileBrowser.prop("files")[0];
                    var filereader = new FileReader();
                    // handle the file
                    filereader.onload = function (event) {
                        // show loading wheel;
                        $("#configurationContent .loader").css("display", "block");
                        var contents = new Int8Array(event.target.result);
                        // file name is "network" + extension of the input file
                        var filename = "./network" + configfile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0];
                        // delete old results file
                        if (_this.resultsFile) {
                            FS.unlink(_this.resultsFile);
                        }
                        // write file into emscriptens virtual file system (FS)
                        FS.writeFile(filename, contents);
                        // save 
                        _this.resultsFile = filename;
                        // start main program
                        setTimeout(function () {
                            _this.saveConfig();
                            removeVisualization();
                            visualizeFile(filename);
                            _this.setConfig(_this.configuration);
                            // hide loading wheel
                            $("#configurationContent .loader").css("display", "none");
                            // show check symbol
                            _this.checkSymbol();
                        }, 100);
                    };
                    filereader.readAsArrayBuffer(configfile);
                    fileBrowser.remove();
                });
                // open file browser
                fileBrowser.click();
            };
            this.setConfig = function (config) {
                // reset visu
                reset();
                // position and scale
                pixiBackend.container.stage.position.x = config.position.x;
                pixiBackend.container.stage.position.y = config.position.y;
                pixiBackend.container.stage.scale.x = config.position.scale;
                pixiBackend.container.stage.scale.y = config.position.scale;
                // horizontal bus segment width
                wafer.busesHorizontalPosition = config.busesHorizontalPosition;
                // redraw overview horizontal buses
                pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesHorizontal);
                for (var i = wafer.enumMin; i <= wafer.enumMax; i++) {
                    overview.drawHorizontalBusSegment(i, wafer.hicanns[i].position.x, wafer.hicanns[i].position.y);
                }
                // route width
                routesOnStage.adjustRouteWidth(config.position.scale);
                pixiBackend.renderer.render();
                // detail level
                detailview.enabled = config.levelOneEnabled;
                detailview.levelTwoEnabled = config.levelTwoEnabled;
                // enter manualmode
                $("#manualmode").click();
                var _loop_2 = function (i) {
                    $("#hicanns_0_" + i + "_OV_checkbox").siblings("ul").children("li").children("input").each(function (index) {
                        if ($(this).prop("checked") !== config.hicannCheckboxes[i].OV[index]) {
                            $(this).click();
                        }
                    });
                    $("#hicanns_0_" + i + "_DV_checkbox").siblings("ul").children("li").children("input").each(function (index) {
                        if ($(this).prop("checked") !== config.hicannCheckboxes[i].DV[index]) {
                            $(this).click();
                        }
                    });
                };
                // restore overview and detailview checkboxes
                for (var i = 0; i < wafer.enumMax; i++) {
                    _loop_2(i);
                }
                // back to automode
                if (config.automode) {
                    $("#automode").click();
                }
                ;
                // hicann images
                if (config.hicannImages !== $("#waferImageCheckbox").prop("checked")) {
                    $("#waferImageCheckbox").click();
                }
                ;
                // routes
                if (config.routes) {
                    // visible routes
                    $("#routes_0_check").siblings("ul").children("li").children("input").each(function (index) {
                        if ($(this).prop("checked") !== config.routeCheckboxes[index]) {
                            $(this).click();
                        }
                    });
                    // selected (highlighted) routes
                    if (config.selectedRoutes.length !== 0) {
                        var selectedRoutes = [];
                        for (var _i = 0, _a = config.selectedRoutes; _i < _a.length; _i++) {
                            var ID = _a[_i];
                            selectedRoutes.push(routesOnStage.routes[ID]);
                        }
                        routesOnStage.handleRouteClick(selectedRoutes);
                    }
                }
            };
            this.uploadConfig = function () {
                // create hidden file browser
                var fileBrowser = $("<input type=\"file\"/>").prop("display", "none");
                $("body").append(fileBrowser);
                fileBrowser.change(function () {
                    // uploaded configuration file
                    var configfile = fileBrowser.prop("files")[0];
                    var filereader = new FileReader();
                    // handle the file
                    filereader.onload = function (event) {
                        var config = JSON.parse(event.target.result);
                        _this.setConfig(config);
                        // show check symbol
                        _this.checkSymbol();
                    };
                    filereader.readAsText(configfile);
                    fileBrowser.remove();
                });
                // open file browser
                fileBrowser.click();
            };
            this.saveConfig = function (saveExternal, saveRoutes) {
                if (saveExternal === void 0) { saveExternal = false; }
                if (saveRoutes === void 0) { saveRoutes = false; }
                var config = {};
                // position and scale
                config.position = {
                    x: pixiBackend.container.stage.position.x,
                    y: pixiBackend.container.stage.position.y,
                    scale: pixiBackend.container.stage.scale.x,
                };
                // horizontal bus segment width
                config.busesHorizontalPosition = wafer.busesHorizontalPosition;
                // detail level
                config.levelOneEnabled = detailview.enabled;
                config.levelTwoEnabled = detailview.levelTwoEnabled;
                // overview and detailview checkboxes for manual mode
                config.hicannCheckboxes = [];
                var _loop_3 = function (i) {
                    var checkboxes = ({
                        OV: [],
                        DV: [],
                    });
                    $("#hicanns_0_" + i + "_OV_checkbox").siblings("ul").children("li").children("input").each(function () {
                        checkboxes.OV.push($(this).prop("checked"));
                    });
                    $("#hicanns_0_" + i + "_DV_checkbox").siblings("ul").children("li").children("input").each(function () {
                        checkboxes.DV.push($(this).prop("checked"));
                    });
                    config.hicannCheckboxes.push(checkboxes);
                };
                for (var i = 0; i < wafer.enumMax; i++) {
                    _loop_3(i);
                }
                // automode/ manualmode
                config.automode = automode.enabled;
                config.manualmode = manualmode.enabled;
                // HICANN images state
                config.hicannImages = $("#waferImageCheckbox").prop("checked");
                // Routes
                if (saveRoutes) {
                    config.routes = true;
                    // visible routes
                    config.routeCheckboxes = [];
                    $("#routes_0_check").siblings("ul").children("li").children("input").each(function () {
                        config.routeCheckboxes.push($(this).prop("checked"));
                    });
                    // selected (highlighted) routes
                    config.selectedRoutes = [];
                    for (var _i = 0, _a = routesOnStage.selectedRoutes; _i < _a.length; _i++) {
                        var route = _a[_i];
                        config.selectedRoutes.push(route.ID);
                    }
                }
                else {
                    config.routes = false;
                }
                _this.configuration = config;
                if (saveExternal) {
                    var json = JSON.stringify(config);
                    var element = document.createElement("a");
                    element.setAttribute("href", "data:text/plain;charset=utf-8," + encodeURIComponent(json));
                    element.setAttribute("download", "visuConfig.json");
                    element.style.display = "none";
                    document.body.appendChild(element);
                    element.click();
                    document.body.removeChild(element);
                }
                // show check symbol
                _this.checkSymbol();
            };
            this.adjustHBusSegment = function (changeRate) {
                var straightHeight = 3 * wafer.busesHorizontalPosition.original.height;
                var busDiff = wafer.busesHorizontalPosition.original.height - straightHeight;
                wafer.busesHorizontalPosition.current.height = wafer.busesHorizontalPosition.original.height - busDiff * changeRate;
                wafer.busesHorizontalPosition.current.y = wafer.busesHorizontalPosition.original.y + (busDiff * changeRate) / 2;
                var repDiff = wafer.repeaterBlockPosition.horizontal.height.original - straightHeight;
                wafer.repeaterBlockPosition.horizontal.height.current = wafer.repeaterBlockPosition.horizontal.height.original - repDiff * changeRate;
                wafer.repeaterBlockPosition.horizontal.left.current.y = wafer.repeaterBlockPosition.horizontal.left.original.y + (repDiff * changeRate) / 2;
                wafer.repeaterBlockPosition.horizontal.right.current.y = wafer.repeaterBlockPosition.horizontal.right.original.y + (repDiff * changeRate) / 2;
                // save visu configuration
                _this.saveConfig(false, true);
                // restore visu configuration
                _this.setConfig(_this.configuration);
                // show check symbol
                _this.checkSymbol();
            };
            this.adjustVBusSegment = function (changeRate) {
                for (var _i = 0, _a = [wafer.repeaterBlockPosition.vertical.top.left, wafer.repeaterBlockPosition.vertical.bottom.left]; _i < _a.length; _i++) {
                    var leftPosition = _a[_i];
                    var diff = Math.abs(leftPosition.original.x - wafer.busesLeftPosition.x);
                    leftPosition.current.x = leftPosition.original.x + changeRate * diff;
                }
                ;
                for (var _b = 0, _c = [wafer.repeaterBlockPosition.vertical.top.right, wafer.repeaterBlockPosition.vertical.bottom.right]; _b < _c.length; _b++) {
                    var rightPosition = _c[_b];
                    var diff = Math.abs(rightPosition.original.x - wafer.busesRightPosition.x);
                    rightPosition.current.x = rightPosition.original.x + changeRate * diff;
                }
                wafer.repeaterBlockPosition.vertical.width.current = (1 - changeRate) * wafer.repeaterBlockPosition.vertical.width.original + changeRate * wafer.busesLeftPosition.width;
                // save visu configuration
                _this.saveConfig(false, true);
                // restore visu configuration
                _this.setConfig(_this.configuration);
                // show check symbol
                _this.checkSymbol();
            };
            this.newRenderer = function (resolution, forceCanvas) {
                pixiBackend.renderer.renderer.destroy();
                $("#pixiJSCanvas").remove();
                pixiBackend.renderer = new pixiBackend.Renderer($("body"), 0x333333, canvasWidth(), canvasHeight(), forceCanvas, resolution);
            };
            this.screenshot = function (resolution) {
                _this.newRenderer(resolution, true);
                pixiBackend.renderer.render();
                pixiBackend.renderer.renderer.view.toBlob(function (blob) {
                    var element = document.createElement("a");
                    var url = URL.createObjectURL(blob);
                    element.setAttribute("href", url);
                    element.setAttribute("download", "screenshot.png");
                    element.style.display = "none";
                    document.body.appendChild(element);
                    element.click();
                    document.body.removeChild(element);
                }, "image/png", 1);
                _this.newRenderer(1, false);
            };
            $("#settings").click(function () {
                if ($("#settingsBox").css("display") === "none") {
                    $("#settingsBox").css("display", "initial");
                    $("#settings").addClass("infoBoxSelected");
                }
                else {
                    $("#settingsBox").css("display", "none");
                    $("#settings").removeClass("infoBoxSelected");
                }
            });
            $("#resetConfig").click(function () {
                reset();
                _this.checkSymbol();
            });
            $("#saveConfig").click(function () {
                _this.saveConfig(true);
            });
            $("#uploadConfig").click(function () {
                _this.uploadConfig();
            });
            $("#reloadFile").click(function () {
                _this.reloadFile();
            });
            $(".settingDescription").children().eq(0).html(this.longestDescription());
            $("#resetConfig").mouseenter(function () {
                $(".settingDescription").children().eq(1).stop(true).html(_this.resetDescription).fadeIn(100);
            });
            $("#saveConfig").mouseenter(function () {
                $(".settingDescription").children().eq(1).stop(true).html(_this.saveDescription).fadeIn(100);
            });
            $("#uploadConfig").mouseenter(function () {
                $(".settingDescription").children().eq(1).stop(true).html(_this.uploadDescription).fadeIn(100);
            });
            $("#reloadFile").mouseenter(function () {
                $(".settingDescription").children().eq(1).stop(true).html(_this.reloadFileDescription).fadeIn(100);
            });
            $("#resetConfig").mouseleave(function () {
                $(".settingDescription").children().eq(1).fadeOut(100);
            });
            $("#saveConfig").mouseleave(function () {
                $(".settingDescription").children().eq(1).fadeOut(100);
            });
            $("#uploadConfig").mouseleave(function () {
                $(".settingDescription").children().eq(1).fadeOut(100);
            });
            $("#reloadFile").mouseleave(function () {
                $(".settingDescription").children().eq(1).fadeOut(100);
            });
            $("#numberHoverCheckbox").change(function (e) {
                var checked = (e || window.event).target.checked;
                hicannNumber.setHover(checked);
            });
            $("#captureScreenshot").click(function () {
                var input = Number($("#screenshotResolution").val());
                if (input === 0) {
                    input = 1;
                }
                ;
                if (isNaN(input)) {
                    alert("please enter a number");
                }
                else {
                    _this.screenshot(input);
                }
            });
        }
        return Settings;
    }());
    internalModule.Settings = Settings;
})(internalModule || (internalModule = {}));
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
var internalModule;
(function (internalModule) {
    var Summary = /** @class */ (function () {
        function Summary() {
            this.open = function () {
                $("#summaryBackground").css("display", "initial");
            };
            this.close = function () {
                $("#summaryBackground").css("display", "none");
            };
            this.checkSymbol = function () {
                $("#settingDescription").css("display", "none");
                $("#checkSymbol").css("display", "block");
                setTimeout(function () {
                    $("#checkSymbol").fadeOut(500);
                }, 2000);
            };
            this.updateSummaryContent = function () {
                // total number of neurons;
                var numNeurons = 0;
                for (var _i = 0, _a = wafer.hicanns; _i < _a.length; _i++) {
                    var hicann = _a[_i];
                    numNeurons += hicann.numNeurons;
                }
                $("#summaryNeurons").html("" + numNeurons);
                // total number of utilized HICANN chips
                var numHicanns = 0;
                for (var _b = 0, _c = wafer.hicanns; _b < _c.length; _b++) {
                    var hicann = _c[_b];
                    if (hicann.hasNeurons || hicann.numBusesVertical || hicann.numBusesHorizontal)
                        numHicanns++;
                }
                $("#summaryHicanns").html("" + numHicanns);
                // total number of utilized FPGAs
                var numFpgas = 0;
                // total number of routes
                var numRoutes = routesOnStage.routes.length;
                $("#summaryRoutes").html("" + numRoutes);
            };
            $("#summary").click(function () {
                if ($("#summaryBox").css("display") === "none") {
                    $("#summaryBox").css("display", "initial");
                    $("#summary").addClass("infoBoxSelected");
                }
                else {
                    $("#summaryBox").css("display", "none");
                    $("#summary").removeClass("infoBoxSelected");
                }
            });
            this.updateSummaryContent();
        }
        return Summary;
    }()); // class Summary
    internalModule.Summary = Summary;
})(internalModule || (internalModule = {})); // internalModule
var internalModule;
(function (internalModule) {
    /**
     * The HICANN number and a border around the HICANN
     * can be drawn for all HICANNs at once (like a lookup plot)
     * and additionally for the HICANN, where the mouse is hovering over.
     * This is controlled by checkboxes.
     */
    var HicannNumber = /** @class */ (function () {
        function HicannNumber(width, height) {
            this.style = {
                font: 'bold 100px Arial',
                fill: pixiBackend.renderer.backgroundColor
            };
            this.styleHighlighted = {
                font: 'bold 100px Arial',
                fill: 0xfbb535
            };
            this.hicannIndex = undefined;
            this.width = width;
            this.height = height;
            this.showAll = false;
            this.showHover = true;
            this.containerAll = pixiBackend.container.numberAll;
            this.containerHover = pixiBackend.container.numberHover;
            this.drawAll();
            this.setAll(false);
        }
        HicannNumber.prototype.handleMouseHover = function (hicannIndex) {
            if (hicannIndex !== this.hicannIndex) {
                // draw new hicann number
                this.drawHover(hicannIndex);
            }
            ;
            this.hicannIndex = hicannIndex;
        };
        /**
         * Set the visibility for all numbers.
         */
        HicannNumber.prototype.setAll = function (visible) {
            this.showAll = visible;
            this.containerAll.visible = visible;
            pixiBackend.renderer.render();
        };
        ;
        /**
         * Set the visibility for the hovering number.
         */
        HicannNumber.prototype.setHover = function (visible) {
            this.showHover = visible;
            pixiBackend.renderer.render();
        };
        /**
         * draw all HICANN numbers into one container
         */
        HicannNumber.prototype.drawAll = function () {
            for (var i = wafer.enumMin; i <= wafer.enumMax; i++) {
                var position = wafer.hicanns[i].position;
                this.draw(i, position.x, position.y, false, true);
            }
        };
        /**
         * Draw the HICANN number, when hovering over a HICANN with the mouse
         */
        HicannNumber.prototype.drawHover = function (number) {
            // remove possible previous numbers
            this.clean();
            this.hicannIndex = number;
            if (this.showHover) {
                var position = wafer.hicanns[number].position;
                this.draw(number, position.x, position.y, this.showAll);
            }
        };
        /**
         * Draw the HICANN number.
         * @param number Index of the HICANN.
         * @param x x-position for the text.
         * @param y y-position for the text.
         */
        HicannNumber.prototype.draw = function (number, x, y, highlighted, all) {
            if (highlighted === void 0) { highlighted = false; }
            if (all === void 0) { all = false; }
            var style = highlighted ? this.styleHighlighted : this.style;
            var container = all ? this.containerAll : this.containerHover;
            // draw new number;
            pixiBackend.drawTextInRectangle(container, x + 0.1 * this.width, y, 0.8 * this.width, this.height, number.toString(), style);
            // draw rectangle border around HICANN
            pixiBackend.drawRectangleBorder(container, x, y, this.width, this.height, 5, style.fill, 1);
        };
        ;
        /**
         * remove all text-graphic objects from the container.
         */
        HicannNumber.prototype.clean = function () {
            pixiBackend.removeAllChildren(this.containerHover);
        };
        ;
        /**
         * update the UI checkboxes.
         */
        HicannNumber.prototype.updateCheckboxes = function () {
            $("#allNumbersCheckbox").prop("checked", this.showAll);
            $("#numberHoverCheckbox").prop("checked", this.showHover);
        };
        ;
        /**
         * Hide everything
         */
        HicannNumber.prototype.disable = function () {
            this.containerAll.visible = false;
            this.containerHover.visible = false;
        };
        /**
         * Make visible again (if the respective checkboxes are checked).
         */
        HicannNumber.prototype.recover = function () {
            this.containerAll.visible = this.showAll;
            this.containerHover.visible = true;
        };
        return HicannNumber;
    }());
    internalModule.HicannNumber = HicannNumber;
})(internalModule || (internalModule = {}));
var internalModule;
(function (internalModule) {
    var HicannInfo = /** @class */ (function () {
        function HicannInfo() {
            this.hicanns = [];
        }
        HicannInfo.prototype.toggleProperties = function (visible) {
            $("#hicannProperties").css("display", visible ? "initial" : "none");
        };
        ;
        HicannInfo.prototype.handleHicannClick = function (hicann, select, selectReticle) {
            var _this = this;
            if (select === void 0) { select = false; }
            if (selectReticle === void 0) { selectReticle = false; }
            if (!select) {
                if (this.hicanns.length === 0) {
                    this.displayProperties(hicann);
                }
            }
            else {
                var hicanns = [hicann];
                if (selectReticle) {
                    console.log("reticle selection not implemented yet");
                    // TODO: add the enums of all hicanns in the Reticle of the selected HICANN to the hicanns list
                    /*
                    corresponding python code:
                    In [1]: import Coordinate as C
          
                    In [2]: w = C.Wafer(33)
          
                    In [3]: h = C.HICANNOnWafer(C.Enum(267))
          
                    In [4]: h_global = C.HICANNGlobal(h, w)
          
                    In [5]: dnc = h_global.toDNCOnWafer()
          
                    In [6]: for h_on_dnc in C.iter_all(C.HICANNOnDNC):
                        ...:     print h_on_dnc.toHICANNOnWafer(dnc)
                        ...:
                        ...:
                    HICANNOnWafer(Enum(264))
                    HICANNOnWafer(Enum(265))
                    HICANNOnWafer(Enum(266))
                    HICANNOnWafer(Enum(267))
                    HICANNOnWafer(Enum(292))
                    HICANNOnWafer(Enum(293))
                    HICANNOnWafer(Enum(294))
                    HICANNOnWafer(Enum(295))
                    */
                }
                for (var _i = 0, hicanns_1 = hicanns; _i < hicanns_1.length; _i++) {
                    var hicann_1 = hicanns_1[_i];
                    if (this.hicanns.indexOf(hicann_1) === -1) {
                        this.hicanns.push(hicann_1);
                    }
                    else {
                        this.hicanns.splice(this.hicanns.indexOf(hicann_1), 1);
                    }
                }
                if (this.hicanns.length === 0) {
                    this.displayProperties(hicann);
                    this.toggleProperties(true);
                    $("#clearSelection").remove();
                }
                else {
                    this.toggleProperties(false);
                    $("#clearSelection").remove();
                    var hicannsString = this.hicanns.toString().split(",").join(", ");
                    var clearButton = $("<button id=\"clearSelection\">clear selection</button>")
                        .click(function () {
                        var lastHicann = _this.hicanns[_this.hicanns.length - 1];
                        _this.hicanns = [];
                        _this.drawSelectionSymbols(_this.hicanns);
                        _this.displayProperties(lastHicann);
                        _this.toggleProperties(true);
                        $("#clearSelection").remove();
                    });
                    $("#hicannNumber")
                        .html("HICANNs: " + hicannsString)
                        .after(clearButton);
                }
                this.drawSelectionSymbols(this.hicanns);
            }
        };
        ;
        /**
         * Show Hicann properties in left info box.
         */
        HicannInfo.prototype.displayProperties = function (hicannIndex) {
            $("#hicannNumber").html("HICANN " + hicannIndex);
            $("#neuronsNumber").html("" + wafer.hicanns[hicannIndex].numNeurons);
            $("#inputsNumber").html("" + wafer.hicanns[hicannIndex].numInputs);
            $("#leftBusesNumber").html("" + wafer.hicanns[hicannIndex].numBusesLeft);
            $("#rightBusesNumber").html("" + wafer.hicanns[hicannIndex].numBusesRight);
            $("#horizontalBusesNumber").html("" + wafer.hicanns[hicannIndex].numBusesHorizontal);
        };
        HicannInfo.prototype.drawSelectionSymbols = function (hicanns) {
            pixiBackend.removeAllChildren(pixiBackend.container.hicannSelection);
            if (hicanns.length !== 0) {
                var graphicsObject = undefined;
                for (var _i = 0, hicanns_2 = hicanns; _i < hicanns_2.length; _i++) {
                    var hicann = hicanns_2[_i];
                    var hicannPosition = wafer.hicanns[hicann].position;
                    graphicsObject = pixiBackend.drawRectangles([hicannPosition.x], [hicannPosition.y], [wafer.hicannWidth], [wafer.hicannHeight], 0xffffff, graphicsObject, 0.3);
                    pixiBackend.drawImage(pixiBackend.container.hicannSelection, "img/select.png", hicannPosition.x + 0.1 * wafer.hicannWidth, hicannPosition.y + 0.1 * wafer.hicannWidth, 0.2 * wafer.hicannWidth, 0.2 * wafer.hicannWidth);
                }
                pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.hicannSelection);
            }
            pixiBackend.renderer.render();
        };
        return HicannInfo;
    }());
    internalModule.HicannInfo = HicannInfo;
})(internalModule || (internalModule = {}));
/*
    * --- remove development mode ----
    * main.ts: const devMode
    * main.ts: devMode area with quickStart function
    * main.ts: modified wafer.loadOverviewData() function calls
    * wafer.ts: loadOverviewData networkFilePath parameter should not be optional!
    * wafer.ts: don't check for networkFilePath, but do 'marocco = new Module.Marocco(networkFilePath);' right away
    */
/// <reference path="modules/jquery.d.ts" />
/// <reference path="modules/jqueryui.d.ts" />
/// <reference path="modules/filesystem.d.ts" />
/// <reference path="modules/stats.d.ts" />
/// <reference path="modules/pixi.d.ts" />
/// <reference path="modules/pixiBackend.ts" />
/// <reference path="modules/wafer.ts" />
/// <reference path="modules/detailview.ts" />
/// <reference path="modules/overview.ts" />
/// <reference path="modules/automode.ts" />
/// <reference path="modules/manualmode.ts" />
/// <reference path="modules/routes.ts" />
/// <reference path="modules/lookupPlot.ts" />
/// <reference path="modules/waferImage.ts" />
/// <reference path="modules/settings.ts" />
/// <reference path="modules/summary.ts" />
/// <reference path="modules/hicannNumber.ts" />
/// <reference path="modules/hicannInfo.ts" />
/**
 * development mode: set to true to skip file upload procedure
 */
var devMode = false;
var mouseIsDown = false;
var mousePosition = {
    x: undefined,
    y: undefined
};
var mouseDownPosition = {
    x: undefined,
    y: undefined
};
var touchPosition = {
    x: undefined,
    y: undefined
};
var touches;
/**
 * HICANN property gradient color
 */
var numNeuronsColorOne = "ffffff";
/**
 * HICANN property gradient color
 */
var numNeuronsColorTwo = "174e75";
/**
 * HICANN property gradient color
 */
var numInputsColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
var numInputsColorTwo = "ef5450";
/**
 * HICANN property gradient color
 */
var numRoutesLeftColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
var numRoutesLeftColorTwo = "fbcb3f";
/**
 * HICANN property gradient color
 */
var numRoutesRightColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
var numRoutesRightColorTwo = "fbcb3f";
/**
 * HICANN property gradient color
 */
var numRoutesHorizontalColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
var numRoutesHorizontalColorTwo = "fbcb3f";
var domObjects = {};
var properties = [
    "neurons",
    "inputs",
    "leftBuses",
    "rightBuses",
    "horizontalBuses",
];
var wafer;
var overview;
var detailview;
var routesOnStage;
var reticlesOnStage;
var waferImage;
var automode;
var manualmode;
var routeInfo;
var settings;
var summary;
var hicannNumber;
var hicannInfo;
var canvasWidth = function () {
    return ($("#waferVisu").width());
};
var canvasHeight = function () {
    return ($("#waferVisu").height());
};
var canvasCenter = function () {
    return ({
        x: $("#leftInfoBox").outerWidth(true) + (canvasWidth() - domObjects.rightInfoBox[0].offsetWidth - $("#leftInfoBox").outerWidth()) / 2,
        y: canvasHeight() / 2,
    });
};
// wait for DOM to load
$(window).on('load', function () {
    console.log("dom ready");
    domReady = true;
});
/**
 * Indicator variable for the state of the static HTML DOM
 */
var domReady = false;
/**
 * Callback function for emscriptens Module.onRuntimeInitialized event.
 * Waiting for the DOM to load and then setting up the upload screen.
 */
function waitingForDOM() {
    console.log("start waiting for DOM");
    var waitingForDOMInterval = window.setInterval(function () {
        if (domReady) {
            clearInterval(waitingForDOMInterval);
            setupScreen();
        }
    }, 10);
}
/**
 * Set up the Filebrowser and drag and drop area for uploading
 * the marocco::results configuration file
 */
function setupScreen() {
    console.log("setting up screen");
    //upload marocco::results file
    var inputFile = undefined;
    // upload via filebrowser
    var fileBrowser = document.querySelector("#networkFile");
    fileBrowser.addEventListener("change", function (event) {
        inputFile = fileBrowser.files[0];
        $("#fileLabel").html(inputFile.name);
        $("#fileLabel").css("color", "#fbb535");
    }, false);
    // upload via drag-n-drop
    var dropZone = document.querySelector("#dropZone");
    dropZone.addEventListener("drop", function (e) {
        var event = e || window.event; //Firefox
        event.preventDefault();
        if (event.dataTransfer.files.length > 1) {
            alert("Please select only one file!");
        }
        else {
            inputFile = event.dataTransfer.files[0];
            $("#fileLabel").html(inputFile.name);
            $("#fileLabel").css("color", "#99ff99");
        }
    });
    dropZone.addEventListener("dragover", function (e) {
        var event = e || window.event;
        event.preventDefault();
    });
    // visual effects
    dropZone.addEventListener("dragenter", function () { $("#dropZone").css("border", "2px solid #fbb535"); });
    dropZone.addEventListener("dragleave", function () { $("#dropZone").css("border", "2px solid #222222"); });
    dropZone.addEventListener("drop", function () { $("#dropZone").css("border", "2px solid ##222222"); });
    // handle upload Button
    var uploadButton = document.querySelector("#upload");
    uploadButton.addEventListener("click", function () {
        if (inputFile === undefined) {
            alert("no file selected");
        }
        else {
            var filereader = new FileReader();
            // event handler for data loaded with filereader
            filereader.onload = function (event) {
                var target = event.target;
                var data = target.result;
                var contents = new Int8Array(data);
                // write file into emscriptens virtual file system (FS)
                // file name is "network" + extension of the input file
                console.log("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0]);
                FS.writeFile("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0], contents);
                // remove upload screen and display loading screen
                $("#uploadScreen").css("display", "none");
                $("#loadingScreen").css("display", "block");
                // start main program
                setTimeout(function () { main("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0]); }, 100);
            };
            filereader.onerror = function (event) {
                console.error("File could not be read! Code $event.target.error.code");
            };
            // read the contents of inputFile and fire onload event
            filereader.readAsArrayBuffer(inputFile);
        }
    });
    // ----- devMode -----
    // skip file upload
    if (devMode) {
        quickStart();
    }
    function quickStart() {
        // remove upload screen and display loading screen
        $("#uploadScreen").css("display", "none");
        $("#loadingScreen").css("display", "block");
        // start main program
        setTimeout(function () { main("undefined"); }, 100);
    }
    // ----- devMode -----
} //setupScreen
/**
 * The main function runs the core visualization program.
 * Instances of all the classes needed for the visualization are created and event handlers defined.
 */
function main(resultsFile) {
    ///////////////////////////
    // FPS, RAM Stats
    var stats = new Stats();
    stats.showPanel(0); // 0: fps, 1: ms, 2: mb, 3+: custom
    stats.dom.style.position = "relative";
    stats.dom.style.marginLeft = "20px";
    document.querySelector("#fpsContainer").appendChild(stats.dom);
    function animate() {
        stats.begin();
        stats.end();
        requestAnimationFrame(animate);
    }
    requestAnimationFrame(animate);
    ///////////////////////////
    console.log("WebGL is" + (!PIXI.utils.isWebGLSupported ? " not" : "") + " supported by this browser.");
    //////////////////////////////////
    // store common DOM elements
    // deprecated???
    referenceDOM();
    // setup pixiBackend
    pixiBackend.renderer = new pixiBackend.Renderer($("#waferVisu"), 0x333333, canvasWidth(), canvasHeight());
    pixiBackend.container.setup();
    visualizeFile(resultsFile);
    waferImage = new internalModule.WaferImage(wafer, "img/hicann.png", pixiBackend.container.hicannImages, wafer.hicannWidth, wafer.hicannHeight);
    // draw images of hicanns on wafer
    waferImage.draw();
    // draw Electronic Visions and HBP Logos
    // TODO: non-frickel solution plz
    (function () {
        // Visions Logo
        var height = 600;
        var width = height * 7246 / 9380;
        var x = 0;
        var y = wafer.hicannHeight * 16 - height;
        pixiBackend.drawImage(pixiBackend.container.logos, "img/visionsLogo.png", x, y, width, height, 0.2);
        // HBP Logo
        width = height;
        x = wafer.hicannWidth * 36 - width;
        pixiBackend.drawImage(pixiBackend.container.logos, "img/HBPLogo.png", x, y, width, height, 0.2);
    })();
    // adjust stage position and scale to show full wafer centered in screen
    centerWafer();
    // start off with automatic zoom mode
    automode.init(undefined, false, false);
    // setup UI using JQueryUI
    setupJQueryUI();
    // UI property gradients in right info panel
    setupPropertyGradients();
    // build UI hicann list in left info panel
    buildElementsTree();
    routeInfo = new internalModule.RouteInfo();
    settings = new internalModule.Settings();
    summary = new internalModule.Summary();
    hicannNumber = new internalModule.HicannNumber(wafer.hicannWidth, wafer.hicannHeight);
    hicannInfo = new internalModule.HicannInfo();
    //////////////////////////////////
    // Event Listeners
    $(window).keydown(function (e) {
        handleKeyDown(e || window.event);
    });
    $("#visuWindow").mousedown(function (e) {
        handleMouseDown((e || window.event));
    });
    document.querySelector("#visuWindow").addEventListener("touchstart", function (e) {
        handleTouchstart(e || window.event);
    });
    document.querySelector("#visuWindow").addEventListener("touchmove", function (e) {
        e.preventDefault();
        handleTouchmove(e || window.event);
    });
    $("#visuWindow").mouseup(function (e) {
        handleMouseUp(e || window.event);
    });
    $("#visuWindow").mousemove(function (e) {
        handleMouseMove(e || window.event);
    });
    $("#visuWindow").mouseout(function (e) {
        mouseIsDown = false;
    });
    $("#visuWindow").dblclick(function (e) {
        var event = e || window.event;
        if (!tools.pointInRectangle({
            x: event.clientX,
            y: event.clientY
        }, {
            x: $("#routeInfoBox").offset().left,
            y: $("#routeInfoBox").offset().top,
            width: $("#routeInfoBox").outerWidth(),
            height: $("#routeInfoBox").outerHeight(),
        })) {
            routesOnStage.handleVisuDoubleClick(event.clientX, event.clientY);
        }
    });
    document.querySelector("#visuWindow").addEventListener("wheel", function (e) {
        handleWheel(e || window.event);
    });
    $("#allNumbersCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        hicannNumber.setAll(checked);
        pixiBackend.renderer.render();
    });
    $("#waferImageCheckbox").change(function (e) {
        waferImage.setVisible((e || window.event).target.checked);
        pixiBackend.renderer.render();
    });
    $("#reticlesCheckbox").change(function (e) {
        reticlesOnStage.setReticles((e || window.event).target.checked);
        pixiBackend.renderer.render();
    });
    $("#numNeuronsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            pixiBackend.container.backgrounds.children[index].visible = checked;
        }
        ;
        manualmode.setAllCheckboxes("numNeurons", checked);
        pixiBackend.renderer.render();
    });
    $("#numInputsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            pixiBackend.container.inputs.children[index].visible = checked;
        }
        ;
        manualmode.setAllCheckboxes("numInputs", checked);
        pixiBackend.renderer.render();
    });
    $("#verticalLeftCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            pixiBackend.container.overviewBusesLeft.children[index].visible = checked;
        }
        ;
        manualmode.setAllCheckboxes("left", checked);
        pixiBackend.renderer.render();
    });
    $("#verticalRightCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            pixiBackend.container.overviewBusesRight.children[index].visible = checked;
        }
        ;
        manualmode.setAllCheckboxes("right", checked);
        pixiBackend.renderer.render();
    });
    $("#horizontalCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            pixiBackend.container.overviewBusesHorizontal.children[index].visible = checked;
        }
        ;
        manualmode.setAllCheckboxes("horizontal", checked);
        pixiBackend.renderer.render();
    });
    $("#verticalLeftDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_bsl");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#verticalRightDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_bsr");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#horizontalDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_bsh");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#synDriverDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_snd");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#synGridDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_sng");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#neuronsDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_neu");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#repeatersDetailsCheckbox").change(function (e) {
        var checked = (e || window.event).target.checked;
        for (var index = 0; index <= wafer.enumMax; index++) {
            var checkbox = $("#hicanns_0_" + index + "_DV_rep");
            if (checkbox.prop("checked") !== checked) {
                checkbox.click();
            }
        }
        pixiBackend.renderer.render();
    });
    $("#automode").click(function () {
        if (!automode.enabled) {
            // store detail level
            var levelOneEnabled = detailview.enabled;
            var levelTwoEnabled = detailview.levelTwoEnabled;
            // determine closest hicann for auto detail view
            var hicannClosestToCenter = detailview.hicannClosestToCenter(canvasCenter());
            // reset view
            manualmode.resetView();
            // start auto Mode
            automode.init(hicannClosestToCenter, levelOneEnabled, levelTwoEnabled);
            manualmode.enabled = false;
        }
    });
    $("#manualmode").click(function () {
        if (!manualmode.enabled) {
            // store detail level
            var levelOneEnabled = detailview.enabled;
            var levelTwoEnabled = detailview.levelTwoEnabled;
            // reset view
            if (levelOneEnabled) {
                automode.startOverview();
            }
            // start manual Mode
            manualmode.init(levelOneEnabled, levelTwoEnabled);
            automode.enabled = false;
        }
    });
    $("#waferList .listHeader button").click(function () {
        var input = Number($("#waferList .listHeader input").val());
        if (isNaN(input)) {
            alert("please enter a number");
        }
        else {
            scrollToHicann(input);
        }
    });
    $("#waferList .listHeader input").keydown(function (event) {
        if (event.which === 13) {
            $("#waferList .listHeader button").click();
        }
        ;
    });
    $("#routesList .listHeader button").eq(0).click(function () {
        var input = Number($("#routesList .listHeader input").eq(0).val());
        if (isNaN(input)) {
            alert("please enter a number");
        }
        else {
            scrollToRoute(input);
        }
    });
    $("#routesList .listHeader input").eq(0).keydown(function (event) {
        if (event.which === 13) {
            $("#routesList .listHeader button").eq(0).click();
        }
    });
    // Resize Canvas, when window is rescaled;
    $(window).resize(function () {
        pixiBackend.renderer.renderer.resize(canvasWidth(), canvasHeight());
        detailview.determineThreshold(canvasHeight());
        setupJQueryUI();
        pixiBackend.renderer.render();
    });
} //main
function visualizeFile(resultsFile) {
    try {
        wafer = new internalModule.Wafer();
        // ----- devMode -----
        devMode ? wafer.loadOverviewData() : wafer.loadOverviewData(resultsFile);
        // ----- devMode -----
        // Adjust color gradients when a HICANN property is zero for every HICANN.
        setHicannPropertyGradients();
        overview = new internalModule.Overview(wafer, {
            numNeuronsColorOne: numNeuronsColorOne,
            numNeuronsColorTwo: numNeuronsColorTwo,
            numInputsColorOne: numInputsColorOne,
            numInputsColorTwo: numInputsColorTwo,
            numRoutesLeftColorOne: numRoutesLeftColorOne,
            numRoutesLeftColorTwo: numRoutesLeftColorTwo,
            numRoutesRightColorOne: numRoutesRightColorOne,
            numRoutesRightColorTwo: numRoutesRightColorTwo,
            numRoutesHorizontalColorOne: numRoutesHorizontalColorOne,
            numRoutesHorizontalColorTwo: numRoutesHorizontalColorTwo
        });
        // draw overview of wafer
        overview.drawWafer();
        detailview = new internalModule.Detailview(wafer);
        routesOnStage = new internalModule.RoutesOnStage(detailview);
        // draw routes
        routesOnStage.drawRoutes();
        // hide routes
        for (var _i = 0, _a = routesOnStage.routes; _i < _a.length; _i++) {
            var route = _a[_i];
            routesOnStage.setRoute(route, false);
        }
        reticlesOnStage = new internalModule.ReticlesOnStage(overview, pixiBackend.container.reticles);
        // threshold where the lookup plot is made visible
        reticlesOnStage.threshold = fullWaferScale() * 2 / 3;
        // draw reticles
        reticlesOnStage.drawReticles();
        // hide reticles
        reticlesOnStage.setReticles(false);
        automode = new internalModule.Automode(overview, detailview);
        manualmode = new internalModule.Manualmode(overview, detailview);
        // display properties for HICANN 0 in right info panel
        // hicannInfo.handleHicannClick(0);
        // build UI routes list in left info panel
        buildRoutesTree();
        // UI property gradients in right info panel
        setupPropertyGradients();
    }
    catch (e) {
        alert("cannot load input file. \nPossible reasons: invalid results file or error in emscripten marocco build");
        throw (new Error("cannot load input file"));
    }
}
function removeVisualization() {
    // remove all elements from the overview containers
    overview.resetOverview();
    // remove all elements from the detailview containers
    detailview.resetDetailview();
    $("#routesTree").empty();
    pixiBackend.renderer.render();
}
/**
 * Center the wafer in middle of the canvas and adjust the zoom-scale, so the full wafer is visible.
 */
function centerWafer() {
    var scale = fullWaferScale();
    var transform = pixiBackend.container.stage.transform;
    transform.scale.x = scale;
    transform.scale.y = scale;
    var hicannNumber = 173;
    var stagePosition = transform.position;
    var hicannPosition = wafer.hicanns[hicannNumber].position;
    var newPosition = {
        x: -(hicannPosition.x + wafer.hicannWidth / 2) * transform.scale.x + canvasCenter().x,
        y: -(hicannPosition.y + wafer.hicannHeight / 2) * transform.scale.y + canvasCenter().y,
    };
    stagePosition.x = newPosition.x;
    stagePosition.y = newPosition.y;
    pixiBackend.renderer.render();
}
/**
 * Calculate the scale, where the full wafer fits onto the canvas.
 */
function fullWaferScale() {
    var waferWidth = 36 * (wafer.hicannWidth + wafer.hicannMargin) * 1.3;
    var waferHeight = 16 * (wafer.hicannHeight + wafer.hicannMargin) * 1.3;
    return (canvasWidth() / waferWidth < canvasHeight() / waferHeight) ? canvasWidth() / waferWidth : canvasHeight() / waferHeight;
}
/**
 * If a property is zero for every HICANN, the color gradient has to be adjusted.
 */
function setHicannPropertyGradients() {
    if (wafer.hicanns === []) {
        throw (new Error("HICANN data has to be loaded into wafer class before gradients can be set."));
    }
    if (wafer.numNeuronsMax === 0) {
        numNeuronsColorTwo = numNeuronsColorOne;
    }
    ;
    if (wafer.numInputsMax === 0) {
        numInputsColorTwo = numInputsColorOne;
    }
    ;
    if (wafer.numBusesLeftMax === 0) {
        numRoutesLeftColorTwo = numRoutesLeftColorOne;
    }
    ;
    if (wafer.numBusesRightMax === 0) {
        numRoutesRightColorTwo = numRoutesRightColorOne;
    }
    ;
    if (wafer.numBusesHorizontalMax === 0) {
        numRoutesHorizontalColorTwo = numRoutesHorizontalColorOne;
    }
    ;
}
/**
 * Build the HTML for the HICANN "tree-style" list in the left info-box
 */
function buildElementsTree() {
    // hicann list
    // hicann surrounding unordered list
    var hicannList = document.createElement("ul");
    $("#elementsTree").append(hicannList);
    var _loop_4 = function (i) {
        var listItem = document.createElement("li");
        hicannList.appendChild(listItem);
        var hicannInput = document.createElement("input");
        hicannInput.type = "checkbox";
        hicannInput.checked = true;
        hicannInput.classList.add("fork");
        hicannInput.id = "hicanns_0_" + i;
        listItem.appendChild(hicannInput);
        var hicannInputLabel = document.createElement("label");
        hicannInputLabel.htmlFor = "hicanns_0_" + i;
        hicannInputLabel.classList.add("checkboxLabel");
        listItem.appendChild(hicannInputLabel);
        var hicann = document.createElement("button");
        hicann.innerText = "HICANN ";
        hicann.addEventListener("click", function () { handleListClickHicann(event); });
        listItem.appendChild(hicann);
        var hicannNumber_1 = document.createElement("span");
        hicannNumber_1.textContent = "" + i;
        hicann.appendChild(hicannNumber_1);
        var elementsList = document.createElement("ul");
        listItem.appendChild(elementsList);
        // overview elements
        var overviewListItem = document.createElement("li");
        elementsList.appendChild(overviewListItem);
        var overviewInput = document.createElement("input");
        overviewInput.type = "checkbox";
        overviewInput.checked = true;
        overviewInput.classList.add("fork");
        overviewInput.id = "hicanns_0_" + i + "_OV";
        overviewListItem.appendChild(overviewInput);
        // overview elements checkbox label
        var overviewInputLabel = document.createElement("label");
        overviewInputLabel.htmlFor = "hicanns_0_" + i + "_OV";
        overviewInputLabel.classList.add("checkboxLabel");
        overviewListItem.appendChild(overviewInputLabel);
        // overview elements checkbox
        var overviewCheckbox = document.createElement("input");
        overviewCheckbox.type = "checkbox";
        overviewCheckbox.checked = true;
        overviewCheckbox.addEventListener("change", function (e) {
            manualmode.overviewCheckbox(i, (e || window.event).target.checked);
        });
        overviewCheckbox.classList.add("hicannElementCheckbox");
        overviewCheckbox.id = "hicanns_0_" + i + "_OV_checkbox";
        overviewListItem.appendChild(overviewCheckbox);
        // overview elements label
        var overviewLabel = document.createElement("label");
        overviewLabel.innerHTML = "Overview";
        overviewLabel.htmlFor = "hicanns_0_" + i + "_OV_checkbox";
        overviewListItem.appendChild(overviewLabel);
        var overviewList = document.createElement("ul");
        overviewListItem.appendChild(overviewList);
        // number of neurons
        overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.numNeurons, pixiBackend.container.backgrounds, "numNeurons", "number of neurons", "neu"));
        // number of inputs
        overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.numInputs, pixiBackend.container.inputs, "numInputs", "number of inputs", "inp"));
        // Buses left
        overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.left, pixiBackend.container.overviewBusesLeft, "left", "vertical left", "bsl"));
        // Buses right
        overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.right, pixiBackend.container.overviewBusesRight, "right", "vertical right", "bsr"));
        // Buses horizontal
        overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.horizontal, pixiBackend.container.overviewBusesHorizontal, "horizontal", "horizontal", "bsh"));
        // detailed elements
        var detailviewListItem = document.createElement("li");
        elementsList.appendChild(detailviewListItem);
        var detailviewInput = document.createElement("input");
        detailviewInput.type = "checkbox";
        detailviewInput.checked = true;
        detailviewInput.classList.add("fork");
        detailviewInput.id = "hicanns_0_" + i + "_DV";
        detailviewListItem.appendChild(detailviewInput);
        // detailed elements checkbox label
        var detailviewInputLabel = document.createElement("label");
        detailviewInputLabel.htmlFor = "hicanns_0_" + i + "_DV";
        detailviewInputLabel.classList.add("checkboxLabel");
        detailviewListItem.appendChild(detailviewInputLabel);
        // detailed elements checkbox
        var detailviewCheckbox = document.createElement("input");
        detailviewCheckbox.type = "checkbox";
        detailviewCheckbox.checked = false;
        detailviewCheckbox.addEventListener("change", function (e) {
            manualmode.detailviewCheckbox(i, (e || window.event).target.checked);
        });
        detailviewCheckbox.classList.add("hicannElementCheckbox");
        detailviewCheckbox.id = "hicanns_0_" + i + "_DV_checkbox";
        detailviewListItem.appendChild(detailviewCheckbox);
        // detailed elements label
        var detailviewLabel = document.createElement("label");
        detailviewLabel.innerHTML = "Detailview";
        detailviewLabel.htmlFor = "hicanns_0_" + i + "_DV_checkbox";
        detailviewListItem.appendChild(detailviewLabel);
        var detailviewList = document.createElement("ul");
        detailviewListItem.appendChild(detailviewList);
        // detailed Buses left
        detailviewList.appendChild(newDetailElement(i, manualmode.busesLeft, "detailLeft", "left", "bsl"));
        // detailed Buses right
        detailviewList.appendChild(newDetailElement(i, manualmode.busesRight, "detailRight", "right", "bsr"));
        // detailed Buses horizontal
        detailviewList.appendChild(newDetailElement(i, manualmode.busesHorizontal, "detailHorizontal", "horizontal", "bsh"));
        // synapse drivers
        detailviewList.appendChild(newDetailElement(i, manualmode.synDriver, "synDriver", "synDriver", "snd"));
        // synapse grid
        detailviewList.appendChild(newDetailElement(i, manualmode.synGrid, "synGrid", "synGrid", "sng"));
        // neurons
        detailviewList.appendChild(newDetailElement(i, manualmode.neurons, "neurons", "neurons", "neu"));
        // repeaters
        detailviewList.appendChild(newDetailElement(i, manualmode.repeaters, "repeaters", "repeaters", "rep"));
    };
    // hicanns add list items containing buttons
    for (var i = 0; i <= wafer.enumMax; i++) {
        _loop_4(i);
    }
    ;
}
/**
 * Create a new list item for the list of detailed elements in the HICANN list.
 * @param hicannIndex Index of the current HICANN
 * @param drawFunction Function in manualmode namespace, that handles drawing and removing
 * elements for a single HICANN. e.g. busesLeft(), synDriver()
 * @param type element type, e.g. neurons, repeaters.
 * @param labelText innerHTML text for the list item.
 * @param typeAbbreviation Abbreviation of the type for the ID of the checkbox. e.g. bsl for busesLeft, neu for neurons
 */
function newDetailElement(hicannIndex, drawFunction, type, labelText, typeAbbreviation) {
    var listItem = document.createElement("li");
    var checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.checked = false;
    checkbox.addEventListener("change", function (e) {
        drawFunction.call(manualmode, hicannIndex, (e || window.event).target.checked);
        pixiBackend.renderer.render();
        manualmode.checkAllCheckboxes(type);
        manualmode.checkAllDetailedElements(hicannIndex);
    });
    checkbox.classList.add("hicannElementCheckbox");
    checkbox.id = "hicanns_0_" + hicannIndex + "_DV_" + typeAbbreviation;
    listItem.appendChild(checkbox);
    var label = document.createElement("label");
    label.innerHTML = labelText;
    listItem.appendChild(label);
    return listItem;
}
/**
 * Create a new list item for the list of overview elements in the HICANN list.
 * @param hicannIndex Index of the current HICANN
 * @param selectedElements array in manual mode that stores selection
 * @param container PIXI JS container, that stores the graphics elements/sprites for the element
 * @param type element type, e.g. neurons, repeaters.
 * @param labelText innerHTML text for the list item.
 * @param typeAbbreviation Abbreviation of the type for the ID of the checkbox. e.g. bsl for busesLeft, neu for neurons
 */
function newOverviewElement(hicannIndex, selectedElements, container, type, labelText, typeAbbreviation) {
    var listItem = document.createElement("li");
    var checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.checked = true;
    checkbox.addEventListener("change", function (e) {
        var checked = (e || window.event).target.checked;
        selectedElements[hicannIndex] = checked;
        container.children[hicannIndex].visible = checked;
        pixiBackend.renderer.render();
        manualmode.checkAllCheckboxes(type);
        manualmode.checkAllOverviewElements(hicannIndex);
    });
    checkbox.classList.add("hicannElementCheckbox");
    checkbox.id = "hicanns_0_" + hicannIndex + "_OV_" + typeAbbreviation;
    listItem.appendChild(checkbox);
    var label = document.createElement("label");
    label.innerHTML = labelText;
    listItem.appendChild(label);
    return listItem;
}
/**
 * Build the HTML for the Routes "tree-style" list in the left info-box
 */
function buildRoutesTree() {
    // select all Routes checkbox
    $("#routes_0_check").click(function (e) {
        var checked = (e || window.event).target.checked;
        for (var _i = 0, _a = routesOnStage.routes; _i < _a.length; _i++) {
            var route = _a[_i];
            routesOnStage.setRoute(route, checked);
            routesOnStage.setCheckbox(route, checked);
        }
        ;
        pixiBackend.renderer.render();
    });
    // routes label
    $("#allRoutes").click(function () {
        routesOnStage.handleRouteDoubleClick();
    });
    // route list
    // route surrounding unordered list
    var routesList = document.createElement("ul");
    $("#routesTree").append(routesList);
    var _loop_5 = function (route) {
        var ID = route.ID;
        var routeListItem = document.createElement("li");
        routesList.appendChild(routeListItem);
        var routeCheckbox = document.createElement("input");
        routeCheckbox.type = "checkbox";
        routeCheckbox.checked = false;
        routeCheckbox.classList.add("routeCheckbox");
        routeCheckbox.id = "routes_0_" + ID;
        routeCheckbox.addEventListener("change", function (e) {
            var checked = (e || window.event).target.checked;
            routesOnStage.setRoute(route, checked);
            routesOnStage.checkAllRoutes();
            pixiBackend.renderer.render();
        });
        routeListItem.appendChild(routeCheckbox);
        var routeLabel = document.createElement("button");
        routeLabel.innerText = "Route ";
        routeLabel.addEventListener("click", function () {
            routesOnStage.handleRouteClick([route]);
        });
        routeListItem.appendChild(routeLabel);
        var routeLabelNumber = document.createElement("span");
        routeLabelNumber.textContent = "" + (ID + 1);
        routeLabel.appendChild(routeLabelNumber);
    };
    // routes: add list items
    for (var _i = 0, _a = routesOnStage.routes; _i < _a.length; _i++) {
        var route = _a[_i];
        _loop_5(route);
    }
}
/**
 * Event handler for clicking on a HICANN in the HICANN list.
 */
function handleListClickHicann(event) {
    var hicannNumber = parseInt(event.path[0].innerText.split(" ")[1]);
    var transform = pixiBackend.container.stage.transform;
    var stagePosition = transform.position;
    var hicannPosition = wafer.hicanns[hicannNumber].position;
    var newPosition = {
        x: -(hicannPosition.x + wafer.hicannWidth / 2) * transform.scale.x + canvasCenter().x,
        y: -(hicannPosition.y + wafer.hicannHeight / 2) * transform.scale.y + canvasCenter().y,
    };
    pixiBackend.animateStagePosition(stagePosition.x, stagePosition.y, newPosition.x, newPosition.y, 700);
    animateBorderAroundHicann(pixiBackend.container.border, hicannPosition.x, hicannPosition.y, wafer.hicannWidth, wafer.hicannHeight, 10, "0xff0066");
    pixiBackend.renderer.render();
    hicannInfo.handleHicannClick(hicannNumber);
}
/**
 * Animate a temporary border around a HICANN.
 * @param container PixiJS container for the border object.
 * @param x x-coordinate of the top left corner of the HICANN.
 * @param y y-coordinate of the top left corner of the HICANN.
 * @param width Width of the HICANN.
 * @param height Height of the HICANN.
 * @param lineWidth Line-Width for the border.
 * @param color Color of border. Requires a hex-color in the form "0xffffff".
 */
function animateBorderAroundHicann(container, x, y, width, height, lineWidth, color) {
    var alpha = 1;
    var timer = setInterval(function () {
        if (container.children.length === 1) {
            pixiBackend.removeChild(container, 0);
        }
        ;
        pixiBackend.drawRectangleBorder(container, x, y, width, height, lineWidth, color, alpha);
        pixiBackend.renderer.render();
        alpha -= 0.01;
        if (Math.round(alpha * 100) / 100 === 0.00) {
            clearInterval(timer);
            pixiBackend.removeChild(container, 0);
        }
        ;
    }, 15);
}
/**
 * Use JQueryUI to set up part of the UI
 */
function setupJQueryUI() {
    // add resizability to wafer list in left info panel
    $("#waferList")
        .resizable({
        handles: "s",
        alsoResizeReverse: "#routesList",
        maxHeight: 0.9 * $(window).innerHeight(),
        minHeight: 0.1 * $(window).innerHeight(),
    });
    /*
    $("#waferVisu")
        .resizable({
            handles: "e",
            alsoResizeReverse: `#graphVisu, #graphVisu div, #graphVisu canvas`,
            maxWidth: 0.9*$(window).innerWidth(),
            minWidth: 0.1*$(window).innerWidth(),
        })
    */
    // route width slider
    $("#routeWidthSlider")
        .slider({
        slide: function (event, ui) {
            routesOnStage.handleRouteWidthSlider(ui.value);
        },
        min: 1,
        max: 5
    });
    // initialize
    $("#routeWidthSlider").slider("value", 2);
    // hBus segment slider
    $("#hBusWidthSlider")
        .slider({
        stop: function (event, ui) {
            settings.adjustHBusSegment(ui.value);
        },
        min: 0,
        max: 1,
        step: 0.01
    });
    // vBus segment slider
    $("#vBusWidthSlider")
        .slider({
        stop: function (event, ui) {
            settings.adjustVBusSegment(ui.value);
        },
        min: 0,
        max: 1,
        step: 0.01
    });
}
/**
 * Helper function to reference the DOM
 */
function addByID(object, id) {
    object[id] = $("#" + id);
}
/**
 * Helper function to reference the DOM
 */
function addProperty(object, property) {
    object[property] = {};
    addByID(object[property], property + "Number");
    addByID(object[property], property + "Gradient");
    addByID(object[property], property + "Min");
    addByID(object[property], property + "Max");
}
/**
 * Store references to DOM objects to save performance.
 */
function referenceDOM() {
    addByID(domObjects, "controlsContainer");
    addByID(domObjects, "rightInfoBox");
    addByID(domObjects, "hicannNumber");
    addByID(domObjects, "elementsTree");
    addByID(domObjects, "routesTree");
    for (var i = 0; i < properties.length; i++) {
        addProperty(domObjects, properties[i]);
    }
    ;
}
/**
 * Set the background colors as well as Min, Max numbers for all HICANN property gradients.
 */
function setupPropertyGradients() {
    domObjects.neurons.neuronsGradient.css("background", "linear-gradient(90deg, #" + numNeuronsColorOne + ", #" + numNeuronsColorTwo + ")");
    domObjects.neurons.neuronsMin.html("0");
    domObjects.neurons.neuronsMax.html(wafer.numNeuronsMax);
    domObjects.inputs.inputsGradient.css("background", "linear-gradient(90deg, #" + numInputsColorOne + ", #" + numInputsColorTwo + ")");
    domObjects.inputs.inputsMin.html("0");
    domObjects.inputs.inputsMax.html(wafer.numInputsMax);
    domObjects.leftBuses.leftBusesGradient.css("background", "linear-gradient(90deg, #" + numRoutesLeftColorOne + ", #" + numRoutesLeftColorTwo + ")");
    domObjects.leftBuses.leftBusesMin.html("0");
    domObjects.leftBuses.leftBusesMax.html(wafer.numBusesLeftMax);
    domObjects.rightBuses.rightBusesGradient.css("background", "linear-gradient(90deg, #" + numRoutesRightColorOne + ", #" + numRoutesRightColorTwo + ")");
    domObjects.rightBuses.rightBusesMin.html("0");
    domObjects.rightBuses.rightBusesMax.html(wafer.numBusesRightMax);
    domObjects.horizontalBuses.horizontalBusesGradient.css("background", "linear-gradient(90deg, #" + numRoutesHorizontalColorOne + ", #" + numRoutesHorizontalColorTwo + ")");
    domObjects.horizontalBuses.horizontalBusesMin.html("0");
    domObjects.horizontalBuses.horizontalBusesMax.html(wafer.numBusesHorizontalMax);
}
/**
 * Event handler for keyboard events
 */
function handleKeyDown(event) {
    var key = event.which;
    switch (key) {
        case 65:// a
            break;
        case 83:// s
            break;
    }
    ;
}
;
/**
 * Event handler for mouse-down event
 */
function handleMouseDown(event) {
    mouseIsDown = true;
    mouseDownPosition = {
        x: event.clientX,
        y: event.clientY
    };
    /*
    if (detailview.enabled) {
        mouseOverSynapse();
        detailview.handleSynapseClick();
    };
    */
    var hicann = mouseOverHicann(mouseDownPosition);
    if (hicann !== undefined) {
        hicannInfo.handleHicannClick(hicann, event.altKey, event.shiftKey);
    }
}
;
/**
 * Event handler for the mouse-up event
 * - in automode: switch to neighbor HICANN
 */
function handleMouseUp(event) {
    mouseIsDown = false;
    var positionDiff = {
        x: (event.clientX - mouseDownPosition.x),
        y: (event.clientY - mouseDownPosition.y)
    };
    if ((positionDiff.x !== 0) || (positionDiff.y !== 0)) {
        if (detailview.enabled && automode.enabled) {
            // horizontal movement
            if (positionDiff.x > 0) {
                if (detailview.westernHicannCloser(canvasCenter())) {
                    hicannInfo.handleHicannClick(detailview.westernHicann);
                    automode.startWesternHicann(detailview.currentHicann);
                }
                ;
            }
            else {
                if (detailview.easternHicannCloser(canvasCenter())) {
                    hicannInfo.handleHicannClick(detailview.easternHicann);
                    automode.startEasternHicann(detailview.currentHicann);
                }
            }
            ;
            // vertical movement
            if (positionDiff.y > 0) {
                if (detailview.northernHicannCloser(canvasCenter())) {
                    hicannInfo.handleHicannClick(detailview.northernHicann);
                    automode.startNorthernHicann(detailview.currentHicann);
                }
            }
            else {
                if (detailview.southernHicannCloser(canvasCenter())) {
                    hicannInfo.handleHicannClick(detailview.southernHicann);
                    automode.startSouthernHicann(detailview.currentHicann);
                }
                ;
            }
            ;
        }
        ;
    }
    else if (!tools.pointInRectangle({
        x: event.clientX,
        y: event.clientY
    }, {
        x: $("#routeInfoBox").offset().left,
        y: $("#routeInfoBox").offset().top,
        width: $("#routeInfoBox").outerWidth(),
        height: $("#routeInfoBox").outerHeight(),
    })) {
        routesOnStage.handleVisuClick(event.clientX, event.clientY);
    }
}
;
/**
 * Event handler for the mouse-move event
 * - stage panning
 */
function handleMouseMove(event) {
    var newMousePosition = {
        x: event.clientX,
        y: event.clientY
    };
    if (mouseIsDown) {
        if (!tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#routeInfoBox") &&
            !tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#settingsBox") &&
            !tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#summaryBox")) {
            var diff = {
                x: (newMousePosition.x - mousePosition.x),
                y: (newMousePosition.y - mousePosition.y),
            };
            // pan effect
            pixiBackend.moveStage(diff.x, diff.y);
        }
    }
    else {
        // display hicann number
        var hicannIndex = mouseOverHicann(mousePosition);
        if (hicannIndex !== undefined) {
            hicannNumber.handleMouseHover(hicannIndex);
        }
    }
    ;
    pixiBackend.renderer.render();
    mousePosition = newMousePosition;
}
;
/**
   * check if mouse is over a HICANN
   */
function mouseOverHicann(mousePosition) {
    for (var index = wafer.enumMin; index <= wafer.enumMax; index++) {
        // loop through hicanns and check if mouse if over them
        if (pixiBackend.mouseInRectangle(mousePosition, wafer.hicanns[index].position.x, wafer.hicanns[index].position.y, wafer.hicannWidth, wafer.hicannHeight)) {
            return index;
        }
    }
    return undefined;
}
/**
 * Event handler for the wheel event
 * - zoom in or out of stage
 * - show lookup plot if zoomed out all the way
 * - adjust routewidth
 * - automode/ manualMode: switch between overview, detailview and detailviewLevelTwo
 */
function handleWheel(event) {
    // prevent horizontal scrolling to prevent accidental page-back
    if (event.deltaX !== 0) {
        event.preventDefault();
    }
    var factor = Math.abs(event.deltaY / 600) + 1;
    var transform = pixiBackend.container.stage.transform;
    var pixiScale = transform.scale.x;
    // limit zooming out
    if ((pixiScale <= reticlesOnStage.threshold) && (event.deltaY > 0)) {
        // show lookup plot (reticle & fpga coordinates)
        reticlesOnStage.setReticles(true);
        pixiBackend.renderer.render();
        // end handleWheel
        return "reached zoom limit";
    }
    if (reticlesOnStage.enabled && (event.deltaY < 0)) {
        // hide lookup plot
        reticlesOnStage.setReticles(false);
        pixiBackend.renderer.render();
    }
    if (!tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#routeInfoBox") &&
        !tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#settingsBox") &&
        !tools.mouseInDiv({ x: event.clientX, y: event.clientY }, "#summaryBox")) {
        if (Math.abs(event.deltaY) !== event.deltaY) {
            // zoom stage
            pixiBackend.zoomIn(factor, event.clientX, event.clientY);
            // auto mode
            if (automode.enabled) {
                // zoom into detail view
                if ((!detailview.enabled) && (pixiScale >= detailview.threshold) && (pixiScale < detailview.threshold2)) {
                    // determine hicann in view
                    var hicannIndex = hicannNumber.hicannIndex || detailview.hicannClosestToCenter(canvasCenter());
                    // display Hicann properties in right Infobox
                    hicannInfo.handleHicannClick(hicannIndex);
                    // start the detailview
                    automode.startDetailview(hicannIndex, true);
                }
                // zoom into detailview level two
                if (pixiScale >= detailview.threshold2) {
                    automode.startDetailviewLevelTwo();
                }
                // manual mode
            }
            else {
                if ((pixiScale >= detailview.threshold) && (!detailview.enabled)) {
                    manualmode.startDetailview();
                }
                if ((pixiScale >= detailview.threshold2) && (!detailview.levelTwoEnabled)) {
                    manualmode.startDetailviewLevelTwo();
                }
            }
            // route width adjustment
            routesOnStage.adjustRouteWidth(pixiScale);
        }
        else {
            // zoom stage
            pixiBackend.zoomOut(factor, event.clientX, event.clientY);
            // auto mode
            if (automode.enabled) {
                // zoom out of detailview level two
                if ((pixiScale < detailview.threshold2) && (pixiScale > detailview.threshold)) {
                    automode.startDetailview(detailview.currentHicann, false);
                }
                ;
                // zoom out of detail view
                if ((pixiScale < detailview.threshold) && (detailview.enabled)) {
                    automode.startOverview();
                }
                ;
                // manual mode
            }
            else {
                if ((pixiScale < detailview.threshold) && (detailview.enabled)) {
                    manualmode.leaveDetailview();
                }
                if ((pixiScale < detailview.threshold2) && (detailview.levelTwoEnabled)) {
                    manualmode.leaveDetailviewLevelTwo();
                }
            }
            // route width adjustment
            routesOnStage.adjustRouteWidth(pixiScale);
        }
        ;
    }
    ;
    pixiBackend.renderer.render();
}
/**
 * event handler for mobile touchmove event
 */
function handleTouchmove(event) {
    // TODO: don't pan or zoom when one touchpoint is in the #routeInfoBox
    for (var touch in event.touches) {
        if (tools.pointInRectangle({
            x: event.pageX,
            y: event.pageY
        }, {
            x: $("#routeInfoBox").offset().left,
            y: $("#routeInfoBox").offset().top,
            width: $("#routeInfoBox").outerWidth(),
            height: $("#routeInfoBox").outerHeight(),
        })) {
            return event;
        }
    }
    var pixiScale = pixiBackend.container.stage.scale.x;
    var position = {
        x: event.pageX,
        y: event.pageY
    };
    var positionDiff = {
        x: position.x - touchPosition.x,
        y: position.y - touchPosition.y
    };
    pixiBackend.moveStage(positionDiff.x, positionDiff.y);
    if (event.touches.length == 2) {
        var distance = Math.sqrt(Math.pow((event.touches[0].pageX - event.touches[1].pageX), 2) + Math.pow((event.touches[0].pageY - event.touches[1].pageY), 2));
        var previousDistance = Math.sqrt(Math.pow((touches[0].pageX - touches[1].pageX), 2) + Math.pow((touches[0].pageY - touches[1].pageY), 2));
        var factor = (distance - previousDistance) / distance;
        var zoomCenter = {
            x: (touches[0].pageX + touches[1].pageX) / 2,
            y: (touches[0].pageY + touches[1].pageY) / 2
        };
        // limit zooming out
        if ((pixiScale <= reticlesOnStage.threshold) && (factor < 0)) {
            // show lookup plot (reticle & fpga coordinates)
            reticlesOnStage.setReticles(true);
            pixiBackend.renderer.render();
            // end handleWheel
            return "reached zoom limit";
        }
        if (reticlesOnStage.enabled && (factor > 0)) {
            // hide lookup plot
            reticlesOnStage.setReticles(false);
            pixiBackend.renderer.render();
        }
        if (factor > 0) {
            // zoom stage
            pixiBackend.zoomIn(factor + 1, zoomCenter.x, zoomCenter.y);
            // auto mode
            if (automode.enabled) {
                // zoom into detail view
                if ((!detailview.enabled) && (pixiScale >= detailview.threshold) && (pixiScale < detailview.threshold2)) {
                    // determine hicann in view
                    var hicannIndex = void 0;
                    if (pixiBackend.container.numberHover.children[0]) {
                        // hicann number text
                        var child = pixiBackend.container.numberHover.children[0];
                        hicannIndex = parseInt(child.text);
                    }
                    else {
                        hicannIndex = detailview.hicannClosestToCenter(canvasCenter());
                    }
                    // display Hicann properties in right Infobox
                    hicannInfo.handleHicannClick(hicannIndex);
                    // start the detailview
                    automode.startDetailview(hicannIndex, true);
                }
                // zoom into detailview level two
                if (pixiScale >= detailview.threshold2) {
                    automode.startDetailviewLevelTwo();
                }
                // manual mode
            }
            else {
                if ((pixiScale >= detailview.threshold) && (!detailview.enabled)) {
                    manualmode.startDetailview();
                }
                if ((pixiScale >= detailview.threshold2) && (!detailview.levelTwoEnabled)) {
                    manualmode.startDetailviewLevelTwo();
                }
            }
            // route width adjustment
            routesOnStage.adjustRouteWidth(pixiScale);
        }
        else if (factor < 0) {
            // zoom stage
            pixiBackend.zoomOut(Math.abs(factor) + 1, zoomCenter.x, zoomCenter.y);
            // auto mode
            if (automode.enabled) {
                // zoom out of detailview level two
                if ((pixiScale < detailview.threshold2) && (pixiScale > detailview.threshold)) {
                    automode.startDetailview(detailview.currentHicann, false);
                }
                ;
                // zoom out of detail view
                if ((pixiScale < detailview.threshold) && (detailview.enabled)) {
                    automode.startOverview();
                }
                ;
                // manual mode
            }
            else {
                if ((pixiScale < detailview.threshold) && (detailview.enabled)) {
                    manualmode.leaveDetailview();
                }
                if ((pixiScale < detailview.threshold2) && (detailview.levelTwoEnabled)) {
                    manualmode.leaveDetailviewLevelTwo();
                }
            }
            // route width adjustment
            routesOnStage.adjustRouteWidth(pixiScale);
        }
        touches = event.touches;
    }
    pixiBackend.renderer.render();
    touchPosition = position;
}
/**
 * event handler for mobile touchstart event
 */
function handleTouchstart(event) {
    touchPosition = {
        x: event.pageX,
        y: event.pageY,
    };
    if (event.touches.length == 2) {
        touches = event.touches;
    }
}
/**
 * reset the whole visualization to the state right after starting
 */
function reset() {
    // reset zoom-scale and position
    centerWafer();
    // reset detail and overview
    if (automode.enabled) {
        automode.startOverview();
        automode.enabled = false;
    }
    for (var i = 0; i < wafer.enumMax; i++) {
        $("#hicanns_0_" + i + "_OV_checkbox").siblings("ul").children("li").children("input").each(function () {
            if (!$(this).prop("checked")) {
                $(this).click();
            }
        });
        $("#hicanns_0_" + i + "_DV_checkbox").siblings("ul").children("li").children("input").each(function () {
            if ($(this).prop("checked")) {
                $(this).click();
            }
        });
    }
    manualmode.enabled = false;
    // reset routes
    $("#allRoutes").click(); // highlight all routes
    if ($("#routes_0_check").prop("checked")) {
        $("#routes_0_check").click();
    }
    else {
        $("#routes_0_check").click();
        $("#routes_0_check").click();
    }
    ;
    routesOnStage.handleRouteWidthSlider(2);
    var scale = pixiBackend.container.stage.scale.x;
    routesOnStage.adjustRouteWidth(scale);
    $("#routeWidthSlider").slider("value", 2);
    // initialize visu to start in automode
    automode.init(undefined, false, false);
    // reset HICANN image state
    if (!$("#waferImageCheckbox").prop("checked")) {
        $("#waferImageCheckbox").click();
    }
    // reset HICANN number state
    if (!$("#numberTextCheckbox").prop("checked")) {
        $("#numberTextCheckbox").click();
    }
    // reset HICANN infos
    hicannInfo.hicanns = [];
    hicannInfo.handleHicannClick(0);
}
function scrollToRoute(routeID) {
    routeID--;
    $("#routes_0_" + routeID)[0].scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
    $("#routes_0_" + routeID).siblings("button")
        .css({
        "border": "2px solid #fbb545",
        "border-radius": "0.2rem"
    })
        .animate({ "border-color": "rgba(0, 0, 0, 0)" }, 4000, function () {
        $("#routes_0_" + routeID).siblings("button")
            .css({ "border-width": 0 });
    });
}
function scrollToHicann(hicannIndex) {
    $("#hicanns_0_" + hicannIndex).siblings("button")[0].scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
    $("#hicanns_0_" + hicannIndex).siblings("button")
        .css({
        "border": "2px solid #fbb545",
        "border-radius": "0.2rem"
    })
        .animate({ "border-color": "rgba(0, 0, 0, 0)" }, 4000, function () {
        $("#hicanns_0_" + hicannIndex).siblings("button")
            .css({ "border-width": 0 });
    });
}
