from pymarocco import PyMarocco as marocco
from pyhmf import *

import pylogging
pylogging.reset()
pylogging.default_config(level=pylogging.LogLevel.INFO)

mapping = marocco()
mapping.backend = marocco.ESS

setup(marocco=mapping)

neuron = Population(1, EIF_cond_exp_isfa_ista, {'v_spike' : -60., 'tau_syn_E' : 20})
source = Population(1, SpikeSourcePoisson, {'rate' : 200})

connector = AllToAllConnector(
        allow_self_connections=True,
        weights=1.)
Projection(source, neuron, connector, target="excitatory")

neuron.record_v()

run(100)
end()
