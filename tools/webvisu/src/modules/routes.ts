/// <reference path="routes_json.d.ts" />
/// <reference path="detailview.ts" />

/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
	function loadRouteData(wafer: internalModule.Wafer) {
		const l1Properties = wafer.marocco.l1_properties();
		const routes = <Route[]>[];

		const types = [
			"hicann", "merger0", "merger1", "merger2", "merger3", "gbitLink", "DNCMerger", "repeaterBlock", "HLine", "VLine", "synapseDriver", "synapse"
		]

		for (let routeIndex=0; routeIndex<l1Properties.size(); routeIndex++) {
			const route = l1Properties.get(routeIndex).route();
			const projections = l1Properties.get(routeIndex).projection_ids();

			const routeElements = [];
			const projectionIDs = <number[]>[];
			let hicann = <number>undefined;
			let index = <number>undefined;
			for (let segmentIndex=0; segmentIndex<route.size(); segmentIndex++) {
				// match for last number in string which is always the Enum value
				index = parseInt(route.get(segmentIndex).to_string().match(/(\d+)(?!.*\d)/)[0]);
				const type = route.get(segmentIndex).which()
				if (type === 0) {
					hicann = index;
				} else {
					routeElements.push(new L1RouteSegment(hicann, index, <any>types[type]))
				}
			}
			for (let i=0; i<projections.size(); i++) {
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
	class L1RouteSegment {
		constructor(hicann: number, index: number,
				type: "merger0" | "merger1" | "merger2" | "merger3" | "gbitLink" | "DNCMerger" | "repeaterBlock" | "HLine" | "VLine" | "synapseDriver" | "synapse",
				position?: {
					x?: number[], y?: number[], width?: number, height?: number, radius?: number,
					cornerOne?: tools.Point, cornerTwo?: tools.Point, cornerThree?: tools.Point,
					lineWidth?: number
				}) {
			this.hicann = hicann;
			this.index = index;
			this.type = type;
			this.position = position;
		}
		/**
		 * Index/coordinate of the HICANN the route-element belongs to.
		 */
		hicann: number;
		/**
		 * Index of the bus, the route uses.
		 */
		index: number;
		/**
		 * vertical or horizontal bus.
		 */
		type: "merger0" | "merger1" | "merger2" | "merger3" | "gbitLink" | "DNCMerger" | "repeaterBlock" | "HLine" | "VLine" | "synapseDriver" | "synapse";
		/**
		 * optional information about drawing position on stage
		 */
		position: {
			x?: number[], y?: number[], width?: number, height?: number, radius?: number,
			cornerOne?: tools.Point, cornerTwo?: tools.Point, cornerThree?: tools.Point,
			lineWidth?: number
		}
	}

	/**
	 * A complete route with source and target HICANN.
	 */
	class Route {
		constructor(routeElements: L1RouteSegment[], projectionIDs: number[], ID: number) {
			this.routeSegments = routeElements;
			this.projectionIDs = projectionIDs;
			this.switchPositions = <{x: number, y: number, radius: number}[]>[];
			this.sourceHicann = this.routeSegments[0].hicann;
			this.targetHicann = this.routeSegments[this.routeSegments.length - 1].hicann;
			this.ID = ID;
			this.color = tools.randomHexColor();
			this.greyedOut = false;
		}
		/**
		 * List of the individual bus segments the route is using.
		 */
		routeSegments: L1RouteSegment[];
		/**
		 * List of the routes projection IDs.
		 */
		projectionIDs: number[];
		/**
		 * List of the positions on the stage of the switches between vertical and horizontal bus segments
		 */
		switchPositions: {x: number, y: number, radius: number}[];
		/**
		 * Index/coordinate of the source HICANN.
		 */
		sourceHicann: number;
		/**
		 * Index/coordinate of the target HICANN.
		 */
		targetHicann: number;
		/**
		 * An individual ID for just this route.
		 */
		ID: number;
		/**
		 * Indicates whether the route will be rendered or not.
		 */
		visible = true;
		/**
		 * Color of the route. Requires a hex-color of the form 0xffffff.
		 */
		color: string;
		/**
		 * Indicates whether the route is currently greyed-out or not.
		 */
		greyedOut: boolean;
	}

	/**
	 * Control the route information in the UI route-info box.
	 */
	export class RouteInfo {
		constructor() {
			this.details = false;

			$("#routeInfo").click(() => {
				if ($("#routeInfoBox").css("display") === "none") {
					$("#routeInfoBox").css("display", "initial");
					$("#routeInfo").addClass("infoBoxSelected");
				} else {
					$("#routeInfoBox").css("display", "none");
					$("#routeInfo").removeClass("infoBoxSelected");
				}
			})
			
		}

		/**
		 * Indicates whether route details are shown or not.
		 */
		details: boolean;
		
		/**
		 * Display information about a list of routes.
		 */
		displayRouteInfo(routes: Route[], routeButtonClickHandler: (routes: Route[]) => void, that) {
			//remove old info
			this.reset();

			if (routes.length !== 1) {

				// display numbers of all selected routes
				const routeNumberParent = $("#routeNumber");
				routeNumberParent.html("Routes: ");
				for (const route of routes) {
					const routeNumber = $("<button></button>").text(`${route.ID + 1}`);
					routeNumber.click(() => {routeButtonClickHandler.call(that, [route])})
					routeNumber.css("color", `#${route.color.slice(2)}`)
					routeNumberParent.append(routeNumber);
				}

			} else {

				// display info about selected route
				const IDs = <number[]>[];
				for (const ID of routes[0].projectionIDs) {
					IDs.push(ID);
				}
				const IDString = IDs.sort((a, b) => {return a-b}).toString().split(",").join(", ");
				$("#routeInfoBox").append(`<p id="projectionIDs" class="routeInfoItem">
						Projection IDs: <span>${IDString}</span></p>`);
				$("#routeNumber").html(`Route ${routes[0].ID + 1}`)
				const sourceHicann = $(`<p id="sourceHicann" class="routeInfoItem">
						Source HICANN: <button>${routes[0].sourceHicann}</button></p>`)
				.click(() => {
					$(`#hicanns_0_${routes[0].sourceHicann}`).siblings("button").click()
				})
				const targetHicann = $(`<p id="targetHicann" class="routeInfoItem">
						Target HICANN: <button>${routes[0].targetHicann}</button></p>`)
				.click(() => {
					$(`#hicanns_0_${routes[0].targetHicann}`).siblings("button").click()
				})
				$("#routeInfoBox").append(sourceHicann);
				$("#routeInfoBox").append(targetHicann);
				
				// expand list to show all route segments
				const routeDetails = $(`<button id="routeDetails">details</button>`)
				routeDetails.click(function() {
					if (this.details) {
						this.removeDetailsList();
						this.details = false;
					} else {
						this.buildRouteDetailsList(routes[0]);
						this.details = true;
					}
				}.bind(this))
				$("#targetHicann").after(routeDetails);

				if (this.details) {
					this.buildRouteDetailsList(routes[0]);
				}
			}
		}

		/**
		 * Build an HTML list of route segments.
		 */
		buildRouteDetailsList(route: Route) {
			let html = "";

			// open containing div
			html += "<div id='routeElementsBox'>"
			
			// build route elements list
			for (const element of route.routeSegments) {
				html += `<p class='routeElementItem'>HICANN <span>${element.hicann}</span>: ${element.type} <span>${element.index}</span></p>`
			}

			// close containing div
			html += "</div>";

			// append
			$("#sourceHicann").after(html)
		}

		/**
		 * Remove the HTML list of route segments
		 */
		removeDetailsList() {
			$("#routeElementsBox").remove();
			$(".routeElementItem").remove();
		}

		/**
		 * reset the UI route-info box.
		 */
		reset() {
			$("#routeNumber").html("Route Info");
			$("#routeDetails").remove();
			$(".routeInfoItem").remove();
			this.removeDetailsList();
		}
	}

	/**
	 * Controls all the routes that are visualized. All routes are stored as a new instance of the class Route.
	 * When a route is selected, an additional route is drawn on top of all the other routes, so selected route are not hidden by other routes.
	 * When the class is constructed with a routes array, the number of positions (first index) have to match the number of routes.
	 */
	export class RoutesOnStage {
		constructor(detailview: Detailview, routes?: Route[]) {
			this.detailview = detailview;
			this.wafer = detailview.wafer;
			// determine current zoomlevel
			const transform: any = pixiBackend.container.stage.transform;
			this.zoomLevels.current = this.currentZoomLevel(transform.scale.x);
			this.routes = loadRouteData(this.wafer);
			this.selectedRoutes = [];
		}

		wafer: Wafer;
		detailview: Detailview;
		/**
		 * All the routes that are to be visualized.
		 */
		routes: Route[];
		/**
		 * Store the routes that have been selected either by clicking in the visualization or in the routes list.
		 */
		selectedRoutes: Route[];
		/**
		 * Different zoom-levels for route width adjustment.
		 */
		zoomLevels = {
			level0: {
				scale: 0.2, // 0.2*detailview.threshold
				originalWidth: 100, // corresponds to width-slider value 2 (out of 5)
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

		/**
		 * total number of routes in the visualization.
		 */
		get numRoutes() {
			return this.routes.length;
		};

		/**
		 * Number of segments of a route.
		 * @param routeID 
		 */
		routeLength(routeID: number) {
			return this.routes[routeID].routeSegments.length;
		};

		/**
		 * Calculate all route positions and draw all routes
		 */
		drawRoutes() {
			// draw already stored routes, recalculate positions
			for (const route of this.routes) {
				this.calcRoutePosition(route);
				this.drawRoute(route)
			}

			// ... and the selected routes
			for (const route of this.selectedRoutes) {
				this.calcRoutePosition(route);
				this.drawRoute(route, true)
			}
		};

		/**
		 * draw a single route
		 * @param selected Set to true, to store the route in the PixiJS container for selected routes.
		 */
		drawRoute(route: Route, selected = false) {
			let graphicsObject = new PIXI.Graphics();
			let graphicsObjectSynapse = new PIXI.Graphics();
			for (const routeSegment of route.routeSegments) {
				switch (routeSegment.type) {
					case "VLine":
					case "HLine":
						graphicsObject = pixiBackend.drawLines(
								routeSegment.position.x, routeSegment.position.y, routeSegment.position.width,
								route.greyedOut ? 0x8c8c8c : route.color, graphicsObject);
						break;
					case "synapseDriver":
						graphicsObject = pixiBackend.drawTriangles(
								[routeSegment.position.cornerOne], [routeSegment.position.cornerTwo], [routeSegment.position.cornerThree],
								route.greyedOut ? 0x8c8c8c : route.color, graphicsObject
						);
						break;
					case "synapse":
						// TODO:	store synaptic weight in route and use this information here
						// 				Will need to divide the weight by the maximum weight of all synapses to get value between 0 and 1 as required for alpha
						// 				better calculate maximum weight before and store in routesOnStage
						const weight = 0.5;
						// TODO: store synaptic input (excitatory || inhibitory) in route and use this information here
						const excitatory = 0;

						graphicsObjectSynapse = pixiBackend.drawRectangles(
							routeSegment.position.x, routeSegment.position.y, [routeSegment.position.width], [routeSegment.position.height],
							route.greyedOut ? 0x8c8c8c : (excitatory ? 0x00ff00 : 0xff0000),
							graphicsObjectSynapse,
							weight, this.detailview.vBusWidth, route.greyedOut ? 0x8c8c8c : route.color
						);
						break;
					default:
						break;
				}
			}
			
			// switches
			const switchX = [];
			const switchY = [];
			const switchRadius = [];
			for (const switchPosition of route.switchPositions) {
				switchX.push(switchPosition.x);
				switchY.push(switchPosition.y);
				switchRadius.push(switchPosition.radius);
			}
			let graphicsObjectSwitches = pixiBackend.drawCircles(switchX, switchY, switchRadius, route.greyedOut ? 0x8c8c8c : 0xffffff);
			
			// container to store routes in
			let routesContainer: PIXI.Container;
			let switchesContainer: PIXI.Container;
			if (selected) {
				routesContainer = pixiBackend.container.selectedRoutes;
				switchesContainer = pixiBackend.container.selectedSwitches;
			} else {
				routesContainer = pixiBackend.container.routes;
				switchesContainer = pixiBackend.container.switches;
			};

			// store as graphics objects
			pixiBackend.storeGraphics(graphicsObject, routesContainer);
			pixiBackend.storeSprite(graphicsObjectSynapse, routesContainer, 16)
			pixiBackend.storeGraphics(graphicsObjectSwitches, switchesContainer);
		}

		calcRoutePosition(route: Route) {
			let previousType = "undefined";
			route.switchPositions = [];
			for (const i in route.routeSegments) {
				const routeSegment = route.routeSegments[i]
				switch (routeSegment.type) {
					case "VLine":
						routeSegment.position = this.calcSegmentVertical(routeSegment);
						if (previousType === "HLine") {
							route.switchPositions.push(this.calcSwitches(routeSegment, route.routeSegments[parseInt(i)-1]));
						}
						break;
					case "HLine":
						routeSegment.position = this.calcSegmentHorizontal(routeSegment);
						if (previousType === "VLine") {
							route.switchPositions.push(this.calcSwitches(routeSegment, route.routeSegments[parseInt(i)-1]));
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

			return route
		}

		calcSwitches(segment1: L1RouteSegment, segment2: L1RouteSegment) {
			let position1: tools.Line;
			let position2: tools.Line;
			let seg1Index = 3;
			let seg2Index = 3;
			
			if (segment1.type === "VLine") {
				seg2Index = (segment1.index < 128) ? 1 : 5;
			} else {
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

			const intersectionPoint = tools.intersectionPoint(position1, position2);
			const routeWidth = this.detailview.vBusWidth * this.zoomLevels.current.width;

			return {x: intersectionPoint.x, y: intersectionPoint.y, radius: routeWidth/2};
		}

		/**
		 * Calculate line that represents a vertical route segment.
		 */
		calcSegmentVerticalOld(segment: L1RouteSegment) {
			const hicannPosition = {
				x: this.wafer.hicanns[segment.hicann].position.x,
				y: this.wafer.hicanns[segment.hicann].position.y,
			}

			const routePositions = (segment.index < 128) ?
					this.detailview.calcBusLeft(hicannPosition, segment.index) :
					this.detailview.calcBusRight(hicannPosition, segment.index - 128);

			const width = this.detailview.vBusWidth * this.zoomLevels.current.width;

			return { x1: routePositions[0], y1: routePositions[1], x2: routePositions[2], y2: routePositions[3], width: width};
		};

		calcSegmentVertical(segment: L1RouteSegment) {
			const hicannPosition = {
				x: this.wafer.hicanns[segment.hicann].position.x,
				y: this.wafer.hicanns[segment.hicann].position.y,
			}

			const routePositions = (segment.index < 128) ?
					this.detailview.calcBusLeft(hicannPosition, segment.index) :
					this.detailview.calcBusRight(hicannPosition, segment.index - 128);

			const width = this.detailview.vBusWidth * this.zoomLevels.current.width;

			return { x: routePositions[0], y: routePositions[1], width: width };
		}

		/**
		 * Calculate line that represents a horizontal route segment.
		 */
		calcSegmentHorizontal(segment: L1RouteSegment) {
			const hicannPosition = {
				x: this.wafer.hicanns[segment.hicann].position.x,
				y: this.wafer.hicanns[segment.hicann].position.y,
			}

			const routePositions = this.detailview.calcBusHorizontal(hicannPosition, segment.index);

			const width = this.detailview.hBusWidth * this.zoomLevels.current.width;

			return { x: routePositions[0], y: routePositions[1], width: width};
		};

		/**
		 * Calculate triangle that represents a synapse driver.
		 */
		calcSegmentSynDriver(segment: L1RouteSegment) {
			const hicannPosition = {
				x: this.wafer.hicanns[segment.hicann].position.x,
				y: this.wafer.hicanns[segment.hicann].position.y,
			}

			const synDriverPosition = this.detailview.calcSynDriver(hicannPosition, segment.index);

			return {cornerOne: synDriverPosition[0], cornerTwo: synDriverPosition[1], cornerThree: synDriverPosition[2]}
		}

		/**
		 * Calculate rectangle that represents a synapse.
		 */
		calcSegmentSynapse(segment: L1RouteSegment) {
			const hicannPosition = {
				x: this.wafer.hicanns[segment.hicann].position.x,
				y: this.wafer.hicanns[segment.hicann].position.y
			}

			// TODO: convert enum coordinate into x, y coordinates
			//			replace with marocco function
			const [xIndex, yIndex] = this.convertSynapseEnumToXY(segment.index);

			const synapsePosition = this.detailview.calcSynapse(hicannPosition, xIndex, yIndex);

			return {x: [synapsePosition[0]], y: [synapsePosition[1]], width: synapsePosition[2], height: synapsePosition[3], lineWidth: synapsePosition[4]};
		}

		/**
		 * TODO: replace with marocco function for that
		 */
		convertSynapseEnumToXY(synEnum: number) {
			return [synEnum%256, Math.floor(synEnum/256)];
		}

		/**
		 * Removes the graphics objects for all routes (and selected routes) from the PixiJS containers.
		 */
		removeRoutesFromContainer() {
			// remove all routes (and switch circles) from pixiJS container
			const numRoutes = pixiBackend.container.routes.children.length;
			for (let i=0; i<numRoutes; i++) {
				pixiBackend.removeChild(pixiBackend.container.routes, 0);
				pixiBackend.removeChild(pixiBackend.container.switches, 0);
			};

			// ... and from selected Route pixiJS container
			const numSelectedRoutes = pixiBackend.container.selectedRoutes.children.length;
			for (let i=0; i<numSelectedRoutes; i++) {
				pixiBackend.removeChild(pixiBackend.container.selectedRoutes, 0);
				pixiBackend.removeChild(pixiBackend.container.selectedSwitches, 0);
			}
		}

		/**
		 * Set a route in the visualization visible or hide it.
		 * @param visible Set to true to make the route visible.
		 */
		setRoute(route: Route, visible: boolean) {
			// set pixiJS route according to input
			pixiBackend.container.routes.children[route.ID*2].visible = visible;
			pixiBackend.container.routes.children[route.ID*2 + 1].visible = visible;

			// set pixiJS switch circle according to input
			pixiBackend.container.switches.children[route.ID].visible = visible;

			// set pixiJS route and switches for selected Route according to input
			if (this.selectedRoutes.indexOf(route) !== -1) {
				pixiBackend.container.selectedRoutes.children[this.selectedRoutes.indexOf(route)*2].visible = visible;
				pixiBackend.container.selectedRoutes.children[this.selectedRoutes.indexOf(route)*2 + 1].visible = visible;
				pixiBackend.container.selectedSwitches.children[this.selectedRoutes.indexOf(route)].visible = visible;
			};

			// update Route.visible property
			route.visible = visible;
		};

		/**
		 * Set all routes in the visualization visible or hide them according to their "visible" property.
		 */
		setAllRoutes() {
			for (const route of this.routes) {
				// set pixiJS route according to the Route.visible property
				pixiBackend.container.routes.children[route.ID*2].visible = route.visible;
				pixiBackend.container.routes.children[route.ID*2 + 1].visible = route.visible;

				// set pixiJS switch circles according to the Route.visible property
				pixiBackend.container.switches.children[route.ID].visible = route.visible;
			
				// set pixiJS route and switch for selected Route according to the Route.visible property
				const indexSelectedRoute = this.selectedRoutes.indexOf(route);
				if (indexSelectedRoute !== -1) {
					pixiBackend.container.selectedRoutes.children[indexSelectedRoute*2].visible = route.visible;
					pixiBackend.container.selectedRoutes.children[indexSelectedRoute*2 + 1].visible = route.visible;
					pixiBackend.container.selectedSwitches.children[indexSelectedRoute].visible = route.visible;
				};
			}
		};

		/**
		 * Calculate the current zoom-level (for route widths) from the current zoom-scale.
		 * @param pixiScale zoom-scale of the "stage" PixiJS container.
		 */
		currentZoomLevel(pixiScale: number) {
			// determine the zoomlevel from current zoom
			if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level4.scale) {
				return this.zoomLevels.level4;
			} else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level3.scale) {
				return this.zoomLevels.level3;
			} else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level2.scale) {
				return this.zoomLevels.level2;
			} else if ((pixiScale / this.detailview.threshold) >= this.zoomLevels.level1.scale) {
				return this.zoomLevels.level1;
			} else {
				return this.zoomLevels.level0;
			};
		};

		/**
		 * Adjust the widths of all routes if a new zoom-level is reached.
		 * @param pixiScale Zoom-scale of the "stage" PixiJS container.
		 */
		adjustRouteWidth(pixiScale: number) {
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

		/**
		 * If all routes are visible (UI checkboxes checked), the Checkbox for all routes is set to checked as well.
		 */
		checkAllRoutes() {
			let allElementsSelected = true;

			// go through all routes and check if visible
			for (const route of this.routes) {
				if (!route.visible) {
					allElementsSelected = false;
					break;
				};
			};

			// set Routes checkbox
			$("#routes_0_check").prop("checked", allElementsSelected);
		};

		/**
		 * set the checkbox for a route. 
		 */
		setCheckbox(route: Route, checked: boolean) {
			// set checkbox of route
			$(`#routes_0_${route.ID}`).prop("checked", checked);
		};

		/**
		 * Check if a route (or multiple routes) was clicked in the visualization
		 */
		handleVisuClick(mouseX: number, mouseY: number) {
			let selectedRoutes = <Route[]>[];

			// check what routes the mouse is over
			for (const route of this.routes) {
				if (this.mouseOverRoute(mouseX, mouseY, route)) {
					selectedRoutes.push(route);
				}
			}

			if (selectedRoutes.length !== 0) {
				this.handleRouteClick(selectedRoutes);
			}
		};

		/**
		 * Handle selected routes.
		 * - update route info box
		 * - highlight selected routes
		 * @param routes 
		 */
		handleRouteClick(routes: Route[]) {
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

		/**
		 * Check if a route (or multiple routes) was doubleclicked in the visualization
		 */
		handleVisuDoubleClick(mouseX: number, mouseY: number) {
			let clickedRoute = undefined;

			// check if mouse is over route
			for (const route of this.routes) {
				if (this.mouseOverRoute(mouseX, mouseY, route)) {
					clickedRoute = route;
					break;
				}
			}

			if (clickedRoute !== undefined) {
				this.handleRouteDoubleClick()
			}
		}

		/**
		 * Double clicking a route resets the routes.
		 * - reset the route info box
		 * - remove highlighting and draw all routes in color
		 */
		handleRouteDoubleClick() {
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
		}

		/**
		 * check is within the boundaries of the segments of a route.
		 */
		mouseOverRoute(mouseX: number, mouseY: number, route: Route) {
			// check if route is visible
			if (route.visible) {
				// check if mouse is over route
				for (const routeSegment of route.routeSegments) {
					switch (routeSegment.type) {
						case "VLine":
						case "HLine":
							for (let i=1; i<routeSegment.position.x.length; i++) {
								if (pixiBackend.mouseInLine(mouseX, mouseY, routeSegment.position.x[i-1], routeSegment.position.y[i-1], routeSegment.position.x[i], routeSegment.position.y[i], routeSegment.position.width)) {
									return true
								}
							}
							break;
						default:
							break;
					}
				}
				
			}
		};

		/**
		 * Set the greyout property for non-selected routes.
		 */
		highlightRoutes(selectedRoutes: Route[]) {
			// set greyedOut property
			for (const route of this.routes) {
				if ((<any>selectedRoutes).includes(route)) {
					route.greyedOut = false;
				} else {
					route.greyedOut = true;
				}
			}
		}

		/**
		 * set the greyed out property to false for all routes.
		 */
		unhighlightRoutes() {
			// set greyedOut property
			for (const route of this.routes) {
				route.greyedOut = false;
			}
		}

		/**
		 * handle the route-width slider in the UI route info box
		 * - remove old routes
		 * - draw routes in new width (store only positions)
		 */
		handleRouteWidthSlider(sliderValue: number) {
			// change the route widths for all zoom levels
			for (const zoomLevel in this.zoomLevels) {
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
		}
	}
}