namespace internalModule {
  /**
   * The HICANN number and a border around the HICANN
   * can be drawn for all HICANNs at once (like a lookup plot)
   * and additionally for the HICANN, where the mouse is hovering over.
   * This is controlled by checkboxes.
   */
  export class HicannNumber {
    constructor(width: number, height: number) {
      this.style = {
        font: 'bold 100px Arial',
        fill: pixiBackend.renderer.backgroundColor
      }
      this.styleHighlighted = {
        font: 'bold 100px Arial',
        fill: 0xfbb535
      }
      this.hicannIndex = undefined;
      this.width = width;
      this.height = height;
      this.showAll = false;
      this.showHover = true;
      this.containerAll = pixiBackend.container.numberAll;
      this.containerHover = pixiBackend.container.numberHover;
      
      this.drawAll();
      this.setAll(false);
    }
  
    hicannIndex: number;
    /**
     * Style for the HICANN number text.
     */
    style: {
      font: string,
      fill: number
    };
    /**
     * Style for the HICANN number text when it is highlighted.
     */
    styleHighlighted: {
      font: string,
      fill: number
    }
    /**
     * Width of the number. Typically the width of the HICANN.
     */
    width: number;
    height: number;
    /**
     * state variable for showing all HICANN numbers
     */
    showAll: boolean;
    /**
     * state variable for showing HICANN number when hovering
     */
    showHover: boolean;
    /**
     * pixiJS container for all numbers (but not the hovering ones)
     */
    containerAll: PIXI.Container;
    /**
     * pixiJS container for the hovering number
     */
    containerHover: PIXI.Container;

    handleMouseHover(hicannIndex: number) {
      if (hicannIndex !== this.hicannIndex) {
        // draw new hicann number
        this.drawHover(hicannIndex);
      };
      this.hicannIndex = hicannIndex;
    }
  
    /**
     * Set the visibility for all numbers.
     */
    setAll(visible: boolean) {
      this.showAll = visible;
      this.containerAll.visible = visible;
      pixiBackend.renderer.render();
    };
  
    /**
     * Set the visibility for the hovering number.
     */
    setHover(visible: boolean) {
      this.showHover = visible;
      pixiBackend.renderer.render();
    }
    
    /**
     * draw all HICANN numbers into one container
     */
    drawAll() {
      for (let i=wafer.enumMin; i<=wafer.enumMax; i++) {
        const position = wafer.hicanns[i].position;
        this.draw(i, position.x, position.y, false, true);
      }
    }

    /**
     * Draw the HICANN number, when hovering over a HICANN with the mouse
     */
    drawHover(number: number) {
      // remove possible previous numbers
      this.clean();
      this.hicannIndex = number;
  
      if (this.showHover) {
        const position = wafer.hicanns[number].position;
        this.draw(number, position.x, position.y, this.showAll);
      }
    }
  
    /**
     * Draw the HICANN number.
     * @param number Index of the HICANN.
     * @param x x-position for the text.
     * @param y y-position for the text.
     */
    draw(number: number, x: number, y: number, highlighted=false, all=false) {
      const style = highlighted ? this.styleHighlighted : this.style;
      const container = all ? this.containerAll : this.containerHover;
      // draw new number;
      pixiBackend.drawTextInRectangle(container, x + 0.1*this.width, y, 0.8*this.width, this.height, number.toString(), style);
      // draw rectangle border around HICANN
      pixiBackend.drawRectangleBorder(container, x, y, this.width, this.height, 5, style.fill, 1);
    };
    
  
    /**
     * remove all text-graphic objects from the container.
     */
    clean() {
      pixiBackend.removeAllChildren(this.containerHover);
    };
  
    /**
     * update the UI checkboxes.
     */
    updateCheckboxes() {
      $("#allNumbersCheckbox").prop("checked", this.showAll);
      $("#numberHoverCheckbox").prop("checked", this.showHover);
    };

    /**
     * Hide everything
     */
    disable() {
      this.containerAll.visible = false;
      this.containerHover.visible = false;
    }

    /**
     * Make visible again (if the respective checkboxes are checked).
     */
    recover() {
      this.containerAll.visible = this.showAll;
      this.containerHover.visible = true;
    }
  }
}
