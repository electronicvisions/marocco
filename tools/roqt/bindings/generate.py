#!/usr/bin/env python
import sys,os
import logging

logging.basicConfig(level=logging.INFO)

from pywrap.wrapper import Wrapper
from pywrap import namespaces
from pyplusplus.module_builder import call_policies

wrap = Wrapper()
module_name = wrap.module_name()
mb = wrap.mb

#Expose classes
classes = [
        'PyRoQt',
        'SynapseRowSource',
        'DriverResult',
        'DriverResultAndSynapseTargetMapping',
        'SynapseRoutingResult',
        #'SynapseDriverList',
    ]

for cls in classes:
    c = mb.class_(cls)
    c.include()

    for base in c.recursive_bases:
        base.related_class.include()

    #createFactory = c.mem_funs('create')
    #c.add_fake_constructors( createFactory )

# @#$)(! vector won't get automatically wrapped
# c = mb.class_('vector<unsigned long, std::allocator<unsigned long> >')
# c.ignore = False

# Exclude boost::serialization and boost::archive
ns_boost = mb.global_ns.namespace('boost')
ns_boost.namespace('serialization').exclude()
ns_boost.namespace('archive').exclude()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'protected')

wrap.set_number_of_files(0)
wrap.finish()
