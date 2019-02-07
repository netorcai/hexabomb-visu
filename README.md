hexabomb-visu
=============
SFML visualization client for [hexabomb].

Getting dependencies
--------------------

hexabomb-visu uses [Boost], [SFML] and [netorcai-client-cpp].
The first two should be installable by your distribution's package manager.
Once they are installed, the following script installs [netorcai-client-cpp]
and its remaining dependency.

```bash
# Adjust this variable to decide where the dependencies should be installed.
# For a system installation, /usr should be fine.
DEPS_INSTALL_DIRECTORY=/tmp/dependencies-install-directory

# Get and install nlohmann_json-3.5.0
git clone https://github.com/nlohmann/json.git -b v3.5.0 --single-branch --depth 1
(cd json && meson build --prefix=${DEPS_INSTALL_DIRECTORY})
(cd json/build && ninja install)

# Tell pkg-config to search for dependencies in the previously set install directory.
# This is not required if the dependencies are installed in a standard path such as /usr
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${DEPS_INSTALL_DIRECTORY}/lib/pkgconfig"

# Get and install netorcai-client-cpp.
git clone https://github.com/netorcai/netorcai-client-cpp.git
(cd netorcai-client-cpp && meson build --prefix=${DEPS_INSTALL_DIRECTORY})
(cd netorcai-client-cpp/build && ninja install)
```

Build instructions
------------------

hexabomb-visu can be built thanks to [Meson] and [Ninja].

```bash
# Tell pkg-config to search for dependencies in the previously set install directory.
# This is not required if the dependencies are installed in a standard path such as /usr
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${DEPS_INSTALL_DIRECTORY}/lib/pkgconfig"

# Create a ninja build directory in ./build
meson build

# Compile the project.
ninja -C build
```

Run instructions
----------------

```bash
# Tell the system that shared libraries can be loaded from the previously set install directory.
# This is not required if the dependencies are installed in a standard path such as /usr
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${DEPS_INSTALL_DIRECTORY}/lib"

# Run the project. Use --help to see available options.
./build/hexabomb-visu
```

[Boost]: https://www.boost.org
[hexabomb]: https://github.com/netorcai/hexabomb
[netorcai-client-cpp]: https://github.com/netorcai/netorcai-client-cpp
[pkg-config]: https://www.freedesktop.org/wiki/Software/pkg-config
[SFML]: https://www.sfml-dev.org
[Meson]: https://mesonbuild.com/
[Ninja]: https://ninja-build.org/
