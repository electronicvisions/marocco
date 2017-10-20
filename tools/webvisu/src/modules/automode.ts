/// <reference path="overview.ts" />
/// <reference path="detailview.ts" />

/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
	/**
	 * The AutoMode controls automatically what details of which HICANN are displayed,
	 * depending on the zoom level as well as the part of the wafer that is currently within in the canvas boundaries.
	 */
	export class Automode{
		constructor(overview: internalModule.Overview, detailview: internalModule.Detailview) {
			this.enabled = undefined;
			this.overview = overview;
			this.detailview = detailview;
			this.wafer = overview.wafer;
			this.options = () => {
				const object = {};
				for (const prop in this.options) {
					object[prop] = this.options[prop]();
				}
				return object;
			}
			this.options.synapses = () => { return $("#autoSynapsesCheckbox").prop("checked")},
			this.options.synDrivers = () => { return $("#autoSynDriversCheckbox").prop("checked")},
			this.options.neurons = () => { return $("#autoNeuronsCheckbox").prop("checked")},
			this.options.leftBuses = () => { return $("#autoLeftCheckbox").prop("checked")},
			this.options.rightBuses = () => { return $("#autoRightCheckbox").prop("checked")},
			this.options.horizontalBuses = () => { return $("#autoHorizontalCheckbox").prop("checked")},
			this.options.repeaters = () => { return $("#autoRepeatersCheckbox").prop("checked")},
			this.options.synGrids = () => { return $("#autoSynGridCheckbox").prop("checked")},
			
			this.detailedHicanns = [];
		}
		/**
		 * Set this property when entering or leaving the auto mode.
		 */
		enabled: boolean;
		overview: internalModule.Overview;
		detailview: internalModule.Detailview;
		wafer: internalModule.Wafer;
		options: any;
		/**
		 * contains the indices of all HICANNs that are displayed in detail.
		 */
		detailedHicanns: number[];

		/**
		 * Initialization of the auto mode. Call this method when entering auto mode.
		 * @param hicannIndex The index/coordinate of the HICANN, whose details should be shown.
		 * @param levelOneEnabled Set to true if the automode should start in detailview.
		 * @param levelTwoEnabled Set to true if the auomode should start in detailviewLevelTwo.
		 */
		init(hicannIndex: number, levelOneEnabled: boolean, levelTwoEnabled: boolean) {
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
			};
			if (levelTwoEnabled) {
				this.startDetailviewLevelTwo();
			};
			// render stage
			pixiBackend.renderer.render();
		}

		/**
		 * Potentially leave a detailview and start the overview.
		 * @param hicannIndex Index of the HICANN, whose detailview is left.
		 */
		startOverview() {
			// reset detailview
			this.detailview.resetDetailview();
			// set parameters in detailview
			this.detailview.enabled = false;
			this.detailview.levelTwoEnabled = false;
			this.detailview.currentHicann = undefined;
			// display overview
			for (const hicannIndex of this.detailedHicanns) {
				this.setOverview(hicannIndex, true);
			}
			// display HICANN numbers
			hicannNumber.recover();
			// hide level one details
			this.setDetailview(false);
			// hide level two details
			this.setDetailviewLevelTwo(false);
		}

		/**
		 * Start the detailview for a specified HICANN.
		 * The Detailview can be entered coming either form the overview or the detailviewLevelTwo.
		 */
		startDetailview(hicannIndex: number, drawElements: boolean) {
			// check if coming from detailview level two
			if (drawElements) {
				this.getDetailedHicanns(hicannIndex);
				// draw detail objects i.e.
				//   synapse array level one and level two
				//	 buses level two
				for (const hicannIndex of this.detailedHicanns) {
					this.detailview.drawHicann(hicannIndex, this.options());
				};
			}
			// hide overview containers
			for (const hicannIndex of this.detailedHicanns) {
				this.setOverview(hicannIndex, false);
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
		}

		/**
		 * Start the detailview level two (graphics objects instead of sprites).
		 * Call this function only if currently in detailview.
		 */
		startDetailviewLevelTwo() {
			// set parameter in detailview
			this.detailview.levelTwoEnabled = true;
			// hide the sprites from detailview level one
			this.setDetailview(false);
			// display graphicsobject details from detailview level two
			this.setDetailviewLevelTwo(true);
		}

		/**
		 * Switch to detailview of the western HICANN.
		 */
		startWesternHicann(hicannIndex: number) {
			let levelTwoEnabled = this.detailview.levelTwoEnabled;
			// end detailview of old hicann
			this.startOverview();
			// start detailview of new hicann
			this.startDetailview(this.detailview.westernHicann, true);
			// if level two was enabled before, start level two on new hicann
			if (levelTwoEnabled) {
				this.startDetailviewLevelTwo();
			};
		}

		/**
		 * Switch to detailview of the eastern HICANN.
		 */
		startEasternHicann(hicannIndex: number) {
			let levelTwoEnabled = this.detailview.levelTwoEnabled;
			// end detailview of old hicann
			this.startOverview();
			// start detailview of new hicann
			this.startDetailview(this.detailview.easternHicann, true);
			// if level two was enabled before, start level two on new hicann
			if (levelTwoEnabled) {
				this.startDetailviewLevelTwo();
			};
		}

		/**
		 * Switch to detailview of the northern HICANN.
		 */
		startNorthernHicann(hicannIndex: number) {
			let levelTwoEnabled = this.detailview.levelTwoEnabled;
			// end detailview of old hicann
			this.startOverview();
			this.startDetailview(this.detailview.northernHicann, true);
			// if level two was enabled before, start level two on new hicann
			if (levelTwoEnabled) {
				this.startDetailviewLevelTwo();
				// start detailview of new hicann
			};
		}

		/**
		 * Switch to detailview of the sourthern HICANN.
		 */
		startSouthernHicann(hicannIndex: number) {
			let levelTwoEnabled = this.detailview.levelTwoEnabled;
			// end detailview of old hicann
			this.startOverview();
			// start detailview of new hicann
			this.startDetailview(this.detailview.southernHicann, true);
			// if level two was enabled before, start level two on new hicann
			if (levelTwoEnabled) {
				this.startDetailviewLevelTwo();
			};
		}

		/**
		 * Set visible properties for the overview elements of a HICANN.
		 * @param hicannIndex HICANN to be set.
		 * @param enabled pass true for visible and false for hidden.
		 */
		setOverview(hicannIndex: number, enabled: boolean) {
			pixiBackend.container.backgrounds.children[hicannIndex].visible = enabled;
			pixiBackend.container.inputs.children[hicannIndex].visible = enabled;
			pixiBackend.container.overviewBusesLeft.children[hicannIndex].visible = enabled;
			pixiBackend.container.overviewBusesRight.children[hicannIndex].visible = enabled;
			pixiBackend.container.overviewBusesHorizontal.children[hicannIndex].visible = enabled;
		}

		/**
		 * Set visible properties for all detailview elements.
		 * @param hicannIndex HICANN to be set.
		 * @param enabled pass true for visible and false for hidden.
		 */
		setDetailview(enabled: boolean) {
			pixiBackend.container.synapsesSprite.visible = this.options.synapses() ? enabled : false;
			pixiBackend.container.neuronsSprite.visible = this.options.neurons() ? enabled : false;
			pixiBackend.container.repeaterBusConnectionsSprite.visible = this.options.repeaters() ? enabled : false;
			pixiBackend.container.busesLeftSprite.visible = this.options.leftBuses() ? enabled : false;
			pixiBackend.container.busesRightSprite.visible = this.options.rightBuses() ? enabled : false;
			pixiBackend.container.busesHorizontalSprite.visible = this.options.horizontalBuses() ? enabled : false;
			pixiBackend.container.synGridSprite.visible = this.options.synGrids() ? enabled : false;
		}

		/**
		 * Set visible properties for all detailviewLevelTwo elements.
		 * @param enabled pass true for visible and false for hidden.
		 */
		setDetailviewLevelTwo(enabled: boolean) {
			pixiBackend.container.synapses.visible = this.options.synapses() ? enabled : false;
			pixiBackend.container.neurons.visible = this.options.neurons() ? enabled : false;
			pixiBackend.container.repeaterBusConnections.visible = this.options.repeaters() ? enabled : false;
			pixiBackend.container.busesLeft.visible = this.options.leftBuses() ? enabled : false;
			pixiBackend.container.busesRight.visible = this.options.rightBuses() ? enabled : false;
			pixiBackend.container.busesHorizontal.visible = this.options.horizontalBuses() ? enabled : false;
			pixiBackend.container.synGrid.visible = this.options.synGrids() ? enabled : false;
		}

		/**
		 * Find the eigth surrounding HICANNs of a HICANN (if existing).
		 */
		getDetailedHicanns(hicannIndex: number) {
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
		}
	}
}