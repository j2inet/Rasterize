// SVG to PNG converter
// Usage: Rasterize <input.svg> [options]
//
// Options:
//   -o, --output <file>     Output PNG file path (default: input filename with .png extension)
//   -w, --width <pixels>    Output width in pixels
//   -h, --height <pixels>   Output height in pixels
//   -s, --scale <factor>    Scale factor (e.g. 2.0 for 2x). Overrides --width/--height.
//   --help                  Show this help message
//
// If only --width or only --height is specified, the other dimension is computed
// to preserve the SVG's aspect ratio. If neither is specified and no scale is
// given, the SVG's natural pixel dimensions are used (1 unit = 1 pixel).

#define NANOSVG_IMPLEMENTATION
#include "../include/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "../include/nanosvgrast.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

static void print_usage(const char* prog)
{
    printf("Usage: %s <input.svg> [options]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -o, --output <file>     Output PNG file (default: <input>.png)\n");
    printf("  -w, --width  <pixels>   Output width in pixels\n");
    printf("  -h, --height <pixels>   Output height in pixels\n");
    printf("  -s, --scale  <factor>   Uniform scale factor (overrides -w/-h)\n");
    printf("      --help              Show this help\n");
    printf("\n");
    printf("When only one of --width/--height is provided the other dimension is\n");
    printf("derived automatically to preserve the aspect ratio.\n");
}

// Replace extension of `path` with ".png"
static std::string default_output_path(const std::string& svg_path)
{
    size_t dot = svg_path.rfind('.');
    if (dot != std::string::npos)
        return svg_path.substr(0, dot) + ".png";
    return svg_path + ".png";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char* input_path  = nullptr;
    const char* output_path_arg = nullptr;
    int   req_width  = 0;   // 0 = not specified
    int   req_height = 0;
    float req_scale  = 0.f; // 0 = not specified

    // --- Argument parsing ---------------------------------------------------
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];

        if (strcmp(arg, "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if ((strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) && i + 1 < argc)
        {
            output_path_arg = argv[++i];
        }
        else if ((strcmp(arg, "-w") == 0 || strcmp(arg, "--width") == 0) && i + 1 < argc)
        {
            req_width = atoi(argv[++i]);
            if (req_width <= 0)
            {
                fprintf(stderr, "Error: --width must be a positive integer.\n");
                return 1;
            }
        }
        else if ((strcmp(arg, "-h") == 0 || strcmp(arg, "--height") == 0) && i + 1 < argc)
        {
            req_height = atoi(argv[++i]);
            if (req_height <= 0)
            {
                fprintf(stderr, "Error: --height must be a positive integer.\n");
                return 1;
            }
        }
        else if ((strcmp(arg, "-s") == 0 || strcmp(arg, "--scale") == 0) && i + 1 < argc)
        {
            req_scale = (float)atof(argv[++i]);
            if (req_scale <= 0.f)
            {
                fprintf(stderr, "Error: --scale must be a positive number.\n");
                return 1;
            }
        }
        else if (arg[0] != '-')
        {
            if (input_path != nullptr)
            {
                fprintf(stderr, "Error: multiple input files specified.\n");
                return 1;
            }
            input_path = arg;
        }
        else
        {
            fprintf(stderr, "Error: unknown option '%s'. Use --help for usage.\n", arg);
            return 1;
        }
    }

    if (input_path == nullptr)
    {
        fprintf(stderr, "Error: no input SVG file specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    // --- Load SVG -----------------------------------------------------------
    // nsvgParseFromFile expects the DPI (used to resolve units like "mm", "pt").
    // 96 DPI matches the CSS / browser standard.
    NSVGimage* image = nsvgParseFromFile(input_path, "px", 96.f);
    if (image == nullptr)
    {
        fprintf(stderr, "Error: failed to parse SVG file '%s'.\n", input_path);
        return 1;
    }

    float svg_w = image->width;
    float svg_h = image->height;

    if (svg_w <= 0.f || svg_h <= 0.f)
    {
        fprintf(stderr, "Error: SVG has zero or negative dimensions (%.1f x %.1f).\n",
                svg_w, svg_h);
        nsvgDelete(image);
        return 1;
    }

    // --- Compute output dimensions & scale ----------------------------------
    float scale_x = 1.f;
    float scale_y = 1.f;

    if (req_scale > 0.f)
    {
        // Explicit uniform scale overrides everything else.
        scale_x = scale_y = req_scale;
    }
    else if (req_width > 0 && req_height > 0)
    {
        // Both dimensions specified – may change aspect ratio intentionally.
        scale_x = (float)req_width  / svg_w;
        scale_y = (float)req_height / svg_h;
    }
    else if (req_width > 0)
    {
        scale_x = scale_y = (float)req_width / svg_w;
    }
    else if (req_height > 0)
    {
        scale_x = scale_y = (float)req_height / svg_h;
    }
    // else: default scale 1:1

    int out_w = (int)ceilf(svg_w * scale_x);
    int out_h = (int)ceilf(svg_h * scale_y);

    printf("SVG natural size : %.0f x %.0f px\n", svg_w, svg_h);
    printf("Output size      : %d x %d px  (scale %.4f x %.4f)\n",
           out_w, out_h, scale_x, scale_y);

    // --- Rasterize ----------------------------------------------------------
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (rast == nullptr)
    {
        fprintf(stderr, "Error: failed to create nanosvg rasterizer.\n");
        nsvgDelete(image);
        return 1;
    }

    // RGBA pixel buffer
    unsigned char* pixels = (unsigned char*)malloc((size_t)out_w * out_h * 4);
    if (pixels == nullptr)
    {
        fprintf(stderr, "Error: out of memory allocating %d x %d pixel buffer.\n",
                out_w, out_h);
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return 1;
    }

    // nsvgRasterizeXY allows independent x/y scales.
    nsvgRasterizeXY(rast, image,
                    0.f, 0.f,        // tx, ty (translation, in source pixels)
                    scale_x, scale_y,
                    pixels,
                    out_w, out_h,
                    out_w * 4);      // stride in bytes

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // --- Write PNG ----------------------------------------------------------
    std::string output_path = output_path_arg
                              ? std::string(output_path_arg)
                              : default_output_path(std::string(input_path));

    int ok = stbi_write_png(output_path.c_str(), out_w, out_h, 4, pixels, out_w * 4);
    free(pixels);

    if (!ok)
    {
        fprintf(stderr, "Error: failed to write PNG to '%s'.\n", output_path.c_str());
        return 1;
    }

    printf("PNG written to   : %s\n", output_path.c_str());
    return 0;
}
