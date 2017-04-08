# Project Carrot Tileset Converter

An utility program to convert a Jazz Jackrabbit 2 tileset file (`.j2t`) into
the format recognized by [Project Carrot](https://github.com/soulweaver91/project-carrot).

You can download prebuilt Windows binaries over at the 
[Project Carrot homepage](https://carrot.soulweaver.fi/).

## Usage

```
pctc name path\to\Tileset.j2t
```

You can pick any name you want; the tileset will be built into a folder by that name
and levels also look up the tileset by also using that name. If the name is omitted,
the filename will be used as the name.

## Development

Compiling requires [SFML 2.x](http://www.sfml-dev.org/download.php) and
[Qt 5.x](http://www.qt.io/download/).

### Windows

To build with Visual Studio 2015, configure the Qt settings with the MSVS Qt 5 plugin,
then set up the macro `SFML_DIR` in the user property sheets (`Microsoft.Cpp.Win32.user` and 
`Microsoft.Cpp.x64.user`) to point to the location of your SFML installation. If you
followed the main repository instructions, you have already done this.

MSVS is not strictly required for building, but you're on your own if you prefer using some other
compiler for building on Windows.

### Linux

First set up the environment as instructed in the repository for
[Project Carrot](https://github.com/soulweaver91/project-carrot), clone this project
to a folder, navigate to it, and:

```shell
qmake -spec linux-clang
make release
```

This will build the executable to `Release/PCTilesetConverter`.

### macOS

Follow the Linux instructions, but use `macx-clang` as the spec instead.

## License
This software is licensed under the [MIT License](https://opensource.org/licenses/MIT).
See the included `LICENSE` file for the licenses of the third-party libraries used.
