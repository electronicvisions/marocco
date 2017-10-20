/// <reference path="module.d.ts" />

/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  /**
   * Representation of a HICANN. Position is the position in the visualization and is added when the HICANN is drawn the first time.
   */
  export class HICANN {
    constructor(
      index: number,
      x: number,
      y: number,
      hasInputs: boolean,
      hasNeurons: boolean,
      isAvailable: boolean,
      numBusesHorizontal: number,
      numBusesLeft: number,
      numBusesRight: number,
      numBusesVertical: number,
      numInputs: number,
      numNeurons: number
    ) {
      this.index = index;
      this.x = x;
      this.y = y;
      this.position = {
        x: undefined,
        y: undefined
      };
      this.hasInputs = hasInputs;
      this.hasNeurons = hasNeurons;
      this.isAvailable = isAvailable;
      this.numBusesHorizontal = numBusesHorizontal;
      this.numBusesLeft = numBusesLeft;
      this.numBusesRight = numBusesRight;
      this.numBusesVertical = numBusesVertical;
      this.numInputs = numInputs;
      this.numNeurons = numNeurons;
    }
    index: number;
    x: number;
    y: number;
    position: {
      x: number,
      y: number
    };
    hasInputs: boolean;
    hasNeurons: boolean;
    isAvailable: boolean;
    numBusesHorizontal: number;
    numBusesLeft: number;
    numBusesRight: number;
    numBusesVertical: number;
    numInputs: number;
    numNeurons: number;
  }
  /**
   * Representation of a HICANN wafer. The data is parsed from a configuration xml-file using the Marocco JavaScript API.
   */
  export class Wafer {
    constructor() {
      this.marocco = undefined;
      // hardcode some wafer properties, because they are not yet wrapped.
      this.hicanns = [];
      this.enumMin = 0;
      this.enumMax = 383;
      this.xMin = 0;
      this.xMax = 35;
      this.yMin = 0;
      this.yMax = 15;
      // settings for HICANN proportions, try to fit graphics to HICANN image
      this.hicannMargin = 0;
      this.hicannWidth = 100;
      this.hicannHeight = 200;
      this.inputTriangleHeight = 30;
      this.busesLeftPosition = {
        x: 4/100 * this.hicannWidth,
        y: 14.5/200 * this.hicannHeight,
        width: 12/100 * this.hicannWidth,
        height: (this.hicannHeight - 2*14.5)/200 * this.hicannHeight
      };
      this.busesRightPosition = {
        x: (this.hicannWidth - 4 - 12)/100 * this.hicannWidth,
        y: 14.5/200 * this.hicannHeight,
        width: 12/100 * this.hicannWidth,
        height: (this.hicannHeight - 2*14.5)/200 * this.hicannHeight
      };
      this.busesHorizontalPosition = {
        original: {
          x: 19.8/100 * this.hicannWidth,
          y: 97/200 * this.hicannHeight,
          width: 60.6/100 * this.hicannWidth,
          height: 6.2/200 * this.hicannHeight
        },
        current: {
          x: 19.8/100 * this.hicannWidth,
          y: 97/200 * this.hicannHeight,
          width: 60.6/100 * this.hicannWidth,
          height: 6.2/200 * this.hicannHeight
        }
      };
      this.synapseArraysTopPosition = {
        x: 23.5/100 * this.hicannWidth,
        y: this.busesLeftPosition.y,
        width: this.hicannWidth - (2 * 23.5/100 * this.hicannWidth),
        height: 71.5/200 * this.hicannHeight,
      };
      this.synapseArraysBottomPosition = {
        x: 23.5/100 * this.hicannWidth,
        y: this.busesLeftPosition.y + this.busesLeftPosition.height - this.synapseArraysTopPosition.height,
        width: this.synapseArraysTopPosition.width,
        height: this.synapseArraysTopPosition.height,
      };
      this.synDriverPosition = {
        topLeft: {
          x: this.synapseArraysTopPosition.x - 1/100 * this.hicannWidth,
          y: this.synapseArraysTopPosition.y,
        },
        topRight: {
          x: this.synapseArraysTopPosition.x + this.synapseArraysBottomPosition.width,
          y: this.synapseArraysTopPosition.y,
        },
        bottomLeft: {
          x: this.synapseArraysBottomPosition.x - 1/100 * this.hicannWidth,
          y: this.synapseArraysBottomPosition.y,
        },
        bottomRight: {
          x: this.synapseArraysBottomPosition.x + this.synapseArraysBottomPosition.width,
          y: this.synapseArraysBottomPosition.y,
        },
        width: 1/100 * this.hicannWidth,
        height: this.synapseArraysTopPosition.height,
      };
      this.neuronArrayPosition = {
        top: {
          x: this.synapseArraysTopPosition.x,
          y: this.synapseArraysTopPosition.y + this.synapseArraysTopPosition.height + 0.5/200 * this.hicannHeight,
        },
        bottom: {
          x: this.synapseArraysBottomPosition.x,
          y: this.synapseArraysBottomPosition.y - (3/200 + 0.5/200) * this.hicannHeight,
        },
        width: this.synapseArraysTopPosition.width,
        height: 3/200 * this.hicannHeight
      };
      this.repeaterBlockPosition = {
        horizontal: {
          left: {
            original: {
              x: 1/100 * this.hicannWidth,
              y: 87.5/200 * this.hicannHeight
            },
            current: {
              x: 1/100 * this.hicannWidth,
              y: 87.5/200 * this.hicannHeight
            }
          },
          right: {
            original: {
              x: (1 - 4/100) * this.hicannWidth,
              y: 87.5/200 * this.hicannHeight
            },
            current: {
              x: (1 - 4/100) * this.hicannWidth,
              y: 87.5/200 * this.hicannHeight
            }
          },
          width: 3/100 * this.hicannWidth,
          height: {
            original: 25/200 * this.hicannHeight,
            current: 25/200 * this.hicannHeight
          }
        },
        vertical: {
          top: {
            left: {
              original: {
                x: 0.8/100 * this.hicannWidth,
                y: 1/200 * this.hicannHeight
              },
              current: {
                x: 0.8/100 * this.hicannWidth,
                y: 1/200 * this.hicannHeight
              }
            },
            right: {
              original: {
                x: 50/100 * this.hicannWidth,
                y: 1/200 * this.hicannHeight
              },
              current: {
                x: 50/100 * this.hicannWidth,
                y: 1/200 * this.hicannHeight
              }
            }
          },
          bottom: {
            left: {
              original: {
                x: 0.8/100 * this.hicannWidth,
                y: (1 - 3/200) * this.hicannHeight
              },
              current: {
                x: 0.8/100 * this.hicannWidth,
                y: (1 - 3/200) * this.hicannHeight
              }
            },
            right: {
              original: {
                x: 50/100 * this.hicannWidth,
                y: (1 - 3/200) * this.hicannHeight
              },
              current: {
                x: 50/100 * this.hicannWidth,
                y: (1 - 3/200) * this.hicannHeight
              }
            }
          },
          width: {
            original: ((50 - 0.8)/100) * this.hicannWidth,
            current: ((50 - 0.8)/100) * this.hicannWidth
          },
          height: 2/200 * this.hicannHeight
        }
      }
      this.numNeuronsMax = 0;
      this.numInputsMax = 0;
      this.numBusesLeftMax = 0;
      this.numBusesRightMax = 0;
      this.numBusesHorizontalMax = 0;
    }
    /**
     * Instance of the C++ Marocco::results::marocco class wrapped to JavaScript via emscripten.
     * This API is used to retrieve all the information from the results file, that is manually uploaded when starting the app.
     */
    marocco: Module.Marocco;
    /**
     * List of all HICANNs. The position is added, when the HICANN is drawn the first time.
     */
    hicanns: HICANN[];
    enumMin: number;
    enumMax: number;
    xMin: number;
    xMax: number;
    yMin: number;
    yMax: number;
    /**
     * Space between neighboring HICANNs.
     */
    hicannMargin: number;
    /**
     * Width of HICANNs on the stage.
     */
    hicannWidth: number;
    /**
     * Height of HICANNs on the stage.
    */
    hicannHeight: number;
    /**
     * Height of the Triangle representing the number of inputs.
     */
    inputTriangleHeight: number;
    /**
     * Position of the left Bus segment (all left buses together) relative to HICANN.
     */
    busesLeftPosition: tools.Box;
    /**
     * Position of the right Bus segment (all right buses together) relative to HICANN.
     */
    busesRightPosition: tools.Box;
    /**
     * Position of the horizontal Bus segment (all horizontal buses together) relative to HICANN.
     */
    busesHorizontalPosition: {
      original: tools.Box,
      current: tools.Box
    }
    /**
     * Position of the top synapse array relative to HICANN.
     */
    synapseArraysTopPosition: tools.Box;
    /**
     * Position of the bottom synapse array relative to HICANN.
     */
    synapseArraysBottomPosition: tools.Box;
    /**
     * Positions of the synapse drivers arrays relative to HICANN
     */
    synDriverPosition: {topLeft: tools.Point, topRight: tools.Point, bottomLeft: tools.Point, bottomRight: tools.Point, width: number, height: number};
    /**
     * Positions of the top and bottom neuron arrays relative to HICANN
     */
    neuronArrayPosition: {top: tools.Point, bottom: tools.Point, width: number, height: number};
    /**
     * Positions of the vertical and horizontal repeater blocks relative to HICANN
     */
    repeaterBlockPosition: {
      horizontal: {
        left: {original: tools.Point, current: tools.Point}, right: {original: tools.Point, current: tools.Point},
        width: number, height: {original: number, current: number}
      },
      vertical: {
        top: {left: {original: tools.Point, current: tools.Point}, right: {original: tools.Point, current: tools.Point}},
        bottom: {left: {original: tools.Point, current: tools.Point}, right: {original: tools.Point, current: tools.Point}},
        width: {original: number, current: number}, height: number
      }
    }
    /**
     * Position of the right synapse drivers array relative to HICANN
     */
    synDriverRightPosition: tools.Box;
    /**
		 * Maximum number of neurons on any HICANN of the wafer.
		 */
    numNeuronsMax: number;
    /**
		 * Maximum number of inputs on any HICANN of the wafer.
		 */
    numInputsMax: number;
    /**
		 * Maximum number of left buses on any HICANN of the wafer.
		 */
    numBusesLeftMax: number;
    /**
		 * Maximum number of right buses on any HICANN of the wafer.
		 */
    numBusesRightMax: number;
    /**
		 * Maximum number of horizontal buses on any HICANN of the wafer.
		 */
    numBusesHorizontalMax: number;

    /**
     * process the marocco results file using the Marocco JavaScript API.
     * - new instances of HICANN are created
     * @param networkFilePath path to the results file, located in the virtual emscripten filesystem
     */
    loadOverviewData(networkFilePath?: string) {
      if (networkFilePath) {
        try {
          this.marocco = Module.Marocco.from_file(networkFilePath);
        } catch (error) {
          this.marocco = new Module.Marocco();
        }
      } else {
        this.marocco = new Module.Marocco();
      }

      $("#setupScreen").fadeTo(1500, 0, () => {$("#setupScreen").css("display", "none")})
      // reading properties from marocco
      for (let i=this.enumMin; i<=this.enumMax; i++) {
        const enumRanged = new Module.HICANNOnWafer_EnumRanged_type(i)
        const hicann = new Module.HICANNOnWafer(enumRanged);
        const properties = this.marocco.properties(hicann);

        this.hicanns.push(new HICANN(
          i,
          hicann.x().value(),
          hicann.y().value(),
          properties.has_inputs(),
          properties.has_neurons(),
          properties.is_available(),
          properties.num_buses_horizontal(),
          properties.num_buses_left(),
          properties.num_buses_right(),
          properties.num_buses_vertical(),
          properties.num_inputs(),
          properties.num_neurons(),
        ));
      }

      this.maxPropertyValues();
    }

    /**
     * Find out the maximum values for HICANN properties
     */
    maxPropertyValues() {
      for (const hicann of this.hicanns) {
        if (hicann.numNeurons > this.numNeuronsMax) {
          this.numNeuronsMax = hicann.numNeurons;
        }
        if (hicann.numInputs > this.numInputsMax) {
          this.numInputsMax = hicann.numInputs;
        }
        if (hicann.numBusesLeft > this.numBusesLeftMax) {
          this.numBusesLeftMax = hicann.numBusesLeft;
        }
        if (hicann.numBusesRight > this.numBusesRightMax) {
          this.numBusesRightMax = hicann.numBusesRight;
        }
        if (hicann.numBusesHorizontal > this.numBusesHorizontalMax) {
          this.numBusesHorizontalMax = hicann.numBusesHorizontal;
        }
      }
    }

    /**
     * Calculate the index/enum-coordinate of the northern HICANN, if existent.
     */
    northernHicann(hicannIndex: number) {
      let northernHicann: HICANN;
      for (let hicann of this.hicanns) {
        if ((hicann.y == this.hicanns[hicannIndex].y - 1)
            && (hicann.x == this.hicanns[hicannIndex].x)) {
          northernHicann = hicann;
          break;
        };
      };
      return(northernHicann ? northernHicann.index : undefined);
    }

    /**
     * Calculate the index/enum-coordinate of the southern HICANN, if existent.
     */
    southernHicann(hicannIndex: number) {
      let southernHicann: HICANN;
      for (let hicann of this.hicanns) {
        if ((hicann.y == this.hicanns[hicannIndex].y + 1)
            && (hicann.x == this.hicanns[hicannIndex].x)) {
          southernHicann = hicann;
          break;
        };
      };
      return(southernHicann ? southernHicann.index : undefined);
    }

    /**
     * Calculate the index/enum-coordinate of the eastern HICANN, if existent.
     */
    easternHicann(hicannIndex: number) {
      let easternHicann: HICANN;
      for (let hicann of this.hicanns) {
        if ((hicann.y == this.hicanns[hicannIndex].y)
            && (hicann.x == this.hicanns[hicannIndex].x + 1)) {
          easternHicann = hicann;
          break;
        };
      };
      return(easternHicann ? easternHicann.index : undefined);
    }

    /**
     * Calculate the index/enum-coordinate of the western HICANN, if existent.
     */
    westernHicann(hicannIndex: number) {
      let westernHicann: HICANN;
      for (let hicann of this.hicanns) {
        if ((hicann.y == this.hicanns[hicannIndex].y)
            && (hicann.x == this.hicanns[hicannIndex].x - 1)) {
          westernHicann = hicann;
          break;
        };
      };
      return(westernHicann ? westernHicann.index : undefined);
    }
  };
}