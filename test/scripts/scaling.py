import logging, numpy

Blue2Red= {
    'red':   (
        (0.0,  0.0, 0.0),
        (1.0,  1.0, 1.0)),
    'green': (
        (0.0,  0.0, 0.0),
        (1.0,  0.0, 0.0)),
    'blue':  (
        (0.0,  1.0, 1.0),
        (1.0,  0.0, 0.0))
    }


def build_network(num_pops, pop_size, marocco):
    from pymarocco import PyMarocco
    import pyhmf as pynn

    logging.info("num_pops: %d, pop_size: %d, total size: %d" %
            (num_pops, pop_size, num_pops*pop_size))

    pynn.setup(marocco=marocco)

    pops = [ pynn.Population(pop_size, pynn.EIF_cond_exp_isfa_ista) for x in
            range(num_pops) ]

    for idx, pop in enumerate(pops):
        connector = pynn.AllToAllConnector(
                allow_self_connections=True,
                weights=1.)

        # build ring like network topology
        pynn.Projection(pop, pops[(idx+1)%len(pops)],
                connector, target='excitatory')

        # add poisson stimulus
        source = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate' : 2})

        pynn.Projection(source, pop, connector, target='excitatory')

    pynn.run(1)
    pynn.end()

    stats = marocco.getStats()
    loss = float(stats.getSynapseLoss())/stats.getSynapses()
    return (num_pops, pop_size, loss)

def plot(results):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from scipy.interpolate import griddata

    results = numpy.array(results)
    x, y , z = results[:,0], results[:,1], results[:,2]

    # define grid & grid data
    xi = numpy.linspace(x.min(), x.max(), 100)
    yi = numpy.linspace(y.min(), y.max(), 100)
    zi = griddata((x, y), z, (xi[None,:], yi[:,None]), method='nearest')

    fig = plt.Figure()

    extent = (x.min(), x.max(), y.min(), y.max()) # extent of the plot
    im = plt.imshow(zi, extent=extent, origin='lower',
            aspect='auto', interpolation='none',
            cmap=matplotlib.cm.get_cmap('Reds'))
            #cmap=matplotlib.colors.LinearSegmentedColormap('BlueRed', BlueRed))
    im.set_norm(matplotlib.colors.Normalize(vmin=0, vmax=1.)) #vmax=z.max()))
    fig.colorbar(im) # draw colorbar
    plt.colorbar()

    ax = plt.gca()
    ax.scatter(x,y,marker='o',c='b',s=5) # plot data points.
    ax.set_xlabel('populations')
    ax.set_ylabel('size')

    return fig

def run():
    results = []
    for num_pops in [ 1, 10, 25, 50, 100, 200, 300, 400 ]:
        for pop_size in [ 1, 16, 32, 64, 96 ]:
            marocco = PyMarocco()
            result = build_network(num_pops, pop_size, marocco)
            results.append(result)
            logging.info(results[-1])

    return numpy.array(results)

def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=str, nargs='?')
    parser.add_argument('--output', type=str, default='')
    args = parser.parse_args()

    data = numpy.loadtxt(args.file)
    fig = plot(data)

    from pylab import *
    if args.output:
        savefig(args.output)
    else:
        show()

if __name__ == '__main__':
    main()
