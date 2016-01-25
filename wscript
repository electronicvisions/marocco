#!/usr/bin/env python


def remove_ndebug_from_pyext(cfg):
    for module in [ 'PYEMBED', 'PYEXT' ]:
        try:
            defines_module = 'DEFINES_%s' % module
            cfg.env[defines_module] = filter(lambda a: a != 'NDEBUG', cfg.env[defines_module])
        except:
            pass


def depends(ctx):
    ctx('rant')
    ctx('halbe')
    ctx('ester')
    ctx('ester', 'mpi')
    ctx('sthal')
    ctx('pyhmf')
    ctx('redman')
    ctx('redman', 'backends')
    ctx('redman', 'pyredman')
    ctx('calibtic')
    ctx('calibtic', 'HMF')
    ctx('calibtic', 'backends')
    ctx('lib-boost-patches')
    ctx('marocco', 'nanoflann')
    ctx('marocco', 'pymarocco')
    ctx('marocco', 'tools/roqt')
    ctx('marocco', 'test')
    ctx('logger')


def options(opt):
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load('post_task')


def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('boost')
    cfg.load('post_task')

    compiler_version_error = "g++-4.7, clang-3.1 or newer required"
    cfg.check_cxx(
        msg      = "Checking compiler version",
        errmsg   = compiler_version_error,
        type     = "cxx",
        cxxflags = '-std=c++11',
        execute  = False,
        fragment = """#if __cplusplus != 201103L
                          #error %(MSG)s
                      #endif
                   """ % { "MSG" : compiler_version_error })


    cfg.check_boost(lib='serialization filesystem system '
            'thread program_options mpi graph regex',
            uselib_store='BOOST4MAROCCO')

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
    flags = {
            "cxxflags" : [
                '-g', '-O0', '-std=c++0x',
                '-pedantic', '-Wall', '-Wextra',
                '-Wno-c++0x-compat',
                '-Wno-literal-suffix', # squelch warnings from openMPI's mpi.h
                '-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE',
                '-DBOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING',
            ],
            "linkflags" : []
        }

    bld(target          = 'marocco_inc',
        use             = [
            'boost_patches',
            'sthal_inc',
            'redman_inc',
            'euter_inc',
            'nanoflann_inc',
            'marocco_coordinates_inc',
            ],
        export_includes = '.')

    bld(target          = 'marocco',
        features        = 'cxx cxxshlib post_task',
        source          =
            bld.path.ant_glob('marocco/**/*.cpp', excl=['marocco/coordinates/**/*']) +
            bld.path.ant_glob('control/**/*.cpp') +
            bld.path.ant_glob('experiment/**/*.cpp') +
            bld.path.ant_glob('pymarocco/*.cpp'),
        install_path    = 'lib',
        use             = [
            'USE4MAROCCO',
            'BOOST4MAROCCO',
            'LOG4CXXMAROCCO',
            'TBB4MAROCCO',
            'marocco_inc',
            'pyroqt_obj',
            'logger_obj',
            'ZTL',
            'rant',
            'euter',
            'mpiconfig', # adds MPI includes, linkflags, ...
            'halbe',
            'sthal',
            'hmf_calibration',
            'redman',
            'marocco_coordinates',
            ],
        post_task = bld.options.with_mongo and ['calibtic_mongo'] or [],
        **flags)

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
        **flags)

    bld(target          = 'mapper',
        features        = 'cxx cxxprogram post_task pyembed', # KHS: quickfix, fixing pyembed could be better
        source          = ['main.cpp', 'marocco/Mapper.cpp'],
        install_path    = 'bin',
        use             = [
            'marocco',
        ],
        post_task = [
            'ester',
            'pyhmf',
            'pyester',
            'pyroqt',
            'pyredman',
            ],
        **flags)

    bld.install_files(
        '${PREFIX}/bin/tools',
        bld.path.ant_glob('tools/*.py'),
        chmod=0755,
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
