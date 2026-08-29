#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "pico/stdlib.h"


static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Quantize float in ~[-1,1) to signed Q15
static void quantize_q15(const float* x, int n, int16_t* y, int* clip_count) {
    int clips = 0;
    for (int i = 0; i < n; i++) {
        float s = clampf(x[i], -0.999969f, 0.999969f); // avoid +1.0 overflow
        if (s != x[i]) clips++;
        int32_t q = (int32_t)lrintf(s * 32768.0f);     // round to nearest
        if (q >  32767) q =  32767;
        if (q < -32768) q = -32768;
        y[i] = (int16_t)q;
    }
    if (clip_count) *clip_count = clips;
}

static void dequantize_q15(const int16_t* x, int n, float* y) {
    for (int i = 0; i < n; i++) y[i] = (float)x[i] / 32768.0f;
}

static void snr_and_error(const float* ref, const float* test, int n,
                          float* snr_db, float* max_abs_err, float* rms_err) {
    double sig = 0.0, err = 0.0;
    float maxe = 0.0f;
    for (int i = 0; i < n; i++) {
        float e = ref[i] - test[i];
        sig += (double)ref[i] * (double)ref[i];
        err += (double)e * (double)e;
        if (fabsf(e) > maxe) maxe = fabsf(e);
    }
    float rms = (float)sqrt(err / (double)n);
    *rms_err = rms;
    *max_abs_err = maxe;
    if (err == 0.0) {
        *snr_db = INFINITY;
    } else {
        *snr_db = 10.0f * log10f((float)(sig / err));
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500); 

    // ---- Settings ----
    const float fs_hz = 2200.0f;   // your IMU sample rate
    const int axes = 3;            // accel/gyro example
    enum { N = 512 };              // sample window for analysis

    // ---- Create a test signal in [-1,1): 0.6*sin + 0.3*sin ----
    // In practice, scale real IMU units (e.g., g or DPS) to [-1,1) before quantization.
    float x[N];
    for (int i = 0; i < N; i++) {
        float t = (float)i / fs_hz;
        x[i] = 0.6f * sinf(2.0f * (float)M_PI * 7.0f * t)
             + 0.3f * sinf(2.0f * (float)M_PI * 23.0f * t);
    }

    // ---- Quantize & dequantize ----
    int16_t q15[N];
    int clips = 0;
    quantize_q15(x, N, q15, &clips);

    float xq[N];
    dequantize_q15(q15, N, xq);

    // ---- Size & throughput math ----
    const int bytes_f32 = sizeof(float) * N;     // 4 bytes/sample
    const int bytes_q15 = sizeof(int16_t) * N;   // 2 bytes/sample
    const float bps_f32 = (float)(axes * 4) * fs_hz; // bytes/s for 3 axes
    const float bps_q15 = (float)(axes * 2) * fs_hz;

    // ---- Fidelity (SNR, errors) ----
    float snr_db, max_abs_err, rms_err;
    snr_and_error(x, xq, N, &snr_db, &max_abs_err, &rms_err);

    printf("\n=== Quantization Validation ===\n");
    printf("Array length: %d samples\n", N);
    printf("Data type sizes: float32=%d bytes, q15=%d bytes\n", (int)sizeof(float), (int)sizeof(int16_t));
    printf("Bytes for this block: float32=%d, q15=%d  (reduction: %.1fx)\n",
           bytes_f32, bytes_q15, (double)bytes_f32/(double)bytes_q15);

    printf("\nAssuming %d axes @ %.1f Hz:\n", axes, fs_hz);
    printf("  Float32 stream: %.1f kB/s\n", bps_f32 / 1000.0f);
    printf("  Q15    stream: %.1f kB/s\n", bps_q15 / 1000.0f);

    printf("\nFidelity vs float reference:\n");
    printf("  SNR: %.2f dB\n", snr_db);
    printf("  RMS error: %.7f\n", rms_err);
    printf("  Max |error|: %.7f\n", max_abs_err);
    printf("  Clip count: %d (of %d)\n", clips, N);

    while (true) tight_loop_contents();
    return 0;
}


