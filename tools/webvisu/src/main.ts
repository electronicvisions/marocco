/*
	* --- remove development mode ----
	* main.ts: const devMode
	* main.ts: devMode area with quickStart function
	* main.ts: modified wafer.loadOverviewData() function calls
	* wafer.ts: loadOverviewData networkFilePath parameter should not be optional!
	* wafer.ts: don't check for networkFilePath, but do 'marocco = new Module.Marocco(networkFilePath);' right away
	*/

/// <reference path="modules/jqueryui.d.ts" />
/// <reference path="modules/filesystem.d.ts" />
/// <reference path="modules/stats.d.ts" />
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

/// <reference types="pixi.js" />

/**
 * development mode: set to true to skip file upload procedure
 */
const devMode = false;
let mouseIsDown = false;
let mousePosition = {
	x: <number>undefined,
	y: <number>undefined
};
let mouseDownPosition = {
		x: <number>undefined,
		y: <number>undefined
};
let touchPosition = {
	x: undefined,
	y: undefined
};
let touches: any;
/**
 * HICANN property gradient color
 */
const numNeuronsColorOne = "ffffff";
/**
 * HICANN property gradient color
 */
let numNeuronsColorTwo = "174e75";
/**
 * HICANN property gradient color
 */
const numInputsColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
let numInputsColorTwo = "ef5450";
/**
 * HICANN property gradient color
 */
const numRoutesLeftColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
let numRoutesLeftColorTwo = "fbcb3f";
/**
 * HICANN property gradient color
 */
const numRoutesRightColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
let numRoutesRightColorTwo = "fbcb3f";
/**
 * HICANN property gradient color
 */
const numRoutesHorizontalColorOne = numNeuronsColorOne;
/**
 * HICANN property gradient color
 */
let numRoutesHorizontalColorTwo = "fbcb3f";
let domObjects: {[key: string]: any} = {};
const properties = [
	"neurons",
	"inputs",
	"leftBuses",
	"rightBuses",
	"horizontalBuses",
];
let wafer: internalModule.Wafer;
let overview: internalModule.Overview;
let detailview: internalModule.Detailview;
let routesOnStage: internalModule.RoutesOnStage;
let reticlesOnStage: internalModule.ReticlesOnStage;
let waferImage: internalModule.WaferImage;
let automode: internalModule.Automode;
let manualmode: internalModule.Manualmode;
let routeInfo: internalModule.RouteInfo;
let settings: internalModule.Settings;
let summary: internalModule.Summary;
let hicannNumber: internalModule.HicannNumber;
let hicannInfo: internalModule.HicannInfo;

const canvasWidth = function() {
	return($("#waferVisu").width());
};
const canvasHeight = function() {
	return($("#waferVisu").height());
};
const canvasCenter = function() {
	return({
		x: $("#leftInfoBox").outerWidth(true) + (canvasWidth() - domObjects.rightInfoBox[0].offsetWidth - $("#leftInfoBox").outerWidth())/2,
		y: canvasHeight()/2,
	})
};
	
// wait for DOM to load
$(window).on('load', () => {
	console.log("dom ready")
	domReady = true;
})
/**
 * Indicator variable for the state of the static HTML DOM
 */
let domReady = false;
/**
 * Callback function for emscriptens Module.onRuntimeInitialized event.
 * Waiting for the DOM to load and then setting up the upload screen.
 */
function waitingForDOM() {
	console.log("start waiting for DOM")
	let waitingForDOMInterval = window.setInterval(() => {
		if (domReady) {
			clearInterval(waitingForDOMInterval);
			setupScreen();
		}
	}, 10)
}
/**
 * Set up the Filebrowser and drag and drop area for uploading
 * the marocco::results configuration file
 */
function setupScreen(){
	console.log("setting up screen");
	//upload marocco::results file
	let inputFile = undefined;
	// upload via filebrowser
	const fileBrowser: HTMLInputElement = document.querySelector("#networkFile");
	fileBrowser.addEventListener("change", event => {
		inputFile = fileBrowser.files[0];
		$("#fileLabel").html(inputFile.name);
		$("#fileLabel").css("color", "#fbb535");
	}, false);
	// upload via drag-n-drop
	const dropZone = document.querySelector("#dropZone");
	dropZone.addEventListener("drop", function(e: DragEvent){
		const event: DragEvent = e || <DragEvent>window.event; //Firefox
		event.preventDefault();
		if(event.dataTransfer.files.length > 1) {
			alert("Please select only one file!")
		} else {
			inputFile = event.dataTransfer.files[0]
			$("#fileLabel").html(inputFile.name);
			$("#fileLabel").css("color", "#99ff99");
		}
	})
	dropZone.addEventListener("dragover", (e: DragEvent) => {
		const event: DragEvent = e || <DragEvent>window.event;
		event.preventDefault();
	})
	// visual effects
	dropZone.addEventListener("dragenter", () => {$("#dropZone").css("border", "2px solid #fbb535")})
	dropZone.addEventListener("dragleave", () => {$("#dropZone").css("border", "2px solid #222222")})
	dropZone.addEventListener("drop", () => {$("#dropZone").css("border", "2px solid ##222222")})

	// handle upload Button
	let uploadButton = document.querySelector("#upload");
	uploadButton.addEventListener("click", function() {
		if(inputFile === undefined) {
			alert("no file selected")
		} else { // process file
			const filereader = new FileReader();
			// event handler for data loaded with filereader
			filereader.onload = function(event) {
				let target: any = event.target;
				let data = target.result;
				let contents = new Int8Array(data);
				// write file into emscriptens virtual file system (FS)
				// file name is "network" + extension of the input file
				console.log("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0])
				FS.writeFile("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0], contents);
				// remove upload screen and display loading screen
				$("#uploadScreen").css("display", "none");
				$("#loadingScreen").css("display", "block");
				// start main program
				setTimeout(() => {main("./network" + inputFile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0])}, 30000);
			};
			filereader.onerror = function(event) {
				console.error(`File could not be read! Code $event.target.error.code`);
			};
			// read the contents of inputFile and fire onload event
			filereader.readAsArrayBuffer(inputFile);
		}
	})

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
		setTimeout(() => {main("undefined")}, 100);
	}
	// ----- devMode -----

}//setupScreen
/**
 * The main function runs the core visualization program.
 * Instances of all the classes needed for the visualization are created and event handlers defined.
 */
function main(resultsFile: string) {
	///////////////////////////
	
	// FPS, RAM Stats
	const stats = new Stats();
	stats.showPanel( 0 ); // 0: fps, 1: ms, 2: mb, 3+: custom
	stats.dom.style.position = "relative"
	stats.dom.style.marginLeft = "20px"
	
	document.querySelector("#fpsContainer").appendChild( stats.dom );
	
	function animate() {
	
		stats.begin();
		stats.end();
	
		requestAnimationFrame( animate );
	
	}
	
	requestAnimationFrame( animate );
	
	///////////////////////////
	
	console.log(`WebGL is${!PIXI.utils.isWebGLSupported ? " not": ""} supported by this browser.`);
	
	//////////////////////////////////
	// store common DOM elements
	// deprecated???
	referenceDOM();

	// setup pixiBackend
	pixiBackend.renderer = new pixiBackend.Renderer($("#waferVisu"), 0x333333, canvasWidth(), canvasHeight());
	pixiBackend.container.setup();

	visualizeFile(resultsFile)

	waferImage = new internalModule.WaferImage(wafer, "img/hicann.png", pixiBackend.container.hicannImages,
					wafer.hicannWidth, wafer.hicannHeight);

	// draw images of hicanns on wafer
	waferImage.draw();

	// draw Electronic Visions and HBP Logos
	// TODO: non-frickel solution plz
	(() => {
		// Visions Logo
		let height = 600;
		let width = height*7246/9380;
		let x = 0;
		let y = wafer.hicannHeight*16 - height;
		pixiBackend.drawImage(pixiBackend.container.logos, "img/visionsLogo.png",x, y, width, height, 0.2);
		// HBP Logo
		width = height;
		x = wafer.hicannWidth*36 - width;
		pixiBackend.drawImage(pixiBackend.container.logos, "img/HBPLogo.png", x, y, width, height, 0.2);
	})()

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
	$(window).keydown( e => {
		handleKeyDown(e || window.event);
	} );
	$("#visuWindow").mousedown( e => {
		handleMouseDown((e || window.event) as JQueryMouseEventObject);
	} );
	document.querySelector("#visuWindow").addEventListener("touchstart", (e)=>{
		handleTouchstart(e || window.event);
	})
	document.querySelector("#visuWindow").addEventListener("touchmove", (e)=>{
		e.preventDefault();
		handleTouchmove(e || window.event);
	})
	$("#visuWindow").mouseup( e => {
		handleMouseUp(e || window.event);
	} );
	$("#visuWindow").mousemove( e => {
		handleMouseMove(e || window.event)
	} );
	$("#visuWindow").mouseout( e => {
		mouseIsDown = false;
	} );
	$("#visuWindow").dblclick( e => {
		const event: any = e || window.event;
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
	})

	document.querySelector("#visuWindow").addEventListener("wheel", function (e) {
		handleWheel(e || window.event);
});
	
	$("#allNumbersCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked;
		hicannNumber.setAll(checked);
		pixiBackend.renderer.render();
	})

	$("#waferImageCheckbox").change( e => {
		waferImage.setVisible(((e || window.event).target as HTMLInputElement).checked)
		pixiBackend.renderer.render();
	})

	$("#reticlesCheckbox").change( e => {
		reticlesOnStage.setReticles(((e || window.event).target as HTMLInputElement).checked);
		pixiBackend.renderer.render();
	})
	
	$("#numNeuronsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked;
		for (let index=0; index<=wafer.enumMax; index++) {
			pixiBackend.container.backgrounds.children[index].visible = checked;
		};
		manualmode.setAllCheckboxes("numNeurons", checked);
		pixiBackend.renderer.render();
	})
	
	$("#numInputsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			pixiBackend.container.inputs.children[index].visible = checked;
		};
		manualmode.setAllCheckboxes("numInputs", checked);
		pixiBackend.renderer.render();
	})
	
	$("#verticalLeftCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			pixiBackend.container.overviewBusesLeft.children[index].visible = checked;
		};
		manualmode.setAllCheckboxes("left", checked);
		pixiBackend.renderer.render();
	})
	
	$("#verticalRightCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			pixiBackend.container.overviewBusesRight.children[index].visible = checked;
		};
		manualmode.setAllCheckboxes("right", checked);
		pixiBackend.renderer.render();
	})
	
	$("#horizontalCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			pixiBackend.container.overviewBusesHorizontal.children[index].visible = checked;
		};
		manualmode.setAllCheckboxes("horizontal", checked);
		pixiBackend.renderer.render();
	})
	
	$("#verticalLeftDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_bsl`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})
	
	$("#verticalRightDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_bsr`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})
	
	$("#horizontalDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_bsh`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})

	$("#synDriverDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_snd`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})

	$("#synGridDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_sng`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})
	
	$("#neuronsDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_neu`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})
	
	$("#repeatersDetailsCheckbox").change( e => {
		const checked = ((e || window.event).target as HTMLInputElement).checked
		for (let index=0; index<=wafer.enumMax; index++) {
			const checkbox = $(`#hicanns_0_${index}_DV_rep`);
			if (checkbox.prop("checked") !== checked) {
				checkbox.click();
			}
		}
		pixiBackend.renderer.render();
	})

	$("#automode").click( () => {
		if (!automode.enabled) {
			// store detail level
			const levelOneEnabled = detailview.enabled;
			const levelTwoEnabled = detailview.levelTwoEnabled;
			// determine closest hicann for auto detail view
			const hicannClosestToCenter = detailview.hicannClosestToCenter(canvasCenter());
			// reset view
			manualmode.resetView();
			// start auto Mode
			automode.init(hicannClosestToCenter, levelOneEnabled, levelTwoEnabled);
			manualmode.enabled = false;	
		}
	})

	$("#manualmode").click( () => {
		if (!manualmode.enabled) {
			// store detail level
			const levelOneEnabled = detailview.enabled;
			const levelTwoEnabled = detailview.levelTwoEnabled;
			// reset view
			if(levelOneEnabled) {
				automode.startOverview();
			}
			// start manual Mode
			manualmode.init(levelOneEnabled, levelTwoEnabled);
			automode.enabled = false;
		}
	})

	$("#waferList .listHeader button").click(() => {
		let input = Number($("#waferList .listHeader input").val());
		if (isNaN(input)) {
			alert("please enter a number");
		} else {
			scrollToHicann(input);
		}
	})

	$("#waferList .listHeader input").keydown(event => {
		if (event.which === 13) {
			$("#waferList .listHeader button").click();
		};
	});

	$("#routesList .listHeader button").eq(0).click(() => {
		let input = Number($("#routesList .listHeader input").eq(0).val());
		if (isNaN(input)) {
			alert("please enter a number");
		} else {
			scrollToRoute(input);
		}
	})

	$("#routesList .listHeader input").eq(0).keydown(event => {
		if (event.which === 13) {
			$("#routesList .listHeader button").eq(0).click();
		}
	})

	// Resize Canvas, when window is rescaled;
	$(window).resize(function() {
		pixiBackend.renderer.renderer.resize(canvasWidth(), canvasHeight());
		detailview.determineThreshold(canvasHeight())
		setupJQueryUI();
		pixiBackend.renderer.render();
	})
	
} //main

function visualizeFile(resultsFile: string) {
	wafer = new internalModule.Wafer();

	try {
		// ----- devMode -----
		devMode ? wafer.loadOverviewData() : wafer.loadOverviewData(resultsFile);
		// ----- devMode -----
	} catch (e) {
		alert(
			`cannot load input file. \nPossible reasons: invalid results file or error in emscripten marocco build`
		);
		throw(new Error("cannot load input file"));
	}
		
	// Adjust color gradients when a HICANN property is zero for every HICANN.
	setHicannPropertyGradients();
	
	overview = new internalModule.Overview(
		wafer,
		{
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
		}
	);
	// draw overview of wafer
	overview.drawWafer();

	detailview = new internalModule.Detailview(wafer);
	
	routesOnStage = new internalModule.RoutesOnStage(detailview);

	// draw routes
	routesOnStage.drawRoutes();
	
	// hide routes
	for (const route of routesOnStage.routes) {
		routesOnStage.setRoute(route, false);
	}

	reticlesOnStage = new internalModule.ReticlesOnStage(overview, pixiBackend.container.reticles);

	// threshold where the lookup plot is made visible
	reticlesOnStage.threshold = fullWaferScale() * 2/3;

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
	const scale = fullWaferScale();
	const transform: any = pixiBackend.container.stage.transform;
	transform.scale.x = scale;
	transform.scale.y = scale;

	const hicannNumber = 173
	let stagePosition = transform.position;
	let hicannPosition = wafer.hicanns[hicannNumber].position;
	const newPosition = {
		x: -(hicannPosition.x + wafer.hicannWidth/2)*transform.scale.x + canvasCenter().x,
		y: -(hicannPosition.y + wafer.hicannHeight/2)*transform.scale.y + canvasCenter().y,
	}
	stagePosition.x = newPosition.x;
	stagePosition.y = newPosition.y;

	pixiBackend.renderer.render();
}
/**
 * Calculate the scale, where the full wafer fits onto the canvas.
 */
function fullWaferScale() {
	const waferWidth = 36 * (wafer.hicannWidth + wafer.hicannMargin) * 1.3
	const waferHeight = 16 * (wafer.hicannHeight + wafer.hicannMargin) * 1.3
	return (canvasWidth()/waferWidth < canvasHeight()/waferHeight) ? canvasWidth()/waferWidth : canvasHeight()/waferHeight;
}
/**
 * If a property is zero for every HICANN, the color gradient has to be adjusted.
 */
function setHicannPropertyGradients() {
	if (wafer.hicanns === []) {
		throw(new Error("HICANN data has to be loaded into wafer class before gradients can be set."))
	}

	if (wafer.numNeuronsMax === 0) {
		numNeuronsColorTwo = numNeuronsColorOne;
	};
	if (wafer.numInputsMax === 0) {
		numInputsColorTwo = numInputsColorOne;
	};
	if (wafer.numBusesLeftMax === 0) {
		numRoutesLeftColorTwo = numRoutesLeftColorOne;
	};
	if (wafer.numBusesRightMax === 0) {
		numRoutesRightColorTwo = numRoutesRightColorOne;
	};
	if (wafer.numBusesHorizontalMax === 0) {
		numRoutesHorizontalColorTwo = numRoutesHorizontalColorOne;
	};
}
/**
 * Build the HTML for the HICANN "tree-style" list in the left info-box
 */
function buildElementsTree() {
	// hicann list
	// hicann surrounding unordered list
	const hicannList = document.createElement("ul");
	$("#elementsTree").append(hicannList);
	// hicanns add list items containing buttons
	for (let i=0; i<=wafer.enumMax; i++) {
		const listItem = document.createElement("li");
		hicannList.appendChild(listItem);

		const hicannInput = document.createElement("input");
		hicannInput.type = "checkbox";
		hicannInput.checked = true;
		hicannInput.classList.add("fork");
		hicannInput.id = `hicanns_0_${i}`;
		listItem.appendChild(hicannInput);

		const hicannInputLabel = document.createElement("label");
		hicannInputLabel.htmlFor = `hicanns_0_${i}`;
		hicannInputLabel.classList.add("checkboxLabel");
		listItem.appendChild(hicannInputLabel);

		const hicann = document.createElement("button");
		hicann.innerText = `HICANN `;
		hicann.addEventListener("click", function(){ handleListClickHicann(event) });
		listItem.appendChild(hicann)

		const hicannNumber = document.createElement("span");
		hicannNumber.textContent = `${i}`;
		hicann.appendChild(hicannNumber)

		const elementsList = document.createElement("ul");
		listItem.appendChild(elementsList);
		// overview elements
		const overviewListItem = document.createElement("li");
		elementsList.appendChild(overviewListItem);

		const overviewInput = document.createElement("input");
		overviewInput.type = "checkbox";
		overviewInput.checked = true;
		overviewInput.classList.add("fork");
		overviewInput.id = `hicanns_0_${i}_OV`;
		overviewListItem.appendChild(overviewInput);
		// overview elements checkbox label
		const overviewInputLabel = document.createElement("label");
		overviewInputLabel.htmlFor = `hicanns_0_${i}_OV`;
		overviewInputLabel.classList.add("checkboxLabel");
		overviewListItem.appendChild(overviewInputLabel);
		// overview elements checkbox
		const overviewCheckbox = document.createElement("input");
		overviewCheckbox.type = "checkbox";
		overviewCheckbox.checked = true;
		overviewCheckbox.addEventListener("change", function(e) {
			manualmode.overviewCheckbox(i, (<any>(e || window.event).target).checked)
		})
		overviewCheckbox.classList.add("hicannElementCheckbox");
		overviewCheckbox.id = `hicanns_0_${i}_OV_checkbox`;
		overviewListItem.appendChild(overviewCheckbox);
		// overview elements label
		const overviewLabel = document.createElement("label");
		overviewLabel.innerHTML = "Overview";
		overviewLabel.htmlFor = `hicanns_0_${i}_OV_checkbox`;
		overviewListItem.appendChild(overviewLabel);

		const overviewList = document.createElement("ul");
		overviewListItem.appendChild(overviewList);
		// number of neurons
		overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.numNeurons,
				pixiBackend.container.backgrounds, "numNeurons", "number of neurons", "neu"));
		// number of inputs
		overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.numInputs,
				pixiBackend.container.inputs, "numInputs", "number of inputs", "inp"));
		// Buses left
		overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.left,
				pixiBackend.container.overviewBusesLeft, "left", "vertical left", "bsl"));
		// Buses right
		overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.right,
				pixiBackend.container.overviewBusesRight, "right", "vertical right", "bsr"));
		// Buses horizontal
		overviewList.appendChild(newOverviewElement(i, manualmode.selectedElements.overview.horizontal,
				pixiBackend.container.overviewBusesHorizontal, "horizontal", "horizontal", "bsh"));

		// detailed elements
		const detailviewListItem = document.createElement("li");
		elementsList.appendChild(detailviewListItem);

		const detailviewInput = document.createElement("input");
		detailviewInput.type = "checkbox";
		detailviewInput.checked = true;
		detailviewInput.classList.add("fork");
		detailviewInput.id = `hicanns_0_${i}_DV`;
		detailviewListItem.appendChild(detailviewInput);
		// detailed elements checkbox label
		const detailviewInputLabel = document.createElement("label");
		detailviewInputLabel.htmlFor = `hicanns_0_${i}_DV`;
		detailviewInputLabel.classList.add("checkboxLabel");
		detailviewListItem.appendChild(detailviewInputLabel);
		// detailed elements checkbox
		const detailviewCheckbox = document.createElement("input");
		detailviewCheckbox.type = "checkbox";
		detailviewCheckbox.checked = false;
		detailviewCheckbox.addEventListener("change", function(e) {
			manualmode.detailviewCheckbox(i, (<any>(e || window.event).target).checked);
		})
		detailviewCheckbox.classList.add("hicannElementCheckbox");
		detailviewCheckbox.id = `hicanns_0_${i}_DV_checkbox`;
		detailviewListItem.appendChild(detailviewCheckbox);
		// detailed elements label
		const detailviewLabel = document.createElement("label");
		detailviewLabel.innerHTML = "Detailview";
		detailviewLabel.htmlFor = `hicanns_0_${i}_DV_checkbox`;
		detailviewListItem.appendChild(detailviewLabel);

		const detailviewList = document.createElement("ul");
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
		detailviewList.appendChild(newDetailElement(i, manualmode.synGrid, "synGrid", "synGrid", "sng"))
		// neurons
		detailviewList.appendChild(newDetailElement(i, manualmode.neurons, "neurons", "neurons", "neu"));
		// repeaters
		detailviewList.appendChild(newDetailElement(i, manualmode.repeaters, "repeaters", "repeaters", "rep"));
	};
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
function newDetailElement(hicannIndex: number,
		drawFunction: (...args: any[]) => void, type: internalModule.elementType,
		labelText: string, typeAbbreviation: string) {
	const listItem = document.createElement("li");

	const checkbox = document.createElement("input");
	checkbox.type = "checkbox";
	checkbox.checked = false;
	checkbox.addEventListener("change", function(e) {
		drawFunction.call(manualmode, hicannIndex, (<any>(e || window.event).target).checked);
		pixiBackend.renderer.render();
		manualmode.checkAllCheckboxes(type);
		manualmode.checkAllDetailedElements(hicannIndex);
	});
	checkbox.classList.add("hicannElementCheckbox");
	checkbox.id = `hicanns_0_${hicannIndex}_DV_${typeAbbreviation}`;
	listItem.appendChild(checkbox);

	const label = document.createElement("label");
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
function newOverviewElement(hicannIndex: number, selectedElements: boolean[],
		container: PIXI.Container, type: internalModule.elementType,
		labelText: string, typeAbbreviation: string) {
	const listItem = document.createElement("li");

	const checkbox = document.createElement("input");
	checkbox.type = "checkbox";
	checkbox.checked = true;
	checkbox.addEventListener("change", function(e) {
		const checked = (<any>(e || window.event).target).checked;
		selectedElements[hicannIndex] = checked;
		container.children[hicannIndex].visible = checked;
		pixiBackend.renderer.render();
		manualmode.checkAllCheckboxes(type);
		manualmode.checkAllOverviewElements(hicannIndex);
	});
	checkbox.classList.add("hicannElementCheckbox");
	checkbox.id = `hicanns_0_${hicannIndex}_OV_${typeAbbreviation}`;
	listItem.appendChild(checkbox);

	const label = document.createElement("label");
	label.innerHTML = labelText;
	listItem.appendChild(label);

	return listItem
}

/**
 * Build the HTML for the Routes "tree-style" list in the left info-box
 */
function buildRoutesTree() {
	// select all Routes checkbox
	$("#routes_0_check").click(e => {
		const checked: boolean = (<any>(e || window.event).target).checked;
		for (const route of routesOnStage.routes) {
			routesOnStage.setRoute(route, checked);
			routesOnStage.setCheckbox(route, checked);
		};
		pixiBackend.renderer.render();
	});
	// routes label
	$("#allRoutes").click(() => {
		routesOnStage.handleRouteDoubleClick();
	})

	
	// route list
	// route surrounding unordered list
	const routesList = document.createElement("ul");
	$("#routesTree").append(routesList);
	// routes: add list items
	for (const route of routesOnStage.routes) {
		const ID = route.ID;
		const routeListItem = document.createElement("li")
		routesList.appendChild(routeListItem);
		
		const routeCheckbox = document.createElement("input");
		routeCheckbox.type = "checkbox";
		routeCheckbox.checked = false;
		routeCheckbox.classList.add("routeCheckbox");
		routeCheckbox.id = `routes_0_${ID}`;
		routeCheckbox.addEventListener("change", function(e) {
			const checked = (<any>(e || window.event).target).checked;
			routesOnStage.setRoute(route, checked);
			routesOnStage.checkAllRoutes();
			pixiBackend.renderer.render();
		});
		routeListItem.appendChild(routeCheckbox);
		
		const routeLabel = document.createElement("button");
		routeLabel.innerText = `Route `;
		routeLabel.addEventListener("click", function(){
			routesOnStage.handleRouteClick([route]);
		})
		routeListItem.appendChild(routeLabel);

		const routeLabelNumber = document.createElement("span");
		routeLabelNumber.textContent = `${ID + 1}`;
		routeLabel.appendChild(routeLabelNumber);
	}
}
/**
 * Event handler for clicking on a HICANN in the HICANN list.
 */
function handleListClickHicann(event) {
	const hicannNumber = parseInt(event.path[0].innerText.split(" ")[1]);
	const transform: any = pixiBackend.container.stage.transform;
	let stagePosition = transform.position;
	let hicannPosition = wafer.hicanns[hicannNumber].position;
	const newPosition = {
		x: -(hicannPosition.x + wafer.hicannWidth/2)*transform.scale.x + canvasCenter().x,
		y: -(hicannPosition.y + wafer.hicannHeight/2)*transform.scale.y + canvasCenter().y,
	}
	pixiBackend.animateStagePosition(stagePosition.x, stagePosition.y, newPosition.x, newPosition.y, 700)
	animateBorderAroundHicann(pixiBackend.container.border, hicannPosition.x, hicannPosition.y,
			wafer.hicannWidth, wafer.hicannHeight, 10, "0xff0066");
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
	let alpha = 1;
	let timer = setInterval(function(){
		if (container.children.length === 1) {
			pixiBackend.removeChild(container, 0)
		};
		pixiBackend.drawRectangleBorder(container, x, y, width, height, lineWidth, color, alpha);
		pixiBackend.renderer.render();
		alpha -= 0.01;
		if (Math.round(alpha*100)/100 === 0.00) {
			clearInterval(timer);
			pixiBackend.removeChild(container, 0)
		};
	}, 15)
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
			maxHeight: 0.9*$(window).innerHeight(),
			minHeight: 0.1*$(window).innerHeight(),
		})

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
		slide: function(event, ui) {
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
		stop: function(event, ui) {
			settings.adjustHBusSegment(ui.value);
		},
		min: 0,
		max: 1,
		step: 0.01
	})

		// vBus segment slider
		$("#vBusWidthSlider")
		.slider({
			stop: function(event, ui) {
				settings.adjustVBusSegment(ui.value);
			},
			min: 0,
			max: 1,
			step: 0.01
		})
}
/**
 * Helper function to reference the DOM
 */
function addByID(object: object, id: string) { 
	object[id] = $(`#${id}`)
}
/**
 * Helper function to reference the DOM
 */
function addProperty(object: object, property: string) {
	object[property] = {};
	addByID(object[property], `${property}Number`);
	addByID(object[property], `${property}Gradient`);
	addByID(object[property], `${property}Min`);
	addByID(object[property], `${property}Max`);
}
/**
 * Store references to DOM objects to save performance.
 */
function referenceDOM() {
	addByID(domObjects,"controlsContainer");
	addByID(domObjects, "rightInfoBox");
	addByID(domObjects, "hicannNumber");
	addByID(domObjects, "elementsTree");
	addByID(domObjects, "routesTree");
	for (let i=0; i<properties.length; i++){
		addProperty(domObjects, properties[i]);
	};
}
/**
 * Set the background colors as well as Min, Max numbers for all HICANN property gradients.
 */
function setupPropertyGradients() {
	domObjects.neurons.neuronsGradient.css("background",
			`linear-gradient(90deg, #${numNeuronsColorOne}, #${numNeuronsColorTwo})`);
	domObjects.neurons.neuronsMin.html(`0`);
	domObjects.neurons.neuronsMax.html(wafer.numNeuronsMax);

	domObjects.inputs.inputsGradient.css("background",
			`linear-gradient(90deg, #${numInputsColorOne}, #${numInputsColorTwo})`);
	domObjects.inputs.inputsMin.html(`0`);
	domObjects.inputs.inputsMax.html(wafer.numInputsMax);
	domObjects.leftBuses.leftBusesGradient.css("background",
			`linear-gradient(90deg, #${numRoutesLeftColorOne}, #${numRoutesLeftColorTwo})`);
	domObjects.leftBuses.leftBusesMin.html(`0`);
	domObjects.leftBuses.leftBusesMax.html(wafer.numBusesLeftMax);

	domObjects.rightBuses.rightBusesGradient.css("background",
			`linear-gradient(90deg, #${numRoutesRightColorOne}, #${numRoutesRightColorTwo})`);
	domObjects.rightBuses.rightBusesMin.html(`0`);
	domObjects.rightBuses.rightBusesMax.html(wafer.numBusesRightMax);
	
	domObjects.horizontalBuses.horizontalBusesGradient.css("background",
			`linear-gradient(90deg, #${numRoutesHorizontalColorOne}, #${numRoutesHorizontalColorTwo})`);
	domObjects.horizontalBuses.horizontalBusesMin.html(`0`);
	domObjects.horizontalBuses.horizontalBusesMax.html(wafer.numBusesHorizontalMax);
}
/**
 * Event handler for keyboard events
 */
function handleKeyDown(event) {
	const key = event.which;
	switch (key) {
		case 65: // a
			break;
		case 83: // s
			break;
	};
};
/**
 * Event handler for mouse-down event
 */
function handleMouseDown(event: JQueryMouseEventObject) {
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
	const hicann = mouseOverHicann(mouseDownPosition);
	if (hicann !== undefined) {
		hicannInfo.handleHicannClick(hicann, event.altKey, event.shiftKey);
	}
};
/**
 * Event handler for the mouse-up event
 * - in automode: switch to neighbor HICANN
 */
function handleMouseUp(event) {
	mouseIsDown = false;
	const positionDiff = {
		x: (event.clientX - mouseDownPosition.x),
		y: (event.clientY - mouseDownPosition.y)
	}
	if ((positionDiff.x !== 0) || (positionDiff.y !== 0)) {
		if (detailview.enabled && automode.enabled) {
			// horizontal movement
			if (positionDiff.x > 0) {
				if (detailview.westernHicannCloser(canvasCenter())) {
					hicannInfo.handleHicannClick(detailview.westernHicann);
					automode.startWesternHicann(detailview.currentHicann);
				};
			} else {
				if (detailview.easternHicannCloser(canvasCenter())) {
					hicannInfo.handleHicannClick(detailview.easternHicann);
					automode.startEasternHicann(detailview.currentHicann);
				}
			};
			// vertical movement
			if (positionDiff.y > 0) {
				if (detailview.northernHicannCloser(canvasCenter())) {
					hicannInfo.handleHicannClick(detailview.northernHicann);
					automode.startNorthernHicann(detailview.currentHicann);
				}
			} else {
				if (detailview.southernHicannCloser(canvasCenter())) {
					hicannInfo.handleHicannClick(detailview.southernHicann);
					automode.startSouthernHicann(detailview.currentHicann);
				};
			};
		};
	} else 	if (!tools.pointInRectangle({
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
};
/**
 * Event handler for the mouse-move event
 * - stage panning
 */
function handleMouseMove(event) {
	let newMousePosition = {
		x: <number>event.clientX,
		y: <number>event.clientY
	};

	if (mouseIsDown) {
		if (
			!tools.mouseInDiv({x: event.clientX, y: event.clientY}, "#routeInfoBox") &&
			!tools.mouseInDiv({x: event.clientX, y: event.clientY}, "#settingsBox") &&
			!tools.mouseInDiv({x:event.clientX, y: event.clientY}, "#summaryBox")
		) {
			let diff = {
				x:(newMousePosition.x - mousePosition.x),
				y:(newMousePosition.y - mousePosition.y),
			};
			// pan effect
			pixiBackend.moveStage(diff.x, diff.y);
		}
	} else {
		// display hicann number
		const hicannIndex = mouseOverHicann(mousePosition);
		if (hicannIndex !== undefined) {
			hicannNumber.handleMouseHover(hicannIndex);
		}
	};
	pixiBackend.renderer.render();

	mousePosition = newMousePosition;
};
 /**
	* check if mouse is over a HICANN
	*/
function mouseOverHicann(mousePosition: tools.Point) {
	for (let index=wafer.enumMin; index<=wafer.enumMax; index++) {
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

	const factor = Math.abs(event.deltaY/600) + 1;
	const transform: any = pixiBackend.container.stage.transform;
	const pixiScale = transform.scale.x;

	// limit zooming out
	if ((pixiScale <= reticlesOnStage.threshold) && (event.deltaY > 0)) {
		// show lookup plot (reticle & fpga coordinates)
		reticlesOnStage.setReticles(true);
		pixiBackend.renderer.render();

		// end handleWheel
		return "reached zoom limit"
	}

	if (reticlesOnStage.enabled && (event.deltaY < 0)) {
		// hide lookup plot
		reticlesOnStage.setReticles(false);
		pixiBackend.renderer.render();
	}

	if (
		!tools.mouseInDiv({x: event.clientX, y: event.clientY}, "#routeInfoBox") &&
		!tools.mouseInDiv({x: event.clientX, y: event.clientY}, "#settingsBox") &&
		!tools.mouseInDiv({x:event.clientX, y: event.clientY}, "#summaryBox")
	) {
		if (Math.abs(event.deltaY) !== event.deltaY) { //zoom in
			// zoom stage
			pixiBackend.zoomIn(factor, event.clientX, event.clientY);
			// auto mode
			if (automode.enabled) {
				// zoom into detail view
				if ((!detailview.enabled) && (pixiScale >= detailview.threshold) && (pixiScale < detailview.threshold2)) {
					// determine hicann in view
					const hicannIndex = hicannNumber.hicannIndex || detailview.hicannClosestToCenter(canvasCenter());
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
			} else {
				if ((pixiScale >= detailview.threshold) && (!detailview.enabled)) {
					manualmode.startDetailview();
				}
				if ((pixiScale >= detailview.threshold2) && (!detailview.levelTwoEnabled)) {
					manualmode.startDetailviewLevelTwo();
				}
			}
			// route width adjustment
			routesOnStage.adjustRouteWidth(pixiScale);
		} else { //zoom out
			// zoom stage
			pixiBackend.zoomOut(factor, event.clientX, event.clientY);
			// auto mode
			if (automode.enabled) {
				// zoom out of detailview level two
				if ((pixiScale < detailview.threshold2) && (pixiScale > detailview.threshold)) {
					automode.startDetailview(detailview.currentHicann, false);
				};
				// zoom out of detail view
				if ((pixiScale < detailview.threshold) && (detailview.enabled || detailview.levelTwoEnabled)) {
					automode.startOverview();
				};
			// manual mode
			} else {
				if ((pixiScale < detailview.threshold) && (detailview.enabled)) {
					manualmode.leaveDetailview();
				}
				if ((pixiScale < detailview.threshold2) && (detailview.levelTwoEnabled)) {
					manualmode.leaveDetailviewLevelTwo();
				}
			}
			// route width adjustment
			routesOnStage.adjustRouteWidth(pixiScale);
		};
	};

	pixiBackend.renderer.render();
}
/**
 * event handler for mobile touchmove event
 */
function handleTouchmove(event: any) {
	// TODO: don't pan or zoom when one touchpoint is in the #routeInfoBox
	for (let touch in event.touches) {
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

	let pixiScale = pixiBackend.container.stage.scale.x;
	let position = {
		x: event.pageX,
		y: event.pageY
	}
	let positionDiff = {
		x: position.x - touchPosition.x,
		y: position.y - touchPosition.y
	}
	pixiBackend.moveStage(positionDiff.x, positionDiff.y);

	if (event.touches.length == 2) {
		let distance = Math.sqrt((event.touches[0].pageX - event.touches[1].pageX)**2 + (event.touches[0].pageY - event.touches[1].pageY)**2);
		let previousDistance = Math.sqrt((touches[0].pageX - touches[1].pageX)**2 + (touches[0].pageY - touches[1].pageY)**2);
		let factor = (distance - previousDistance) / distance;
		let zoomCenter = {
			x: (touches[0].pageX + touches[1].pageX) / 2,
			y: (touches[0].pageY + touches[1].pageY) / 2
		}

		// limit zooming out
		if ((pixiScale <= reticlesOnStage.threshold) && (factor < 0)) {
			// show lookup plot (reticle & fpga coordinates)
			reticlesOnStage.setReticles(true);
			pixiBackend.renderer.render();

			// end handleWheel
			return "reached zoom limit"
		}

		if (reticlesOnStage.enabled && (factor > 0)) {
			// hide lookup plot
			reticlesOnStage.setReticles(false);
			pixiBackend.renderer.render();
		}

		if (factor > 0) { // zoom in
			// zoom stage
			pixiBackend.zoomIn(factor + 1, zoomCenter.x, zoomCenter.y)
			// auto mode
			if (automode.enabled) {
				// zoom into detail view
				if ((!detailview.enabled) && (pixiScale >= detailview.threshold) && (pixiScale < detailview.threshold2)) {
					// determine hicann in view
					let hicannIndex: number;
					if (pixiBackend.container.numberHover.children[0]) {
						// hicann number text
						const child = <PIXI.Text>pixiBackend.container.numberHover.children[0];
						hicannIndex = parseInt(child.text)
					} else {
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
			} else {
				if ((pixiScale >= detailview.threshold) && (!detailview.enabled)) {
					manualmode.startDetailview();
				}
				if ((pixiScale >= detailview.threshold2) && (!detailview.levelTwoEnabled)) {
					manualmode.startDetailviewLevelTwo();
				}
			}
			// route width adjustment
			routesOnStage.adjustRouteWidth(pixiScale);
		} else if (factor < 0) { // zoom out
			// zoom stage
			pixiBackend.zoomOut(Math.abs(factor) + 1, zoomCenter.x, zoomCenter.y)
			// auto mode
			if (automode.enabled) {
				// zoom out of detailview level two
				if ((pixiScale < detailview.threshold2) && (pixiScale > detailview.threshold)) {
					automode.startDetailview(detailview.currentHicann, false);
				};
				// zoom out of detail view
				if ((pixiScale < detailview.threshold) && (detailview.enabled || detailview.levelTwoEnabled)) {
					automode.startOverview();
				};
			// manual mode
			} else {
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
function handleTouchstart(event: any) {
	touchPosition = {
		x: event.pageX,
		y: event.pageY,
	}
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
	if (automode.enabled){
		automode.startOverview();
		automode.enabled = false;
	}
	for (let i=0; i<wafer.enumMax; i++) {
		$(`#hicanns_0_${i}_OV_checkbox`).siblings("ul").children("li").children("input").each(function() {
			if (!$(this).prop("checked")) {
				$(this).click()
			}
		})
		$(`#hicanns_0_${i}_DV_checkbox`).siblings("ul").children("li").children("input").each(function() {
			if ($(this).prop("checked")) {
				$(this).click()
			}
		})
	}
	manualmode.enabled = false;

	// reset routes
	$("#allRoutes").click(); // highlight all routes
	if ($("#routes_0_check").prop("checked")) {
		$("#routes_0_check").click();
	} else {
		$("#routes_0_check").click();
		$("#routes_0_check").click();
	};
	routesOnStage.handleRouteWidthSlider(2);
	const scale = pixiBackend.container.stage.scale.x;
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

function scrollToRoute(routeID: number) {
	routeID--;
	$(`#routes_0_${routeID}`)[0].scrollIntoView({
		behavior: "smooth",
		block: "start"
	});
	$(`#routes_0_${routeID}`).siblings("button")
	.css({
		"border" : "2px solid #fbb545",
		"border-radius" : "0.2rem"
	})
	.animate({ "border-color" : "rgba(0, 0, 0, 0)" }, 4000, function() {
		$(`#routes_0_${routeID}`).siblings("button")
			.css({ "border-width" : 0})
	});
}

function scrollToHicann(hicannIndex: number) {
	$(`#hicanns_0_${hicannIndex}`).siblings("button")[0].scrollIntoView({
		behavior: "smooth",
		block: "start"
	});
	$(`#hicanns_0_${hicannIndex}`).siblings("button")
	.css({
		"border" : "2px solid #fbb545",
		"border-radius" : "0.2rem"
	})
	.animate({ "border-color" : "rgba(0, 0, 0, 0)" }, 4000, function() {
		$(`#hicanns_0_${hicannIndex}`).siblings("button")
			.css({ "border-width" : 0})
	});
}
