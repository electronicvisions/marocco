#!/usr/bin/env python

import textwrap
import logging

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, classes

wrap = Wrapper()
mb = wrap.mb

ns_marocco = mb.namespace('::marocco')
ns_marocco.include()

wrap.ishell()

# Where applicable expose iterator interface instead of raw begin()/end().
for cl in ns_marocco.classes(allow_empty=True):
    begin_funs = list(cl.mem_funs("begin", allow_empty=True))
    end_funs = list(cl.mem_funs("end", allow_empty=True))
    if not (begin_funs and end_funs):
        continue
    for fun in begin_funs + end_funs:
        fun.exclude()
    cl.add_registration_code(
        'def("__iter__", bp::iterator< {} >())'.format(cl.decl_string))

# Wrapping for halbe-like coordinates (derived from RantWrapper or BaseType)
for cl in ns_marocco.classes(allow_empty=True):
    if not any(b.parent.decl_string.startswith("::HMF::Coordinate")
               for b in classes.get_all_bases(cl)):
        continue

    cl.add_registration_code('def(bp::self_ns::str(bp::self_ns::self))')
    classes.add_pickle_suite(cl)
    classes.add_comparison_operators(cl)
    # classes.expose_std_hash(cl)

# Register to-python converters for boost::variant objects.
for td in ns_marocco.typedefs(allow_empty=True):
    if not classes.add_variant_converters_for(mb, td.target_decl):
        continue

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
