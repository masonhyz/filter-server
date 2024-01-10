#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"


/*
 * Main filter loop.
 * This function is responsible for doing the following:
 *   1. Read in pixels one at a time (because greyscale is a pixel-by-pixel transformation).
 *   2. Immediately write out the greyscale transformed version of each pixel.
 *
 * Note that this function should allocate space only for a single Pixel;
 * do *not* store more than one Pixel at a time, it isn't necessary here!
 */
void greyscale(Bitmap *bmp) {
    Pixel *pix = malloc(sizeof(Pixel));
    for (int i = 0; i < (bmp->width * bmp->height); i++) {
        if (fread(pix, sizeof(Pixel), 1, stdin) != 1) {
            fprintf(stderr, "End of file reached.\n");
            exit(1);
        }
        int average = (pix->blue + pix->green + pix->red) / 3;
        pix->blue = average, pix->green = average, pix->red = average;
        if (fwrite(pix, sizeof(Pixel), 1, stdout) != 1) {
            perror("fwrite");
            exit(1);
        }
    }
    free(pix);
}

int main() { 
    run_filter(greyscale, 1);
    return 0;
}