/**
 * A collection of functions useful functions, that are not specific to the visualization.
 */
namespace tools {
	/**
	 * For a color gradient, where "colorOne" corresponds to "zero" and "colorTwo" corresponds to "max",
	 * the color corresponding to "value" is calculated.
	 */
	export function colorInGradient(colorOne: string, colorTwo: string, max: number, value: number) {
		let frac = max ? value/max : 0;
		let c1 = {
			r: parseInt(colorOne.slice(0,2), 16),
			g: parseInt(colorOne.slice(2,4), 16),
			b: parseInt(colorOne.slice(4,6), 16)
		};
		let c2 = {
			r: parseInt(colorTwo.slice(0,2), 16),
			g: parseInt(colorTwo.slice(2,4), 16),
			b: parseInt(colorTwo.slice(4,6), 16)
		};
		let diff = {
			r:c2.r - c1.r,
			g:c2.g - c1.g,
			b:c2.b - c1.b
		}
		let cnew = {
			r: Math.floor(diff.r*frac + c1.r),
			g: Math.floor(diff.g*frac + c1.g),
			b: Math.floor(diff.b*frac + c1.b)
		};
		let cnew_hex = {
			r: ((cnew.r).toString(16).length === 2) ?
					(cnew.r).toString(16) : (0).toString(16) + (cnew.r).toString(16),
			g: ((cnew.g).toString(16).length === 2) ?
					(cnew.g).toString(16) : (0).toString(16) + (cnew.g).toString(16),
			b: ((cnew.b).toString(16).length === 2) ?
					(cnew.b).toString(16) : (0).toString(16) + (cnew.b).toString(16),
		}
		
		let result = "0x" + cnew_hex.r + cnew_hex.g + cnew_hex.b;
		return result;
	}
	/**
	 * returns all teh digits in a string, concatenated.
	 * e.g. "Hello13Visu08" -> 1308
	 */
	export function numberInString(string: any) {
		let number = "";
		for (const letter in string) {
			number += (!isNaN(parseInt(string[letter]))) ? string[letter] : "";
		}
		return parseInt(number);
	}
	/**
	 * returns a random number between "bottom" and "top".
	 */
	function randomNumber(bottom: number, top: number) {
		return (Math.floor(Math.random() * (top-bottom+1) + bottom))
	};
	/**
	 * returns a semi-random color in hexadecimal form (e.g. 0xffffff).
	 */
	export function randomHexColor() {
		// semi-random color
		const goldenRatioConjugate = 0.618033988749895;
		const hue = (Math.random() + goldenRatioConjugate) % 1;
		const hsv = hsvToRGB(hue, 0.6, 0.95);

		const color = {
			r: hsv[0],
			g: hsv[1],
			b: hsv[2]
		}
		// convert to Hex color
		const colorHex = {
			r: ((color.r).toString(16).length === 2) ?
					(color.r).toString(16) : (0).toString(16) + (color.r).toString(16),
			g: ((color.g).toString(16).length === 2) ?
					(color.g).toString(16) : (0).toString(16) + (color.g).toString(16),
			b: ((color.b).toString(16).length === 2) ?
					(color.b).toString(16) : (0).toString(16) + (color.b).toString(16),
		}

		// concatenate and return
		return "0x" + colorHex.r + colorHex.g + colorHex.b;
	};
	/**
	 * HSV values in [0..1[
	 * returns [r, g, b] values from 0 to 255
	 * @param h hue
	 * @param s saturation
	 * @param v value
	 */
	function hsvToRGB(h: number, s: number, v: number) {
		const h_i = Math.floor(h*6)
		const f = h*6 - h_i
		const p = v * (1 - s)
		const q = v * (1 - f*s)
		const t = v * (1 - (1 - f) * s)
		let r: number;
		let g: number;
		let b: number;
		switch (h_i) {
			case 0:
				r = v;
				g = t;
				b = p;
				break;
			case 1:
				r = q;
				g = v;
				b = p;
				break;
			case 2:
				r = p;
				g = v;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = v;
				break;
			case 4:
				r = t;
				g = p;
				b = v;
				break;
			default: // 5
				r = v;
				g = p;
				b = q;
		}
		return [Math.floor(r*256), Math.floor(g*256), Math.floor(b*256)]
	}
	/**
	 * interface for a box/rectangle
	 */
	export interface Box {
		x: number;
		y: number;
		width: number;
		height: number;
	}
	/**
	 * interaface for a point
	 */
	export interface Point {
		x: number;
		y: number;
	}
	export interface Line {
		x1: number;
		y1: number;
		x2: number;
		y2: number;
		width?: number;
	}
	/**
	 * Calculate the intersection of two rectangles.
	 * caveat: works only in the special case of two intersecting bus segments
	 */
	export function intersectionRectangle(rect1: Box, rect2: Box) {
		let position = <Box>{};
		
		// intersection x & width
		if (rect1.width < rect2.width) {
			position.x = rect1.x;
			position.width = rect1.width;
		} else {
			position.x = rect2.x;
			position.width = rect2.width;
		};

		// intersection y & height
		if (rect1.height < rect2.height) {
			position.y = rect1.y;
			position.height = rect1.height;
		} else {
			position.y = rect2.y;
			position.height = rect2.height;
		};

		return (position);
	};
	export function intersectionPoint(line1: Line, line2: Line) {
		let position = <Point>{};

		if (line1.y1 === line1.y2) { // line1 is horizontal line
			position = {
				x: line2.x1,
				y: line1.y1,
			}
		} else {
			position = {
				x: line1.x1,
				y: line2.y1,
			}
		}

		return position;
	}
	/**
	 * calculates center position and radius of a circle that fits exactly into the square
	 */
	export function circleInSquare(square: Box) {
		// circle x
		const x = square.x + square.width/2;

		// circle y
		const y = square.y + square.height/2;

		// circle radius
		const radius = square.width/2;

		return({
			x: x,
			y: y,
			radius: radius,
		})
	}
	/**
	 * check if point is inside boundaries of rectangle
	 */
	export function pointInRectangle(point: Point, rectangle: Box) {
		if ((point.x>=rectangle.x && point.x <= (rectangle.x + rectangle.width))
				&& (point.y >= rectangle.y && point.y <= (rectangle.y + rectangle.height))) {
			return true;
		} else {
			return false;
		}
	}
	/**
	 * check if mouse is inside boundaries of HTML div
	 * @param mouse position of the mouse
	 * @param id ID of the div (must include the #!)
	 */
	export function mouseInDiv(mouse: Point, id: string) {
		return (tools.pointInRectangle({
			x: mouse.x,
			y: mouse.y
		}, {
			x: $(id).offset().left,
			y: $(id).offset().top,
			width: $(id).outerWidth(),
			height: $(id).outerHeight(),
		}))
	}
	/**
	 * Gaußsche Summenformel "kleiner Gauss"
	 */
	export function kleinerGauss(n: number) {
		return (n**2 + n)/2
	}
	/**
	 * Calculate the distance between two two-dimensional points
	 */
	export function distanceBetweenPoints(point1: {x: number, y: number}, point2: {x: number, y: number}) {
		const distance = Math.sqrt((point1.x - point2.x)**2
				+ (point1.y - point2.y)**2);
		return(distance);
	}

	function shiftToZero(x, y, x2, y2) {
    if (y < y2) {
      	x2 -= x;
        y2 -= y;
        x -= x;
        y -= y;
    } else {
        x -= x2;
        y -= y2;
        x2 -= x2;
        y2 -= y2;
    }
    return [x, y, x2, y2];
	}

	function calcShift(x, y, x2, y2) {
			return [-x, -y];
	}

	function calcAlpha(x, y, x2, y2) {
        return (Math.atan((x2-x)/(y2-y)));
	}

	function rotate(x, y, alpha) {
    const x1 = Math.cos(alpha)*x - Math.sin(alpha)*y;
    const y1 = Math.sin(alpha)*x + Math.cos(alpha)*y;
    return [x1, y1];
	}

	export function pointInLine(pointX: number, pointY: number, lineX1: number, lineY1: number, lineX2: number, lineY2: number, width: number) {
		if ((lineY1 > lineY2) || ((lineY1 === lineY2) && (lineX1 > lineX2))) {
			[lineX1, lineX2] = [lineX2, lineX1];
			[lineY1, lineY2] = [lineY2, lineY1];
		}
		const shift = calcShift(lineX1, lineY1, lineX2, lineY2);
		pointX += shift[0];
		pointY += shift[1];
		lineX1 += shift[0];
		lineY1 += shift[1];
		lineX2 += shift[0];
		lineY2 += shift[1];
		const alpha = calcAlpha(lineX1, lineY1, lineX2, lineY2);
		[pointX, pointY] = rotate(pointX, pointY, alpha);
		[lineX1, lineY1] = rotate(lineX1, lineY1, alpha);
		[lineX2, lineY2] = rotate(lineX2, lineY2, alpha);

		if ((pointX >= (lineX1 - width/2)) && (pointX <= (lineX1 + width/2)) && (pointY >= lineY1) && (pointY <= lineY2)) {
			return true;
		} else {
			return false;
		};
	}
}