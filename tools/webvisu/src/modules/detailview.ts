/// <reference path="tools.ts" />
/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  /**
   * The Detailview includes detailed representations of HICANN elements.
   * Buses are drawn as thin rectangles and the synapse arrays as arrays of small squares.
   * The detailview is divided into two sub-levels. The first one (detailview) drawing elements as Sprites,
   * the second one (detialviewLevelTwo) drawing real graphics elements.
   */
  export class Detailview {
    constructor(wafer: internalModule.Wafer) {
      this.wafer = wafer;
      this.determineThreshold($(window).height());
    }
    wafer: internalModule.Wafer;
    
    /**
     * set this property when detailview is entered or left
     */
    enabled = false;
    /**
     * set this property when detailviewLevelTwo is entered or left.
     */
    levelTwoEnabled = false;
    /**
     * Index of the HICANN that is currently in detailview. Used for auto mode.
     */
    currentHicann = undefined;
    /**
     * Index of northern HICANN of the one currently in detailview. Used for auto mode.
     */
    northernHicann = undefined;
    /**
     * Index of southern HICANN of the one currently in detailview. Used for auto mode.
     */
    southernHicann = undefined;
    /**
     * Index of eastern HICANN of the one currently in detailview. Used for auto mode.
     */
    easternHicann = undefined;
    /**
     * Index of western HICANN of the one currently in detailview. Used for auto mode.
     */
    westernHicann = undefined;
    /**
     * zoom-scale threshold to start detailview
     */
    threshold = NaN;
    /**
     * zoom-scale threshold to start detailviewLevelTwo
     */
    threshold2 = NaN;

    /**
     * Hardcoded number of neurons on a HICANN.
     */
    numNeurons = 256;
    /**
     * Hardcoded number of synapse rows in a synapse array on a HICANN.
     */
    numSynapsesVertical = 224;
    /**
     * Number of synapse drivers on one HICANN.
     */
    numSynapseDrivers = 224;
    /**
     * Hardcoded number of vertical buses on a HICANN.
     */
    numBusesVertical = 128;
    /**
     * Hardcoded number of horizontal buses on a HICANN.
     */
    numBusesHorizontal = 64;
    /**
     * Unit distances between synapse array and Buses.
     */
    gap = 4;
    /**
     * Number of repeaters in the left and right block together.
     */
    numRepeatersHorizontal = 64;
    /**
     * Number of repeaters in the top and bottom blocks together.
     */
    numRepeatersVertical = 256;
    
    /**
     * Distance between vertical repeaters (top and bottom)
     */
    get vRepeaterDistance() {
      return this.wafer.repeaterBlockPosition.vertical.width.current * 2 / this.numRepeatersVertical; 
    }
    /**
     * Distance between horizontal repeaters (left and right)
     */
    get hRepeaterDistance() {
      return this.wafer.repeaterBlockPosition.horizontal.height.current / (this.numRepeatersHorizontal);
    }
    /**
     * Distance between vertical buses (left and right)
     */
    get vBusDistance() {
      return this.wafer.busesLeftPosition.width/this.numBusesVertical;
    }
    /**
     * Distance between horizontal buses
     */
    get hBusDistance() {
      return this.wafer.busesHorizontalPosition.current.height/this.numBusesHorizontal;
    }
    /**
     * Width of a vertical bus
     */
    get vBusWidth() {
      return this.vBusDistance/2;
    }
    /**
     * Width of a horizontal bus
     */
    get hBusWidth() {
      return this.vBusDistance/2;
    }
    /**
     * Margin around HICANN. used to calculate when to start the detailview.
     */
    get edge() {
      return(this.wafer.hicannWidth/4)
    }

    /**
     * Calculate the position of the center of a HICANN
     */
    hicannCenter(hicannIndex: number) {
      const transform: any = pixiBackend.container.stage.transform;
      const scale = transform.scale.x;
      const stagePosition = transform.position;
      const hicannCenter = {
        x: (this.wafer.hicanns[hicannIndex].x * (this.wafer.hicannWidth + this.wafer.hicannMargin) + this.wafer.hicannWidth/2)
            * scale + stagePosition.x,
        y: (this.wafer.hicanns[hicannIndex].y * (this.wafer.hicannHeight + this.wafer.hicannMargin) + this.wafer.hicannHeight/2)
            * scale + stagePosition.y,
      };
      return(hicannCenter);
    }
  
    /**
     * Find the HICANN that is closest to the center of the canvas.
     */
    hicannClosestToCenter(canvasCenter: {x: number, y: number}) {
      let minDistance = Infinity;
      let closestHicann = <number>undefined;
      for (let hicannIndex=this.wafer.enumMin; hicannIndex<=this.wafer.enumMax; hicannIndex++) {
        const hicannCenter = this.hicannCenter(hicannIndex);
        const distance = tools.distanceBetweenPoints(hicannCenter, canvasCenter);
        if (distance<minDistance) {
          minDistance = distance;
          closestHicann = hicannIndex;
        };
      }
      return(closestHicann);
    }

    /**
     * Find indices of neighboring HICANNS and update class properties.
     */
    updateSurroundingHicanns() {
      this.northernHicann = this.wafer.northernHicann(this.currentHicann);
      this.southernHicann = this.wafer.southernHicann(this.currentHicann);
      this.easternHicann = this.wafer.easternHicann(this.currentHicann);
      this.westernHicann = this.wafer.westernHicann(this.currentHicann);
    }

    /**
     * Draw all detailed elements of a HICANN
     * @param options Information about which parts of the HICANNs to draw in detail according to checkboxes.
     */
    drawHicann(newHicann: number, options: {synapses: boolean, synDrivers: boolean, neurons: boolean,
        leftBuses: boolean, rightBuses: boolean, horizontalBuses: boolean, repeaters: boolean, synGrids: boolean}) {
      const hicannPosition = this.wafer.hicanns[newHicann].position;
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
    }

    /**
     * Determine the zoom-scale where the detailview should begin.
     * The HICANN is fully in taking up almost the whole window at that point.
     */
    determineThreshold(canvasHeight: number) {
      const fullHicannScale = canvasHeight / (this.wafer.hicannHeight + 2 * this.edge);
      this.threshold = fullHicannScale;
      this.threshold2 = fullHicannScale * 8;
    }

    drawNeurons(hicannPosition: {x: number, y: number}) {
      const positions = {
        xOne: <number[]>[],
        yOne: <number[]>[],
        xTwo: <number[]>[],
        yTwo: <number[]>[],
        width: <number[]>[],
      }
      const segmentWidth = this.wafer.neuronArrayPosition.width / this.numNeurons;
      for (let i=0; i<this.numNeurons; i++) {
        // top
        for (const basePosition of [this.wafer.neuronArrayPosition.top, this.wafer.neuronArrayPosition.bottom]) {
          positions.xOne.push(basePosition.x + hicannPosition.x + (i + 0.5) * segmentWidth);
          positions.yOne.push(basePosition.y + hicannPosition.y);
          positions.xTwo.push(basePosition.x + hicannPosition.x + (i + 0.5) * segmentWidth);
          positions.yTwo.push(basePosition.y + hicannPosition.y + this.wafer.neuronArrayPosition.height);
          positions.width.push(segmentWidth/2);
        }
      }
      const neuronColor = 0x33aabb;
      const graphicsObject = pixiBackend.drawStraightLines(
        positions.xOne, positions.yOne, positions.xTwo, positions.yTwo, positions.width, neuronColor
      );
      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.neuronsSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.neurons);
    }

    /**
     * draw both synapse arrays of a HICANN, both as sprites and graphics objects.
     * @param hicannPosition top left corner of the HICANN.
     */
    drawAllSynapses(hicannPosition: {x: number, y: number}) {
      const positions = {
        x: <number[]>[],
        y: <number[]>[],
        width: <number[]>[],
        height: <number[]>[]
      }

      for (let x=0; x<this.numNeurons; x++) {
        for (let y=0; y<2*this.numSynapsesVertical; y++) {
          const position = this.calcSynapse(hicannPosition, x, y);
          positions.x.push(position[0]);
          positions.y.push(position[1]);
          positions.width.push(position[2]);
          positions.height.push(position[3]);
        }
      }

      const graphicsObject = pixiBackend.drawRectangles(positions.x, positions.y, positions.width, positions.height, 0xfbcb3f);
      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.synapsesSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synapses);
    }

    drawSynGrid(hicannPosition: {x: number, y: number}) {
      const positions = {
        xOne: <number[]>[],
        yOne: <number[]>[],
        xTwo: <number[]>[],
        yTwo: <number[]>[],
        width: <number[]>[]
      }

      const segmentHeight = this.wafer.synapseArraysTopPosition.height / this.numSynapsesVertical;
      const segmentWidth = this.wafer.synapseArraysTopPosition.width / this.numNeurons;

      for (let i=0; i<=this.numSynapsesVertical; i++) {
        //const synDriverLeft = (Math.floor(i/2))%2;
        const synDriverLeft = false;
          positions.xOne.push(hicannPosition.x + (synDriverLeft ? this.wafer.synDriverPosition.topLeft.x : this.wafer.synapseArraysTopPosition.x));
          positions.yOne.push(hicannPosition.y + this.wafer.synapseArraysTopPosition.y + i * segmentHeight);
          positions.xTwo.push(hicannPosition.x + this.wafer.synDriverPosition.topRight.x + (synDriverLeft ? 0 : this.wafer.synDriverPosition.width));
          positions.yTwo.push(hicannPosition.y + this.wafer.synapseArraysTopPosition.y + i * segmentHeight);

          positions.xOne.push(hicannPosition.x + (!synDriverLeft ? this.wafer.synDriverPosition.bottomLeft.x : this.wafer.synapseArraysBottomPosition.x));
          positions.yOne.push(hicannPosition.y + this.wafer.synapseArraysBottomPosition.y + i * segmentHeight);
          positions.xTwo.push(hicannPosition.x + this.wafer.synDriverPosition.bottomRight.x + (!synDriverLeft ? 0 : this.wafer.synDriverPosition.width));
          positions.yTwo.push(hicannPosition.y + this.wafer.synapseArraysBottomPosition.y + i * segmentHeight);
      }

      for (let i=0; i<=this.numNeurons; i++) {
        for (const basePosition of [this.wafer.synapseArraysTopPosition, this.wafer.synapseArraysBottomPosition]) {
          positions.xOne.push(hicannPosition.x + basePosition.x + i * segmentWidth);
          positions.yOne.push(hicannPosition.y + basePosition.y);
          positions.xTwo.push(hicannPosition.x + basePosition.x + i * segmentWidth);
          positions.yTwo.push(hicannPosition.y + basePosition.y + basePosition.height);
        }
      }

      for (let _=0; _<positions.xOne.length; _++) {
        positions.width.push(this.vBusWidth);
      }

      let graphicsObject = pixiBackend.drawStraightLines(
          positions.xOne, positions.yOne, positions.xTwo, positions.yTwo, positions.width, 0xffffff)

      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.synGridSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synGrid);
    }

    /**
     * draw synapse all the synapse drivers of a HICANN, as graphics objects.
     * @param hicannPosition top left corner of the HICANN.
     */
    drawSynDrivers(hicannPosition: tools.Point) {
      let cornerOne = <tools.Point[]>[];
      let cornerTwo = <tools.Point[]>[];
      let cornerThree = <tools.Point[]>[];

      const synDriverColor = 0xffffff;
      
      for (let i=0; i<this.numSynapseDrivers; i++) {
        const synDriverPosition = this.calcSynDriver(hicannPosition, i);
        cornerOne = cornerOne.concat(synDriverPosition[0]);
        cornerTwo = cornerTwo.concat(synDriverPosition[1]);
        cornerThree = cornerThree.concat(synDriverPosition[2]);
      }

      // Graphics Objects
      let graphicsObject = pixiBackend.drawTriangles(
          cornerOne, cornerTwo, cornerThree, synDriverColor);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.synDrivers);
    }

    calcSynDriver(hicannPosition: tools.Point, index: number) {
      const top = index<(this.numSynapseDrivers/2);
      const left = top ? (index%2 === 1) : (index%2 === 0);

      index = index%(this.numSynapseDrivers/2);

      let cornerOne = <tools.Point>undefined;
      let cornerTwo = <tools.Point>undefined;
      let cornerThree = <tools.Point>undefined;

      const segmentHeight = this.wafer.synDriverPosition.height / (this.numSynapsesVertical/2)
      
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
          })
        } else {
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
      } else {
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
        } else {
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
    }
    
    calcSynapse(hicannPosition: tools.Point, xIndex: number, yIndex: number) {
      const top = yIndex < this.numSynapsesVertical;
      yIndex = top ? yIndex : (yIndex - this.numSynapsesVertical);

      let x = <number>undefined;
      let y = <number>undefined;

      const segmentWidth = (top ? this.wafer.synapseArraysTopPosition.width : this.wafer.synapseArraysBottomPosition.width) / this.numNeurons;
      const segmentHeight = (top ? this.wafer.synapseArraysTopPosition.height : this.wafer.synapseArraysBottomPosition.height) / this.numSynapsesVertical;

      const baseX = hicannPosition.x + (top ? this.wafer.synapseArraysTopPosition.x : this.wafer.synapseArraysBottomPosition.x);
      const baseY = hicannPosition.y + (top ? this.wafer.synapseArraysTopPosition.y : this.wafer.synapseArraysBottomPosition.y);

      x = baseX + xIndex * segmentWidth;
      y = baseY + yIndex * segmentHeight;

      return [x, y, segmentWidth, segmentHeight, this.vBusWidth];
    }

    /**
     * Draw all vertical left routes of a HICANN as graphics objects and sprite.
     * @param hicannPosition Top left corner of the HICANN.
     */
    drawBusesLeft(hicannPosition: tools.Point) {
      let graphicsObject = new PIXI.Graphics;;

      for (let i=0; i<this.numBusesVertical; i++) {
        const positions = this.calcBusLeft(hicannPosition, i);
        graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.vBusWidth, 0xffffff, graphicsObject);
      };

      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesLeftSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesLeft);
    }

    /**
     * Calculate the x and y values of the two points for all the lines, that make up a vertical left bus.
     * This includes the connecting lines between repeaters and buses.
     * @param hicannPosition Top left corner of the HICANN.
     * @param index Index of the vertical line.
     */
    calcBusLeft(hicannPosition: tools.Point, index: number) {
      const x = <number[]>[];
      const y = <number[]>[];

      const repeaterBaseX = hicannPosition.x + this.wafer.repeaterBlockPosition.vertical.top.left.current.x + (index+0.5)*this.vRepeaterDistance;
      const busBaseX = hicannPosition.x + this.wafer.busesLeftPosition.x + index*this.vBusDistance;
      const busShiftTop = this.vRepeaterDistance * (index<(this.numBusesVertical-2) ? 1 : -(this.numBusesVertical-2)/2);
      const busShiftBottom = this.vRepeaterDistance * (index>1 ? -1 : (this.numBusesVertical-2)/2);

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
    }

    /**
     * Draw all vertical right routes of a HICANN as graphics objects and sprite.
     * @param hicannPosition Top left corner of the HICANN.
     */
    drawBusesRight(hicannPosition: tools.Point) {
      let graphicsObject = new PIXI.Graphics;

      for (let i=0; i<this.numBusesVertical; i++) {
        const positions = this.calcBusRight(hicannPosition, i);
        graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.vBusWidth, 0xffffff, graphicsObject);
      };
      
      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesRightSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesRight);
    }
    
    /**
     * Calculate the x and y values of the two points for all the lines, that make up a vertical right bus.
     * This includes the connecting lines between repeaters and buses.
     * @param hicannPosition Top left corner of the HICANN.
     * @param index Index of the vertical line. Caveat: the indices start at 0 again for the right bus block.
     */
    calcBusRight(hicannPosition: tools.Point, index: number) {
      const x = <number[]>[];
      const y = <number[]>[];
      
      const repeaterBaseX = hicannPosition.x + this.wafer.repeaterBlockPosition.vertical.top.right.current.x + (index+0.5)*this.vRepeaterDistance;
      const busBaseX = hicannPosition.x + this.wafer.busesRightPosition.x + index*this.vBusDistance;
      const busShiftTop = this.vRepeaterDistance * (index>(1) ? -1 : (this.numBusesVertical-2)/2);
      const busShiftBottom = this.vRepeaterDistance * (index<(this.numBusesVertical-2) ? 1 : -(this.numBusesVertical-2)/2);
      
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
    }

    /**
     * Draw all horizontal buses of a HICANN as graphics objects and sprite.
     * @param hicannPosition Top left corner of the HICANN.
     */
    drawBusesHorizontal(hicannPosition: tools.Point) {
      let graphicsObject = new PIXI.Graphics;
      
      for (let i=0; i<this.numBusesHorizontal; i++) {
        const positions = this.calcBusHorizontal(hicannPosition, i);
        graphicsObject = pixiBackend.drawLines(positions[0], positions[1], this.hBusWidth, 0xffffff, graphicsObject);
      };

      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesHorizontalSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesHorizontal);

      this.drawHVBusSwitches(hicannPosition);
    }

    /**
     * Calculate the x and y values of the two points for all the lines, that make up a horizontal bus.
     * This includes the connecting lines between repeaters and buses.
     * @param hicannPosition Top left corner of the HICANN.
     * @param index Index of the horizontal line.
     */
    calcBusHorizontal(hicannPosition: tools.Point, index: number) {
      const x = <number[]>[];
      const y = <number[]>[];

      const repeaterLevelBaseY = hicannPosition.y + this.wafer.repeaterBlockPosition.horizontal.left.current.y + (index+0.5)*this.hRepeaterDistance;
      const busBaseY = hicannPosition.y + this.wafer.busesHorizontalPosition.current.y + index*this.hBusDistance;
      
      // left repeater bus connections
      x.push(hicannPosition.x);
      y.push(repeaterLevelBaseY + this.hRepeaterDistance * (index>1 ? -1 : (this.numRepeatersHorizontal-2)/2));
      
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
      y.push(repeaterLevelBaseY + this.hRepeaterDistance * (index<(this.numRepeatersHorizontal-2) ? 1 : -(this.numRepeatersHorizontal-2)/2));
      
      return [x, y];
    }

    /**
     * Draw all repeaters of a HICANN as graphics objects.
     * @param hicannPosition Top left corner of the HICANN.
     */
    drawRepeaters(hicannPosition: tools.Point) {
      const horizontal = {
        leftTriangle: {
          left: <tools.Point[]>[],
          top: <tools.Point[]>[],
          bottom: <tools.Point[]>[]
        },
        middleBar: {
          x: <number[]>[],
          y: <number[]>[],
          width: <number[]>[],
          height: <number[]>[]
        },
        rightTriangle: {
          right: <tools.Point[]>[],
          top: <tools.Point[]>[],
          bottom: <tools.Point[]>[]
        }
      }
      const vertical = {
        topTriangle: {
          top: <tools.Point[]>[],
          left: <tools.Point[]>[],
          right: <tools.Point[]>[]
        },
        middleBar: {
          x: <number[]>[],
          y: <number[]>[],
          width: <number[]>[],
          height: <number[]>[]
        },
        bottomTriangle: {
          bottom: <tools.Point[]>[],
          left: <tools.Point[]>[],
          right: <tools.Point[]>[]
        }
      }
      // vertical
      const vSegmentWidth = wafer.repeaterBlockPosition.vertical.width.current * 2 / (this.numRepeatersVertical);
      for (let i=0; i<this.numRepeatersVertical; i++) {
        const top = (i<this.numRepeatersVertical/2) ? (i%2 === 1) : (i%2 === 0);
        const left = (i<this.numRepeatersVertical/2);
        const baseX = hicannPosition.x + (top ?
            (left ? wafer.repeaterBlockPosition.vertical.top.left.current.x : wafer.repeaterBlockPosition.vertical.top.right.current.x) : 
            (left ? wafer.repeaterBlockPosition.vertical.bottom.left.current.x : wafer.repeaterBlockPosition.vertical.bottom.right.current.x)) +
            (i % (this.numRepeatersVertical/2)) * vSegmentWidth;
        const baseY = hicannPosition.y + (top ?
          (left ? wafer.repeaterBlockPosition.vertical.top.left.current.y : wafer.repeaterBlockPosition.vertical.top.right.current.y) : 
          (left ? wafer.repeaterBlockPosition.vertical.bottom.left.current.y : wafer.repeaterBlockPosition.vertical.bottom.right.current.y))
        // top triangle
        vertical.topTriangle.top.push({
            x: baseX + vSegmentWidth/2,
            y: baseY});
        vertical.topTriangle.left.push({
            x: baseX, 
            y: baseY + wafer.repeaterBlockPosition.vertical.height/3});
        vertical.topTriangle.right.push({
            x: baseX + vSegmentWidth,
            y: baseY + wafer.repeaterBlockPosition.vertical.height/3});
        // middle bar, connecting the triangles
        vertical.middleBar.x.push(baseX + vSegmentWidth/4);
        vertical.middleBar.y.push(baseY + wafer.repeaterBlockPosition.vertical.height/3);
        vertical.middleBar.width.push(vSegmentWidth/2);
        vertical.middleBar.height.push(wafer.repeaterBlockPosition.vertical.height/3);
        // bottom triangle
        vertical.bottomTriangle.bottom.push({
            x: baseX + vSegmentWidth/2,
            y: baseY + this.wafer.repeaterBlockPosition.vertical.height})
        vertical.bottomTriangle.left.push({
            x: baseX,
            y: baseY + 2/3 * this.wafer.repeaterBlockPosition.vertical.height})
        vertical.bottomTriangle.right.push({
            x: baseX + vSegmentWidth,
            y: baseY + 2/3 * this.wafer.repeaterBlockPosition.vertical.height})
      }
      // horizontal
      const hSegmentHeight = this.wafer.repeaterBlockPosition.horizontal.height.current / (this.numRepeatersHorizontal);
      for (let i=0; i<this.numRepeatersHorizontal; i++) {
        const left = (i%2 === 0);
        const baseX = hicannPosition.x + (left ? this.wafer.repeaterBlockPosition.horizontal.left.current.x : this.wafer.repeaterBlockPosition.horizontal.right.current.x);
        const baseY = hicannPosition.y + wafer.repeaterBlockPosition.horizontal.left.current.y + i*hSegmentHeight;
        // left triangle
        horizontal.leftTriangle.top.push({
            x: baseX + wafer.repeaterBlockPosition.horizontal.width/3,
            y: baseY});
        horizontal.leftTriangle.left.push({
            x: baseX, 
            y: baseY + hSegmentHeight/2});
        horizontal.leftTriangle.bottom.push({
            x: baseX + wafer.repeaterBlockPosition.horizontal.width/3,
            y: baseY + hSegmentHeight});
        // middle bar, connecting the triangles
        horizontal.middleBar.x.push(baseX + wafer.repeaterBlockPosition.horizontal.width/3);
        horizontal.middleBar.y.push(baseY + hSegmentHeight/4);
        horizontal.middleBar.width.push(wafer.repeaterBlockPosition.horizontal.width/3);
        horizontal.middleBar.height.push(hSegmentHeight/2);
        // bottom triangle
        horizontal.rightTriangle.top.push({
            x: baseX + wafer.repeaterBlockPosition.horizontal.width*2/3,
            y: baseY})
        horizontal.rightTriangle.right.push({
            x: baseX + wafer.repeaterBlockPosition.horizontal.width,
            y: baseY + hSegmentHeight/2})
        horizontal.rightTriangle.bottom.push({
            x: baseX + wafer.repeaterBlockPosition.horizontal.width*2/3,
            y: baseY + hSegmentHeight})
      }

      // draw top triangle
      let repeaterGraphics = pixiBackend.drawTriangles(
          vertical.topTriangle.left, vertical.topTriangle.right, vertical.topTriangle.top, 0xffffff);
      // add the middle bar
      repeaterGraphics = pixiBackend.drawRectangles(
          vertical.middleBar.x, vertical.middleBar.y, vertical.middleBar.width,
          vertical.middleBar.height, 0xffffff, repeaterGraphics);
      // add the bottom triangle
      repeaterGraphics = pixiBackend.drawTriangles(
          vertical.bottomTriangle.left, vertical.bottomTriangle.right,
          vertical.bottomTriangle.bottom, 0xffffff, repeaterGraphics);

      // add the left triangle
      repeaterGraphics = pixiBackend.drawTriangles(
          horizontal.leftTriangle.top, horizontal.leftTriangle.left,
          horizontal.leftTriangle.bottom, 0xffffff, repeaterGraphics);
      // add the middle bar
      repeaterGraphics = pixiBackend.drawRectangles(
          horizontal.middleBar.x, horizontal.middleBar.y, horizontal.middleBar.width,
          horizontal.middleBar.height, 0xffffff, repeaterGraphics);
      // add the right triangle
      repeaterGraphics = pixiBackend.drawTriangles(
          horizontal.rightTriangle.top, horizontal.rightTriangle.right,
          horizontal.rightTriangle.bottom, 0xffffff, repeaterGraphics);
      // store the graphics element in the container
      pixiBackend.storeGraphics(repeaterGraphics, pixiBackend.container.repeaters);
    }

    drawHVBusSwitches(hicannPosition: tools.Point) {
      let x = <number[]>[];
      let y = <number[]>[];
      const radius = <number[]>[];
      const color = 0x000000;

      for (let i=0; i<this.numBusesVertical*2; i++) {
        x = x.concat(this.calcHVBusSwitch(hicannPosition, i)[0]);
        y = y.concat(this.calcHVBusSwitch(hicannPosition, i)[1]);
      }
      for (let _=0; _<x.length; _++) {
        radius.push(this.vBusWidth);
      }

      let graphicsObject = pixiBackend.drawCircles(x, y, radius, color);
      pixiBackend.storeSprite(graphicsObject, pixiBackend.container.busesHorizontalSprite);
      pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.busesHorizontal);
    }

    calcHVBusSwitch(hicannPosition: tools.Point, vBusCoord: number) {
      let xCoords = <number[]>[];
      let yCoords = <number[]>[];

      const left = vBusCoord<128;

      const xCoord = (vBusCoord) => {
        return hicannPosition.x
            + (left ? this.wafer.busesLeftPosition.x : this.wafer.busesRightPosition.x)
            + (vBusCoord%128) * this.vBusDistance;
      };
      const yCoord = (hBusCoord) => {
        return hicannPosition.y
            + this.wafer.repeaterBlockPosition.horizontal.left.current.y
            + (hBusCoord+0.5)*this.hRepeaterDistance;
      };

      xCoords.push(xCoord(vBusCoord));
      xCoords.push(xCoord(vBusCoord));
      if (left) {
        yCoords.push(yCoord(63 - 2*(vBusCoord%32)));
        yCoords.push(yCoord(63 - 2*(vBusCoord%32) - 1));
      } else {
        yCoords.push(yCoord(2*(vBusCoord%32)));
        yCoords.push(yCoord(2*(vBusCoord%32) + 1));
      }

      return [xCoords, yCoords];
    }

    /**
     * remove all the detailed elements
     */
    resetDetailview() {
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
    }

    /**
     * Check if the northern HICANN is closer to the canvas center than the one currently in detailview.
     * Needed for auto mode.
     */
    northernHicannCloser(canvasCenter: {x: number, y: number}) {
      if (this.northernHicann) {
        const distanceCurrentHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
        const distanceNorthernHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.northernHicann), canvasCenter);
        if (distanceNorthernHicann + 2*this.edge < distanceCurrentHicann) {
          return true;
        } else {
          return false;
        };
      };
    }

    /**
     * Check if the southern HICANN is closer to the canvas center than the one currently in detailview.
     * Needed for auto mode.
     */
    southernHicannCloser(canvasCenter: {x: number, y: number}) {
      if (this.southernHicann) {
        const distanceCurrentHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
        const distanceSouthernHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.southernHicann), canvasCenter);
        if (distanceSouthernHicann + 2*this.edge < distanceCurrentHicann) {
          return true;
        } else {
          return false;
        };
      };
    }

    /**
     * Check if the eastern HICANN is closer to the canvas center than the one currently in detailview.
     * Needed for auto mode.
     */
    easternHicannCloser(canvasCenter: {x: number, y: number}) {
      if (this.easternHicann) {
        const distanceCurrentHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
        const distanceEasternHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.easternHicann), canvasCenter);
        if (distanceEasternHicann + 4*this.edge < distanceCurrentHicann) {
          return true;
        } else {
          return false;
        };
      };
    }

    /**
     * Check if the western HICANN is closer to the canvas center than the one currently in detailview.
     * Needed for auto mode.
     */
    westernHicannCloser(canvasCenter: {x: number, y: number}) {
      if (this.westernHicann) {
        const distanceCurrentHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.currentHicann), canvasCenter);
        const distanceWesternHicann =
            tools.distanceBetweenPoints(this.hicannCenter(this.westernHicann), canvasCenter);
        if (distanceWesternHicann + 4*this.edge < distanceCurrentHicann) {
          return true;
        } else {
          return false;
        };
      };
    }
  }
}