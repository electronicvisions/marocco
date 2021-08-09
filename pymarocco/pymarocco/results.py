from pymarocco_results import *


def _patch_methods():
    """
    Monkey-patch member functions of result containers to provide
    convenient access using pynn objects.
    """
    import functools
    import inspect
    import pyhmf
    import pymarocco_results
    from . import coordinates

    def wrap(fun):
        @functools.wraps(fun)
        def wrapper(self, *args, **kwargs):
            def convert(arg):
                # Convert PyID to BioNeuron coordinate.
                if isinstance(arg, pyhmf.PyID):
                    return coordinates.BioNeuron(arg)

                # Convert Projection to euter id.
                if isinstance(arg, pyhmf.Projection):
                    return arg.euter_id()
                return arg

            args = [convert(v) for v in args]
            kwargs = {k: convert(v) for k, v in kwargs.items()}
            result = fun(self, *args, **kwargs)

            # Convert iterable results to list to allow random access.
            members = set(name for name, _
                          in inspect.getmembers(result, inspect.ismethod))
            if "__iter__" in members and members.issubset(
                    ["__iter__", "__reduce__", "begin", "empty", "end"]):
                result = list(result)

            return result
        return wrapper

    for (klassname, klass) in inspect.getmembers(
            pymarocco_results, inspect.isclass):
        # We do not want / need to patch STL containers etc.
        if klassname[0].islower() or klassname.startswith("_"):
            continue

        for (name, fun) in inspect.getmembers(klass, inspect.ismethod):
            if name.startswith("_"):
                continue

            setattr(klass, name, wrap(fun))

_patch_methods()
del _patch_methods
