from pyhalco_hicann_v2 import HICANNOnWafer, DNCMergerOnHICANN, DNCMergerOnWafer
from pyhalco_hicann_v2 import VLineOnHICANN, VLineOnWafer, SynapseSwitchRowOnHICANN
from pyhalco_hicann_v2 import HLineOnHICANN, HLineOnWafer, CrossbarSwitchOnHICANN
from pyhalco_common import X, Y, iter_all, Side
from pymarocco.results import Marocco
from collections import defaultdict
from itertools import combinations
import pysthal


###
# Extracts used V/HLineOnWafer grouped by common source (DNCMergerOnWafer)
# and CrossbarSwitchesOnHICANN grouped per HICANN from routing results
###
def get_components(result):
    routes = defaultdict(set)
    crossbars = defaultdict(set)

    for route in result.l1_routing:
        merger = route.source()
        hicann = None
        prev = None
        for elem in route.route():
            if isinstance(elem, VLineOnHICANN):
                line = VLineOnWafer(elem, hicann)
                if isinstance(prev, HLineOnHICANN):
                    cs = CrossbarSwitchOnHICANN(X(line.value()), Y(prev.value()))
                    crossbars[hicann].add(cs)
                prev = elem
            elif isinstance(elem, HLineOnHICANN):
                line = HLineOnWafer(elem, hicann)
                if isinstance(prev, VLineOnHICANN):
                    cs = CrossbarSwitchOnHICANN(X(prev.value()), Y(line.value()))
                    crossbars[hicann].add(cs)
                prev = elem
            elif isinstance(elem, HICANNOnWafer):
                hicann = elem
                prev = elem
                continue
            elif isinstance(elem, DNCMergerOnHICANN):
                assert(merger == DNCMergerOnWafer(elem, hicann))
                prev = elem
                continue
            routes[merger].add(line)
    print("Routes extracted")
    return(routes, crossbars)


###
# Checks if buses are uniquely used in routes of different dncmergers
###
def check_unique_bus(routes):
    for a, b in combinations(routes.values(), 2):
        if len(a & b):
            raise RuntimeError("Routes from different dnc mergers use same bus")
    print("No duplicate bus usage in routes from different dncmergers found")


###
# Checks if there is only one crossbar switch used per row and column on one HICANN
###
def check_crossbars(crossbars):
    for switches in crossbars.values():
        length = len(switches)
        # Check for crossbar switches in same column
        if len({s.x() for s in switches}) != length:
            raise RuntimeError("More than one crossbar switch in same column on same HICANN")
        # Check for crossbar switches in same row
        if len({s.y() for s in switches}) != length:
            raise RuntimeError("More than one crossbar switch in same row on same HICANN")

    print("No usage of more than one crossbar switch per row or column on same HICANN")


###
# Checks if same crossbar switches are set in routing and sthal wafer
###
def check_switches_on_hw(wafer, crossbars):
    for hicann_c in wafer.getAllocatedHicannCoordinates():
        hicann = wafer[hicann_c]
        switches = crossbars[hicann_c]
        for cs in switches:
            if not hicann.crossbar_switches.get(VLineOnHICANN(cs.x()), HLineOnHICANN(cs.y())):
                raise RuntimeError("Switch used in routing not set on hardware")
        # count switches used on hardware
        n_switches = 0
        for hl in iter_all(HLineOnHICANN):
            for s in iter_all(Side):
                n_switches += hicann.crossbar_switches.get_row(hl, s).count(True)
        if n_switches != len(switches):
            raise RuntimeError("More crossbar switches set on hardware compared to routing")
    print("Same switches are used on hardware and routing")


###
# Checks synapse switches
# Checks if there is only one synapse switch used per vline on one HICANN
# Checks if there is only one synapse switch connected to each synapse driver,
# also considering the driver chain. Neglecting possible connections from neighboring HICANNs
# Checks driver chain length restriction
# Checks if same synapse switches are set in routing and sthal wafer
###
def check_synapse_switches(wafer, result):
    for hicann_c in wafer.getAllocatedHicannCoordinates():
        hicann = wafer[hicann_c]
        switches = result.synapse_routing[hicann_c].synapse_switches()
        used_vlines = [s.source() for s in switches]
        if len(used_vlines) != len(set(used_vlines)):
            raise RuntimeError("One VLine uses more than one synapse switch (Might also be allowed)")
        # Neglect injection by neighboring HICANN
        used_drivers = [d for s in switches for d in s.connected_drivers().drivers()]
        if len(used_drivers) != len(set(used_drivers)):
            raise RuntimeError("One Synapse Driver connects to more than one SynapseSwitch")
        for s in switches:
            if len(s.connected_drivers().drivers()) > 3:
                raise RuntimeError("Driver chain length > 3 used")
            if not hicann.synapse_switches.get(s.source(), s.connected_drivers().primary_driver().y()):
                raise RuntimeError("Synapse switches used in routing not set on hardware")
        # count switches used on hardware
        n_switches_hw = 0
        for sr in iter_all(SynapseSwitchRowOnHICANN):
            n_switches_hw += hicann.synapse_switches.get_row(sr).count(True)
        n_switches_routing = sum(1 for s in switches)
        if n_switches_hw != n_switches_routing:
            raise RuntimeError("More synapse switches set on hardware compared to routing")
    print("Checked synapse switches")


def run_all_tests(routing_results_path, wafer_results_path):
    result = Marocco.from_file(routing_results_path)
    print("Results loaded")
    routes, crossbars = get_components(result)
    check_unique_bus(routes)
    check_crossbars(crossbars)
    if wafer_results_path is not None:
        wafer = pysthal.Wafer()
        wafer.load(wafer_results_path)
        check_switches_on_hw(wafer, crossbars)
        check_synapse_switches(wafer, result)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('routing_results', type=str, help="Marocco routing results")
    parser.add_argument('wafer_results', default=None, type=str, help="Wafer settings results")
    args = parser.parse_args()

    run_all_tests(args.routing_results, args.wafer_results)
