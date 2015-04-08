#!/usr/bin/env python
import sys, os

try:
    from waflib.extras import symwaf2ic
    recurse = lambda ctx: None
except ImportError:
    from symwaf2ic import recurse_depends
    recurse = lambda ctx: recurse_depends(depends, ctx)


def check_version_cxx(ctx):
    error = "g++-4.7, clang-3.1 or newer required"

    ctx.check_cxx(
        msg      = "Checking compiler version",
        errmsg   = error,
        type     = "cxx",
        cxxflags = '-std=c++0x',
        execute  = False,
        fragment = """
                       #if __cplusplus != 201103L
                         #error %(MSG)s
                       #endif
                   """ % { "MSG" : error })

def remove_ndebug_from_pyext(cfg):
    for module in [ 'PYEMBED', 'PYEXT' ]:
        try:
            defines_module = 'DEFINES_%s' % module
            cfg.env[defines_module] = filter(lambda a: a != 'NDEBUG', cfg.env[defines_module])
        except:
            pass

def load_waf_modules(ctx, modules):
    for mo in modules:
        ctx.load(mo)

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
    ctx('marocco', 'metis/current')
    ctx('marocco', 'nanoflann')
    ctx('marocco', 'pymarocco')
    ctx('marocco', 'tools/roqt')
    ctx('marocco', 'test')
    ctx('symap2ic', 'src/logging')

def options(opt):
    recurse(opt)
    load_waf_modules(opt, ['compiler_cxx', 'boost', 'post_task'])

def configure(cfg):
    # first check deps of all dependent modules
    recurse(cfg)

    # then check own dependencies
    load_waf_modules(cfg, ['compiler_cxx', 'boost', 'post_task'])
    check_version_cxx(cfg)

    cfg.check_boost(lib='serialization filesystem system '
            'thread program_options mpi graph_parallel regex',
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
    recurse(bld)

    flags = {
            "cxxflags" : [
                '-g', '-O0', '-std=c++0x',
                '-pedantic', '-Wall', '-Wextra',
                '-Wno-c++0x-compat',
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
            ],
        export_includes = '.')

    bld(target          = 'marocco',
        features        = 'cxx cxxshlib post_task',
        source          =
            bld.path.ant_glob('marocco/**/*.cpp') +
            bld.path.ant_glob('control/**/*.cpp') +
            bld.path.ant_glob('experiment/**/*.cpp'),
        install_path    = 'lib',
        use             = [
            'USE4MAROCCO',
            'BOOST4MAROCCO',
            'LOG4CXXMAROCCO',
            'TBB4MAROCCO',
            'marocco_inc',
            'pymarocco_lib',
            'pyroqt_obj',
            'logger_obj',
            'ZTL',
            'rant',
            'metis',
            'euter',
            'mpiconfig', # adds MPI includes, linkflags, ...
            'halbe',
            'sthal',
            'hmf_calibration',
            'redman',
            ],
        post_task = bld.options.with_mongo and ['calibtic_mongo'] or [],
        **flags)

    bld(target          = 'mapper',
        features        = 'cxx cxxprogram post_task',
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
