project('hexabomb-visu', 'cpp',
    default_options: ['cpp_std=c++17'],
    version: '0.1.0'
)

netorcai_client_cpp_dep = dependency('netorcai-client-cpp',
    fallback: ['netorcai-client-cpp', 'netorcai_client_cpp_dep']
)

player = executable('hexabomb-visu', ['src/main.cpp'],
    dependencies: [netorcai_client_cpp_dep],
    install: true, install_dir: 'bin'
)
