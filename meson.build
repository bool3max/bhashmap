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

incdir = include_directories('src/include/')

lib_main = library(
    'bhashmap',
    'src/bhashmap.c',
    include_directories: incdir,
    c_args: cargs
)

if get_option('build_benchmarks')
    word_linecount = run_command('bash', '-c', 'wc -l $MESON_SOURCE_ROOT/words.txt | cut -f 1 -d " "', check: true).stdout().strip()
    executable(
        'bench_words400k',
        'src/benchmarks/words400k.c',
        include_directories: incdir,
        link_with: lib_main,
        c_args: [
            '-std=gnu11',
            '-DWORDSTXT_COUNT=' + word_linecount
        ]
    )

endif