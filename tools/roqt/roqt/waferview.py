from PySide import QtGui


class WaferView(QtGui.QGraphicsView):
    def __init__(self, scene):
        super(WaferView, self).__init__()
        self.setScene(scene)
        self.setRenderHints(
            QtGui.QPainter.Antialiasing | QtGui.QPainter.SmoothPixmapTransform)
        self.setResizeAnchor(QtGui.QGraphicsView.AnchorViewCenter)
        self.setTransformationAnchor(QtGui.QGraphicsView.AnchorUnderMouse)
        self.setDragMode(QtGui.QGraphicsView.ScrollHandDrag)

    def wheelEvent(self, event):
        # See documentation for QWheelEvent::delta(): "Returns the
        # distance that the wheel is rotated, in eighths of a degree.
        # [...] Most mouse types work in steps of 15 degrees, [...]"
        num_degrees = event.delta() / 8
        num_steps = num_degrees / 15
        factor = 1 + num_steps * 0.1
        self.scale(factor, factor)
