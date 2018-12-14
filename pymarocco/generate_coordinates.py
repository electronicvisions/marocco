#!/usr/bin/env python

import textwrap
import logging

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, classes, functions

import helpers

wrap = Wrapper()
mb = wrap.mb

ns_marocco = mb.namespace('::marocco')
ns_marocco.include()

# propagate "explictness" to python :)
for ns in [ns_marocco]:
    for c in ns.classes(allow_empty=True):
        c.constructors(lambda c: c.explicit == True, allow_empty=True).allow_implicit_conversion = False

wrap.ishell()

for cl in map(ns_marocco.class_, [
        "BioNeuron",
        "L1AddressOnWafer",
        "L1Route",
        "L1RouteTree",
        "LogicalNeuron"
]):
    cl.add_registration_code('def(bp::self_ns::str(bp::self_ns::self))')
    classes.add_pickle_suite(cl)
    classes.add_comparison_operators(cl)
    if cl.mem_funs("hash", allow_empty=True):
        classes.expose_std_hash(cl)

for cl in ns_marocco.classes(allow_empty=True):
    # Where applicable expose iterator interface instead of raw begin()/end().
    helpers.expose_iterator_interface(cl)
    # Convert vector<T&> return type to copies.
    for fun in cl.mem_funs(allow_empty=True):
        if not functions.convert_vector_of_references_return_type(fun):
            continue
    # Register builder pattern if applicable
    helpers.builder_pattern(cl)

for td in ns_marocco.typedefs(allow_empty=True):
    # Register to-python converters for boost::variant objects.
    classes.add_variant_converters_for(mb, td.target_decl)
    # Do not expose typedefs (prevent AttributeErrors on import)
    td.exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
