import pyhmf as pynn
from pymarocco import PyMarocco

neuron_size = 4

marocco = PyMarocco()
marocco.placement.setDefaultNeuronSize(neuron_size)

pynn.setup(marocco=marocco)

con_alltoall = pynn.AllToAllConnector(weights=0.04)
con_fixednumberpre = pynn.FixedNumberPreConnector(n=6, weights=0.04)

n_neurons = 112
pop = pynn.Population(n_neurons, pynn.IF_cond_exp)
neuron_pool = range(n_neurons)

in_0 = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times': [0]})
for _ in xrange(4):
    pynn.Projection(in_0, pop[neuron_pool[0:4]], con_alltoall, target='excitatory')

n_pops = 6
n_in_pop = 10

for p in xrange(n_pops):

    l_a = neuron_pool[p*n_in_pop:(p+1)*n_in_pop-1]
    l_b = neuron_pool[(p+1)*n_in_pop:(p+2)*n_in_pop-1]

    pynn.Projection(pop[l_a], pop[l_b], con_fixednumberpre, target='excitatory')

pynn.run(10000)
pynn.end()
