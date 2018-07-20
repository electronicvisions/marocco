#!/usr/bin/env python
import unittest

class Issue1550(unittest.TestCase):

    def test(self):

        import pyhmf as pynn
        from pymarocco import PyMarocco
        import pylogging, pyhalbe
        pyhalbe.Debug.change_loglevel(2)
        pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.TRACE)
        pylogging.set_loglevel(pylogging.get("sthal"), pylogging.LogLevel.DEBUG)

        marocco = PyMarocco()
        marocco.neuron_placement.default_neuron_size(4)

        pynn.setup(marocco=marocco)

        neuron1 = pynn.Population(1, pynn.IF_cond_exp)

        inh = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times':  [0]})
        exc = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times':  [0]})
        exc_2 = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times':[0]})
        exc_3 = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times':[0]})

        c_exc = pynn.FixedProbabilityConnector(p_connect=1.0, weights=1)

        proj1 = pynn.Projection(inh, neuron1, c_exc, target='excitatory')
        proj2 = pynn.Projection(exc, neuron1, c_exc, target='excitatory')
        proj3 = pynn.Projection(exc_2, neuron1, c_exc, target='excitatory')
        proj4 = pynn.Projection(exc_3, neuron1, c_exc, target='inhibitory')

        pynn.run(10000)
        pynn.end()

if __name__ == "__main__":
    unittest.main()
