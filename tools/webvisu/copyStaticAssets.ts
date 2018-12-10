import * as shell from "shelljs";

shell.cp("-R", "src/img", "dist/img");
shell.cp("-R", "src/libraries/", "dist/libraries");
shell.cp("-R", "src/jquery-ui.css", "dist/jquery-ui.css");
shell.cp("-R", "src/main.css", "dist/main.css");
shell.cp("-R", "src/main.html", "dist/main.html");
