#!/usr/bin/env python

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, classes

wrap = Wrapper()
mb = wrap.mb

ns = mb.namespace('::pymarocco::runtime')
for name in ["sthal::Wafer", "marocco::results::Marocco", "marocco::placement::algorithms::PlacePopulationsBase"]:
    mb.add_registration_code(
        "bp::register_ptr_to_python< boost::shared_ptr< " +
        name + " > >();")

namespaces.extend_array_operators(ns)
for cl in ns.classes(allow_empty=True):
    cl.include()
    classes.add_pickle_suite(cl)
    factory = list(cl.mem_funs("create", allow_empty=True))
    if factory:
        cl.add_fake_constructors(factory)
    # propagate "explictness" to python :)
    cl.constructors(lambda c: c.explicit == True, allow_empty=True).allow_implicit_conversion = False


# expose member variables of type `boost::optional<vector<...>>`
uns = mb.namespace('::marocco::placement::algorithms')
namespaces.extend_array_operators(uns)
for cl in uns.classes(allow_empty=True):
    cl.include()
    for variable in cl.variables(allow_empty=True):
        classes.add_optional_vector_wrapper_for(mb, variable)
    classes.add_pickle_suite(cl)
    cl.constructors(lambda c: c.explicit == True, allow_empty=True).allow_implicit_conversion = False



# Do not expose typedefs (prevent AttributeErrors on import)
for td in mb.typedefs(allow_empty=True):
    td.exclude()

# expose only public interfaces and protected interfaces,
# protected variables cannot be exposed :(
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')

mb.namespace('::boost::serialization').exclude()

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
