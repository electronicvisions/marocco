#!/usr/bin/env python
import sys,os, logging
logging.basicConfig(level=logging.INFO)

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, matchers, classes
from pyplusplus.module_builder import call_policies

wrap = Wrapper()
mb = wrap.mb

ns_marocco = mb.namespace('::marocco')
ns_pymarocco = mb.namespace('::pymarocco')

for ns in [ns_pymarocco] + list(ns_marocco.namespaces('parameters')):
    namespaces.extend_array_operators(ns)
    ns.include()

    # propagate "explictness" to python :)
    for c in ns.classes(allow_empty=True):
        c.constructors(lambda c: c.explicit == True, allow_empty=True).allow_implicit_conversion = False

for name in ['PyMarocco']:
    cl = mb.class_(name)
    createFactory = cl.mem_funs('create')
    cl.add_fake_constructors( createFactory )

# Allow for by value conversion of hicann_configurator object
mb.add_registration_code('bp::register_ptr_to_python<boost::shared_ptr<sthal::HICANNConfigurator>>();')

# Do not expose typedefs (prevent AttributeErrors on import)
for td in mb.typedefs(allow_empty=True):
    td.exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

mb.namespace('::boost::serialization').exclude()

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
