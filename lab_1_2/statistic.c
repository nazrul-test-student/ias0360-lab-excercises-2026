#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define N   64       // number of samples
#define FS  2200.0f  // sample rate 

// ---------- small helpers (statistics) ----------

// arithmetic mean
static float mean_f32(const float* x, int n) {
    double acc = 0.0;
    for (int i = 0; i < n; i++) acc += x[i];
    return (float)(acc / (double)n);
}

// sample variance (denominator n-1)
static float variance_f32(const float* x, int n, float mean) {
    if (n <= 1) return 0.0f;
    double acc = 0.0;
    for (int i = 0; i < n; i++) {
        double d = (double)x[i] - (double)mean;
        acc += d * d;
    }
    return (float)(acc / (double)(n - 1));
}

// standard deviation from variance
static float stddev_f32(float variance) {
    return sqrtf(variance);
}

// min & max
static void min_max_f32(const float* x, int n, float* mn, float* mx) {
    float a = x[0], b = x[0];
    for (int i = 1; i < n; i++) {
        if (x[i] < a) a = x[i];
        if (x[i] > b) b = x[i];
    }
    *mn = a; *mx = b;
}

// median (sorts a local copy; insertion sort — fine for small N)
static float median_f32(const float* x, int n) {
    float tmp[n];
    for (int i = 0; i < n; i++) tmp[i] = x[i];

    // insertion sort (ascending)
    for (int i = 1; i < n; i++) {
        float key = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > key) {
            tmp[j + 1] = tmp[j];
            j--;
        }
        tmp[j + 1] = key;
    }

    if (n & 1) return tmp[n / 2];
    return 0.5f * (tmp[n / 2 - 1] + tmp[n / 2]);
}

// mode (for discrete/repeated values). If all counts are 1, no mode.
// eps lets you treat near-equal floats as equal (use 0 for exact).
static float mode_f32(const float* x, int n, int* count_out, float eps) {
    int best_count = 0;
    float best_val = NAN;

    for (int i = 0; i < n; i++) {
        int cnt = 1;
        for (int j = i + 1; j < n; j++) {
            if (fabsf(x[j] - x[i]) <= eps) cnt++;
        }
        if (cnt > best_count) {
            best_count = cnt;
            best_val = x[i];
        }
    }

    if (count_out) *count_out = best_count;
    return best_val; // if best_count==1, there is no mode
}

int main(void) {
    stdio_init_all();
    sleep_ms(1200);

    // ---- synthetic IMU-like data (already scaled to about [-1,1)) ----
    float ax[N], ay[N], az[N];
    for (int n = 0; n < N; n++) {
        float t = (float)n / FS;
        ax[n] = 0.20f + 0.80f * sinf(2.0f * (float)M_PI * 3.0f   * t);                 // bias + low-freq swing
        ay[n] = -0.10f + 0.50f * cosf(2.0f * (float)M_PI * 1.7f * t);                  // bias + different freq
        az[n] = 0.05f + 0.30f * sinf(2.0f * (float)M_PI * 0.5f * t)
                      + 0.10f * sinf(2.0f * (float)M_PI * 7.0f * t);                   // mix
    }

    // ---- statistics per axis ----
    float meanx = mean_f32(ax, N);
    float meany = mean_f32(ay, N);
    float meanz = mean_f32(az, N);

    float varx  = variance_f32(ax, N, meanx);
    float vary  = variance_f32(ay, N, meany);
    float varz  = variance_f32(az, N, meanz);

    float stdx = stddev_f32(varx);
    float stdy = stddev_f32(vary);
    float stdz = stddev_f32(varz);

    float medx = median_f32(ax, N);
    float medy = median_f32(ay, N);
    float medz = median_f32(az, N);

    int mcount_x, mcount_y, mcount_z;
    // eps=0.0f -> exact repeats only. Use e.g. 1e-4f if you want near-equality.
    float modex = mode_f32(ax, N, &mcount_x, 0.0f);
    float modey = mode_f32(ay, N, &mcount_y, 0.0f);
    float modez = mode_f32(az, N, &mcount_z, 0.0f);

    float mnx, Mx, mny, My, mnz, Mz;
    min_max_f32(ax, N, &mnx, &Mx);
    min_max_f32(ay, N, &mny, &My);
    min_max_f32(az, N, &mnz, &Mz);

    printf("=== IMU Statistics Lab ===\n");
    printf("N=%d, fs=%.1f Hz\n\n", N, FS);

    printf("Raw stats per-axis:\n");
    printf("  ax: mean=% .5f  median=% .5f  var=% .5f  std=% .5f  min=% .5f  max=% .5f\n",
           meanx, medx, varx, stdx, mnx, Mx);
    if (mcount_x > 1) printf("      mode=% .5f (count=%d)\n", modex, mcount_x);
    else              printf("      mode: none (no repeated values)\n");

    printf("  ay: mean=% .5f  median=% .5f  var=% .5f  std=% .5f  min=% .5f  max=% .5f\n",
           meany, medy, vary, stdy, mny, My);
    if (mcount_y > 1) printf("      mode=% .5f (count=%d)\n", modey, mcount_y);
    else              printf("      mode: none (no repeated values)\n");

    printf("  az: mean=% .5f  median=% .5f  var=% .5f  std=% .5f  min=% .5f  max=% .5f\n",
           meanz, medz, varz, stdz, mnz, Mz);
    if (mcount_z > 1) printf("      mode=% .5f (count=%d)\n", modez, mcount_z);
    else              printf("      mode: none (no repeated values)\n");

    // ---- optional: peek first 10 samples ----
    printf("\nFirst 10 samples (ax, ay, az):\n");
    for (int i = 0; i < 10; i++) {
        printf("%3d | % .5f % .5f % .5f\n", i, ax[i], ay[i], az[i]);
    }

    while (true) tight_loop_contents();
    return 0;
}
