/// <reference path="detailview.ts" />
/// <reference path="overview.ts" />

/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
	export type elementType = "numNeurons" | "numInputs" | "left" | "right" | "horizontal" |
			"detailLeft" | "detailRight" | "detailHorizontal" |
			"synDriver" | "neurons" | "repeaters" | "synGrid";

	/**
	 * The manual mode aims at giving the user full control over what details to show for which HICANN.
	 * Clicking checkboxes in the UI sets the visible property for the respective pixiJS containers.
	 * Switching between detailview and detailviewLevelTwo (sprites vs. graphics objects) is still done automatically.
	 */
	export class Manualmode {
		constructor(overview: internalModule.Overview, detailview: internalModule.Detailview) {
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
			}

			this.containerIndices = {
				left: [],
				right: [],
				horizontal: [],
				synDriver: [],
				neurons: [],
				repeaters: [],
				synGrids: []
			}

			this.initSelectedElements();
		};

		enabled: boolean;

		detailview: internalModule.Detailview;

		overview: internalModule.Overview;

		wafer: internalModule.Wafer;

		/**
		 * Store which elements are checked (i.e. visible) as boolean values for all elements.
		 * For example: if selectedElements.overview.left[5] is "true", 
		 * then the left buses in overview representation for the 5th HICANN are visible.
		 */
		selectedElements: {
			overview: {
				numNeurons: boolean[],
				numInputs: boolean[],
				left: boolean[],
				right: boolean[],
				horizontal: boolean[]
			},
			detailview: {
				left: boolean[],
				right: boolean[],
				horizontal: boolean[],
				synDriver: boolean[],
				neurons: boolean[],
				repeaters: boolean[],
				synGrids: boolean[]
			}
		};

		/**
		 * Store the HICANN indices for the bus-segments that are drawn as graphics objects (in detailviewLevelTwo)
		 * in the order in that they are first drawn. This is necessary to be able to delete those bus-segments again later.
		 * If not all bus-segments are drawn, the left bus-segment of the 5th HICANN, for example, will in general not be at fifth place in the respective container.
		 */
		containerIndices: {
			left: number[],
			right: number[],
			horizontal: number[],
			synDriver: number[],
			neurons: number[],
			repeaters: number[],
			synGrids: number[],
		};

		/**
		 * Initialize the manual mode. Call this method when starting the manual mode.
		 * @param levelOneEnabled Set to true if the manual mode is started in detailviewLevelOne.
		 * @param levelTwoEnabled Set to true if the manual mode is started in detailviewLevelTwo.
		 */
		init(levelOneEnabled: boolean, levelTwoEnabled: boolean) {
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
		}

		/**
		 * initialize selected Elements with standard overview values.
		 */
		initSelectedElements() {
			for (let i=this.wafer.enumMin; i<=this.wafer.enumMax; i++) {
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
		}

		/**
		 * Reset all elements to a plain "overview".
		 * Call this method when switching to automode.
		 */
		resetView() {
			// remove detailview elements
			this.detailview.resetDetailview();
			// empty container-indices arrays
			for (const type in this.containerIndices) {
				this.containerIndices[type] = [];
			}
			// show overview
			this.setOverview(true, true);
		}

		recoverView(levelOneEnabled: boolean, levelTwoEnabled: boolean) {
			for (const hicannIndex in this.selectedElements.detailview.left) {
				this.busesLeft(parseInt(hicannIndex), this.selectedElements.detailview.left[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.right) {
				this.busesRight(parseInt(hicannIndex), this.selectedElements.detailview.right[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.horizontal) {
				this.busesHorizontal(parseInt(hicannIndex), this.selectedElements.detailview.horizontal[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.synDriver) {
				this.synDriver(parseInt(hicannIndex), this.selectedElements.detailview.synDriver[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.neurons) {
				this.neurons(parseInt(hicannIndex), this.selectedElements.detailview.neurons[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.repeaters) {
				this.repeaters(parseInt(hicannIndex), this.selectedElements.detailview.repeaters[hicannIndex], true);
			}
			for (const hicannIndex in this.selectedElements.detailview.synGrids) {
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
			} else {
				this.leaveDetailviewLevelTwo()
			};
		}

		/**
		 * Start the detailview. Only hides the HICANN number, because the rest is managed manually.
		 */
		startDetailview() {
			hicannNumber.disable()
			this.detailview.enabled = true;
		}

		/**
		 * Leave the detailview.
		 */
		leaveDetailview() {
			hicannNumber.recover();
			this.detailview.enabled = false;
		}

		/**
		 * Start the DetailviewLevelTwo to switch from sprites to graphics objects.
		 */
		startDetailviewLevelTwo() {
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
		}

		/**
		 * Leave the detailviewLevelTwo to switch from graphics objects back to sprites.
		 */
		leaveDetailviewLevelTwo() {
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
		}

		/**
		 * Set the visible properties for all elements of the overview.
		 * @param viewChecked Set to true if checked (in UI checkbox) elements should be set.
		 * @param viewUnchecked Set to true if unchecked (in UI checkbox) elements should be set.
		 */
		setOverview(viewChecked: boolean, viewUnchecked: boolean) {
			// loop through detailed Buses and hide/display checked ones
			for (let i=this.wafer.enumMin; i<=this.wafer.enumMax; i++) {

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
		}
		
		/**
		 * handle clicking the checkbox for overview elements of a HICANN in the HICANN list.
		 * @param hicannIndex Index of the selected HICANN
		 * @param checked state of the checkbox
		 */
		overviewCheckbox(hicannIndex: number, checked: boolean) {
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
			$(`#hicanns_0_${hicannIndex}_OV_neu`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_OV_inp`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_OV_bsl`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_OV_bsr`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_OV_bsh`).prop("checked", checked);
			// make sure checkboxes are consistent
			manualmode.checkAllCheckboxes("numNeurons");
			manualmode.checkAllCheckboxes("numInputs");
			manualmode.checkAllCheckboxes("left");
			manualmode.checkAllCheckboxes("right");
			manualmode.checkAllCheckboxes("horizontal");
		}

		/**
		 * handle clicking the checkbox for detailed elements of a HICANN in the HICANN list.
		 * @param hicannIndex Index of the selected HICANN
		 * @param checked state of the checkbox
		 */
		detailviewCheckbox(hicannIndex: number, checked: boolean) {
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
			$(`#hicanns_0_${hicannIndex}_DV_bsl`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_bsr`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_bsh`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_snd`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_neu`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_rep`).prop("checked", checked);
			$(`#hicanns_0_${hicannIndex}_DV_sng`).prop("checked", checked);
			// make sure, checkboxes are consistent
			this.checkAllCheckboxes("detailLeft");
			this.checkAllCheckboxes("detailRight");
			this.checkAllCheckboxes("detailHorizontal");
			this.checkAllCheckboxes("synDriver");
			this.checkAllCheckboxes("neurons");
			this.checkAllCheckboxes("repeaters");
			this.checkAllCheckboxes("synGrid");
		}

		/**
		 * Handle clicking the checkbox for a vertical left buses of a HICANN in the HICANN list.
		 * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
		 */
		busesLeft(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.left[hicannIndex] !== checked) || skipDoubleCheck) {
				if (checked) {
					this.detailview.drawBusesLeft(this.wafer.hicanns[hicannIndex].position);
					this.containerIndices.left.push(hicannIndex);
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.left.indexOf(hicannIndex);
					pixiBackend.removeChild(pixiBackend.container.busesLeft, containerIndex);
					pixiBackend.removeChild(pixiBackend.container.busesLeftSprite, containerIndex);
					this.containerIndices.left.splice(containerIndex, 1);
				}
				this.selectedElements.detailview.left[hicannIndex] = checked;
			}
		}

		/**
		 * Handle clicking the checkbox for a vertical right buses of a HICANN in the HICANN list.
		 * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
		 */
		busesRight(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.right[hicannIndex] !== checked) || skipDoubleCheck) {
				if (checked) {
					this.detailview.drawBusesRight(this.wafer.hicanns[hicannIndex].position);
					this.containerIndices.right.push(hicannIndex);
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.right.indexOf(hicannIndex);
					pixiBackend.removeChild(pixiBackend.container.busesRight, containerIndex);
					pixiBackend.removeChild(pixiBackend.container.busesRightSprite, containerIndex);
					this.containerIndices.right.splice(containerIndex, 1);
				}
				this.selectedElements.detailview.right[hicannIndex] = checked;
			}
		}

		/**
		 * Handle clicking the checkbox for a horizontal buses of a HICANN in the HICANN list.
		 * If checked, the graphics elements and sprite for that bus are drawn, if unchecked the graphics element and sprite removed.
		 */
		busesHorizontal(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.horizontal[hicannIndex] !== checked) || skipDoubleCheck) {
				// number of children stored in the container when drawing horizontal buses for one HICANN
				const childrenPerHicann = 2;
				if (checked) {
					this.detailview.drawBusesHorizontal(this.wafer.hicanns[hicannIndex].position);
					for (let _=0; _<childrenPerHicann; _++) {
						this.containerIndices.horizontal.push(hicannIndex);
					}
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.horizontal.indexOf(hicannIndex);
					for (let _=0; _<childrenPerHicann; _++) {
						pixiBackend.removeChild(pixiBackend.container.busesHorizontal, containerIndex);
						pixiBackend.removeChild(pixiBackend.container.busesHorizontalSprite, containerIndex);
					}
					this.containerIndices.horizontal.splice(containerIndex, childrenPerHicann);
				}
				this.selectedElements.detailview.horizontal[hicannIndex] = checked;
			}
		}

		/**
		 * Handle clicking the checkbox for a synapse driver of a HICANN in the HICANN list.
		 * If checked, the graphics elements for these synapse drivers are drawn, if unchecked, they are removed.
		 */
		synDriver(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.synDriver[hicannIndex] !== checked) || skipDoubleCheck) {
				// number of children stored in the container when drawing synDrivers for one HICANN
				const childrenPerHicann = 4
				if (checked) {
					this.detailview.drawSynDrivers(this.wafer.hicanns[hicannIndex].position);
					for (let _=0; _<childrenPerHicann; _++) {
						this.containerIndices.synDriver.push(hicannIndex);
					}
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.synDriver.indexOf(hicannIndex);
					for (let _=0; _<childrenPerHicann; _++) {
						pixiBackend.removeChild(pixiBackend.container.synDrivers, containerIndex);
					}
					this.containerIndices.synDriver.splice(containerIndex, childrenPerHicann);
				}
				this.selectedElements.detailview.synDriver[hicannIndex] = checked;
			}
		}

		/**
		 * Handle clicking the checkbox for neurons of a HICANN in the HICANN list.
		 * If checked, the graphics elements and sprites for these neurons are drawn, if unchecked, they are removed.
		 */
		neurons(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.neurons[hicannIndex] !== checked) || skipDoubleCheck) {
				// number of children stored in the container when drawing neurons for one HICANN
				const childrenPerHicann = 1;
				if (checked) {
					this.detailview.drawNeurons(this.wafer.hicanns[hicannIndex].position);
					for (let _=0; _<childrenPerHicann; _++) {
						this.containerIndices.neurons.push(hicannIndex);
					}
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.neurons.indexOf(hicannIndex);
					for (let _=0; _<childrenPerHicann; _++) {
						pixiBackend.removeChild(pixiBackend.container.neurons, containerIndex);
						pixiBackend.removeChild(pixiBackend.container.neuronsSprite, containerIndex);
					}
					this.containerIndices.neurons.splice(containerIndex, childrenPerHicann);
				}
				this.selectedElements.detailview.neurons[hicannIndex] = checked;
			}
		}

		/**
		 * Handle clicking the checkbox for repeaters of a HICANN in the HICANN list.
		 * If checked, the graphics elements for these repeaters are drawn, if unchecked, they are removed.
		 */
		repeaters(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.repeaters[hicannIndex] !== checked) || skipDoubleCheck) {
				// number of children stored in the container when drawing synDrivers for one HICANN
				const childrenPerHicann = 1;
				if (checked) {
					this.detailview.drawRepeaters(this.wafer.hicanns[hicannIndex].position);
					for (let _=0; _<childrenPerHicann; _++) {
						this.containerIndices.repeaters.push(hicannIndex);
					}
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.repeaters.indexOf(hicannIndex);
					for (let _=0; _<childrenPerHicann; _++) {
						pixiBackend.removeChild(pixiBackend.container.repeaters, containerIndex);
						pixiBackend.removeChild(pixiBackend.container.repeaterBusConnections, containerIndex);
						pixiBackend.removeChild(pixiBackend.container.repeaterBusConnectionsSprite, containerIndex);
					}
					this.containerIndices.repeaters.splice(containerIndex, childrenPerHicann);
				}
				this.selectedElements.detailview.repeaters[hicannIndex] = checked;
			}
		}

		synGrid(hicannIndex: number, checked: boolean, skipDoubleCheck=false) {
			if ((this.selectedElements.detailview.synGrids[hicannIndex] !== checked) || skipDoubleCheck) {
				const childrenPerHicann = 1;
				if (checked) {
					this.detailview.drawSynGrid(this.wafer.hicanns[hicannIndex].position);
					for (let _=0; _<childrenPerHicann; _++) {
						this.containerIndices.synGrids.push(hicannIndex);
					}
				} else if (!skipDoubleCheck) {
					const containerIndex = this.containerIndices.synGrids.indexOf(hicannIndex);
					for (let _=0; _<childrenPerHicann; _++) {
						pixiBackend.removeChild(pixiBackend.container.synGrid, containerIndex);
						pixiBackend.removeChild(pixiBackend.container.synGridSprite, containerIndex);
					}
					this.containerIndices.synGrids.splice(containerIndex, childrenPerHicann);
				}
				this.selectedElements.detailview.synGrids[hicannIndex] = checked;
			}
		}

		/**
		 * synchronize the checkboxes in the HICANN list when the all elements of one type are drawn at once.
		 * @param element type of the element. "numNeurons" for example are are the colored HICANN backgrounds of the overview.
		 */
		checkAllCheckboxes(element: elementType) {
			let allElementsSelected = true;
			switch (element) {
				case "numNeurons":
					for (let i=0; i<this.selectedElements.overview.numNeurons.length; i++) {
						if (this.selectedElements.overview.numNeurons[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#numNeuronsCheckbox").prop("checked", allElementsSelected);
					break;
				case "numInputs":
					for (let i=0; i<this.selectedElements.overview.numInputs.length; i++) {
						if (this.selectedElements.overview.numInputs[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#numInputsCheckbox").prop("checked", allElementsSelected);
					break;
				case "left":
					for (let i=0; i<this.selectedElements.overview.left.length; i++) {
						if (this.selectedElements.overview.left[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#verticalLeftCheckbox").prop("checked", allElementsSelected);
					break;
				case "right":
					for (let i=0; i<this.selectedElements.overview.right.length; i++) {
						if (this.selectedElements.overview.right[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#verticalRightCheckbox").prop("checked", allElementsSelected);
					break;
				case "horizontal":
					for (let i=0; i<this.selectedElements.overview.horizontal.length; i++) {
						if (this.selectedElements.overview.horizontal[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#horizontalCheckbox").prop("checked", allElementsSelected);
					break;
				case "detailLeft":
					for (let i=0; i<this.selectedElements.detailview.left.length; i++) {
						if (this.selectedElements.detailview.left[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#verticalLeftDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "detailRight":
					for (let i=0; i<this.selectedElements.detailview.right.length; i++) {
						if (this.selectedElements.detailview.right[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#verticalRightDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "detailHorizontal":
					for (let i=0; i<this.selectedElements.detailview.horizontal.length; i++) {
						if (this.selectedElements.detailview.horizontal[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#horizontalDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "synDriver":
					for (let i=0; i<this.selectedElements.detailview.synDriver.length; i++) {
						if (this.selectedElements.detailview.synDriver[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#synDriverDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "synGrid":
					for (const synGrid of this.selectedElements.detailview.synGrids) {
						if (synGrid === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#synGridDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "neurons":
					for (let i=0; i<this.selectedElements.detailview.neurons.length; i++) {
						if (this.selectedElements.detailview.neurons[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#neuronsDetailsCheckbox").prop("checked", allElementsSelected);
					break;
				case "repeaters":
					for (let i=0; i<this.selectedElements.detailview.repeaters.length; i++) {
						if (this.selectedElements.detailview.repeaters[i] === false) {
							allElementsSelected = false;
							break;
						};
					};
					$("#repeatersDetailsCheckbox").prop("checked", allElementsSelected);
					break;
			};
		}

		/**
		 * Update the selectedElements list and the UI checkboxes in the HICANN list when all elements of one type are set at once.
		 * @param element type of the element. "numNeurons" for example are are the colored HICANN backgrounds of the overview.
		 */
		setAllCheckboxes(element: elementType, checked: boolean) {
			switch (element) {
				case "numNeurons":
					for (let i=0; i<this.selectedElements.overview.numNeurons.length; i++) {
						this.selectedElements.overview.numNeurons[i] = checked;
						$(`#hicanns_0_${i}_OV_neu`).prop("checked", checked)
					};
					break;
				case "numInputs":
					for (let i=0; i<this.selectedElements.overview.numInputs.length; i++) {
						this.selectedElements.overview.numInputs[i] = checked;
						$(`#hicanns_0_${i}_OV_inp`).prop("checked", checked)
					};
					break;
				case "left":
					for (let i=0; i<this.selectedElements.overview.left.length; i++) {
						this.selectedElements.overview.left[i] = checked;
						$(`#hicanns_0_${i}_OV_bsl`).prop("checked", checked)
					};
					break;
				case "right":
					for (let i=0; i<this.selectedElements.overview.right.length; i++) {
						this.selectedElements.overview.right[i] = checked;
						$(`#hicanns_0_${i}_OV_bsr`).prop("checked", checked)
					};
					break;
				case "horizontal":
					for (let i=0; i<this.selectedElements.overview.horizontal.length; i++) {
						this.selectedElements.overview.horizontal[i] = checked;
						$(`#hicanns_0_${i}_OV_bsh`).prop("checked", checked)
					};
					break;
				case "detailLeft":
					for (let i=0; i<this.selectedElements.detailview.left.length; i++) {
						this.selectedElements.detailview.left[i] = checked;
						$(`#hicanns_0_${i}_DV_bsl`).prop("checked", checked)
					};
					break;
				case "detailRight":
					for (let i=0; i<this.selectedElements.detailview.right.length; i++) {
						this.selectedElements.detailview.right[i] = checked;
						$(`#hicanns_0_${i}_DV_bsr`).prop("checked", checked)
					};
					break;
				case "detailHorizontal":
					for (let i=0; i<this.selectedElements.detailview.horizontal.length; i++) {
						this.selectedElements.detailview.horizontal[i] = checked;
						$(`#hicanns_0_${i}_DV_bsh`).prop("checked", checked)
					};
					break;
				case "synDriver":
					for (let i=0; i<this.selectedElements.detailview.synDriver.length; i++) {
						this.selectedElements.detailview.synDriver[i] = checked;
						$(`#hicanns_0_${i}_DV_snd`).prop("checked", checked)
					};
					break;
				case "neurons":
					for (let i=0; i<this.selectedElements.detailview.neurons.length; i++) {
						this.selectedElements.detailview.neurons[i] = checked;
						$(`#hicanns_0_${i}_DV_neu`).prop("checked", checked)
					};
					break;
				case "repeaters":
					for (let i=0; i<this.selectedElements.detailview.repeaters.length; i++) {
						this.selectedElements.detailview.repeaters[i] = checked;
						$(`#hicanns_0_${i}_DV_rep`).prop("checked", checked);
					};
					break;
				case "synGrid":
					for (let i=0; i<this.selectedElements.detailview.synGrids.length; i++) {
						this.selectedElements.detailview.synGrids[i] = checked;
						$(`#hicanns_0_${i}_DV_sng`).prop("checked", checked);
					}
			};
		}

		checkAllDetailedElements(hicannIndex: number) {
			let allPropertiesSelected = true;
			for (const key in this.selectedElements.detailview) {
				if (this.selectedElements.detailview[key][hicannIndex] === false) {
					allPropertiesSelected = false;
					break;
				}
			}
			$(`#hicanns_0_${hicannIndex}_DV_checkbox`).prop("checked", allPropertiesSelected);
		}

		checkAllOverviewElements(hicannIndex: number) {
			let allPropertiesSelected = true;
			for (const key in this.selectedElements.overview) {
				if (this.selectedElements.overview[key][hicannIndex] === false) {
					allPropertiesSelected = false;
					break;
				}
			}
			$(`#hicanns_0_${hicannIndex}_OV_checkbox`).prop("checked", allPropertiesSelected);
		}
	}
}