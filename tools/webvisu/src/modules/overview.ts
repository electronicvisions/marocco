/// <reference path="tools.ts" />
/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
	/**
	 * The overview does not show the HICANN elements such as buses in detail but provides cumulative information about a HICANN.
	 * All bus segments (vertical left, vertical right, horizontal) are drawn as one rectangle each in a color that represents the number of routes running over that segment.
	 * The number of inputs on a HICANN are represented by a colored triangle at the bottom of the HICANN.
	 * The number of neurons on a HICANN are represented by a colored rectangle in the background.
	 */
	export class Overview {
		constructor(
			wafer: internalModule.Wafer,
			hicannColors: {
				numNeuronsColorOne: string,
				numNeuronsColorTwo: string,
				numInputsColorOne: string,
				numInputsColorTwo: string,
				numRoutesLeftColorOne: string,
				numRoutesLeftColorTwo: string,
				numRoutesRightColorOne: string,
				numRoutesRightColorTwo: string,
				numRoutesHorizontalColorOne: string,
				numRoutesHorizontalColorTwo: string,
			},
		) {
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
		wafer: internalModule.Wafer;
		/**
		 * Left color on the gradient for the number of neurons on a HICANN.
		 * Corresponds to the minimum number of neurons on any HICANN of the wafer.
		 */
		numNeuronsColorOne: string;
		/**
		 * Right color on the gradient for teh number of neurons on a HICANN.
		 * Corresponds to the maximum number of neurons on any HICANN of the wafer.
		 */
		numNeuronsColorTwo: string;
		/**
		 * Left color on the gradient for the number of inputs on a HICANN.
		 * Corresponds to the minimum number of inputs on any HICANN of the wafer.
		 */
		numInputsColorOne: string;
		/**
		 * Right color on the gradient for the number of inputs on a HICANN.
		 * Corresponds to the maximum number of inputs on any HICANN of the wafer.
		 */
		numInputsColorTwo: string;
		/**
		 * Left color on the gradient for the number of left routes on a HICANN.
		 * Corresponds to the minimum number of routes running over the left bus segment of any HICANN of the wafer.
		 */
		numRoutesLeftColorOne: string;
		/**
		 * Right color on the gradient for the number of left routes on a HICANN.
		 * Corresponds to the maximum number of routes running over the left bus segment of any HICANN of the wafer.
		 */
		numRoutesLeftColorTwo: string;
		/**
		 * Left color on the gradient for the number of right routes on a HICANN.
		 * Corresponds to the minimum number of routes running over the right bus segment of any HICANN of the wafer.
		 */
		numRoutesRightColorOne: string;
		/**
		 * Right color on the gradient for the number of right routes on a HICANN.
		 * Corresponds to the maximum number of routes running over the right bus segment of any HICANN of the wafer.
		 */
		numRoutesRightColorTwo: string;
		/**
		 * Left color on the gradient for the number of horizontal routes on a HICANN.
		 * Corresponds to the minimum number of routes running over the horizontal bus segment of any HICANN of the wafer.
		 */
		numRoutesHorizontalColorOne: string;
		/**
		 * Right color on the gradient for the number of horizontal routes on a HICANN.
		 * Corresponds to the maximum number of routes running over the horizontal bus segment of any HICANN of the wafer.
		 */
		numRoutesHorizontalColorTwo: string;

		/**
		 * Draw HICANN background inputs and vertical left, right and horizontal buses for all HICANNs.
		 */
		drawWafer() {
			// loop through hicanns on wafer
			for (let hicannIndex=this.wafer.enumMin; hicannIndex<=this.wafer.enumMax; hicannIndex++) {
				// calculate Hicann position in pixels
				let hicannX = this.wafer.hicanns[hicannIndex].x * ( this.wafer.hicannWidth + this.wafer.hicannMargin );
				let hicannY = this.wafer.hicanns[hicannIndex].y * ( this.wafer.hicannHeight + this.wafer.hicannMargin );
				this.wafer.hicanns[hicannIndex].position = {x: hicannX, y: hicannY};
		
				// draw rectangle as hicann representation, color scale for number of neurons
				this.drawHicannBackground(hicannIndex, hicannX, hicannY);

				// draw triangle in color scale for number of hicann inputs
				this.drawInputs(hicannIndex, hicannX, hicannY);
				
				// draw "H" ín color scale for number of route-segment
				this.drawBusH(hicannIndex, hicannX, hicannY);
			};
			// render stage
			pixiBackend.renderer.render();
		}

		/**
		 * Draw a rectangle in the HICANN background in a color representing the number of neurons on that HICANN.
		 */
		drawHicannBackground(hicannIndex: number, x: number, y: number) {
			// calculate color on number of neurons color gradient
			let colorNumNeurons = tools.colorInGradient( this.numNeuronsColorOne, this.numNeuronsColorTwo,
					this.wafer.numNeuronsMax, this.wafer.hicanns[hicannIndex].numNeurons );
			// draw rectangle as hicann representation
			pixiBackend.drawRectangle(pixiBackend.container.backgrounds, x, y, this.wafer.hicannWidth, this.wafer.hicannHeight, colorNumNeurons);
		}

		/**
		 * Draw a rectangle on the bottom of a HICANN in a color representing the number of inputs on that HICANN.
		 */
		drawInputs(hicannIndex: number, x: number, y: number) {
			// calculate color on number of inputs color gradient
			let colorNumInputs = tools.colorInGradient( this.numInputsColorOne, this.numInputsColorTwo,
					this.wafer.numInputsMax, this.wafer.hicanns[hicannIndex].numInputs );
			// draw triangle in color scale as number of hicann inputs representation
			pixiBackend.drawTriangle(pixiBackend.container.inputs,
					x + this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width,
					y + this.wafer.busesLeftPosition.y + this.wafer.busesLeftPosition.height,
					this.wafer.busesRightPosition.x - (this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width),
					this.wafer.inputTriangleHeight, colorNumInputs);
		}

		/**
		 * Draw vertical left, vertical right and horizontal buses of a HICANN
		 */
		drawBusH(hicannIndex: number, x: number, y: number) {
			// draw three segments of "H" seperately
			this.drawLeftBusSegment(hicannIndex, x, y);
			this.drawRightBusSegment(hicannIndex, x, y);
			this.drawHorizontalBusSegment(hicannIndex, x, y);
		}

		/**
		 * Draw all vertical left buses as one colored rectangle for a HICANN (as graphics object).
		 */
		drawLeftBusSegment(hicannIndex: number, x: number, y: number) {
			let colorNumBuses = tools.colorInGradient( this.numRoutesLeftColorOne, this.numRoutesLeftColorTwo,
					this.wafer.numBusesLeftMax, this.wafer.hicanns[hicannIndex].numBusesLeft);
			pixiBackend.drawRectangle(pixiBackend.container.overviewBusesLeft,
					x + this.wafer.busesLeftPosition.x, y + this.wafer.busesLeftPosition.y,
					this.wafer.busesLeftPosition.width, this.wafer.busesLeftPosition.height, colorNumBuses);
		}

		/**
		 * Draw all vertical right buses as one colored rectangle for a HICANN (as graphics object).
		 */
		drawRightBusSegment(hicannIndex: number, x: number, y: number) {
			let colorNumBuses = tools.colorInGradient( this.numRoutesRightColorOne, this.numRoutesRightColorTwo,
					this.wafer.numBusesRightMax, this.wafer.hicanns[hicannIndex].numBusesRight);
			pixiBackend.drawRectangle(pixiBackend.container.overviewBusesRight,
					x + this.wafer.busesRightPosition.x, y + this.wafer.busesRightPosition.y,
					this.wafer.busesRightPosition.width, this.wafer.busesRightPosition.height, colorNumBuses);
		}

		/**
		 * Draw all horizontal buses as one colored rectangle for a HICANN (as graphics object).
		 */
		drawHorizontalBusSegment(hicannIndex: number, x: number, y: number) {
			let colorNumBuses = tools.colorInGradient( this.numRoutesHorizontalColorOne, this.numRoutesHorizontalColorTwo,
					this.wafer.numBusesHorizontalMax, this.wafer.hicanns[hicannIndex].numBusesHorizontal);
			pixiBackend.drawRectangle(pixiBackend.container.overviewBusesHorizontal,
					x + this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width,
					y + this.wafer.busesHorizontalPosition.current.y,
					this.wafer.busesRightPosition.x - (this.wafer.busesLeftPosition.x + this.wafer.busesLeftPosition.width),
					this.wafer.busesHorizontalPosition.current.height, colorNumBuses);
		}

		resetOverview() {
			pixiBackend.removeAllChildren(pixiBackend.container.backgrounds);
			pixiBackend.removeAllChildren(pixiBackend.container.inputs);
			pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesLeft);
			pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesRight);
			pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesHorizontal);
			pixiBackend.removeAllChildren(pixiBackend.container.routes);
			pixiBackend.removeAllChildren(pixiBackend.container.switches);
			pixiBackend.removeAllChildren(pixiBackend.container.selectedRoutes);
			pixiBackend.removeAllChildren(pixiBackend.container.selectedSwitches);
		}
	}
}