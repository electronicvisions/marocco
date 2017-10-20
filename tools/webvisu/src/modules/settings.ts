/**
 * The namespace contains a number of classes that each have their separate purposes but have dependencies on each other.
 * They are written into separate files to keep a clear structure.
 */
namespace internalModule {
  export class Settings {
    constructor() {
			$("#settings").click(() => {
				if ($("#settingsBox").css("display") === "none") {
          $("#settingsBox").css("display", "initial");
          $("#settings").addClass("infoBoxSelected");
				} else {
          $("#settingsBox").css("display", "none");
          $("#settings").removeClass("infoBoxSelected");
				}
			})
      $("#resetConfig").click(() => {
        reset();
        this.checkSymbol();
      });
      $("#saveConfig").click(() => {
        this.saveConfig(true);
      });
      $("#uploadConfig").click(() => {
        this.uploadConfig();
      });
      $("#reloadFile").click(() => {
        this.reloadFile();
      });

      $(".settingDescription").children().eq(0).html(this.longestDescription());
      
      $("#resetConfig").mouseenter(() => {
        $(".settingDescription").children().eq(1).stop(true).html(this.resetDescription).fadeIn(100);
      });
      $("#saveConfig").mouseenter(() => {
        $(".settingDescription").children().eq(1).stop(true).html(this.saveDescription).fadeIn(100);
      });
      $("#uploadConfig").mouseenter(() => {
        $(".settingDescription").children().eq(1).stop(true).html(this.uploadDescription).fadeIn(100);
      });
      $("#reloadFile").mouseenter(() => {
        $(".settingDescription").children().eq(1).stop(true).html(this.reloadFileDescription).fadeIn(100);
      });
      $("#resetConfig").mouseleave(() => {
        $(".settingDescription").children().eq(1).fadeOut(100);
      });
      $("#saveConfig").mouseleave(() => {
        $(".settingDescription").children().eq(1).fadeOut(100);
      });
      $("#uploadConfig").mouseleave(() => {
        $(".settingDescription").children().eq(1).fadeOut(100);
      });
      $("#reloadFile").mouseleave(() => {
        $(".settingDescription").children().eq(1).fadeOut(100);
      });

      $("#numberHoverCheckbox").change((e) => {
        const checked = ((e || window.event).target as HTMLInputElement).checked;
        hicannNumber.setHover(checked);
      })

      $("#captureScreenshot").click(() => {
        let input = Number($("#screenshotResolution").val());
        if (input === 0) {input = 1};
        if (isNaN(input)) {
          alert("please enter a number");
        } else {
          this.screenshot(input);
        }
      })
    }

    configuration: any
    resultsFile = <string>undefined;

    resetDescription = "Reset the visualization to the initial state.";
    saveDescription = "Save the current state of the visualization as an external file.";
    uploadDescription = "Upload an external configuration file for the visualization.";
    reloadFileDescription = "Upload a different results file. The visu configuration will be automatically restored."
    longestDescription = () => {
      let length = 0;
      let longest = <string>undefined;
      for (const description of [this.resetDescription, this.saveDescription, this.uploadDescription, this.reloadFileDescription]) {
        if (description.length > length) {
          length = description.length;
          longest = description;
        }
      }
      return longest;
    }

    visualizeFile: (resultsFile: string) => void;
    reset: () => void;

    open = () => {
      $("#settingsBackground").css("display", "initial");
    };

    close = () => {
      $("#settingsBackground").css("display", "none");
    };

    checkSymbol = () => {
      $(".settingDescription").children().eq(1).css("display", "none");
      $("#checkSymbol").css("display", "block");
      setTimeout(() => {
        $("#checkSymbol").fadeOut(500);
      }, 2000)
    }

    reloadFile = () => {
      // create hidden file browser
      const fileBrowser = $(`<input type="file"/>`).prop("display", "none");
      $("body").append(fileBrowser);
    
      fileBrowser.change(() => {
        // uploaded configuration file
        const configfile = fileBrowser.prop("files")[0];
        const filereader = new FileReader();
        // handle the file
        filereader.onload = (event) => {
          // show loading wheel;
          $("#configurationContent .loader").css("display", "block");

          const contents = new Int8Array((<any>event.target).result);
          // file name is "network" + extension of the input file
          const filename = "./network" + configfile.name.match(/\.[a-z,A-Z,0-9,\.]+/)[0];
          // delete old results file
          if (this.resultsFile) {
            FS.unlink(this.resultsFile);
          }
          // write file into emscriptens virtual file system (FS)
          FS.writeFile(filename, contents);
          // save 
          this.resultsFile = filename;
  
          // start main program
          setTimeout(() => {
            this.saveConfig();
            removeVisualization();
            visualizeFile(filename)
            this.setConfig(this.configuration);
            // hide loading wheel
            $("#configurationContent .loader").css("display", "none");
            // show check symbol
            this.checkSymbol();
          }, 100);
        }
        filereader.readAsArrayBuffer(configfile);
    
        fileBrowser.remove();
      })
      // open file browser
      fileBrowser.click();
    }

    setConfig = (config) => {
      // reset visu
      reset();
      // position and scale
      pixiBackend.container.stage.position.x = config.position.x;
      pixiBackend.container.stage.position.y = config.position.y;
      pixiBackend.container.stage.scale.x = config.position.scale;
      pixiBackend.container.stage.scale.y = config.position.scale;
      // horizontal bus segment width
      wafer.busesHorizontalPosition = config.busesHorizontalPosition;
      // redraw overview horizontal buses
      pixiBackend.removeAllChildren(pixiBackend.container.overviewBusesHorizontal);
      for (let i=wafer.enumMin; i<=wafer.enumMax; i++) {
        overview.drawHorizontalBusSegment(i, wafer.hicanns[i].position.x, wafer.hicanns[i].position.y)
      }
      // route width
      routesOnStage.adjustRouteWidth(config.position.scale)
      pixiBackend.renderer.render();
      // detail level
      detailview.enabled = config.levelOneEnabled;
      detailview.levelTwoEnabled = config.levelTwoEnabled;
      // enter manualmode
      $("#manualmode").click();
      // restore overview and detailview checkboxes
      for (let i=0; i<wafer.enumMax; i++) {
        $(`#hicanns_0_${i}_OV_checkbox`).siblings("ul").children("li").children("input").each(function(index) {
          if ($(this).prop("checked") !== config.hicannCheckboxes[i].OV[index]) {
            $(this).click();
          }
        })
        $(`#hicanns_0_${i}_DV_checkbox`).siblings("ul").children("li").children("input").each(function(index) {
          if ($(this).prop("checked") !== config.hicannCheckboxes[i].DV[index]) {
            $(this).click();
          }
        })
      }
      // back to automode
      if (config.automode) {
        $("#automode").click();
      };
      // hicann images
      if (config.hicannImages !== $("#waferImageCheckbox").prop("checked")) {
        $("#waferImageCheckbox").click();
      };
      // routes
      if (config.routes) {
        // visible routes
        $("#routes_0_check").siblings("ul").children("li").children("input").each(function(index){
          if ($(this).prop("checked") !== config.routeCheckboxes[index]) {
            $(this).click();
          }
        })
        // selected (highlighted) routes
        if (config.selectedRoutes.length !== 0) {
          const selectedRoutes = [];
          for (const ID of config.selectedRoutes) {
            selectedRoutes.push(routesOnStage.routes[ID]);
          }
          routesOnStage.handleRouteClick(selectedRoutes);
        }
      }
    }

    uploadConfig = () => {
      // create hidden file browser
      const fileBrowser = $(`<input type="file"/>`).prop("display", "none");
      $("body").append(fileBrowser);
    
      fileBrowser.change(() => {
        // uploaded configuration file
        const configfile = fileBrowser.prop("files")[0];
        const filereader = new FileReader();
        // handle the file
        filereader.onload = (event) => {
          const config = JSON.parse((<any>event.target).result);
          this.setConfig(config);
          // show check symbol
          this.checkSymbol();
        }
        filereader.readAsText(configfile);
        fileBrowser.remove();
      })
      // open file browser
      fileBrowser.click();
    }

    saveConfig = (saveExternal = false, saveRoutes = false) => {
      const config = <any>{};
      // position and scale
      config.position = {
        x: pixiBackend.container.stage.position.x,
        y: pixiBackend.container.stage.position.y,
        scale: pixiBackend.container.stage.scale.x,
      }
      // horizontal bus segment width
      config.busesHorizontalPosition = wafer.busesHorizontalPosition;
      // detail level
      config.levelOneEnabled = detailview.enabled;
      config.levelTwoEnabled = detailview.levelTwoEnabled;
      // overview and detailview checkboxes for manual mode
      config.hicannCheckboxes = [];
      for (let i=0; i<wafer.enumMax; i++) {
        const checkboxes = ({
          OV: [],
          DV: [],
        })
        $(`#hicanns_0_${i}_OV_checkbox`).siblings("ul").children("li").children("input").each(function(){
          checkboxes.OV.push($(this).prop("checked"));
        });
        $(`#hicanns_0_${i}_DV_checkbox`).siblings("ul").children("li").children("input").each(function(){
          checkboxes.DV.push($(this).prop("checked"));
        });
        config.hicannCheckboxes.push(checkboxes);
      }
      // automode/ manualmode
      config.automode = automode.enabled;
      config.manualmode = manualmode.enabled;
      // HICANN images state
      config.hicannImages = $("#waferImageCheckbox").prop("checked");
      // Routes
      if (saveRoutes) {
        config.routes = true;
        // visible routes
        config.routeCheckboxes = [];
        $("#routes_0_check").siblings("ul").children("li").children("input").each(function(){
          config.routeCheckboxes.push($(this).prop("checked"));
        })
        // selected (highlighted) routes
        config.selectedRoutes = [];
        for (const route of routesOnStage.selectedRoutes) {
          config.selectedRoutes.push(route.ID);
        }
      } else {
        config.routes = false;
      }
    
      this.configuration = config;
    
      if (saveExternal) {
        const json = JSON.stringify(config);
        const element = document.createElement("a");
        element.setAttribute("href", `data:text/plain;charset=utf-8,${encodeURIComponent(json)}`);
        element.setAttribute("download", "visuConfig.json");
        element.style.display = "none";
        document.body.appendChild(element);
        element.click();
        document.body.removeChild(element);
      }
      // show check symbol
      this.checkSymbol();
    }

    adjustHBusSegment = (changeRate: number) => {
      const straightHeight = 3 * wafer.busesHorizontalPosition.original.height;
      const busDiff = wafer.busesHorizontalPosition.original.height - straightHeight;
      wafer.busesHorizontalPosition.current.height = wafer.busesHorizontalPosition.original.height - busDiff * changeRate;
      wafer.busesHorizontalPosition.current.y = wafer.busesHorizontalPosition.original.y + (busDiff * changeRate)/2;
      const repDiff = wafer.repeaterBlockPosition.horizontal.height.original - straightHeight;
      wafer.repeaterBlockPosition.horizontal.height.current = wafer.repeaterBlockPosition.horizontal.height.original - repDiff * changeRate;
      wafer.repeaterBlockPosition.horizontal.left.current.y = wafer.repeaterBlockPosition.horizontal.left.original.y + (repDiff * changeRate)/2;
      wafer.repeaterBlockPosition.horizontal.right.current.y = wafer.repeaterBlockPosition.horizontal.right.original.y + (repDiff * changeRate)/2;

      // save visu configuration
      this.saveConfig(false, true);
      // restore visu configuration
      this.setConfig(this.configuration);
      // show check symbol
      this.checkSymbol();
    }

    adjustVBusSegment = (changeRate: number) => {
      for (const leftPosition of [wafer.repeaterBlockPosition.vertical.top.left, wafer.repeaterBlockPosition.vertical.bottom.left]) {
        const diff = Math.abs(leftPosition.original.x - wafer.busesLeftPosition.x);
        leftPosition.current.x = leftPosition.original.x + changeRate * diff;
      };
      for (const rightPosition of [wafer.repeaterBlockPosition.vertical.top.right, wafer.repeaterBlockPosition.vertical.bottom.right]) {
        const diff = Math.abs(rightPosition.original.x - wafer.busesRightPosition.x);
        rightPosition.current.x = rightPosition.original.x + changeRate * diff;
      }
      wafer.repeaterBlockPosition.vertical.width.current = (1 - changeRate) * wafer.repeaterBlockPosition.vertical.width.original + changeRate * wafer.busesLeftPosition.width;
      
      // save visu configuration
      this.saveConfig(false, true);
      // restore visu configuration
      this.setConfig(this.configuration);
      // show check symbol
      this.checkSymbol();
    }
    
    newRenderer = (resolution: number, forceCanvas: boolean) => {
      pixiBackend.renderer.renderer.destroy();
      $("#pixiJSCanvas").remove();
      pixiBackend.renderer = new pixiBackend.Renderer($("body"), 0x333333, canvasWidth(), canvasHeight(), forceCanvas, resolution);
    }
    
    screenshot = (resolution: number) => {
      this.newRenderer(resolution, true)

      pixiBackend.renderer.render();
      pixiBackend.renderer.renderer.view.toBlob((blob) => {
        const element = document.createElement("a");
        const url = URL.createObjectURL(blob);
        element.setAttribute("href", url);
        element.setAttribute("download", "screenshot.png");
        element.style.display = "none";
        document.body.appendChild(element);
        element.click();
        document.body.removeChild(element);
      }, "image/png", 1)
      
      this.newRenderer(1, false);
    }
  }
}