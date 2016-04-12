import operator
import random
from PySide import QtGui
from pyhalbe.Coordinate import HICANNOnWafer, Enum, X, Y, HICANNGlobal, \
        HLineOnHICANN, VLineOnHICANN, SynapseSwitchRowOnHICANN, SynapseDriverOnHICANN
from pyhalbe.Coordinate import left as LEFT, right as RIGHT, top as TOP, bottom as BOTTOM

from roqt import HICANN


class Wafer(object):
    SPACING = 20
    COLORS = [QtGui.QColor(r, g, b, 255) for (r, g, b) in [
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

    def draw_from_pyroqt(self, pyroqt):
        crossbars = pyroqt.crossbar()
        synapserows = pyroqt.synapserow()
        routing_graph = pyroqt
        print '# HICANNs:', crossbars.size()

        # need to guess all possible wafers (HICANNGlobal is not iterable anymore)
        max_wafers = 30
        max_hicann_global_enum = max_wafers*HICANNOnWafer.enum_type.end

        for hh in [HICANNGlobal(Enum(ii)) for ii in xrange(max_hicann_global_enum)]:
            if crossbars.exists(hh):
                print 'crossbars routing: ', hh
                for local_route in crossbars.at(hh):
                    self.colorize_route(hh, local_route.route(), routing_graph, synapserows)

        self.draw_connections_between_hicanns()

    def next_color(self):
        color = self._colors[1]
        self._colors = self._colors[1:]
        self._colors.append(color)
        return color

    def colorize_route(self, h, route, routing_graph, synapserows):
        import pyroqt
        brush = QtGui.QBrush(self.next_color())
        pen = QtGui.QPen(brush, 2)

        def getHICANN(hicann_global):
            return self.hicann(hicann_global)

        def getHICANNfromBus(bus):
            return getHICANN(bus.hicann())

        for target_hicann, segment_list in route.getSegments():
            bus = routing_graph.getL1Bus(segment_list[0])
            vline = VLineOnHICANN(bus.getBusId())
            dest_hicann = bus.hicann()
            hicann = getHICANN(target_hicann)
            is_adj = dest_hicann != target_hicann # route from adj HICANN

            # find the corresponding synapse driver result (there can be up
            # to two with same vline, because vline X can be eiter from
            # this or adjacent HICANN).
            for driver_result in synapserows.at(target_hicann).driver_result:
                if driver_result.line()==vline and driver_result.from_adjacent()==is_adj:
                    # iterate over all primary drivers
                    for drv in driver_result.drivers():
                        primary = drv.key

                        # colorize switch row
                        line = hicann.synapsedriver[primary][0]
                        line.setPen(pen)
                        line.setZValue(1)
                        line.show()

                        # colorize primary
                        hicann.synapsedriver[primary][1].setBrush(brush)

                        # colorize adjacent (non-primary) drivers
                        for adj_drv in driver_result.getDrivers(primary):
                            hicann.synapsedriver[adj_drv][1].setBrush(brush)

                        # for adjacent insertions, also colorize switch row on
                        # adjacent HICANN.
                        if not is_adj:
                            # draw switch
                            xx = hicann.verticalLines[bus.getBusId()]
                            hicann.drawSwitch(xx, line)
                        else:
                            adj_hicann = getHICANN(dest_hicann)
                            line = adj_hicann.synapseswitchLines[
                                    SynapseSwitchRowOnHICANN(
                                        Y(primary.line().value()),
                                        X(LEFT if primary.side()==RIGHT else RIGHT))]
                            line.setPen(pen)
                            line.setZValue(1)
                            line.show()

                            # draw switch
                            xx = adj_hicann.verticalLines[bus.getBusId()]
                            adj_hicann.drawSwitch(xx, line)

                    # we can stop here, we found the corresponding vline
                    break

            # no colorize the other segments
            last_segment = None
            for seg in segment_list:
                bus = routing_graph.getL1Bus(seg)

                hicann = getHICANNfromBus(bus)
                direction = bus.getDirection()
                busid = bus.getBusId()

                if direction == pyroqt.L1Bus.Vertical:
                    cur = hicann.verticalLines[busid]
                    if last_segment and last_segment[1] != direction:
                        hicann.drawSwitch(last_segment[0], cur)
                elif direction == pyroqt.L1Bus.Horizontal:
                    cur = hicann.horizontalLines[busid]
                    if last_segment and last_segment[1] != direction:
                        hicann.drawSwitch(cur, last_segment[0])
                cur.setPen(pen)
                cur.setZValue(1)

                last_segment = (cur, direction)

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
