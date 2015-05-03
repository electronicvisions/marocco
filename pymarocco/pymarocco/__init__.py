from _pymarocco import *

import os
__version__ = open(os.path.join(os.path.dirname(__file__), 'VERSION')).read().strip()

def _patch_Version():
    PyMarocco.__version__ = __version__

_patch_Version()
del _patch_Version

def _patch_MappingStats():
    import pyhmf

    _getWeights = MappingStats.getWeights

    def getWeights(self, p):
        if not isinstance(p, pyhmf.Projection):
            raise TypeError('not a pyhmf.Projection')
        return _getWeights(self, p.euter_id())

    MappingStats.getWeights = getWeights

_patch_MappingStats()
del _patch_MappingStats

def _patch_Placement():
    import pyhmf

    _add = Placement.add

    def add(self, p, *args):
        #self.minSPL1 = False
        if not isinstance(p, pyhmf.Population):
            raise TypeError('not a pyhmf.Population')
        if len(p) > 256:
            import pyhalbe
            if isinstance(args[0], pyhalbe.Coordinate.HICANNGlobal):
                raise RuntimeError('Populations with size()<=256 can be placed manually ')
        return _add(self, p.euter_id(), *args)
    add.__doc__ = _add.__doc__

    Placement.add = add

_patch_Placement()
del _patch_Placement
