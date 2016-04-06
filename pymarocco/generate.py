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

for name in ['PyMarocco']:
    cl = mb.class_(name)
    createFactory = cl.mem_funs('create')
    cl.add_fake_constructors( createFactory )

# Do not expose typedefs (prevent AttributeErrors on import)
for td in mb.typedefs(allow_empty=True):
    td.exclude()

for name in ['hw_id', 'bio_id']:
    ns_marocco.classes(name).include()
    ns_pymarocco.typedefs(name).include()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

mb.namespace('::boost::serialization').exclude()

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
