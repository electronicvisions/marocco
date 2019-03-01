from _pymarocco import *


def _patch_Version():
    global __version__
    import os
    __version__ = open(os.path.join(os.path.dirname(__file__), 'VERSION')).read().strip()
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


def _patch_ManualPlacement():
    import functools
    import inspect
    import pyhmf
    import pywrapstdvector

    def wrap(fun):
        @functools.wraps(fun)
        def wrapper(self, pop, *args):
            if isinstance(pop, pyhmf.Population):
                return fun(self, pop.euter_id(), pywrapstdvector.Vector_Int32(range(0,pop.size)), *args)
            if isinstance(pop, pyhmf.PopulationView):
                return fun(self, pop.euter_id(), pywrapstdvector.Vector_Int32(pop.mask()), *args)
            else:
                raise TypeError('first argument has to be a pyhmf Population or PopulationView')
        return wrapper

    for (name, fun) in inspect.getmembers(ManualPlacement, inspect.ismethod):
        if name not in ['on_hicann', 'on_neuron_block', 'on_neuron', 'with_size']:
            continue
        setattr(ManualPlacement, name, wrap(fun))

_patch_ManualPlacement()
del _patch_ManualPlacement
