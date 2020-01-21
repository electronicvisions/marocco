#!/usr/bin/env python

import argparse

from pysthal.command_line_util import parse_hicann
from pyhalco_hicann_v2 import *
import pyalone
import pymarocco_coordinates
import pylogging


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--source-hicann', required=True, type=parse_hicann,
        metavar='<enum>|<x>,<y>', help='source HICANNOnWafer')
    parser.add_argument(
        '--source-hline', required=False, type=int,
        help='source HLineOnHICANN')
    parser.add_argument(
        '--source-vline', required=False, type=int,
        help='source VLineOnHICANN')
    parser.add_argument(
        '--target-hicann', required=True, type=parse_hicann,
        metavar='<enum>|<x>,<y>', help='target HICANNOnWafer')
    parser.add_argument(
        '--target-vertical', action='store_true',
        help='target should be VLineOnHICANN')
    parser.add_argument(
        '--without-hicann', default=[], type=parse_hicann, nargs="+",
        metavar='<enum>|<x>,<y>', help='unavailable HICANNOnWafer')
    args = parser.parse_args()

    pylogging.reset()
    pylogging.default_config(date_format='absolute')

    alone = pyalone.Alone()
    for hicann in iter_all(HICANNOnWafer):
        if hicann in args.without_hicann:
            continue
        alone.add(hicann)

    if args.source_hline:
        line = HLineOnHICANN(args.source_hline)
    elif args.source_vline:
        line = VLineOnHICANN(args.source_vline)
    else:
        parser.error(
            'Please specify one of --source-hline / --source-vline')
    source = pyalone.L1BusOnWafer(args.source_hicann, line)
    target = pyalone.Target(
        args.target_hicann,
        vertical if args.target_vertical else horizontal)
    routes = alone.find_routes(source, target)
    for route in routes:
        print route

if __name__ == '__main__':
    main()
