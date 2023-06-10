project(
    'bhashmap',
    'c',
    license: 'MIT',
    license_files: 'LICENSE',
    default_options: [
        'c_std=c11'
    ]
)

cargs = []
if get_option('debug_functions')
    cargs += '-DBHM_DEBUG'
endif

lib_main = library(
    'bhashmap',
    'src/bhashmap.c',
    include_directories: include_directories('src/include/'),
    c_args: cargs
)