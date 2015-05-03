#!/usr/bin/env python
import sys,os, logging
logging.basicConfig(level=logging.INFO)

from pywrap.wrapper import Wrapper
from pywrap import namespaces, containers, matchers, classes
from pyplusplus.module_builder import call_policies

def add_numpy_construtor_to_strong_typedefs(ns):
    matcher = matchers.match_std_container_t("array")
    def _filter(c):
        return any([ matcher(base.related_class) for base in c.bases])

    for c in ns.classes(_filter, allow_empty=True):
        classes.add_numpy_construtor(c)
        for b in c.bases:
            b.related_class.exclude()


wrap = Wrapper()
module_name = wrap.module_name()
mb = wrap.mb


my_classes = ['PyMarocco']
for cls in my_classes:
    c = mb.class_(cls)
    c.include()
    createFactory = c.mem_funs('create')
    c.add_fake_constructors( createFactory )

my_classes = [
    'MappingStats',
    'Placement',
    'Defects',
    'hw_id',
    'bio_id'
    ]
for cls in my_classes:
    c = mb.class_(cls)
    c.include()

my_classes = [
    '::pymarocco::Placement::List'
    ]
for td in my_classes:
    classes.add_from_pyiterable_converter_to(mb.typedef(td).target_decl)

# Exclude boost::serialization and boost::archive
ns_boost = mb.global_ns.namespace('boost')
ns_boost.namespace('serialization').exclude()
ns_boost.namespace('archive').exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')


ns_pymarocco = mb.namespace('::pymarocco')
containers.extend_std_containers(mb)
add_numpy_construtor_to_strong_typedefs(ns_pymarocco)
namespaces.extend_array_operators(ns_pymarocco)

wrap.set_number_of_files(0)
wrap.finish()
