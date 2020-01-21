"""
Test for issue #3086
Based on code sniplet in issue #3086
"""

import unittest

from pysthal.command_line_util import init_logger

import pyhmf as pynn
from pymarocco import PyMarocco
from pymarocco.coordinates import LogicalNeuron

from pyhalco_common import X, Y, Enum
import pyhalco_hicann_v2 as C

init_logger("TRACE", [])


class issue3086(unittest.TestCase):
    def test(self):

        wafer = 99999  # a wafer for which no redman data is availale
        hicann = 82
        neuron_number = 12

        marocco = PyMarocco()
        marocco.neuron_placement.default_neuron_size(4)
        marocco.backend = PyMarocco.None
        marocco.default_wafer = C.Wafer(wafer)

        used_hicann = C.HICANNGlobal(
            C.HICANNOnWafer(Enum(hicann)),
            C.Wafer(wafer))

        used_hicann  # prevent pep8 warning of unused variable

        pynn.setup(marocco=marocco)

        pop = pynn.Population(1, pynn.IF_cond_exp)
        topleft = C.NeuronOnWafer(C.NeuronOnHICANN(X(neuron_number), Y(0)),
                                  C.HICANNOnWafer(Enum(hicann)))
        logical_neuron = LogicalNeuron.rectangular(topleft, size=4)
        marocco.manual_placement.on_neuron(pop, logical_neuron)

        with self.assertRaises(RuntimeError):
            pynn.run(0)
            pynn.end()


if __name__ == "__main__":
    unittest.main()
