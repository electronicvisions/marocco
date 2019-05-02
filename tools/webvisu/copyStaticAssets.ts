/// <reference types="shelljs" />

import * as shell from "shelljs";

shell.cp("-R", "src/img", "dist/img");
shell.cp("-R", "src/jquery-ui.css", "dist/jquery-ui.css");
shell.cp("-R", "src/main.css", "dist/main.css");
shell.cp("-R", "src/main.html", "dist/main.html");

// install external libraries
shell.mkdir("dist/libraries");
shell.cp("-R", "src/libraries/*", "dist/libraries");
shell.cp("-R", "node_modules/jquery/dist/jquery.min.*", "dist/libraries");
shell.cp("-R", "node_modules/jquery-ui-dist/jquery-ui.*", "dist/libraries");
shell.cp("-R", "node_modules/jquery-ui-touch-punch/jquery.ui.touch-punch.min.*", "dist/libraries");
shell.cp("-R", "node_modules/pixi.js/dist/pixi.*", "dist/libraries");
