#include "ppm.hpp"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

using namespace std;

void free_image(Image* img)
{
    if (!img)
    {
        return;
    }

    free(img->pixels);
    
    img->pixels = nullptr;
    img->width = 0;
    img->height = 0;
    img->maxVal = 0;
}

int next_token(FILE* fp, char* buf, int buflen)
{
    int ch;

    if (!fp || !buf || buflen < 2)
    {
        return 1;
    }

    while (true)
    {
        
        if (fscanf(fp, " %255s", buf) != 1)
        {
            return 2;
        }

        if (buf[0] == '#')
        {
            while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}

            continue;
        }

        return 0;
    }
}

int parse_int(const char* s, int* outVal)
{
    errno = 0;
    char* end = nullptr;
    long v = strtol(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0')
    {
        return 1;
    }

    *outVal = (int)v;

    return 0;
}

int read_ppm(const char *path, Image *out)
{
    char token[256];
    int w = 0;
    int h = 0;
    int maxV = 0;
    int r = 0;
    int g = 0;
    int b = 0;

    if (!out)
    {
        fprintf(stderr, "Error: Invalid output image pointer\n");

        return 1;
    }

    out->width = 0;
    out->height = 0;
    out->maxVal = 0;
    out->pixels = nullptr;

    FILE* fp = fopen(path, "r");

    if (!fp)
    {
        fprintf(stderr, "Error: Could not open input file\n");

        return 2;
    }

    
    if (next_token(fp, token, 256) != 0 || strcmp(token, "P3") != 0)
    {
        fprintf(stderr, "Error: Not a P3 PPM file\n");

        fclose(fp);

        return 3;
    }

    
    if (next_token(fp, token, 256) != 0 || parse_int(token, &w) != 0 ||
        next_token(fp, token, 256) != 0 || parse_int(token, &h) != 0 ||
        next_token(fp, token, 256) != 0 || parse_int(token, &maxV) != 0)
    {
        fprintf(stderr, "Error: Invalid header\n");

        fclose(fp);

        return 4;
    }
    
    if (w <= 0 || h <= 0)
    {
        fprintf(stderr, "Error: Invalid dimensions\n");

        fclose(fp);

        return 5;
    }

    if (maxV <= 0 || maxV > 255)
    {
        fprintf(stderr, "Error: Invalid maxVal\n");

        fclose(fp);

        return 6;
    }

    Color* pixels = (Color*)malloc(sizeof(Color) * (size_t)(w * h));

    if (!pixels)
    {
        fprintf(stderr, "Error: Out of memory\n");

        fclose(fp);

        return 7;
    }

    
    for (int i = 0; i < w * h; i++)
    {
        if (next_token(fp, token, 256) != 0 || parse_int(token, &r) != 0 ||
            next_token(fp, token, 256) != 0 || parse_int(token, &g) != 0 ||
            next_token(fp, token, 256) != 0 || parse_int(token, &b) != 0)
        {
            fprintf(stderr, "Error: Unexpected EOF\n");

            free(pixels);

            fclose(fp);

            return 8;
        }

        
        if (r < 0 || r > maxV || g < 0 || g > maxV || b < 0 || b > maxV)
        {
            fprintf(stderr, "Error: Pixel value out of range\n");

            free(pixels);

            fclose(fp);

            return 9;
        }

        pixels[i].red = (uint8_t)r;
        pixels[i].green = (uint8_t)g;
        pixels[i].blue = (uint8_t)b;
    }

    fclose(fp);

    out->width = w;
    out->height = h;
    out->maxVal = maxV;
    out->pixels = pixels;

    return 0;
}

int write_ppm(const char* path, const Image* img)
{
    if (!img || !img->pixels)
    {
        fprintf(stderr, "Error: Invalid image data\n");
        
        return 1;
    }

    FILE *fp = fopen(path, "w");

    if (!fp)
    {
        fprintf(stderr, "Error: Could not open output file\n");

        return 2;
    }

    fprintf(fp, "P3\n%d %d\n%d\n", img->width, img->height, img->maxVal);

    for (int i = 0; i < img->width * img->height; i++)
    {
        fprintf(fp, "%d %d %d ",
            img->pixels[i].red,
            img->pixels[i].green,
            img->pixels[i].blue);

        if ((i + 1) % 4 == 0)
        {
            fprintf(fp, "\n");
        }
    }

    fclose(fp);

    return 0;
}

int mirror_index(int i, int max)
{
    if (i < 0)
    {
        return (-i - 1);
    }

    if (i >= max)
    {
        return (2 * max - i - 1);
    }

    return i;
}

uint8_t clamp_u8(int v, int maxVal)
{
    if (v < 0)
    {
        return 0;
    }

    if (v > maxVal)
    {
        return (uint8_t)maxVal;
    }

    return (uint8_t)v;
}

int gaussian_blur(const Image* src, Image* dst)
{
    if (!src || !dst || !src->pixels)
    {
        fprintf(stderr, "Error: Invalid image pointers\n");

        return 1;
    }

    dst->width = src->width;
    dst->height = src->height;
    dst->maxVal = src->maxVal;
    dst->pixels = nullptr;

    dst->pixels = (Color*)malloc(sizeof(Color) * (size_t)(src->width * src->height));

    if (!dst->pixels)
    {
        fprintf(stderr, "Error: Out of memory\n");

        return 2;
    }

    int kernel[5][5] =
    {
        {1, 2, 3, 2, 1},
        {2, 4, 6, 4, 2},
        {3, 6, 9, 6, 3},
        {2, 4, 6, 4, 2},
        {1, 2, 3, 2, 1}
    };

    
    for (int y = 0; y < src->height; y++)
    {
        for (int x = 0; x < src->width; x++)
        {
            int sr = 0;
            int sg = 0;
            int sb = 0;

            for (int ky = -2; ky <= 2; ky++)
            {
                for (int kx = -2; kx <= 2; kx++)
                {
                    int sx = mirror_index(x + kx, src->width);
                    int sy = mirror_index(y + ky, src->height);
                    int w = kernel[ky + 2][kx + 2];
                    Color p = src->pixels[sy * src->width + sx];

                    sr += w * p.red;
                    sg += w * p.green;
                    sb += w * p.blue;
                }
            }

            int idx = y * dst->width + x;

            dst->pixels[idx].red   = clamp_u8(sr / 81, src->maxVal);
            dst->pixels[idx].green = clamp_u8(sg / 81, src->maxVal);
            dst->pixels[idx].blue  = clamp_u8(sb / 81, src->maxVal);
        }
    }

    return 0;
}