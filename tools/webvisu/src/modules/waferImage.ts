/// <reference path="pixiBackend.ts" />
/// <reference path="wafer.ts" />
/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  
  /**
   * An image of the HICANN can be display in the background.
   * Those images can be set visible or hidden via a checkbox in the UI.
   */
  export class WaferImage {
    constructor(wafer: internalModule.Wafer, hicannImagePath: string, container: PIXI.Container, width: number, height: number) {
      this.wafer = wafer;
      this.hicannImagePath = hicannImagePath;
      this.container = container;
      this.width = width;
      this.height = height;
      
      // preload the hicann Image
      if (!PIXI.loader.resources.hasOwnProperty("img/hicann.png")) {
        PIXI.loader.add(hicannImagePath);
      }
    }

    wafer: internalModule.Wafer;
    hicannImagePath: string;
    /**
     * PixiJS container to hold the images.
     */
    container: PIXI.Container;
    /**
     * Width of the image. Should typically be the HICANN width.
     */
    width: number;
    /**
     * Height of the image. Should typically be the HICANN height.
     */
    height: number;

    /**
     * Draw the images for all HICANNs.
     */
    draw() {
      for (const hicann of this.wafer.hicanns) {
        // draw png-image of hicann
        pixiBackend.drawImage(this.container, this.hicannImagePath, hicann.position.x, hicann.position.y, this.width, this.height);
      }

      // render stage when hicann image finished loading
      PIXI.loader.load(() => {
        pixiBackend.renderer.render();
      })
    }

    /**
     * Set the images to visible or hide them.
     */
    setVisible(visible: boolean) {
      // set pixiJS container property
      this.container.visible = visible;
    }
  }
}