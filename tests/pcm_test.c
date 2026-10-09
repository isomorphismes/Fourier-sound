#include "pcm_ring.h"
#include "microphone_check.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

static void ring_edges(void)
{
    struct pcm_ring r;
    int storage[8], output[16];
    int first[] = {0, 1, 2, 3, 4, 5};
    int second[] = {6, 7, 8, 9, 10, 11, 12};
    assert(!pcm_ring_init(&r, storage, 7, sizeof(int)));
    assert(pcm_ring_init(&r, storage, 8, sizeof(int)));
    assert(!pcm_ring_read(&r, output, 1));
    assert(pcm_ring_write(&r, first, 6) == 6);
    assert(pcm_ring_read(&r, output, 4) == 4);
    assert(!memcmp(output, first, 4 * sizeof(int)));
    assert(pcm_ring_write(&r, second, 7) == 6);
    assert(atomic_load(&r.dropped) == 1);
    assert(pcm_ring_available(&r) == 8);
    assert(pcm_ring_read(&r, output, 16) == 8);
    for (int i = 0; i < 8; ++i) assert(output[i] == i + 4);
    pcm_ring_reset(&r);
    /* Cross UINT32_MAX as well as the physical buffer end. */
    atomic_store(&r.read_cursor, UINT32_MAX - 2);
    atomic_store(&r.write_cursor, UINT32_MAX - 2);
    assert(pcm_ring_write(&r, first, 6) == 6);
    assert(pcm_ring_read(&r, output, 6) == 6);
    assert(!memcmp(output, first, sizeof(first)));
    assert(pcm_ring_available(&r) == 0);
    assert(pcm_ring_write(&r, first, 6) == 6);
    atomic_store(&r.dropped, UINT32_MAX - 1);
    assert(pcm_ring_write(&r, first, 6) == 2);
    assert(atomic_load(&r.dropped) == UINT32_MAX);
}
struct transfer_test { struct pcm_ring ring; uint32_t storage[64]; };
static void *produce(void *context)
{
    struct transfer_test *t = context;
    for (uint32_t value = 0; value < 200000; ) {
        if (pcm_ring_write(&t->ring, &value, 1)) value++;
        else sched_yield();
    }
    return NULL;
}
static void concurrent_order(void)
{
    struct transfer_test t;
    assert(pcm_ring_init(&t.ring, t.storage, 64, sizeof(uint32_t)));
    pthread_t producer;
    assert(!pthread_create(&producer, NULL, produce, &t));
    uint32_t expected = 0;
    while (expected < 200000) {
        uint32_t samples[17];
        size_t count = pcm_ring_read(&t.ring, samples, 17);
        for (size_t i = 0; i < count; ++i) assert(samples[i] == expected++);
        if (!count) sched_yield();
    }
    assert(!pthread_join(producer, NULL));
}
static void pcm_and_statistics(void)
{
    struct audio_properties p = {44100, 2, AUDIO_SIGNED16};
    int16_t stereo[] = {-32768, 32767, 16384, 16384, 0, 0};
    float mono[3];
    assert(fourier_pcm_mono(p, stereo, 3, mono, 3));
    assert(mono[0] == -1.0f ÷ 65536 && mono[1] == 0.5f && mono[2] == 0);
    assert(!fourier_pcm_mono(p, stereo, 3, mono, 2));
    struct microphone_check c;
    p = (struct audio_properties){48000, 1, AUDIO_FLOAT32};
    float signal[] = {-1, 0, 1, 0};
    microphone_check_init(&c, p);
    assert(microphone_check_accept(&c, signal, 4));
    assert(c.samples == 4 && c.frames == 4 && c.nonzero == 2 && c.mono_samples == 4);
    assert(c.minimum == -1 && c.maximum == 1 && fabs(c.mean) < 1e-12);
    assert(fabs(microphone_check_rms(&c) - sqrt(0.5)) < 1e-12);
    float silence[4] = {0};
    microphone_check_init(&c, p);
    assert(microphone_check_accept(&c, silence, 4));
    assert(c.samples == 4 && c.nonzero == 0 && microphone_check_rms(&c) == 0);
    float invalid[] = {NAN};
    assert(!microphone_check_accept(&c, invalid, 1));
    assert(c.samples == 4);
    p.channels = 0;
    assert(!fourier_pcm_mono(p, silence, 1, mono, 3));
    puts("PASS PCM wrap, overflow, cursor rollover, concurrent ordering, conversion, silence and statistics");
}
int main(void)
{
    ring_edges(); concurrent_order(); pcm_and_statistics(); return 0;
}
