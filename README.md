# Rasterize – SVG to PNG Converter

A lightweight command-line tool (Visual Studio 2022 / C++17) that converts an SVG file to a PNG image at an arbitrary resolution.

## Features

- Accepts any SVG file as input
- Output width, height, or uniform scale factor can be specified on the command line
- Aspect ratio is preserved automatically when only one dimension is given
- Zero runtime dependencies – all rendering is done by the embedded [nanosvg](https://github.com/memononen/nanosvg) library; PNG output uses [stb_image_write](https://github.com/nothings/stb)

## Building

Open `Rasterize.sln` in **Visual Studio 2022** (or later) and build the solution.
Supported configurations: `Debug|Win32`, `Release|Win32`, `Debug|x64`, `Release|x64`.

The compiled binary is placed in `bin\<Platform>\<Configuration>\Rasterize.exe`.

## Usage

```
Rasterize <input.svg> [options]

Options:
  -o, --output <file>     Output PNG file (default: <input>.png)
  -w, --width  <pixels>   Output width in pixels
  -h, --height <pixels>   Output height in pixels
  -s, --scale  <factor>   Uniform scale factor (e.g. 2.0 for 2×)
      --help              Show this help
```

### Examples

```bat
REM Convert at the SVG's natural size
Rasterize logo.svg

REM Convert to a fixed width; height is computed to keep aspect ratio
Rasterize logo.svg --width 1920

REM Convert to an exact size (may change aspect ratio)
Rasterize logo.svg --width 1920 --height 1080

REM Scale by 3× and write to a specific file
Rasterize logo.svg --scale 3.0 --output logo_3x.png
```

## Third-party libraries

| Library | License | Location |
|---|---|---|
| [nanosvg](https://github.com/memononen/nanosvg) | zlib | `Rasterize/include/nanosvg.h`, `nanosvgrast.h` |
| [stb_image_write](https://github.com/nothings/stb) | MIT / Public Domain | `Rasterize/include/stb_image_write.h` |
