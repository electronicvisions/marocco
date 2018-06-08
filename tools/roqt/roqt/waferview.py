from PySide import QtGui

class WaferView(QtGui.QGraphicsView):
    def wheelEvent(self, event):
        self.setTransformationAnchor(self.AnchorUnderMouse)

        # Scale the view / do the zoom
        scaleFactor = 1.15;
        if(event.delta() > 0.): # Zoom in
            self.scale(scaleFactor, scaleFactor)
        else: # Zooming out
            self.scale(1.0 / scaleFactor, 1.0 / scaleFactor)
