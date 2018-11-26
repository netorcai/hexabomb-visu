project('hexabomb-visu', 'cpp',
    default_options: ['cpp_std=c++17'],
    version: '0.1.0'
)

netorcai_client_cpp_dep = dependency('netorcai-client-cpp',
    fallback: ['netorcai-client-cpp', 'netorcai_client_cpp_dep']
)
sfml_graphics_dep = dependency('sfml-graphics', required: true)

src = [
    'src/main.cpp',
    'src/hexabomb-parse.cpp',
    'src/hexabomb-parse.hpp',
    'src/renderer.cpp',
    'src/renderer.hpp'
]

visu = executable('hexabomb-visu', src,
    dependencies: [netorcai_client_cpp_dep, sfml_graphics_dep],
    install: true, install_dir: 'bin'
)
