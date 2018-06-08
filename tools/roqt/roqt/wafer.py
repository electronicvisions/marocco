import random
from PySide import QtGui
from pyhalbe.Coordinate import HICANNOnWafer, Enum, X, Y, HICANNGlobal, \
        HLineOnHICANN, VLineOnHICANN, SynapseSwitchRowOnHICANN
from pyhalbe.Coordinate import left as LEFT, right as RIGHT, top as TOP, bottom as BOTTOM

from roqt import HICANN

class Wafer(object):
    SPACING = 20
    COLORS = [
            QtGui.QColor(255,   0,   0, 255),
            QtGui.QColor(  0, 255,   0, 255),
            QtGui.QColor(  0,   0, 255, 255),
            QtGui.QColor(  0, 255, 255, 255),
            QtGui.QColor(255,   0, 255, 255),
            QtGui.QColor(255, 255,   0, 255), ]

    def __init__(self, scene, _pyroqt, draw_switches=False):
        self.draw_switches = draw_switches
        self.scene = scene
        self.hicanns = {}
        self.pyroqt = _pyroqt

    def draw_hicann(self, id):
        self.hicanns[id] = HICANN(self.scene,
                id.x().value()*(HICANN.WIDTH+self.SPACING),
                id.y().value()*(HICANN.HEIGHT+self.SPACING),
                self.draw_switches,
                                  id)
        return self.hicanns[id]

    def draw_all_routes(self):
        crossbars = self.pyroqt.crossbar()
        synapserows = self.pyroqt.synapserow()
        routing_graph = self.pyroqt
        print '# HICANNs:', crossbars.size()

        # need to guess all possible wafers (HICANNGlobal is not iterable anymore)
        max_wafers = 30
        max_hicann_global_enum = max_wafers*HICANNOnWafer.enum_type.end

        for hh in [HICANNGlobal(Enum(ii)) for ii in xrange(max_hicann_global_enum)]:
            if crossbars.exists(hh):
                print 'crossbars routing: ', hh
                for local_route in crossbars.at(hh):
                    self.colorize_route(hh, local_route.route(), routing_graph, synapserows)

        Wafer.connect_hicanns(self.scene, self.hicanns)

    @staticmethod
    def _get_color():
        return Wafer.COLORS[random.randint(0, len(Wafer.COLORS)-1)]

    def colorize_route(self, h, route, routing_graph, synapserows):
        import pyroqt
        brush = QtGui.QBrush(self._get_color())
        pen = QtGui.QPen(brush, 2)

        def getHICANN(hicann_global):
            hicann = self.hicanns.get(hicann_global.on_wafer())
            if not hicann:
                hicann = self.draw_hicann(hicann_global.on_wafer())
            return hicann

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
            for driver_result in synapserows.at(target_hicann):
                if driver_result.line()==vline and driver_result.from_adjacent()==is_adj:
                    # iterate over all primary drivers
                    for drv in driver_result.drivers():
                        primary = drv.key

                        # colorize switch row
                        line = hicann.synapsedriver[primary][0]
                        line.setPen(pen)
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

                last_segment = (cur, direction)



    @staticmethod
    def draw_horizontal_connect(scene, hicanns, key, val):
        try:
            east = hicanns.get(key.east())
        except RuntimeError:
            east = None


        if not east:
            return
        for idx, hline in enumerate(val.horizontalLines):
            eline = east.horizontalLines[HLineOnHICANN(idx).east().value()]
            pen = hline.pen() if hline.pen() == eline.pen() else QtGui.QPen()
            scene.addLine(
                    hline.line().x2(), hline.line().y2(),
                    eline.line().x1(), eline.line().y1(),
                    pen)

    @staticmethod
    def draw_vertical_connect(scene, hicanns, key, val):
        try:
            south = hicanns.get(key.south())
        except RuntimeError:
            south = None

        if not south:
            return
        for idx, vline in enumerate(val.verticalLines):
            sline = south.verticalLines[VLineOnHICANN(idx).south().value()]
            pen = vline.pen() if vline.pen() == sline.pen() else QtGui.QPen()
            scene.addLine(
                    vline.line().x2(), vline.line().y2(),
                    sline.line().x1(), sline.line().y1(),
                    pen)

    @staticmethod
    def connect_hicanns(scene, hicanns):
        for key, val in hicanns.iteritems():
            for fun in ['horizontal', 'vertical']:
                f = getattr(Wafer, 'draw_%s_connect'%fun)
                f(scene, hicanns, key, val)
