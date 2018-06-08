import sys, os, argparse, roqt
from PySide import QtGui, QtCore, QtUiTools, QtSvg
from roqt import Wafer, HICANN, WaferView

def loadUi(fname):
    loader = QtUiTools.QUiLoader()
    loader.registerCustomWidget(WaferView)
    file = QtCore.QFile(fname)
    file.open(QtCore.QFile.ReadOnly)
    widget = loader.load(file, None)
    file.close()
    return widget

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
            help='skip drawing switches for faster rendering')
    args = parser.parse_args()

    main_window = loadUi('./ui/main.ui')
    scene = QtGui.QGraphicsScene()

    # draw wafer scene
    import pyroqt
    _pyroqt = pyroqt.PyRoQt()
    _pyroqt.load(args.file)
    wafer = Wafer(scene, _pyroqt, args.switches)
    wafer.draw_all_routes()

    if args.o:
        save_figure(scene, args.o)
    else:
        main_window.graphicsView.setScene(scene)
        main_window.graphicsView.setRenderHints(QtGui.QPainter.Antialiasing | QtGui.QPainter.SmoothPixmapTransform)
        main_window.graphicsView.setDragMode(
                QtGui.QGraphicsView.ScrollHandDrag)
        main_window.show()

        sys.exit(app.exec_())

if __name__== '__main__':
    main()
