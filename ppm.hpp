#ifndef PPM_HPP
#define PPM_HPP

#include <stdio.h>
#include <stdint.h>

struct Color 
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Image 
{
    int width;
    int height;
    int maxVal;
    Color *pixels;
};

int read_ppm(const char *path, Image *out);

int write_ppm(const char *path, const Image *img);

void free_image(Image *img);

int next_token(FILE *fp, char *buf, int buflen);

int parse_int(const char *s, int *outVal);

int mirror_index(int i, int max);

uint8_t clamp_u8(int v, int maxVal);

int gaussian_blur(const Image *src, Image *dst);

#endif