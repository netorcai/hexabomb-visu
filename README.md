hexabomb-visu
=============
SFML Visualization client for [hexabomb].

Getting dependencies
--------------------

hexabomb-visu uses [Boost], [SFML] and [netorcai-client-cpp].
The first two should be installable by your distribution's package manager.
Once they are installed, the following script installs [netorcai-client-cpp]
and its remaining dependency.

``` bash
INSTALL_DIRECTORY=/usr
# IMPORTANT NOTE: If you change the install directory,
# make sure ${INSTALL_DIRECTORY}/lib/pkgconfig is in your pkg-config path
# (environment variable $PKG_CONFIG_PATH)

# Get and install nlohmann_json-3.5.0
git clone https://github.com/nlohmann/json.git -b v3.5.0 --single-branch --depth 1
(cd json && meson build --prefix=${INSTALL_DIRECTORY})
(cd json/build && ninja install)

# Get and install netorcai-client-cpp
git clone https://github.com/netorcai/netorcai-client-cpp.git
(cd netorcai-client-cpp && meson build --prefix=${INSTALL_DIRECTORY})
(cd netorcai-client-cpp/build && ninja install)
```

Build instructions
------------------

```bash
meson build
(cd build && ninja)
```

Run instructions
----------------

```bash
./build/hexabomb-visu
```

[Boost]: https://www.boost.org
[hexabomb]: https://github.com/netorcai/hexabomb
[netorcai-client-cpp]: https://github.com/netorcai/netorcai-client-cpp
[pkg-config]: https://www.freedesktop.org/wiki/Software/pkg-config
[SFML]: https://www.sfml-dev.org
