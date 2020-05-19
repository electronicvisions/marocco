#!/usr/bin/env python


def remove_ndebug_from_pyext(cfg):
    for module in [ 'PYEMBED', 'PYEXT' ]:
        try:
            defines_module = 'DEFINES_%s' % module
            cfg.env[defines_module] = filter(lambda a: a != 'NDEBUG', cfg.env[defines_module])
        except:
            pass


def depends(ctx):
    ctx('hate')
    ctx('rant')
    ctx('halbe')
    ctx('sthal')
    ctx('pyhmf')
    ctx('redman')
    ctx('redman', 'pyredman')
    ctx('calibtic')
    ctx('lib-boost-patches')
    ctx('marocco', 'nanoflann')
    ctx('marocco', 'pymarocco')
    ctx('marocco', 'tools')
    ctx('marocco', 'test')
    ctx('logger')


def options(opt):
    opt.load('compiler_cxx')
    opt.load('boost')


def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('boost')

    cfg.check_boost(lib='serialization filesystem system '
            'thread program_options mpi graph regex',
            uselib_store='BOOST4MAROCCO')
    cfg.check_boost(lib='serialization', uselib_store='BOOST4MAROCCO_RESULTS')

    cfg.check_cxx(lib='log4cxx', uselib_store='LOG4CXXMAROCCO', mandatory=1)
    cfg.check_cxx(lib='tbb', uselib_store='TBB4MAROCCO', mandatory=1)

    cfg.env.DEFINES_USE4MAROCCO = [
            '__MAPPING__',
            # '-D_GLIBCXX_PROFILE',
            # '-D_GLIBCXX_DEBUG',
        ]

    # finally remove NDEBUG from env, previously added by python feature
    remove_ndebug_from_pyext(cfg)


def build(bld):
    cxxflags = [
        '-Wno-literal-suffix',  # squelch warnings from openMPI's mpi.h
    ]

    bld(target          = 'marocco_inc',
        export_includes = '.')

    bld(target          = 'marocco',
        features        = 'cxx cxxshlib',
        source          =
            bld.path.ant_glob(
                'marocco/**/*.cpp',
                excl=[
                    'marocco/coordinates/**/*',
                    'marocco/**/parameters/*',
                    'marocco/**/results/*',
                    'marocco/**/algorithms/*',
                ]) +
            bld.path.ant_glob('control/**/*.cpp'),
        install_path    = 'lib',
        use             = [
            'USE4MAROCCO',
            'BOOST4MAROCCO',
            'LOG4CXXMAROCCO',
            'TBB4MAROCCO',
            'marocco_inc',
            'logger_obj',
            'ZTL',
            'rant',
            'euter',
            'halbe',
            'sthal',
            'hmf_calibration',
            'redman',
            'marocco_coordinates',
            'marocco_parameters',
            'marocco_algorithms',
            'marocco_results',
            'pymarocco_cpp',
            'marocco_runtime',
            'nanoflann_inc'
            ],
        defines=['BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS'],
        export_defines=['BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS'],
        cxxflags=cxxflags)

    bld(target='marocco_coordinates_inc',
        export_includes='.')

    bld(target='marocco_coordinates',
        features='cxx cxxshlib',
        source=bld.path.ant_glob('marocco/coordinates/**/*.cpp'),
        install_path='lib',
        use=[
            'marocco_coordinates_inc',
            'halbe',
        ],
        # gccxml requires non-variadic implementation of boost::variant for python wrappers
        defines='BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES',
        export_defines='BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES',
        cxxflags=cxxflags)

    bld(target='marocco_parameters_inc',
        export_includes='.')

    bld(target='marocco_parameters',
        features='cxx cxxshlib',
        source=bld.path.ant_glob('marocco/**/parameters/*.cpp'),
        install_path='lib',
        use=[
            'marocco_parameters_inc',
            'marocco_algorithms',
            'marocco_coordinates',
            'halbe',
        ],
        cxxflags=cxxflags)

    bld(target='marocco_algorithms_inc',
        export_includes='.')

    bld(target='marocco_algorithms',
        features='cxx cxxshlib',
        source= bld.path.ant_glob('marocco/**/algorithms/*.cpp')
            + bld.path.ant_glob('marocco/**/internal/*.cpp')
            + bld.path.ant_glob('marocco/assignment/*.cpp')
            + bld.path.ant_glob('marocco/Logger.cpp')
            + bld.path.ant_glob('marocco/routing/results/*.cpp')
            + bld.path.ant_glob('marocco/placement/results/*.cpp')
        ,install_path='lib',
        use=[
            'marocco_algorithms_inc',
            'marocco_coordinates',
            'halbe',
            'euter',
            'calibtic',
        ],
        defines=['BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS'],
        export_defines=['BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS'],
        cxxflags=cxxflags)

    bld(target='marocco_results_inc',
        export_includes='.')

    bld(target='marocco_results',
        features='cxx cxxshlib',
        source=bld.path.ant_glob('marocco/**/results/*.cpp', excl='marocco/**/results/embind.cpp'),
        install_path='lib',
        use=[
            'marocco_results_inc',
            'marocco_coordinates',
            'halbe',
            'BOOST4MAROCCO_RESULTS',
            'euter',
        ],
        cxxflags=cxxflags)

    mapper_uses = ['marocco',
                   'pyhmf',
                   'pyredman',
                  ]

    bld.install_files(
        '${PREFIX}/bin/tools',
        bld.path.ant_glob('tools/*.py'),
        chmod=0o755,
        relative_trick=False
    )

def doc(dcx):
    '''build documentation (doxygen)'''

    dcx(
            features    = 'doxygen',                # the feature to use
            doxyfile    = 'doc/Doxyfile',           # a doxyfile, use doxygen -g to generate a template
            pars        = {'STRIP_FROM_PATH'   : dcx.path.get_src().abspath(),}, # a dict of doxy-pars: overrides doxyfile pars
    #        pdffile     = 'HMF_marocco-manual.pdf', # a pdf file to generate, relative to OUTPUT_DIRECTORY
    )
