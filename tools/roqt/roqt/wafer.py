import operator
import random
from PySide import QtGui
from pyhalbe.Coordinate import HICANNOnWafer, Enum, X, Y, \
        HLineOnHICANN, VLineOnHICANN, SynapseSwitchRowOnHICANN, SynapseDriverOnHICANN
from pyhalbe.Coordinate import left as LEFT, right as RIGHT, top as TOP, bottom as BOTTOM

from roqt import HICANN


def is_bus(segment):
    return (isinstance(segment, HLineOnHICANN) or
            isinstance(segment, VLineOnHICANN))


class Wafer(object):
    SPACING = 20
    COLORS = [QtGui.QColor(r, g, b) for (r, g, b) in [
        (1, 0, 103), (213, 255, 0), (255, 0, 86), (158, 0, 142), (14, 76, 161),
        (255, 229, 2), (0, 95, 57), (0, 255, 0), (149, 0, 58), (255, 147, 126),
        (164, 36, 0), (0, 21, 68), (145, 208, 203), (98, 14, 0),
        (107, 104, 130), (0, 0, 255), (0, 125, 181), (106, 130, 108),
        (0, 174, 126), (194, 140, 159), (190, 153, 112), (0, 143, 156),
        (95, 173, 78), (255, 0, 0), (255, 0, 246), (255, 2, 157), (104, 61, 59),
        (255, 116, 163), (150, 138, 232), (152, 255, 82), (167, 87, 64),
        (1, 255, 254), (255, 238, 232), (254, 137, 0), (189, 198, 255),
        (1, 208, 255), (187, 136, 0), (117, 68, 177), (165, 255, 210),
        (255, 166, 254), (119, 77, 0), (122, 71, 130), (38, 52, 0), (0, 71, 84),
        (67, 0, 44), (181, 0, 255), (255, 177, 103), (255, 219, 102),
        (144, 251, 146), (126, 45, 210), (189, 211, 147), (229, 111, 254),
        (222, 255, 116), (0, 255, 120), (0, 155, 255), (0, 100, 1),
        (0, 118, 255), (133, 169, 0), (0, 185, 23), (120, 130, 49),
        (0, 255, 198), (255, 110, 65), (232, 94, 190),
    ]]

    def __init__(self, scene, draw_switches=False):
        self.draw_switches = draw_switches
        self.scene = scene
        self._hicanns = {}
        self._colors = self.COLORS[:]

    def hicann(self, coord):
        if coord not in self._hicanns:
            self._hicanns[coord] = HICANN(
                self.scene,
                coord.x().value() * (HICANN.WIDTH + self.SPACING),
                coord.y().value() * (HICANN.HEIGHT + self.SPACING),
                self.draw_switches, coord)
        return self._hicanns[coord]

    def draw_from_pyroqt(self, results):
        from pymarocco.coordinates import L1Route
        import pymarocco.results

        def tail_with_drivers(base, hicann, drivers):
            route = L1Route()
            route.append(base.target_hicann(), base.back())
            for dr in drivers:
                route.append(hicann, dr)
            return route

        for route_item in results.l1_routing:
            route = route_item.route()
            brush = self.draw_route(route)

            is_adjacent = route.target_hicann() != route_item.target()
            vline = route.back()
            for dr in results.synapse_routing.at(
                    route_item.target()).driver_result:
                if dr.line() != vline or dr.from_adjacent() != is_adjacent:
                    continue

                for primary in dr.drivers().keys():
                    drivers = list(dr.drivers()[primary])
                    if not drivers:
                        continue

                    before = []
                    while drivers[0] != primary:
                        before.insert(0, drivers.pop(0))
                    before.insert(0, primary)
                    after = drivers

                    self.draw_route(
                        tail_with_drivers(
                            route, route_item.target(), before),
                        brush)
                    self.draw_route(
                        tail_with_drivers(
                            route, route_item.target(), after),
                        brush)

        self.draw_connections_between_hicanns()

    def draw_routes(self, routes):
        for route in routes:
            self.draw_route(route)
        self.draw_connections_between_hicanns()

    def draw_route(self, route, brush=None):
        brush = brush or QtGui.QBrush(self.next_color())
        pen = QtGui.QPen(brush, 2)

        hicann = self.hicann(route.source_hicann())
        last_segments = []
        for segment in route:
            if isinstance(segment, HICANNOnWafer):
                hicann = self.hicann(segment)
            elif is_bus(segment):
                if isinstance(segment, HLineOnHICANN):
                    cur = hicann.horizontalLines[segment.value()]
                else:
                    cur = hicann.verticalLines[segment.value()]
                if is_bus(last_segments[-1]):
                    if isinstance(last_segments[-1], HLineOnHICANN):
                        prev = hicann.horizontalLines[last_segments[-1].value()]
                    else:
                        prev = hicann.verticalLines[last_segments[-1].value()]
                    switch = hicann.drawSwitch(prev, cur)
                    switch.setZValue(2)
                cur.setPen(pen)
                cur.setZValue(1)
            elif isinstance(segment, SynapseDriverOnHICANN):
                if not isinstance(last_segments[-1], SynapseDriverOnHICANN):
                    line = hicann.synapsedriver[segment][0]

                    hicann_ = hicann
                    vline = last_segments[-1]
                    # adjacent insertion
                    if isinstance(last_segments[-1], HICANNOnWafer):
                        hicann_ = last_segments[-1]
                        vline = last_segments[-2]
                        line = hicann_.synapseswitchLines[
                            SynapseSwitchRowOnHICANN(
                                Y(segment.line().value()),
                                X(LEFT if segment.side() == RIGHT else RIGHT))]
                    assert isinstance(vline, VLineOnHICANN)
                    line.setPen(pen)
                    line.setZValue(1)
                    line.show()
                    # draw switch
                    xx = hicann_.verticalLines[vline.value()]
                    switch = hicann_.drawSwitch(xx, line)
                    switch.setZValue(2)
                hicann.synapsedriver[segment][1].setBrush(brush)
            last_segments.append(segment)
            last_segments = last_segments[-2:]

        return brush

    def next_color(self):
        color = self._colors[1]
        self._colors = self._colors[1:]
        self._colors.append(color)
        return color

    def draw_connections_between_hicanns(self):
        for coord, hicann in self._hicanns.iteritems():
            for ctor, move, lines in [
                    (HLineOnHICANN,
                     operator.methodcaller("east"),
                     operator.attrgetter("horizontalLines")),
                    (VLineOnHICANN,
                     operator.methodcaller("south"),
                     operator.attrgetter("verticalLines"))]:
                try:
                    other = self._hicanns[move(coord)]
                except (RuntimeError, KeyError):
                    continue

                for idx, line in enumerate(lines(hicann)):
                    other_line = lines(other)[move(ctor(idx)).value()]
                    pen = line.pen() if line.pen() == other_line.pen() else QtGui.QPen()
                    self.scene.addLine(
                        line.line().x2(), line.line().y2(),
                        other_line.line().x1(), other_line.line().y1(),
                        pen)
