/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  export class Summary {
    constructor() {
      $("#summary").click(() => {
				if ($("#summaryBox").css("display") === "none") {
          $("#summaryBox").css("display", "initial");
          $("#summary").addClass("infoBoxSelected");
				} else {
          $("#summaryBox").css("display", "none");
          $("#summary").removeClass("infoBoxSelected");
				}
			})
      this.updateSummaryContent();
    }

    open = () => {
      $("#summaryBackground").css("display", "initial");
    };

    close = () => {
      $("#summaryBackground").css("display", "none");
    };

    checkSymbol = () => {
      $("#settingDescription").css("display", "none");
      $("#checkSymbol").css("display", "block");
      setTimeout(() => {
        $("#checkSymbol").fadeOut(500);
      }, 2000)
    };

    updateSummaryContent = () => {
      // total number of neurons;
      let numNeurons = 0;
      for (const hicann of wafer.hicanns) {
        numNeurons += hicann.numNeurons;
      }
      $("#summaryNeurons").html(`${numNeurons}`);
      // total number of utilized HICANN chips
      let numHicanns = 0;
      for (const hicann of wafer.hicanns) {
        if (hicann.hasNeurons || hicann.numBusesVertical || hicann.numBusesHorizontal) numHicanns++;
      }
      $("#summaryHicanns").html(`${numHicanns}`);
      // total number of utilized FPGAs
      let numFpgas = 0;
      // total number of routes
      const numRoutes = routesOnStage.routes.length;
      $("#summaryRoutes").html(`${numRoutes}`);
    };
  } // class Summary
} // internalModule