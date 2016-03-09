#!/usr/bin/env python

from pywrap.wrapper import Wrapper
import pywrap.namespaces as ns

from pywrap import containers, namespaces, matchers, classes

wrap = Wrapper()
mb = wrap.mb

mb.namespace('::marocco').exclude()
mb.namespace('::marocco::alone').include()

ns_routing = mb.namespace('::marocco::routing')
ns_routing.class_('Target').include()
ns_routing.class_('L1BusOnWafer').include()
for fun in ns_routing.free_functions('configure'):
    fun.include()

# expose only public interfaces
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'private')
namespaces.exclude_by_access_type(
    mb, ['variables', 'calldefs', 'classes', 'typedefs'], 'protected')

containers.extend_std_containers(mb)

wrap.set_number_of_files(0)
wrap.finish()
