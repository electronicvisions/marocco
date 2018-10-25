/**
 * A backend to draw graphics on a HTML 5 canvas using the library PixiJS. 
 * The backend includes functions to draw primitive shapes either as graphics objects or as sprites with a specified resolution.
 * Functions to store multiple primitive shapes of one type as one PixiJS graphics object are also included.
 * 
 */
namespace pixiBackend {
    /**
     * The PixiJS containers are objects to hold graphics elements that are rendered on the canvas.
     * They can be nested to build something like a folderstructure. The substructure is accessible via the "childrens" of a container.
     */
    export const container = {
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
      setup: function() {
        this.stage.addChild(this.hicannImages);
        this.stage.addChild(this.backgrounds);
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
    }
    /**
     * A renderer instance is needed to render graphics on the canvas
     */
    export class Renderer {
      constructor (domParent: JQuery<HTMLElement>, backgroundColor: number, canvasWidth: number, canvasHeight: number, forceCanvas = false, resolution=1) {
        this.backgroundColor = backgroundColor;
        if (!forceCanvas) {
          this.renderer = PIXI.autoDetectRenderer({
            width: canvasWidth,
            height: canvasHeight,
            backgroundColor: backgroundColor,
            autoResize: true,
            resolution: resolution
          });
        } else {
          this.renderer = new PIXI.CanvasRenderer({
            width: canvasWidth,
            height: canvasHeight,
            backgroundColor: backgroundColor,
            autoResize: true,
            resolution: resolution
          })
        }
        this.renderer.view.style.position = 'absolute';
        this.renderer.view.style.display = 'block';
        this.renderer.view.id = "pixiJSCanvas";
        domParent.append(this.renderer.view);
      }
      /**
      * The renderer is by default the WebGL renderer but uses the canvas renderer as a fallback option,
      * if the WebGL renderer is not supported by the browser.
      */
      renderer: PIXI.WebGLRenderer | PIXI.CanvasRenderer
      /**
       * Background color for the canvas
       */
      backgroundColor: number
      /**
       * Render the stage (including all the substructure) on the canvas.
       * Call this method every time changes are made to the graphics elements that should be displayed.
       */
      render() {
        this.renderer.render(pixiBackend.container.stage);
      }
    }

    /**
     * Instance of Renderer class
     */
    export let renderer: Renderer;
    
    /**
     * Draw a rectangle as a graphics object.
     * @param container PixiJS container to hold the graphics object
     * @param x x-coordinate of the top left corner of the rectangle.
     * @param y y-coordinate of the top left corner of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color of the rectangle. Requires hex-color in the form "0xffffff".
     * @param alpha Transparency of the rectangle. Requires value between 0 and 1.
     * @param interactive Set to true to allow mouse interactivity with the object.
     * @param mouseoverFunction Any callback function for the mouseover event.
     * @param mouseoutFunction Any callback function for the mouseout event.
     * @param clickFunction Any callback function for the mouseclick event.
     */
    export function drawRectangle(container: PIXI.Container,
        x: number, y: number, width: number, height: number, color: string, alpha = 1,
        interactive=false, mouseoverFunction=undefined, mouseoutFunction=undefined, clickFunction=undefined) {
      const rectangle: any = new PIXI.Graphics();
      rectangle.beginFill(color);
      rectangle.drawRect(x, y, width, height);
      rectangle.endFill();
      rectangle.alpha = alpha;
      container.addChild(rectangle);
  
      if (interactive) {
        rectangle.interactive = true;
        rectangle.hitArea = new PIXI.Rectangle(x, y, width, height);
        rectangle.mouseover = mouseoverFunction;
        rectangle.mouseout = mouseoutFunction;
        rectangle.click = clickFunction;
      };
    }

    /**
     * Draw a circle as graphics object.
     * @param container PixiJS container to hold the graphics object.
     * @param x x-coordinate of the top left corner.
     * @param y y-coordinate of the top left corner.
     * @param radius Radius of the circle.
     * @param color Fill color for the circle. Requires hex-color in the form "0xffffff".
     */
    export function drawCircle(container: PIXI.Container, x: number, y: number, radius: number, color) {
      const circle = new PIXI.Graphics();
      circle.beginFill(color);
      circle.drawCircle(x, y, radius);
      circle.endFill();
      container.addChild(circle);
    }

    /**
     * Draw a triangle as graphics object. The triangles base is on the bottom
     * @param container PixiJS container to hold the graphics object.
     * @param x x-coordinate of the top left corner of the triangle.
     * @param y y-coordinate of the top left corner of the triangle.
     * @param width Width of the triangle.
     * @param height Height of the triangle.
     * @param color Fill color of the triangle. Requires hex-color in the form "0xffffff".
     */
    export function drawTriangle(container: PIXI.Container, x: number, y: number, width: number, height: number, color) {
      const path = [x, y, x+width, y, x+width/2, y-height, x, y];
      const triangle = new PIXI.Graphics();
      triangle.beginFill(color);
      triangle.drawPolygon(path);
      triangle.endFill();
      container.addChild(triangle);
    }
    /**
     * Calculates the graphics data for triangles from the specified input parameters.
     * @param cornerOne X and Y coordinates of the first corners.
     * @param cornerTwo X and Y coordinates of the second corners.
     * @param cornerThree X and Y coordinates of the third corners.
     * @param color fill color.
     * @param triangles Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    export function drawTriangles(
        cornerOne: tools.Point[], cornerTwo: tools.Point[], cornerThree: tools.Point[], color: number | string, triangles = new PIXI.Graphics()) {
      if ((cornerOne.length !== cornerTwo.length) || (cornerOne.length !== cornerThree.length)) {
        throw(new Error("Lengths of cornerOne, cornerTwo, and cornerThree values are not matching"));
      }
      for (let i=0; i<cornerOne.length; i++) {
        const path = [
          cornerOne[i].x, cornerOne[i].y,
          cornerTwo[i].x, cornerTwo[i].y,
          cornerThree[i].x, cornerThree[i].y,
          cornerOne[i].x, cornerOne[i].y
        ];
        triangles.beginFill(<any>color);
        triangles.drawPolygon(path);
        triangles.endFill();
      }
      return triangles;
    }

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
    export function drawRectangles(
        x: number[], y: number[], width: number[], height: number[], color: number | string, rectangles = new PIXI.Graphics(), alpha=1, lineWidth=0, lineColor=<number | string>0) {
      if ((x.length !== y.length) || (x.length !== width.length) || (x.length !== height.length)) {
        throw(new Error("Lengths of xValues, yValues, widthValues and heightValues do not match"));
      }
      rectangles.lineStyle(lineWidth, <any>lineColor);
      for (let i=0; i<x.length; i++) {
        rectangles.beginFill(<any>color, alpha);
        rectangles.drawRect(x[i], y[i], width[i], height[i]);
        rectangles.endFill();
      }
      rectangles.lineStyle(0, 0)
      return rectangles;
    }

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
    export function drawStraightLines(
        xOne: number[], yOne: number[], xTwo: number[], yTwo: number[], width: number[], color: number | string, lines = new PIXI.Graphics()
      ) {
      if ((xOne.length !== yOne.length) || (xOne.length !== xTwo.length) || (xOne.length !== width.length)) {
        throw(new Error("Lengths of xOne, yOne, xTwo, yTwo and width values do not match"));
      }
      for (let i=0; i<xOne.length; i++) {
        lines.lineStyle(width[i], <any>color);
        lines.moveTo(xOne[i], yOne[i]);
        lines.lineTo(xTwo[i], yTwo[i]);
      }
      lines.lineStyle(0,0)
      return lines;
    }

    export function drawLines(
      x: number[], y: number[], width: number, color: number | string, lines = new PIXI.Graphics()
      ) {
      if ((x.length !== y.length)) {
        throw(new Error("Lengths of x and y do not match"));
      }
      if (x.length < 2) {
        return;
      }
      lines.lineStyle(width, <any>color)
      lines.moveTo(x[0], y[0])
      for (let i=1; i<x.length; i++) {
        lines.lineTo(x[i], y[i]);
      }
      lines.lineStyle(0, 0)
      return lines;
    }

    /**
     * Calculates the graphics data for circles from the specified input parameters
     * @param x The X coordinates of the centers of the circles.
     * @param y The Y coordinates of the centers of the circles.
     * @param radius The radii of the circles.
     * @param color The fill color
     * @param circles Optional graphics object.
     * If specified, the new graphics data will be added to that Graphics object and the new one returned.
     */
    export function drawCircles(
        x: number[], y: number[], radius: number[], color: number | string, circles = new PIXI.Graphics()) {
      if ((x.length !== y.length) || (x.length !== radius.length)) {
        throw(new Error("Lengths of x, y and radius Values do not match"));
      }
      for (let i=0; i<x.length; i++) {
        circles.beginFill(<any>color);
        circles.drawCircle(x[i], y[i], radius[i]);
        circles.endFill();
      }
      return circles;
    }

    /**
     * Store graphics data as graphics object.
     * @param graphicsObject graphics data to store.
     * @param container container to store the sprite in.
     */
    export function storeGraphics(graphicsObject: PIXI.Graphics, container: PIXI.Container) {
      container.addChild(graphicsObject);
    }

    /**
     * Store graphics data as a sprite. The graphics data is renderered as a sprite with fixed resolution.
     * @param graphicsObject graphics data to store.
     * @param container container to store the sprite in.
     * @param resolution resolution of the sprite.
     */
    export function storeSprite(graphicsObject: PIXI.Graphics, container: PIXI.Container, resolution=10) {
      const sprite = new PIXI.Sprite(graphicsObject.generateCanvasTexture(0, resolution));
      sprite.position = new PIXI.Point(graphicsObject.getBounds().x, graphicsObject.getBounds().y);
      container.addChild(sprite);
    }

    /**
     * Draws text in the boundaries of the rectangle. The text is sized to fit exactly in, either by width or by height.
     * @param container PixiJS container to hold the text object.
     * @param x x-coordinate of the top left corner of the text.
     * @param y y-coordinate of the top left corner of the text.
     * @param rectWidth Width of the rectangle, the text should fit in.
     * @param rectHeight Height of the rectangle, the text should fit in.
     * @param style style-object for the text.
     */
    export function drawTextInRectangle(container: PIXI.Container, x: number, y: number, rectWidth: number, rectHeight: number,
        textContent: string, style: object) {
      const text = new PIXI.Text(textContent, style);
      text.x = x;
      text.y = y;
      // set width & height
      const originalWidth = text.width;
      let originalHeight = text.height;
      text.width = rectWidth;
      text.height *= text.width/originalWidth;
      if (text.height > rectHeight) {
        originalHeight = text.height;
        text.height = rectHeight;
        text.width *= text.height/originalHeight;
      }
      container.addChild(text);
    }

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
    export function drawRectangleBorder(container: PIXI.Container,
        x: number, y: number, width: number, height: number, lineWidth: number, color, alpha: number) {
      const rectangle = new PIXI.Graphics();
      rectangle.lineStyle(lineWidth, color, alpha);
      rectangle.drawRect(x, y, width, height);
      container.addChild(rectangle);
    }

    /**
     * Draw an image from a local url.
     * @param container PixiJS container to hold the sprite.
     * @param url Image file-path.
     * @param x x-coordinate of the top left corner of the image.
     * @param y y-coordinate of the top left corner of the image.
     * @param width Width of the image.
     * @param height Height of the image.
     */
    export function drawImage(container: PIXI.Container, url: string,
        x: number, y: number, width: number, height: number, alpha=1) {
      const image = PIXI.Sprite.fromImage(url);
      image.position = new PIXI.Point(x, y);
      image.width = width;
      image.height = height;
      image.alpha = alpha;
      container.addChild(image);
    }

    /**
     * Remove a child of any type from a PixiJS container.
     * @param container PixiJS container to remove the child from.
     * @param childIndex Index of the child to be removed (starts at 0).
     */
    export function removeChild(container: PIXI.Container, childIndex: number) {
      if (childIndex !== -1) {
        container.removeChild(container.children[childIndex]);
      } else {
        throw(new Error(`Cannot delete child ${childIndex} in container ${container}.`))
      }
    }

    /**
     * Remove all children of any type from a PixiJS container.
     * @param container PixiJS container to remove children from.
     */
    export function removeAllChildren(container: PIXI.Container) {
      const numChildren = container.children.length;
      for (let i=0; i<numChildren; i++) {
        container.removeChild(container.children[0]);
      }
    }

    /**
     * Zoom the whole stage in.
     * @param factor zoom-factor.
     * @param x x-coordinate of the target position. Typically the x-coordinate of the mouse position.
     * @param y y-coordinate of the target position. Typically the y-coordinate of the mouse position.
     */
    export function zoomIn(factor: number, x: number, y: number) {
      let scale = this.container.stage.transform.scale;
      let position = this.container.stage.transform.position;
      const oldScale = scale.x;
      scale.x *= factor;
      scale.y *= factor;
      position.x -= (x - position.x) * Math.abs(scale.x/oldScale -1);
      position.y -= (y - position.y) * Math.abs(scale.x/oldScale -1);
    }

    /**
     * Zoom the whole stage out.
     * @param factor zoom-factor.
     * @param x x-coordinate of the target position. Typically the x-coordinate of the mouse position.
     * @param y y-coordinate of the target position. Typically the y-coordinate of the mouse position.
     */
    export function zoomOut(factor: number, x: number, y: number) {
      let scale = this.container.stage.transform.scale;
      let position = this.container.stage.transform.position;
      const oldScale = scale.x;
      scale.x /= factor;
      scale.y /= factor;
      position.x += (x - position.x) * Math.abs(scale.x/oldScale -1);
      position.y += (y - position.y) * Math.abs(scale.x/oldScale -1);
    }

    /**
     * Move the whole stage (panning).
     * @param deltaX Shift in x-direction (positive or negative value).
     * @param deltaY Shift in y-direction (positive or negative value).
     */
    export function moveStage(deltaX: number, deltaY: number) {
      this.container.stage.position.x += deltaX;
      this.container.stage.position.y += deltaY;
    }

    /**
     * Animate panning of the whole stage.
     * @param x1 Start x-coordinate
     * @param y1 Start y-coordinate
     * @param x2 Target x-coordinate
     * @param y2 Target y-coordinate
     * @param duration Animation duration in milliseconds
     */
    export function animateStagePosition(x1: number, y1: number, x2: number, y2: number, duration: number) {
      const numberSteps = Math.floor(duration/20);
      const step = {
        x: (x2 - x1) / numberSteps,
        y: (y2 - y1) / numberSteps,
      }
      let stepTracker = 0;
      let timer = setInterval(function(){
        pixiBackend.moveStage(step.x, step.y);
        pixiBackend.renderer.render();
        stepTracker++;
        if (stepTracker === numberSteps) {
          clearInterval(timer)
        }
      }, 20)
    }

    /**
     * Checks if the mouse is within the boundaries of a rectangle.
     * @param mousePosition x-y-position of the mouse.
     * @param x x-coordinate of the rectangle.
     * @param y y-coordinate of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    export function mouseInRectangle(mousePosition: {x: number, y: number}, x: number, y: number, width: number, height: number) {
      const scale = this.container.stage.transform.scale.x;
      const stagePosition = this.container.stage.transform.position;
      if((mousePosition.x >= x*scale + stagePosition.x)
          && (mousePosition.x <= (x + width)*scale + stagePosition.x)
          && (mousePosition.y >= y*scale + stagePosition.y)
          && (mousePosition.y <= (y + height)*scale + stagePosition.y)) {
        return (true);
      };
    }
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
    export function mouseInLine(mouseX: number, mouseY: number, lineX1: number, lineY1: number, lineX2: number, lineY2: number, width: number) {
      const scale = this.container.stage.transform.scale.x;
      const stagePosition = this.container.stage.transform.position;
      lineX1 = lineX1*scale + stagePosition.x;
      lineY1 = lineY1*scale + stagePosition.y;
      lineX2 = lineX2*scale + stagePosition.x;
      lineY2 = lineY2*scale + stagePosition.y;
      width *= scale;
      return tools.pointInLine(mouseX, mouseY, lineX1, lineY1, lineX2, lineY2, width);
    }

    /**
     * Checks if a rectangle is at least partially within in the boundaries of the window.
     * @param x x-coordinate of the rectangle.
     * @param y y-coordinate of the rectangle.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    export function rectanglePartiallyInWindow(x: number, y: number, width: number, height: number) { //x, y, width, height are the rectangle parameters
      const view = {
        width: $(window).width(),
        height: $(window).height(),
      };
      const scale = this.container.stage.transform.scale.x;
      const stagePosition = this.container.stage.transform.position;
      x *= scale;
      y *= scale;
      width *= scale;
      height *= scale;
      if((x + stagePosition.x < view.width)
          && (x + width + stagePosition.x > 0)
          && (y + stagePosition.y < view.height)
          && (y + height + stagePosition.y> 0)) {
        return true;
      } else { return false };
    }
  }
