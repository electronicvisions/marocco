namespace internalModule {
  export class HicannInfo {
    constructor() {
      this.hicanns = [];
    }
    hicanns: number[];

    toggleProperties(visible: boolean) {
      $("#hicannProperties").css("display", visible ? "initial" : "none");
    };

    handleHicannClick(hicann: number, select=false, selectReticle=false) {
      if (!select) {
        if (this.hicanns.length === 0) {
          this.displayProperties(hicann);
        }
      } else {
        const hicanns = [hicann];
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
        for (const hicann of hicanns) {
          if (this.hicanns.indexOf(hicann) === -1) {
            this.hicanns.push(hicann);
          } else {
            this.hicanns.splice(this.hicanns.indexOf(hicann), 1);
          }
        }
        if (this.hicanns.length === 0) {
          this.displayProperties(hicann);
          this.toggleProperties(true);
          $("#clearSelection").remove();
        } else {
          this.toggleProperties(false);
          $("#clearSelection").remove();
          const hicannsString = this.hicanns.toString().split(",").join(", ");
          const clearButton = $(`<button id="clearSelection">clear selection</button>`)
          .click(() => {
            const lastHicann = this.hicanns[this.hicanns.length - 1]
            this.hicanns = [];
            this.drawSelectionSymbols(this.hicanns);
            this.displayProperties(lastHicann);
            this.toggleProperties(true);
            $("#clearSelection").remove();
          })
          $("#hicannNumber")
            .html(`HICANNs: ${hicannsString}`)
            .after(clearButton);
        }
        this.drawSelectionSymbols(this.hicanns);

      }
    };

    /**
     * Show Hicann properties in left info box.
     */
    displayProperties(hicannIndex) {
      $("#hicannNumber").html(`HICANN ${hicannIndex}`)
      $("#neuronsNumber").html(`${wafer.hicanns[hicannIndex].numNeurons}`);
      $("#inputsNumber").html(`${wafer.hicanns[hicannIndex].numInputs}`);
      $("#leftBusesNumber").html(`${wafer.hicanns[hicannIndex].numBusesLeft}`);
      $("#rightBusesNumber").html(`${wafer.hicanns[hicannIndex].numBusesRight}`);
      $("#horizontalBusesNumber").html(`${wafer.hicanns[hicannIndex].numBusesHorizontal}`);
    }

    drawSelectionSymbols(hicanns: number[]) {
      pixiBackend.removeAllChildren(pixiBackend.container.hicannSelection);
      if (hicanns.length !== 0) {
        let graphicsObject = <PIXI.Graphics>undefined;
        for (const hicann of hicanns) {
          const hicannPosition = wafer.hicanns[hicann].position;
          graphicsObject = pixiBackend.drawRectangles([hicannPosition.x], [hicannPosition.y], [wafer.hicannWidth], [wafer.hicannHeight], 0xffffff, graphicsObject, 0.3);
          pixiBackend.drawImage(pixiBackend.container.hicannSelection, "img/select.png", hicannPosition.x + 0.1*wafer.hicannWidth, hicannPosition.y + 0.1*wafer.hicannWidth, 0.2*wafer.hicannWidth, 0.2*wafer.hicannWidth);
        }
        pixiBackend.storeGraphics(graphicsObject, pixiBackend.container.hicannSelection);
      }
      pixiBackend.renderer.render();
    }
  }
}