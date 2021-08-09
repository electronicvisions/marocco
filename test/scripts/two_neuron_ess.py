import copy
from pymarocco import *
from pyhalco_hicann_v2 import *
from pyhalco_common import *
from pyhmf import *

marocco = PyMarocco()
marocco.backend = PyMarocco.ESS

setup(marocco=marocco)

# create neuron with v_rest below v_thresh
p0 = Population(1, EIF_cond_exp_isfa_ista, {
         'v_rest': -50.,
         'v_thresh': -60.,
         'v_reset': -70.6,
    })
p1 = Population(1, EIF_cond_exp_isfa_ista)

# place them on different HICANNs
marocco.placement.add(p0, HICANNGlobal(Enum(0)))
marocco.placement.add(p1, HICANNGlobal(Enum(1)))

connector = AllToAllConnector(
        allow_self_connections=True,
        weights=1.)

proj = Projection(p0, p1, connector, target='excitatory')
stored_weights = copy.deepcopy(proj.getWeights())

run(1)
end()

print("python synapse loss: ", marocco.stats.getSynapseLoss())

if marocco.stats.getWeights(proj) != stored_weights:
    raise RuntimeError("weights don't match")
