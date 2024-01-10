#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"


void gaussian_blur(Bitmap *bmp) {
    int width = bmp->width;
    int height = bmp->height;

    Pixel *row0;
    if ((row0 = malloc(sizeof(Pixel) * width)) == NULL) {
        perror("malloc");
        exit(1);
    }
    Pixel *row1;
    if ((row1 = malloc(sizeof(Pixel) * width)) == NULL) {
        perror("malloc");
        exit(1);
    }
    Pixel *row2;
    if ((row2 = malloc(sizeof(Pixel) * width)) == NULL) {
        perror("malloc");
        exit(1);
    }

    if (fread(row0, sizeof(Pixel), width, stdin) != width) {
        fprintf(stderr, "End of file reached.\n");
        exit(1);
    }
    if (fread(row1, sizeof(Pixel), width, stdin) != width) {
        fprintf(stderr, "End of file reached.\n");
        exit(1);
    }
    if (fread(row2, sizeof(Pixel), width, stdin) != width) {
        fprintf(stderr, "End of file reached.\n");
        exit(1);
    }

    // writing the first row
    Pixel p = apply_edge_detection_kernel(row0, row1, row2);
    if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
        perror("fwrite");
        exit(1);
    }
    for (int j = 0; j < width - 2; j++) {
        p = apply_edge_detection_kernel(row0 + j, row1 + j, row2 + j);
        if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
            perror("fwrite");
            exit(1);
        }
    }
    p = apply_edge_detection_kernel(row0 + width - 3, row1 + width - 3, row2 + width - 3);
    if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
        perror("fwrite");
        exit(1);
    }

    // writing the middle rows
    for (int i = 1; i < height - 1; i++) {
        p = apply_edge_detection_kernel(row0, row1, row2);
        if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
            perror("fwrite");
            exit(1);
        }
        for (int j = 0; j < width - 2; j++) {
            p = apply_edge_detection_kernel(row0 + j, row1 + j, row2 + j);
            if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
                perror("fwrite");
                exit(1);
            }
        }
        p = apply_edge_detection_kernel(row0 + width - 3, row1 + width - 3, row2 + width - 3); 
        if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
            perror("fwrite");
            exit(1);
        }

        // update the rows
        if (i != height - 2) {
            Pixel *row_new;
            if ((row_new = malloc(sizeof(Pixel) * width)) == NULL) {
                perror("malloc");
                exit(1);
            }
            free(row0);
            row0 = row1;
            row1 = row2;
            row2 = row_new;
            if (fread(row2, sizeof(Pixel), width, stdin) != width) {
                fprintf(stderr, "End of file reached.\n");
                exit(1);
            } 
        }
    }

    // writing the last row
    p = apply_edge_detection_kernel(row0, row1, row2);
    if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
        perror("fwrite");
        exit(1);
    }
    for (int j = 0; j < width - 2; j++) {
        p = apply_edge_detection_kernel(row0 + j, row1 + j, row2 + j);
        if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
            perror("fwrite");
            exit(1);
        }
    }
    p = apply_edge_detection_kernel(row0 + width - 3, row1 + width - 3, row2 + width - 3);
    if (fwrite(&p, sizeof(Pixel), 1, stdout) != 1) {
        perror("fwrite");
        exit(1);
    }

    // free the pixel pointers
    free(row0);
    free(row1);
    free(row2);
}

int main() {
    run_filter(gaussian_blur, 1);
    return 0;
}