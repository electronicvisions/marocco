#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
functions for debugging the HICANN configuration
"""

from pyhalco_common import top, bottom, left, right
import pyhalco_hicann_v2 as Coord
import pysthal
import pymarocco


def load_hicann_cfg(wafer_cfg, h):
    """
    load HICANN config from file

    params:
      wafer_cfg - filename of dumped wafer config (cf. PyMarocco.wafer_cfg)
      h         - HICANNOnWafer coordinate
    """
    assert isinstance(h, Coord.HICANNOnWafer)
    w = pysthal.Wafer()
    w.load(wafer_cfg)
    return w[h]

def get_syn_in_side(row_config):
    """
    extracts from a synapse row config the side, to which all synaptic input of a synapse row are connected to.
    returns either left or right.
    """

    left_cfg = row_config.get_syn_in(left)
    right_cfg = row_config.get_syn_in(right)
    assert int(left_cfg) + int(right_cfg) == 1
    if left_cfg:
        return left
    else:
        return right

def find_synapses(synapses, drv, addr):
    """
    finds synapses with a given l1 addr on a synapse driver
    returns the Coordinates of all synapses on a synapse driver, which decode
    the L1 address, by comparing the 2 MSB per strobe Pattern and the 4 LSB per
    synapse

    params:
      synapses - a pysthal.SynapseArray object
      drv      - synapse driver coordinate
      addr     - L1Address of source neuron
    """

    sd = addr.getSynapseDecoderMask()
    dd = addr.getDriverDecoderMask()

    rv = []
    drv_cfg = synapses[drv]

    for row_on_driver in (top,bottom):
        row_addr = Coord.SynapseRowOnHICANN(drv,row_on_driver)
        decoders = synapses[row_addr].decoders
        # check 2 MSB
        row_cfg = drv_cfg[Coord.RowOnSynapseDriver(row_on_driver)]
        # halbe uses SideVertical to represent even and odd columns
        for parity in (top,bottom):
            if row_cfg.get_decoder(parity) == dd:
                for i in range(int(parity),len(decoders),2):
                    if decoders[i] == sd:
                        rv.append(Coord.SynapseOnHICANN(row_addr, Coord.SynapseColumnOnHICANN(i)))
    return rv
