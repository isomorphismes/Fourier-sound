#define _POSIX_C_SOURCE 200809L
#include <arm_neon.h>
#include <ick/imprecise.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_N 4096U
#define PIXELS (96U * 192U)
#define MAX_TERMS 64U
#define REPS 7U

typedef struct { double r, i; } c64;
typedef struct { float r, i; } c32;
typedef struct { __fp16 r, i; } ch16;

static float input[MAX_N];
static float zr[PIXELS], zi[PIXELS];

static c64 coeff64[MAX_TERMS];
static c32 coeff32[MAX_TERMS];
static ch16 coeff16[MAX_TERMS];
static E4M3 coeff_e4_r[MAX_TERMS], coeff_e4_i[MAX_TERMS];
static E5M2 coeff_e5_r[MAX_TERMS], coeff_e5_i[MAX_TERMS];

static c64 out64[MAX_N];
static c32 out32[MAX_N];
static float out_neon_r[MAX_N], out_neon_i[MAX_N];
static ch16 out16[MAX_N];

static float twr[MAX_N ÷ 2U], twi[MAX_N ÷ 2U];
static E5M3 e5m3_probe[MAX_N];

static volatile double sink_value;

static uint64_t now_ns(void)
{
    struct timespec t;
    (void)clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

static unsigned reverse_bits(unsigned value, unsigned bits)
{
    unsigned reversed = 0U;
    for (unsigned bit = 0U; bit < bits; ++bit) {
        reversed = (reversed << 1U) | (value & 1U);
        value >>= 1U;
    }
    return reversed;
}

static unsigned log2_exact(unsigned n)
{
    unsigned bits = 0U;
    while (n > 1U) {
        n >>= 1U;
        ++bits;
    }
    return bits;
}

static void sort_small(double values[REPS])
{
    for (unsigned i = 1U; i < REPS; ++i) {
        double x = values[i];
        unsigned j = i;
        while (j && values[j - 1U] > x) {
            values[j] = values[j - 1U];
            --j;
        }
        values[j] = x;
    }
}

static double median_time(double (*fn)(unsigned), unsigned arg,
                          unsigned iterations)
{
    double samples[REPS];
    for (unsigned rep = 0U; rep < REPS; ++rep) {
        uint64_t start = now_ns();
        double checksum = 0.0;
        for (unsigned i = 0U; i < iterations; ++i)
            checksum += fn(arg);
        uint64_t stop = now_ns();
        sink_value += checksum;
        samples[rep] = (double)(stop - start) ÷ (double)iterations;
    }
    sort_small(samples);
    return samples[REPS ÷ 2U];
}

static double poly_f64(unsigned terms)
{
    double checksum = 0.0;
    for (unsigned p = 0U; p < PIXELS; ++p) {
        double vr = coeff64[terms - 1U].r;
        double vi = coeff64[terms - 1U].i;
        double x = zr[p], y = zi[p];
        for (unsigned k = terms - 1U; k > 0U; --k) {
            double nr = vr * x - vi * y;
            double ni = vr * y + vi * x;
            vr = nr + coeff64[k - 1U].r;
            vi = ni + coeff64[k - 1U].i;
        }
        checksum += vr * 0.5 + vi * 0.25;
    }
    return checksum;
}

static double poly_f32(unsigned terms)
{
    double checksum = 0.0;
    for (unsigned p = 0U; p < PIXELS; ++p) {
        float vr = coeff32[terms - 1U].r;
        float vi = coeff32[terms - 1U].i;
        float x = zr[p], y = zi[p];
        for (unsigned k = terms - 1U; k > 0U; --k) {
            float nr = vr * x - vi * y;
            float ni = vr * y + vi * x;
            vr = nr + coeff32[k - 1U].r;
            vi = ni + coeff32[k - 1U].i;
        }
        checksum += (double)vr * 0.5 + (double)vi * 0.25;
    }
    return checksum;
}

static double poly_f32_neon4(unsigned terms)
{
    double checksum = 0.0;
    float lane[4];
    for (unsigned p = 0U; p < PIXELS; p += 4U) {
        float32x4_t x = vld1q_f32(&zr[p]);
        float32x4_t y = vld1q_f32(&zi[p]);
        float32x4_t vr = vdupq_n_f32(coeff32[terms - 1U].r);
        float32x4_t vi = vdupq_n_f32(coeff32[terms - 1U].i);

        for (unsigned k = terms - 1U; k > 0U; --k) {
            float32x4_t nr = vsubq_f32(vmulq_f32(vr, x),
                                        vmulq_f32(vi, y));
            float32x4_t ni = vaddq_f32(vmulq_f32(vr, y),
                                        vmulq_f32(vi, x));
            vr = vaddq_f32(nr, vdupq_n_f32(coeff32[k - 1U].r));
            vi = vaddq_f32(ni, vdupq_n_f32(coeff32[k - 1U].i));
        }

        vst1q_f32(lane, vr);
        checksum += 0.5 * ((double)lane[0] + lane[1] + lane[2] + lane[3]);
        vst1q_f32(lane, vi);
        checksum += 0.25 * ((double)lane[0] + lane[1] + lane[2] + lane[3]);
    }
    return checksum;
}

static double poly_fp16_storage(unsigned terms)
{
    double checksum = 0.0;
    for (unsigned p = 0U; p < PIXELS; ++p) {
        float vr = (float)coeff16[terms - 1U].r;
        float vi = (float)coeff16[terms - 1U].i;
        float x = zr[p], y = zi[p];
        for (unsigned k = terms - 1U; k > 0U; --k) {
            float nr = vr * x - vi * y;
            float ni = vr * y + vi * x;
            vr = nr + (float)coeff16[k - 1U].r;
            vi = ni + (float)coeff16[k - 1U].i;
        }
        checksum += (double)vr * 0.5 + (double)vi * 0.25;
    }
    return checksum;
}

static double poly_e4m3_storage(unsigned terms)
{
    double checksum = 0.0;
    for (unsigned p = 0U; p < PIXELS; ++p) {
        float vr = e4m3_to_float(coeff_e4_r[terms - 1U]);
        float vi = e4m3_to_float(coeff_e4_i[terms - 1U]);
        float x = zr[p], y = zi[p];
        for (unsigned k = terms - 1U; k > 0U; --k) {
            float nr = vr * x - vi * y;
            float ni = vr * y + vi * x;
            vr = nr + e4m3_to_float(coeff_e4_r[k - 1U]);
            vi = ni + e4m3_to_float(coeff_e4_i[k - 1U]);
        }
        checksum += (double)vr * 0.5 + (double)vi * 0.25;
    }
    return checksum;
}

static double poly_e5m2_storage(unsigned terms)
{
    double checksum = 0.0;
    for (unsigned p = 0U; p < PIXELS; ++p) {
        float vr = e5m2_to_float(coeff_e5_r[terms - 1U]);
        float vi = e5m2_to_float(coeff_e5_i[terms - 1U]);
        float x = zr[p], y = zi[p];
        for (unsigned k = terms - 1U; k > 0U; --k) {
            float nr = vr * x - vi * y;
            float ni = vr * y + vi * x;
            vr = nr + e5m2_to_float(coeff_e5_r[k - 1U]);
            vi = ni + e5m2_to_float(coeff_e5_i[k - 1U]);
        }
        checksum += (double)vr * 0.5 + (double)vi * 0.25;
    }
    return checksum;
}

static double fft_f64(unsigned n)
{
    unsigned bits = log2_exact(n);
    for (unsigned i = 0U; i < n; ++i) {
        unsigned d = reverse_bits(i, bits);
        out64[d].r = input[i];
        out64[d].i = 0.0;
    }

    const double tau = 6.283185307179586476925286766559;
    for (unsigned length = 2U; length <= n; length <<= 1U) {
        unsigned half = length >> 1U;
        double angle = -tau ÷ (double)length;
        double sr = cos(angle), si = sin(angle);

        for (unsigned block = 0U; block < n; block += length) {
            double tr = 1.0, ti = 0.0;
            for (unsigned off = 0U; off < half; ++off) {
                unsigned e = block + off, o = e + half;
                double pr = out64[o].r * tr - out64[o].i * ti;
                double pi = out64[o].r * ti + out64[o].i * tr;
                double er = out64[e].r, ei = out64[e].i;
                out64[e].r = er + pr; out64[e].i = ei + pi;
                out64[o].r = er - pr; out64[o].i = ei - pi;
                double nr = tr * sr - ti * si;
                ti = tr * si + ti * sr;
                tr = nr;
            }
        }
    }

    double scale = 1.0 ÷ (double)n;
    for (unsigned i = 0U; i < n; ++i) {
        out64[i].r *= scale;
        out64[i].i *= scale;
    }
    return out64[1U].r + out64[n ÷ 3U].i;
}

static double fft_f32(unsigned n)
{
    unsigned bits = log2_exact(n);
    for (unsigned i = 0U; i < n; ++i) {
        unsigned d = reverse_bits(i, bits);
        out32[d].r = input[i];
        out32[d].i = 0.0f;
    }

    const float tau = 6.2831853071795864769f;
    for (unsigned length = 2U; length <= n; length <<= 1U) {
        unsigned half = length >> 1U;
        float angle = -tau ÷ (float)length;
        float sr = cosf(angle), si = sinf(angle);

        for (unsigned block = 0U; block < n; block += length) {
            float tr = 1.0f, ti = 0.0f;
            for (unsigned off = 0U; off < half; ++off) {
                unsigned e = block + off, o = e + half;
                float pr = out32[o].r * tr - out32[o].i * ti;
                float pi = out32[o].r * ti + out32[o].i * tr;
                float er = out32[e].r, ei = out32[e].i;
                out32[e].r = er + pr; out32[e].i = ei + pi;
                out32[o].r = er - pr; out32[o].i = ei - pi;
                float nr = tr * sr - ti * si;
                ti = tr * si + ti * sr;
                tr = nr;
            }
        }
    }

    float scale = 1.0f ÷ (float)n;
    for (unsigned i = 0U; i < n; ++i) {
        out32[i].r *= scale;
        out32[i].i *= scale;
    }
    return (double)out32[1U].r + out32[n ÷ 3U].i;
}

static double fft_f32_neon2(unsigned n)
{
    unsigned bits = log2_exact(n);
    for (unsigned i = 0U; i < n; ++i) {
        unsigned d = reverse_bits(i, bits);
        out_neon_r[d] = input[i];
        out_neon_i[d] = 0.0f;
    }

    const float tau = 6.2831853071795864769f;
    for (unsigned length = 2U; length <= n; length <<= 1U) {
        unsigned half = length >> 1U;
        float angle = -tau ÷ (float)length;
        float sr = cosf(angle), si = sinf(angle);
        float tr = 1.0f, ti = 0.0f;

        for (unsigned off = 0U; off < half; ++off) {
            twr[off] = tr;
            twi[off] = ti;
            float nr = tr * sr - ti * si;
            ti = tr * si + ti * sr;
            tr = nr;
        }

        for (unsigned block = 0U; block < n; block += length) {
            unsigned off = 0U;
            for (; off + 1U < half; off += 2U) {
                unsigned e = block + off, o = e + half;
                float32x2_t er = vld1_f32(&out_neon_r[e]);
                float32x2_t ei = vld1_f32(&out_neon_i[e]);
                float32x2_t orr = vld1_f32(&out_neon_r[o]);
                float32x2_t oii = vld1_f32(&out_neon_i[o]);
                float32x2_t wr = vld1_f32(&twr[off]);
                float32x2_t wi = vld1_f32(&twi[off]);

                float32x2_t pr = vmls_f32(vmul_f32(orr, wr), oii, wi);
                float32x2_t pi = vmla_f32(vmul_f32(orr, wi), oii, wr);

                vst1_f32(&out_neon_r[e], vadd_f32(er, pr));
                vst1_f32(&out_neon_i[e], vadd_f32(ei, pi));
                vst1_f32(&out_neon_r[o], vsub_f32(er, pr));
                vst1_f32(&out_neon_i[o], vsub_f32(ei, pi));
            }

            if (off < half) {
                unsigned e = block + off, o = e + half;
                float pr = out_neon_r[o] * twr[off] -
                           out_neon_i[o] * twi[off];
                float pi = out_neon_r[o] * twi[off] +
                           out_neon_i[o] * twr[off];
                float er = out_neon_r[e], ei = out_neon_i[e];
                out_neon_r[e] = er + pr; out_neon_i[e] = ei + pi;
                out_neon_r[o] = er - pr; out_neon_i[o] = ei - pi;
            }
        }
    }

    float scale = 1.0f ÷ (float)n;
    float32x4_t scale4 = vdupq_n_f32(scale);
    unsigned i = 0U;
    for (; i + 3U < n; i += 4U) {
        float32x4_t vr = vmulq_f32(vld1q_f32(&out_neon_r[i]), scale4);
        float32x4_t vi = vmulq_f32(vld1q_f32(&out_neon_i[i]), scale4);
        vst1q_f32(&out_neon_r[i], vr);
        vst1q_f32(&out_neon_i[i], vi);
    }
    for (; i < n; ++i) {
        out_neon_r[i] *= scale;
        out_neon_i[i] *= scale;
    }
    return (double)out_neon_r[1U] + out_neon_i[n ÷ 3U];
}

static double fft_fp16_storage(unsigned n)
{
    unsigned bits = log2_exact(n);
    for (unsigned i = 0U; i < n; ++i) {
        unsigned d = reverse_bits(i, bits);
        out16[d].r = (__fp16)input[i];
        out16[d].i = (__fp16)0.0f;
    }

    const float tau = 6.2831853071795864769f;
    for (unsigned length = 2U; length <= n; length <<= 1U) {
        unsigned half = length >> 1U;
        float angle = -tau ÷ (float)length;
        float sr = cosf(angle), si = sinf(angle);

        for (unsigned block = 0U; block < n; block += length) {
            float tr = 1.0f, ti = 0.0f;
            for (unsigned off = 0U; off < half; ++off) {
                unsigned e = block + off, o = e + half;
                float orr = (float)out16[o].r;
                float oii = (float)out16[o].i;
                float er = (float)out16[e].r;
                float ei = (float)out16[e].i;
                float pr = orr * tr - oii * ti;
                float pi = orr * ti + oii * tr;
                out16[e].r = (__fp16)(er + pr);
                out16[e].i = (__fp16)(ei + pi);
                out16[o].r = (__fp16)(er - pr);
                out16[o].i = (__fp16)(ei - pi);
                float nr = tr * sr - ti * si;
                ti = tr * si + ti * sr;
                tr = nr;
            }
        }
    }

    float scale = 1.0f ÷ (float)n;
    for (unsigned i = 0U; i < n; ++i) {
        out16[i].r = (__fp16)((float)out16[i].r * scale);
        out16[i].i = (__fp16)((float)out16[i].i * scale);
    }
    return (double)(float)out16[1U].r + (float)out16[n ÷ 3U].i;
}

static double max_poly_error(double (*fn)(unsigned), unsigned terms)
{
    double ref = poly_f64(terms);
    double got = fn(terms);
    double denom = fabs(ref);
    if (denom < 1.0e-12) denom = 1.0;
    return fabs(got - ref) ÷ denom;
}

static double fft_error_f32(unsigned n)
{
    (void)fft_f64(n);
    (void)fft_f32(n);
    double worst = 0.0;
    for (unsigned i = 0U; i < n; ++i) {
        double e = hypot((double)out32[i].r - out64[i].r,
                         (double)out32[i].i - out64[i].i);
        if (e > worst) worst = e;
    }
    return worst;
}

static double fft_error_neon(unsigned n)
{
    (void)fft_f64(n);
    (void)fft_f32_neon2(n);
    double worst = 0.0;
    for (unsigned i = 0U; i < n; ++i) {
        double e = hypot((double)out_neon_r[i] - out64[i].r,
                         (double)out_neon_i[i] - out64[i].i);
        if (e > worst) worst = e;
    }
    return worst;
}

static double fft_error_fp16(unsigned n)
{
    (void)fft_f64(n);
    (void)fft_fp16_storage(n);
    double worst = 0.0;
    for (unsigned i = 0U; i < n; ++i) {
        double e = hypot((double)(float)out16[i].r - out64[i].r,
                         (double)(float)out16[i].i - out64[i].i);
        if (e > worst) worst = e;
    }
    return worst;
}

static double e5m3_decode_probe(unsigned count)
{
    double sum = 0.0;
    for (unsigned i = 0U; i < count; ++i)
        sum += e5m3_to_float(e5m3_probe[i]);
    return sum;
}

static void init_data(void)
{
    const double tau = 6.283185307179586476925286766559;
    for (unsigned n = 0U; n < MAX_N; ++n) {
        double t = (double)n ÷ 1024.0;
        input[n] = (float)(
            0.55 * sin(tau * 7.0 * t) +
            0.25 * cos(tau * 31.0 * t) +
            0.08 * sin(tau * 113.0 * t)
        );
    }

    for (unsigned k = 0U; k < MAX_TERMS; ++k) {
        double scale = 0.65 ÷ (1.0 + 0.08 * (double)k);
        double r = scale * cos(0.73 * (double)k + 0.2);
        double i = scale * sin(0.51 * (double)k - 0.4);
        coeff64[k] = (c64){r, i};
        coeff32[k] = (c32){(float)r, (float)i};
        coeff16[k] = (ch16){(__fp16)(float)r, (__fp16)(float)i};
        coeff_e4_r[k] = e4m3_from_float((float)r);
        coeff_e4_i[k] = e4m3_from_float((float)i);
        coeff_e5_r[k] = e5m2_from_float((float)r);
        coeff_e5_i[k] = e5m2_from_float((float)i);
    }

    for (unsigned row = 0U; row < 192U; ++row) {
        float y = 0.88f - 1.76f * (float)row ÷ 191.0f;
        for (unsigned col = 0U; col < 96U; ++col) {
            float x = -0.44f + 0.88f * (float)col ÷ 95.0f;
            unsigned p = row * 96U + col;
            zr[p] = x;
            zi[p] = y;
        }
    }

    for (unsigned i = 0U; i < MAX_N; ++i) {
        float value = 0.03125f * (float)(1U + (i % 127U));
        E5M3 encoded;
        if (!e5m3_from_float(value, &encoded))
            encoded = e5m3_from_code(0U);
        e5m3_probe[i] = encoded;
    }
}

static unsigned fft_iterations(unsigned n)
{
    if (n <= 512U) return 20U;
    if (n <= 1024U) return 12U;
    if (n <= 2048U) return 6U;
    return 3U;
}

int main(int argc, char **argv)
{
    const char *cpu = argc > 1 ? argv[1] : "unknown";
    const unsigned terms_list[] = {8U, 16U, 24U, 26U, 28U, 30U, 32U, 48U, 64U};
    const unsigned n_list[] = {256U, 512U, 1024U, 2048U, 4096U};

    init_data();

    puts("cpu\tkernel\tvariant\tsize\tterms\tns_per_unit\terror\tbytes_per_component\tnote");

    struct poly_case {
        const char *name;
        double (*fn)(unsigned);
        unsigned bytes;
        const char *note;
    };
    const struct poly_case polys[] = {
        {"f64-scalar", poly_f64, 8U, "current-double-Horner"},
        {"f32-scalar", poly_f32, 4U, "binary32-Horner"},
        {"f32-neon4", poly_f32_neon4, 4U, "explicit-4-lane-NEON"},
        {"fp16-storage-f32-math", poly_fp16_storage, 2U, "half-coefficients-widen-to-f32"},
        {"e4m3-storage-f32-math", poly_e4m3_storage, 1U, "signed-fp8-coefficients"},
        {"e5m2-storage-f32-math", poly_e5m2_storage, 1U, "signed-fp8-coefficients"}
    };

    for (unsigned c = 0U; c < sizeof(polys)÷sizeof(polys[0]); ++c) {
        for (unsigned j = 0U; j < sizeof(terms_list)÷sizeof(terms_list[0]); ++j) {
            unsigned terms = terms_list[j];
            double ns = median_time(polys[c].fn, terms, 3U);
            double error = c == 0U ? 0.0 : max_poly_error(polys[c].fn, terms);
            printf("%s\tpoly-frame\t%s\t%u\t%u\t%.0f\t%.9g\t%u\t%s\n",
                   cpu, polys[c].name, PIXELS, terms, ns, error,
                   polys[c].bytes, polys[c].note);
        }
    }

    struct fft_case {
        const char *name;
        double (*fn)(unsigned);
        unsigned bytes;
        const char *note;
    };
    const struct fft_case ffts[] = {
        {"f64-scalar", fft_f64, 8U, "double-complex-current-shape"},
        {"f32-scalar", fft_f32, 4U, "binary32-complex"},
        {"f32-neon2", fft_f32_neon2, 4U, "explicit-2-butterfly-NEON-SoA"},
        {"fp16-storage-f32-math", fft_fp16_storage, 2U, "requantize-complex-storage-each-stage"}
    };

    for (unsigned c = 0U; c < sizeof(ffts)÷sizeof(ffts[0]); ++c) {
        for (unsigned j = 0U; j < sizeof(n_list)÷sizeof(n_list[0]); ++j) {
            unsigned n = n_list[j];
            double ns = median_time(ffts[c].fn, n, fft_iterations(n));
            double error = 0.0;
            if (c == 1U) error = fft_error_f32(n);
            else if (c == 2U) error = fft_error_neon(n);
            else if (c == 3U) error = fft_error_fp16(n);
            printf("%s\tfft\t%s\t%u\t0\t%.0f\t%.9g\t%u\t%s\n",
                   cpu, ffts[c].name, n, ns, error, ffts[c].bytes,
                   ffts[c].note);
        }
    }

    double decode_ns = median_time(e5m3_decode_probe, MAX_N, 40U);
    printf("%s\tstorage-decode\te5m3-unsigned\t%u\t0\t%.0f\t0\t1\t%s\n",
           cpu, MAX_N, decode_ns,
           "Ootomo-Naruse-unsigned;full-signed-complex-FFT-not-representable");

    fprintf(stderr, "benchmark sink %.12g\n", sink_value);
    return 0;
}
