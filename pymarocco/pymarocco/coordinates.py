from pymarocco_coordinates import *


def _patch_BioNeuron():
    import functools
    import pyhmf

    def wrap(ctor):
        @functools.wraps(ctor)
        def wrapper(self, *args):
            if not args:
                # default constructor
                return ctor(self)
            arg, args = args[0], args[1:]
            # Convert PyID to BioNeuron coordinate.
            if isinstance(arg, pyhmf.PyID):
                parent = arg.parent()
                # Calculate the relative neuron index.
                index = int(arg) - int(parent[0])
                if hasattr(parent, "mask"):
                    # Correct for offset of parent[0], when parent is
                    # a PopulationView.
                    index += parent.mask()[0]
                return ctor(self, parent.euter_id(), index, *args)
            return ctor(self, arg, *args)
        return wrapper

    BioNeuron.__init__ = wrap(BioNeuron.__init__)

_patch_BioNeuron()
del _patch_BioNeuron
