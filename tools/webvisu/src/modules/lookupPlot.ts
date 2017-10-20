/// <reference path="overview.ts" />

/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  /**
   * Calculate the coordinate of the top left HICANN of a reticle.
   */
  function topLeftHicannInReticle(reticleCoord: number) {
    return (reticleCoord + reticlesInFullRows(row(reticleCoord + 1) - 1))*4;
  }

  /**
   * Calculate the row of a reticle. (row is not the HICANN y coordinate!)
   */
  function row(reticleCoord: number) {
    let row = <number>undefined;
  
    if (reticleCoord <= 3) {
      row = 1;
    } else if (reticleCoord <= 8) {
      row = 2;
    } else if (reticleCoord <= 15) {
      row = 3;
    } else if (reticleCoord <= 24) {
      row = 4;
    } else if (reticleCoord <= 33) {
      row = 5;
    } else if (reticleCoord <= 40) {
      row = 6;
    } else if (reticleCoord <= 45) {
      row = 7;
    } else if (reticleCoord <= 48) {
      row = 8
    } else {
      throw(new Error("reticle coordinate out of range"))
    };
  
    return row;
  }
  
  /**
   * Calculate the number of reticles in all the rows up to the one passed as argument.
   */
  function reticlesInFullRows(row: number) {
    if (row <= 4) {
      return 2*tools.kleinerGauss(row-1) + 3*row;
    } else if (row <= 8) {
      return 2 * (2*tools.kleinerGauss(3) + 12) - (2*tools.kleinerGauss(7-row) + 3*(8 - row));
    } else {
      throw (new Error("row out of range"))
    }
  }

  /**
   * Convert a reticle coordinate into the respective fpga coordinate.
   */
  function fpgaCoords(reticleCoord) {
    const fpgaCoords = [12, 13, 11, 16, 14, 15, 10, 9, 20, 17, 19, 7, 6, 8, 3, 22, 21, 23, 18, 5, 4,
        0, 2, 1, 25, 26, 24, 28, 43, 42, 47, 45, 46, 27, 29, 30, 41, 40, 38, 44, 31, 32, 39, 37, 36, 33, 34, 35];

    return fpgaCoords[reticleCoord];
  }

  /**
   * The reticle class stores information about the graphical representation of the reticle.
   */
  class Reticle {
    constructor(reticleCoord: number, fpgaCoord: number, x: number, y: number, width: number, height: number, color: string) {
      this.reticleCoord = reticleCoord;
      this.fpgaCoord = fpgaCoords(reticleCoord);
      this.position = {
        x: x,
        y: y,
        width: width,
        height: width,
      }
      this.color = color;
    }
  
    reticleCoord: number;
    fpgaCoord: number;
    position: tools.Box;
    color: string;
  }
  
  /**
   * ReticlesOnStage controls the visualization of the lookup plot including reticle and fpga coordinates.
   */
  export class ReticlesOnStage {
    constructor(overview: internalModule.Overview, container: PIXI.Container) {
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
  
    overview: internalModule.Overview;
    wafer: internalModule.Wafer;
    /**
     * PixiJS container to store the graphics elements.
     */
    container: PIXI.Container;
    /**
     * All the reticles that get drawn.
     */
    reticles: Reticle[];
    /**
     * Total number of reticles
     */
    numReticles: number;
    /**
     * 
     */
    threshold: number;

    /**
     * Set this property to make the graphics visible or hide them.
     */
    
    get enabled() {
      return this.container.visible;
    }

    set enabled(enabled: boolean) {
      this.container.visible = enabled;
    }

    /**
     * Calculate the positions for all reticles on the wafer and instantiate new reticle classes
     */
    buildReticles() {
      for (let reticleCoord=0; reticleCoord<this.numReticles; reticleCoord++) {
        // hicann in top left corner of reticle
        const hicannTopLeft = topLeftHicannInReticle(reticleCoord);
        
        // reticle position x
        const x = this.wafer.hicanns[hicannTopLeft].position.x - this.wafer.hicannMargin/2;

        // reticle position y
        const y = this.wafer.hicanns[hicannTopLeft].position.y - this.wafer.hicannMargin/2;

        // reticle width
        const width = 4 * (this.wafer.hicannWidth + this.wafer.hicannMargin);

        // reticle height
        const height = 2 * (this.wafer.hicannHeight + this.wafer.hicannMargin);

        // reticle color
        const color = "0xffffff";

        // store reticle
        this.reticles.push(new Reticle(reticleCoord, 3, x, y, width, height, color));
      }
    }

    /**
     * Draw reticle boundaries as well as reticle and fpga coordinates as text in the visualization
     */
    drawReticles() {
      // style for reticle and fpga coordinate
      const reticleStyle = {
				font: 'normal 100px Arial',
				fill: "0x177076"
      }

      const fpgaStyle = {
				font: 'normal 100px Arial',
				fill: "0xff581e"
      }
      
      // draw reticles
      for (const reticle of this.reticles) {
        // draw background rectangle
        const graphicsObject = pixiBackend.drawRectangles(
            [reticle.position.x], [reticle.position.y], [reticle.position.width], [reticle.position.height],
            reticle.color, new PIXI.Graphics(), 1, 10, "0x000000");
        pixiBackend.storeGraphics(graphicsObject, this.container);

        // draw reticle (=DNC) coordinate
        pixiBackend.drawTextInRectangle(this.container,
            reticle.position.x, reticle.position.y, reticle.position.width, reticle.position.height/2,
            "F " + reticle.fpgaCoord.toString(), reticleStyle);
        
        // draw FPGA coordinate
        pixiBackend.drawTextInRectangle(this.container,
            reticle.position.x, reticle.position.y + reticle.position.height/2, reticle.position.width, reticle.position.height/2,
            "D " + reticle.reticleCoord.toString(), fpgaStyle);
      }

      // render stage
      pixiBackend.renderer.render();
    }

    /**
     * Pass "true" to make the reticles visible and "false" to hide them
     */
    setReticles(visible: boolean) {
      // set pixiJS container visibility
      this.enabled = visible;

      // set UI reticle checkbox
      this.setCheckbox(visible);
    }

    /**
     * Set the UI checkbox
     */
    setCheckbox(checked: boolean) {
      // set UI reticle checkbox
      $("#reticlesCheckbox").prop("checked", checked)
    }

    
  }
}