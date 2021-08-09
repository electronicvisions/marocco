import os, sys, argparse
from pylab import *

def myplot(plots = {}, title='', xlabel='', ylabel=''):
    for fname, cols in plots.items():
        a = loadtxt(fname)
        if not isinstance(cols, list):
            cols = [cols,]

        for col in cols:
            plot(a[:,col])

    gca().set_xlabel(xlabel)
    gca().set_ylabel(ylabel)
    gca().set_title(title)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('files', metavar='F', type=str, nargs='+',
            help='list of files and/or columns to be plottet')
    parser.add_argument( '--title', type=str, default='')
    parser.add_argument('--xlabel', type=str, default='')
    parser.add_argument('--ylabel', type=str, default='')
    parser.add_argument('--output', '-o', type=str)
    args = parser.parse_args()

    plots = {}
    last = ''
    for fname in args.files + [last]:
        try:
            if not last:
                raise ValueError()
            col = int(fname)
            plots[last] = plots.get(last, []) + [col,]
        except ValueError:
            if last and not plots.get(last, None):
                plots[last] = [1,]
            last = str(fname)

    myplot(plots, args.title, args.xlabel, args.ylabel)

    if args.output:
        savefig('%s' % args.output)
    else:
        show()
