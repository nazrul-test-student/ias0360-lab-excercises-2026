#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"

static inline char to_ascii(uint8_t v) {
    // Simple intensity-to-ASCII mapping
    if (v > 200) return '#';
    if (v >  80) return '+';
    return '.';
}

// out[y*w + x] = clamp( sqrt(gx^2 + gy^2), 0..255 )
static void sobel3x3_u8(const uint8_t* img, int w, int h, uint8_t* out) {
    // zero borders
    for (int x = 0; x < w; x++) { out[x] = 0; out[(h-1)*w + x] = 0; }
    for (int y = 0; y < h; y++) { out[y*w] = 0; out[y*w + (w-1)] = 0; }

    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {
            int i = y*w + x;
            int gx =
                - img[i - w - 1] - 2*img[i - 1] - img[i + w - 1]
                + img[i - w + 1] + 2*img[i + 1] + img[i + w + 1];
            int gy =
                - img[i - w - 1] - 2*img[i - w] - img[i - w + 1]
                + img[i + w - 1] + 2*img[i + w] + img[i + w + 1];

            int mag = (int)(sqrtf((float)(gx*gx + gy*gy)));
            if (mag < 0)   mag = 0;
            if (mag > 255) mag = 255;
            out[i] = (uint8_t)mag;
        }
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1200);

    // --- Bigger 16x16 test: white square in the center ---
    enum { W = 16, H = 16 };
    uint8_t img[W*H] = {0};

    // Centered 8x8 white square (rows 4..11, cols 4..11)
    for (int y = 4; y <= 11; y++) {
        for (int x = 4; x <= 11; x++) {
            img[y*W + x] = 255;
        }
    }

    uint8_t edges[W*H] = {0};
    sobel3x3_u8(img, W, H, edges);

    // Print ORIGINAL and SOBEL side-by-side
    printf("Original (left) vs Sobel edges (right)\n");
    printf("(.: low, +: mid, #: high)\n\n");

    for (int y = 0; y < H; y++) {
        // original
        for (int x = 0; x < W; x++) {
            putchar(to_ascii(img[y*W + x]));
        }
        printf("   |   ");
        // sobel
        for (int x = 0; x < W; x++) {
            putchar(to_ascii(edges[y*W + x]));
        }
        putchar('\n');
    }

    // Keep console alive
    while (1) tight_loop_contents();
    return 0;
}
