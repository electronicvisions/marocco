class Point(object):
    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    def __init__(self, x, y):
        self._x = x
        self._y = y
