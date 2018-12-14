#!/usr/bin/env python

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, classes

wrap = Wrapper()
mb = wrap.mb

ns = mb.namespace('::pymarocco::runtime')
for name in ["sthal::Wafer", "marocco::results::Marocco"]:
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

# Do not expose typedefs (prevent AttributeErrors on import)
for td in mb.typedefs(allow_empty=True):
    td.exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

mb.namespace('::boost::serialization').exclude()

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
