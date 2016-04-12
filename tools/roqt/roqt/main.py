import sys
import os
import argparse
from PySide import QtGui, QtCore, QtUiTools, QtSvg
from roqt import Wafer, WaferView


class MainWindow(QtGui.QMainWindow):
    def __init__(self, scene):
        super(MainWindow, self).__init__()
        self.scene = scene
        self.view = WaferView(scene)
        self.setCentralWidget(self.view)
        self.setWindowTitle("Routing Qt Visualizer")


def invoke_painter(scene, img):
    painter = QtGui.QPainter(img)
    scene.render(painter)
    painter.end()


def save_figure(scene, fname):
    suffix = os.path.splitext(fname)[-1]
    if suffix == '.svg':
        img = QtSvg.QSvgGenerator()
        img.setFileName(fname)
        invoke_painter(scene, img)
    else:
        img = QtGui.QImage(3000, 3000, QtGui.QImage.Format_ARGB32)
        invoke_painter(scene, img)
        img.save(fname)


def main():
    app = QtGui.QApplication(sys.argv)

    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=str, help='pyroqt routing data')
    parser.add_argument('-o', type=str, default=None,
            help='write rendering to file, e.g.: *.png, *.svg')
    parser.add_argument('--switches', action='store_true',
                        help='draw switches (slows down rendering)')
    args = parser.parse_args()

    scene = QtGui.QGraphicsScene()

    wafer = Wafer(scene, args.switches)

    import pyroqt
    _pyroqt = pyroqt.PyRoQt()
    _pyroqt.load(args.file)
    wafer.draw_from_pyroqt(_pyroqt)

    if args.o:
        save_figure(scene, args.o)
    else:
        window = MainWindow(scene)
        window.show()

        sys.exit(app.exec_())


if __name__ == '__main__':
    main()
