from pymarocco_results import *

def _patch_find():
    import functools
    import inspect
    import pyhmf
    from . import coordinates

    def wrap(fun):
        @functools.wraps(fun)
        def wrapper(self, arg, *args):
            # Convert PyID to BioNeuron coordinate.
            if isinstance(arg, pyhmf.PyID):
                arg = coordinates.BioNeuron(arg)
            return fun(self, arg, *args)
        return wrapper

    for (name, fun) in inspect.getmembers(Placement, inspect.ismethod):
        if name not in ['find']:
            continue
        setattr(Placement, name, wrap(fun))

_patch_find()
del _patch_find
