hexabomb-visu
=============
SFML Visualization client for [hexabomb].

Getting dependencies
--------------------

hexabomb-visu uses [netorcai-client-cpp], [SFML] and [Boost].
Make sure the first two are installed in your system and found from [pkg-config].
Boost should be installed in your system.

**Important note**: Using graphical Nix packages on non-NixOS machines can be
[very tricky](https://github.com/NixOS/nixpkgs/issues/9415),
installing [SFML] from your distribution package manager is therefore recommended.

``` bash
# This should return no error.
pkg-config --cflags --libs netorcai-client-cpp

# Same here.
pkg-config --cflags --libs sfml-graphics
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
