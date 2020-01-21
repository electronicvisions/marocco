import unittest

from pyhalco_common import Enum
import pyhalco_hicann_v2 as C
import pyhmf as pynn
import pylogging
import pysthal

import utils

import pyredman
from pymarocco.runtime import Runtime


class Test_Switch_Usage(utils.TestWithResults):
    def test_L1_detour_at_side_switch_usage(self):
        """
                                  [155]
                                   191
            [223]  224  225 x226x {227}

            test detour and predecessor settings at the edge of a wafer

        """

        pylogging.set_loglevel(pylogging.get("marocco"),
                               pylogging.LogLevel.TRACE)
        pylogging.set_loglevel(pylogging.get("Calibtic"),
                               pylogging.LogLevel.ERROR)

        self.marocco.persist = ''  # or add test suite TestWithRuntime?

        runtime = Runtime(self.marocco.default_wafer)
        pynn.setup(marocco=self.marocco, marocco_runtime=runtime)

        settings = pysthal.Settings.get()

        settings.synapse_switches.max_switches_per_column_per_side = 1
        settings.crossbar_switches.max_switches_per_row = 1

        source = pynn.Population(1, pynn.IF_cond_exp, {})
        target1 = pynn.Population(1, pynn.IF_cond_exp, {})
        target2 = pynn.Population(1, pynn.IF_cond_exp, {})

        proj = pynn.Projection(
            source, target1, pynn.AllToAllConnector(weights=1.))
        proj = pynn.Projection(
            source, target2, pynn.AllToAllConnector(weights=1.))

        source_hicann = C.HICANNOnWafer(Enum(227))
        target1_hicann = C.HICANNOnWafer(Enum(155))
        target2_hicann = C.HICANNOnWafer(Enum(225))

        self.marocco.manual_placement.on_hicann(source, source_hicann)
        self.marocco.manual_placement.on_hicann(target1, target1_hicann)
        self.marocco.manual_placement.on_hicann(target2, target2_hicann)

        disabled_hicanns = [226, 263]
        wafer = self.marocco.default_wafer
        self.marocco.defects.set(pyredman.Wafer(runtime.wafer().index()))
        for hicann in C.iter_all(C.HICANNOnWafer):
            if hicann.toEnum().value() in disabled_hicanns:
                self.marocco.defects.wafer().hicanns().disable(C.HICANNGlobal(hicann, wafer))
            continue

        pynn.run(0)
        pynn.end()

        for hicann in runtime.wafer().getAllocatedHicannCoordinates():
            h = runtime.wafer()[hicann]
            print hicann, h.check()
            self.assertEqual(h.check(), "")

    def test_L1_detour_switch_usage(self):
        """
             [331]
             {351} x352x [353]

             test detour and predecessor settings in the wafer

        """

        pylogging.set_loglevel(pylogging.get("marocco"),
                               pylogging.LogLevel.TRACE)
        pylogging.set_loglevel(pylogging.get("Calibtic"),
                               pylogging.LogLevel.ERROR)

        self.marocco.persist = ''

        runtime = Runtime(self.marocco.default_wafer)
        pynn.setup(marocco=self.marocco, marocco_runtime=runtime)

        settings = pysthal.Settings.get()

        settings.synapse_switches.max_switches_per_column_per_side = 1
        settings.crossbar_switches.max_switches_per_row = 1

        source = pynn.Population(1, pynn.IF_cond_exp, {})
        target1 = pynn.Population(1, pynn.IF_cond_exp, {})
        target2 = pynn.Population(1, pynn.IF_cond_exp, {})

        proj = pynn.Projection(
            source, target1, pynn.AllToAllConnector(weights=1.))
        proj = pynn.Projection(
            source, target2, pynn.AllToAllConnector(weights=1.))

        source_hicann = C.HICANNOnWafer(Enum(351))
        target1_hicann = C.HICANNOnWafer(Enum(331))
        target2_hicann = C.HICANNOnWafer(Enum(353))

        self.marocco.manual_placement.on_hicann(source, source_hicann)
        self.marocco.manual_placement.on_hicann(target1, target1_hicann)
        self.marocco.manual_placement.on_hicann(target2, target2_hicann)

        disabled_hicanns = [352]
        wafer = self.marocco.default_wafer
        self.marocco.defects.set(pyredman.Wafer(runtime.wafer().index()))
        for hicann in C.iter_all(C.HICANNOnWafer):
            if hicann.toEnum().value() in disabled_hicanns:
                self.marocco.defects.wafer().hicanns().disable(C.HICANNGlobal(hicann, wafer))
            continue

        pynn.run(0)
        pynn.end()

        for hicann in runtime.wafer().getAllocatedHicannCoordinates():
            h = runtime.wafer()[hicann]
            print hicann, h.check()
            self.assertEqual(h.check(), "")


if __name__ == '__main__':
    unittest.main()
