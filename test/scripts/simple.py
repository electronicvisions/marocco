from pymarocco import PyMarocco as marocco
from pyester import PyEster as ester
from pyhmf import *
from pyoneer import PyOneer

mapping = marocco()
mapping.backend = marocco.ESS

#pyoneer = PyOneer()
#pyoneer.experiment.id = 123
#pyoneer.useMongo = False

e = ester()

#setup(marcco=mapping, pyoneer=pyoneer)
setup(marocco=mapping)

neuron = Population(1, EIF_cond_exp_isfa_ista, {'v_spike' : -60., 'tau_syn_E' : 20})
source = Population(1, SpikeSourcePoisson, {'rate' : 2})

connector = AllToAllConnector(
        allow_self_connections=True,
        weights=1.)
Projection(source, neuron, connector, target="excitatory")

neuron.record_v()

run(10000)
end()
