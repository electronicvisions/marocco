#!/usr/bin/python
import unittest
import pylogging
import pyhmf as pynn
import pymarocco

def ESS_available():
    try:
        marocco = pymarocco.PyMarocco()
        marocco.backend = pymarocco.PyMarocco.ESS
        pynn.setup(marocco=marocco)
        pynn.run(1.)
        return True
    except RuntimeError as err:
        if err.message == "ESS not available (compile with ESS)":
            return False
        else:
            raise err


@unittest.skipUnless(ESS_available(), "Test requires ESS")
class TestIssue1586(unittest.TestCase):
    """
    The ESS experiment below resulted in a IndexError in ReverseMapping when 2
    or more HICANNS are used.

    Note: the experiment represents the first exc. group of the synfire chain.
    """

    def runTest(self):
        pylogging.reset()
        pylogging.default_config(pylogging.LogLevel.INFO)

        marocco = pymarocco.PyMarocco()
        marocco.neuron_placement.default_neuron_size(8)

        marocco.backend = pymarocco.PyMarocco.ESS
        marocco.experiment_time_offset=5e-7

        n_exc = 100 # Number of excitatory neurons per group

        sim_duration = 200.

        pp_start = 50. # start = center of pulse-packet

        weight_exc = 0.002 # uS weight for excitatory to excitatory connections
                           # (double than in reference paper)

        pynn.setup(
            max_delay = 20.,
            marocco=marocco,
            )

        # v_thresh close to v_rest to make sure there are some spikes
        neuron_params = {
            'v_rest': -65.,
            'v_thresh': -62.5,
            }

        exc_pop = pynn.Population(n_exc, pynn.IF_cond_exp, neuron_params)
        exc_pop.record()
        pop_stim = pynn.Population(n_exc, pynn.SpikeSourceArray, {'spike_times':[pp_start]})
        conn = pynn.FixedNumberPreConnector(60, weights=weight_exc, delays=20.)
        pynn.Projection( pop_stim, exc_pop, conn, target='excitatory')

        pynn.run(sim_duration)
        pynn.end()


if __name__ == '__main__':
    unittest.main()
