project(
    'bhashmap',
    'c',
    license: 'MIT',
    license_files: 'LICENSE',
    default_options: [
        'warning_level=2'
    ]
)

cargs = []
if get_option('debug_functions')
    cargs += '-DBHM_DEBUG'
endif

if get_option('debug_benchmark')
    cargs += '-DBHM_DEBUG_BENCHMARK'
endif

incdir = include_directories('src/include/')

lib_main = library(
    'bhashmap',
    'src/bhashmap.c',
    include_directories: incdir,
    c_args: cargs,
    install: true
)

install_headers('src/include/bhashmap.h')

if get_option('build_benchmarks')
    executable(
        'bench_words400k',
        'src/benchmarks/words400k.c',
        include_directories: incdir,
        link_with: lib_main
    )
endif
