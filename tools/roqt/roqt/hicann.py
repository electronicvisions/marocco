from PySide import QtGui, QtCore
import numpy as np
import pyhalbe
from pyhalbe.Coordinate import HICANNOnWafer, HLineOnHICANN, VLineOnHICANN, \
    SynapseSwitchRowOnHICANN, Enum
from pyhalbe.Coordinate import left as LEFT, right as RIGHT, top as TOP, bottom as BOTTOM

from roqt.point import Point

class HICANN(object):
    ASPECT = 3./2
    WIDTH = 700.
    HEIGHT = ASPECT*WIDTH

    def __init__(self, scene, x=0, y=0, show_switches=False, hid=None):
        self.scene = scene
        self.offset = Point(x, y)
        self.show_switches = show_switches

        self.verticalLines = []
        self.horizontalLines = []
        self.synapseswitchLines = {}
        self.synapsedriver = {}
        self.switches = {}
        self.id = hid

        self.draw()

    def draw(self):
        off = self.offset
        VSPACING = self.WIDTH/400
        HSPACING = VSPACING*self.ASPECT

        # draw HICANN outline
        self.scene.addRect(off.x, off.y, self.WIDTH, self.HEIGHT,
            QtGui.QPen(QtGui.QColor(0, 0, 0)),
            QtGui.QBrush(QtGui.QColor(200, 200, 200)))

        t = self.scene.addText("{}\n\nEnum({})".format(self.id,self.id.id().value()))
        t.setPos(off.x + self.WIDTH / 2 - 100., off.y + 100.)

        # draw horizontal lines
        for ii in range(HLineOnHICANN.end):
            Y = off.y+self.HEIGHT/2+(ii-HLineOnHICANN.end/2)*HSPACING
            self.horizontalLines.append(
                    self.scene.addLine(off.x, Y, off.x+self.WIDTH, Y, QtGui.QPen('gray')))

        # draw vertical lines
        for ii in range(VLineOnHICANN.end):
            X = off.x+ii*VSPACING if ii<VLineOnHICANN.end/2 \
                    else off.x+self.WIDTH+(ii-VLineOnHICANN.end)*VSPACING
            self.verticalLines.append(self.scene.addLine(
                X, off.y, X, off.y+self.HEIGHT, QtGui.QPen('gray')))

        # draw synapse drivers and switch rows
        for side in [ LEFT, RIGHT ]:
            LENGTH = self.HEIGHT/2-(HLineOnHICANN.end+6)/2*HSPACING
            END = SynapseSwitchRowOnHICANN.y_type.end
            X0 = off.x if side==LEFT else off.x+self.WIDTH
            DX = VLineOnHICANN.end/2*VSPACING
            DIR = 1 if side==LEFT else -1

            for vside in [ TOP, BOTTOM ]:
                start = off.y+self.HEIGHT if int(vside) else off.y+LENGTH
                DY = LENGTH/(END/2)
                for idx, yy in enumerate(np.linspace(start-LENGTH, start, END/2)):
                    row = SynapseSwitchRowOnHICANN(side,
                            pyhalbe.Coordinate.Y(int(vside)*END/2+idx))

                    try:
                        # there is a synapse driver:
                        drv = row.driver()
                        B = X0+DIR*(DX+20)
                        line = self.scene.addLine(X0, yy, X0+DIR*(DX+20), yy)
                        line.hide()

                        # draw driver polygon
                        poly = QtGui.QPolygonF()
                        poly.append(QtCore.QPointF(B, yy+4)) # upper
                        poly.append(QtCore.QPointF(B+DIR*5, yy)) # mid
                        poly.append(QtCore.QPointF(B, yy-4)) # lower
                        p = self.scene.addPolygon(poly)

                        self.synapseswitchLines[row] = line
                        self.synapsedriver[drv] = (line, p)

                    except ValueError as e:
                        # row has NO local synapse driver:
                        self.synapseswitchLines[row] = \
                                self.scene.addLine(X0, yy, X0+DIR*DX, yy)
                        self.synapseswitchLines[row].hide()

        if self.show_switches:
            self.drawSwitches()

    def drawSwitch(self, xx, yy):
        SWITCH_SIZE = 2.

        if (xx, yy) not in self.switches:
            point = self.intersect(xx, yy)
            self.switches[(xx, yy)] = self.scene.addEllipse(
                point.x() - SWITCH_SIZE / 2, point.y() - SWITCH_SIZE / 2,
                SWITCH_SIZE, SWITCH_SIZE,
                QtGui.QPen(), QtGui.QBrush(QtGui.QColor(0, 0, 0)))
        return self.switches[(xx, yy)]

    def drawSwitches(self):
        # draw crossbar switches
        cb = pyhalbe.HICANN.Crossbar()
        for idx, xx in enumerate(self.verticalLines):
            for idy, yy in enumerate(self.horizontalLines):
                if cb.exists(VLineOnHICANN(idx), HLineOnHICANN(idy)):
                    ellipse = self.drawSwitch(xx, yy)

        # draw synapse switches
        ssw = pyhalbe.HICANN.SynapseSwitch()
        for idx, xx in enumerate(self.verticalLines):
            for row, yy in self.synapseswitchLines.iteritems():
                if ssw.exists(VLineOnHICANN(idx), row.line()):
                    ellipse = self.drawSwitch(xx, yy)


    @staticmethod
    def intersect(a, b):
        b, point = a.line().intersect(b.line())
        assert(b)
        return point
