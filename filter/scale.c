#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

int scale_factor;

void scale_filter(Bitmap *bmp) {
    Pixel *row;
    if ((row = malloc(sizeof(Pixel) * bmp->width)) == NULL) {
        perror("malloc");
        exit(1);
    }
    for (int i = 0; i < bmp->height; i++) {
        if (fread(row, sizeof(Pixel), bmp->width, stdin) != bmp->width) {
            fprintf(stderr, "End of file reached.\n");
            exit(1);
        }
        for (int k = 0; k < scale_factor; k++) {
            for (int j = 0; j < bmp->width; j++) {
                for (int l = 0; l < scale_factor; l++) {
                    if (fwrite(row + j, sizeof(Pixel), 1, stdout) != 1) {
                        perror("fwrite");
                        exit(1);
                    }
                }
            }
        }
    }
    free(row);
}


int main(int argc, char *argv[]) {
    scale_factor = strtol(argv[1], NULL, 10);
    run_filter(scale_filter, scale_factor);
    return 0;
}
