# Project Carrot Tileset Converter

An utility program to convert a Jazz Jackrabbit 2 tileset file (`.j2t`) into
the format recognized by [Project Carrot](https://github.com/soulweaver91/project-carrot).
At this time, this is simply a pair of images, one for the tiles themselves and
one for their masks.

You can download prebuilt Windows binaries over at the 
[Project Carrot homepage](https://carrot.soulweaver.fi/).

## Usage

```
pctc name path\to\Tileset.j2t
```

You can pick any name you want; the tileset will be built into a folder by that name
and levels also look up the tileset by also using that name.

## Development

Compiling requires [SFML 2.x](http://www.sfml-dev.org/download.php),
[Qt 5.x](http://www.qt.io/download/) and Microsoft Visual Studio 2015.
Configure the Qt settings with the MSVS Qt 5 plugin, then set up the macro
`SFML_DIR` in the user property sheets (`Microsoft.Cpp.Win32.user` and 
`Microsoft.Cpp.x64.user`) to point to the location of your SFML installation.

MSVS is not strictly required as long as you know how to set up a build
system manually, though.

## License
This software is licensed under the [MIT License](https://opensource.org/licenses/MIT).
See the included `LICENSE` file for the licenses of the third-party libraries used.
