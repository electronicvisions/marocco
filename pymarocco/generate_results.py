#!/usr/bin/env python

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, classes, functions

import helpers

wrap = Wrapper()
mb = wrap.mb

ns_marocco = mb.namespace('::marocco')

for ns in ns_marocco.namespaces('results'):
    namespaces.extend_array_operators(ns)
    for cl in ns.classes(allow_empty=True):
        cl.include()
        # dynamically generated classes which do not have serialize functions
        if not cl.name.endswith("Properties"):
            classes.add_pickle_suite(cl)

for cl in ns_marocco.classes(allow_empty=True):
    # Where applicable expose iterator interface instead of raw
    # begin()/end().
    helpers.expose_iterator_interface(cl)
    # Register builder pattern if applicable
    helpers.builder_pattern(cl)
    for fun in cl.mem_funs(allow_empty=True):
        # Wrap `marocco::iterable` classes that are used as return
        # values.
        helpers.iterable_return_type(fun)
        # Where applicable, unwrap boost::optional<T> return type
        # (return None or T).
        functions.return_optional_by_value(fun)

# Do not expose typedefs (prevent AttributeErrors on import)
for td in mb.typedefs(allow_empty=True):
    td.exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
