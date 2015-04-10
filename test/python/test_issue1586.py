#!/usr/bin/python
import pylogging
pylogging.default_config(pylogging.LogLevel.INFO)

def log_to_file():
    layout = pylogging.ColorLayout(False)
    for domain in ["ESS","hal2ess","Calibtic", "marocco"]:
        appender = pylogging.FileAppender(layout, "{}.log".format(domain), False)
        log = pylogging.get(domain)
        log.addAppender(appender)
        log.setAdditivity(False)
        pylogging.set_loglevel(log, pylogging.LogLevel.TRACE)

from pyhmf import *
from pymarocco import PyMarocco
from pyhalbe.Coordinate import *

marocco = PyMarocco()
marocco.placement.setDefaultNeuronSize(8)
#hicannIndices=[0,1,2,3,4,5,6,7,12,13,14,15,16,17,18,19,28,29,30,31,32,33,34,35,48,49,50,51,52,53,54,55]
#marocco.placement.use = [HICANNGlobal(Enum(ii)) for ii in hicannIndices]
marocco.backend = PyMarocco.ESS
marocco.experiment_time_offset=5e-7

n_exc = 100 # Number of excitatory neurons per group

sim_duration = 200.

pp_start = 50. # start = center of pulse-packet

weight_exc = 0.002 # uS weight for excitatory to excitatory connections (double than in reference paper)

setup(
    max_delay = 20.,
    marocco=marocco,
    )

exc_pop = Population(n_exc, IF_cond_exp)
exc_pop.record()
pop_stim = Population(n_exc, SpikeSourceArray, {'spike_times':[pp_start]},label= "pop_stim")
Projection( pop_stim, exc_pop, FixedNumberPreConnector(60, weights=weight_exc, delays=20.), target='excitatory')

log_to_file()
run(sim_duration)
end()
