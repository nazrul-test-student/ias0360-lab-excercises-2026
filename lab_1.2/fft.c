#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// ---------- Simple complex type ----------
typedef struct { float re, im; } c32;

// ---------- Small utilities ----------
static inline float fsqrtf(float x){ return sqrtf(x); }
static inline float fclampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }

// ---------- Hamming window (in-place) ----------
static void hamming_window(float* x, int n){
    for(int i=0;i<n;i++){
        x[i] *= 0.54f - 0.46f * cosf(2.0f*(float)M_PI*(float)i/(float)(n-1));
    }
}

// ---------- Bit helpers ----------
static int is_power_of_two(int n){ return (n>0) && ((n & (n-1))==0); }
static unsigned reverse_bits(unsigned v, int nbits){
    unsigned r = 0u;
    for(int i=0;i<nbits;i++){ r = (r<<1) | (v & 1u); v >>= 1u; }
    return r;
}

// ---------- In-place radix-2 Cooley–Tukey FFT ----------
// dir = +1 for FFT, -1 for IFFT
static int fft_radix2(c32* x, int n, int dir){
    if(!is_power_of_two(n)) return -1;
    int logn = 0; while((1<<logn) < n) logn++;

    // Bit-reversal permutation
    for(unsigned i=0;i<(unsigned)n;i++){
        unsigned j = reverse_bits(i, logn);
        if(j>i){ c32 t = x[i]; x[i]=x[j]; x[j]=t; }
    }

    const float sgn = (dir >= 0) ? -1.0f : 1.0f;
    for(int s=1; s<=logn; s++){
        int m = 1<<s;
        int m2 = m>>1;
        float theta = sgn * (float)M_PI / (float)m2;
        float wpr = -2.0f * sinf(0.5f*theta) * sinf(0.5f*theta);
        float wpi = sinf(theta);
        for(int k=0; k<n; k+=m){
            float wr = 1.0f, wi = 0.0f;
            for(int j=0; j<m2; j++){
                int t = k + j + m2;
                int u = k + j;
                float tr = wr*x[t].re - wi*x[t].im;
                float ti = wr*x[t].im + wi*x[t].re;
                float ur = x[u].re, ui = x[u].im;
                x[t].re = ur - tr; x[t].im = ui - ti;
                x[u].re = ur + tr; x[u].im = ui + ti;
                // twiddle update (CORDIC-free recurrence)
                float tmp = wr;
                wr = wr + (wr*wpr - wi*wpi);
                wi = wi + (wi*wpr + tmp*wpi);
            }
        }
    }

    if(dir < 0){
        float inv = 1.0f/(float)n;
        for(int i=0;i<n;i++){ x[i].re *= inv; x[i].im *= inv; }
    }
    return 0;
}

// ---------- Magnitude spectrum ----------
static void fft_mag(const c32* X, int n, float* mag){
    for(int i=0;i<n;i++){
        mag[i] = fsqrtf(X[i].re*X[i].re + X[i].im*X[i].im);
    }
}

// ---------- Peak picking (top-K, single-sided) ----------
static void top_k_peaks(const float* mag, int n_half, int k_exclude_dc, int K,
                        int* out_idx, float* out_val, int* out_count){
    // simple selection without sorting the full array
    int count = 0;
    for(int k=0; k<K; k++){
        int best_i = -1; float best_v = -1.0f;
        for(int i=k_exclude_dc; i<n_half; i++){
            // skip already taken
            bool taken = false;
            for(int j=0;j<count;j++) if(out_idx[j]==i){ taken = true; break; }
            if(taken) continue;
            if(mag[i] > best_v){ best_v = mag[i]; best_i = i; }
        }
        if(best_i >= 0){
            out_idx[count] = best_i;
            out_val[count] = best_v;
            count++;
        }
    }
    *out_count = count;
}

int main(void){

    stdio_init_all();
    sleep_ms(1500); 

    // ---- Settings ----
    const float fs = 2200.0f;   // IMU sample rate (Hz)
    enum { N = 256 };           // power-of-two FFT size
    const float df = fs / (float)N;

    if(!is_power_of_two(N)){
        printf("N must be power of two\n");
        while(true) tight_loop_contents();
    }

    // ---- Build a synthetic signal: 120 Hz and 440 Hz
    float x[N];
    for(int n=0; n<N; n++){
        float t = (float)n / fs;
        x[n] = 0.7f*sinf(2.0f*(float)M_PI*120.0f*t)
             + 0.3f*sinf(2.0f*(float)M_PI*440.0f*t);
    }

    // ---- Window and pack into complex buffer
    hamming_window(x, N);
    c32 X[N];
    for(int i=0;i<N;i++){ X[i].re = x[i]; X[i].im = 0.0f; }

    // ---- FFT
    if(fft_radix2(X, N, +1) != 0){
        printf("FFT error\n");
        while(true) tight_loop_contents();
    }

    // ---- Magnitude spectrum (single-sided bins 0..N/2)
    float mag[N];
    fft_mag(X, N, mag);

    const float scale = (2.0f / (float)N) / 0.54f;
    for(int k=0;k<=N/2;k++) mag[k] *= scale;

    // ---- Report
    printf("\n=== IMU FFT Demo ===\n");
    printf("fs=%.1f Hz, N=%d, resolution df=%.3f Hz\n", fs, N, df);
    printf("Looking for top 5 peaks (excluding DC):\n");

    int idx[5]; float val[5]; int found=0;
    top_k_peaks(mag, N/2+1, /*exclude up to index*/1, /*K*/5, idx, val, &found);

    for(int i=0;i<found;i++){
        float fk = df * (float)idx[i];
        printf("  Peak %d: bin=%d  freq=%.2f Hz  amplitude≈%.4f\n",
               i+1, idx[i], fk, val[i]);
    }

    printf("\n");
    printf("TO COPY ONTO A TEXT FILE FOR COMPARSION WITH PC RUN FFT\n");

    // Print the raw values onto the serial in one line
    for (int n = 0; n < N; n++) { if (n) printf(","); printf("%.6f", (double)x[n]); } printf("\n");
    printf("\n");
    printf("TO COPY ONTO A TEXT FILE FOR COMPARSION WITH PC RUN FFT\n");
    
    // Print to the serial the output of the fft to compare with the host run python implmentation of the fft filter
    printf("bin,freq,amp\n");
    for(int i=0;i<found;i++)
    {
        float fk = df * (float)idx[i];
        printf("%d,%.6f,%.6f\n", idx[i], fk, val[i]);
    }

    while(true) tight_loop_contents();
    return 0;
}
