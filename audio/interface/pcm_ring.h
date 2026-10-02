#ifndef FOURIER_PCM_RING_H
#define FOURIER_PCM_RING_H
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Single producer, single consumer. Cursor wrap is intentional unsigned
 * arithmetic. Reset only while both sides are quiescent. No allocation. */
struct pcm_ring {
    unsigned char *storage;
    uint32_t capacity;
    size_t frame_bytes;
    _Atomic uint32_t read_cursor, write_cursor, dropped;
};
bool pcm_ring_init(struct pcm_ring *ring, void *storage,
                   uint32_t capacity, size_t frame_bytes);
void pcm_ring_reset(struct pcm_ring *ring);
size_t pcm_ring_write(struct pcm_ring *ring, const void *frames, size_t count);
size_t pcm_ring_read(struct pcm_ring *ring, void *frames, size_t capacity);
uint32_t pcm_ring_available(const struct pcm_ring *ring);
#endif
