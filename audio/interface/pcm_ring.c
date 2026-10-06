#include "pcm_ring.h"
#include <string.h>

bool pcm_ring_init(struct pcm_ring *r, void *storage, uint32_t n, size_t bytes)
{
    if (!r || !storage || n < 2 || n > UINT32_MAX / 2 ||
        (n & (n - 1)) || !bytes || bytes > SIZE_MAX / n) return false;
    r->storage = storage; r->capacity = n; r->frame_bytes = bytes;
    atomic_init(&r->read_cursor, 0); atomic_init(&r->write_cursor, 0);
    atomic_init(&r->dropped, 0);
    return atomic_is_lock_free(&r->read_cursor) &&
        atomic_is_lock_free(&r->write_cursor) && atomic_is_lock_free(&r->dropped);
}
void pcm_ring_reset(struct pcm_ring *r)
{
    atomic_store(&r->read_cursor, 0); atomic_store(&r->write_cursor, 0);
    atomic_store(&r->dropped, 0);
}
static void transfer(struct pcm_ring *r, uint32_t cursor, void *frames,
                     size_t count, bool writing)
{
    size_t offset = cursor & (r->capacity - 1);
    size_t first = r->capacity - offset;
    if (first > count) first = count;
    size_t bytes = r->frame_bytes;
    if (writing) {
        memcpy(r->storage + offset * bytes, frames, first * bytes);
        memcpy(r->storage, (unsigned char *)frames + first * bytes,
               (count - first) * bytes);
    } else {
        memcpy(frames, r->storage + offset * bytes, first * bytes);
        memcpy((unsigned char *)frames + first * bytes, r->storage,
               (count - first) * bytes);
    }
}
size_t pcm_ring_write(struct pcm_ring *r, const void *frames, size_t count)
{
    uint32_t w = atomic_load_explicit(&r->write_cursor, memory_order_relaxed);
    uint32_t rd = atomic_load_explicit(&r->read_cursor, memory_order_acquire);
    size_t room = r->capacity - (uint32_t)(w - rd);
    size_t accepted = count < room ? count : room;
    if (accepted) {
        transfer(r, w, (void *)frames, accepted, true);
        atomic_store_explicit(&r->write_cursor, w + (uint32_t)accepted,
                              memory_order_release);
    }
    size_t lost = count - accepted;
    if (lost) {
        uint32_t old = atomic_load_explicit(&r->dropped, memory_order_relaxed);
        uint32_t next = lost > UINT32_MAX - old ? UINT32_MAX : old + (uint32_t)lost;
        atomic_store_explicit(&r->dropped, next, memory_order_relaxed);
    }
    return accepted;
}
size_t pcm_ring_read(struct pcm_ring *r, void *frames, size_t capacity)
{
    uint32_t rd = atomic_load_explicit(&r->read_cursor, memory_order_relaxed);
    uint32_t w = atomic_load_explicit(&r->write_cursor, memory_order_acquire);
    size_t count = (uint32_t)(w - rd);
    if (count > capacity) count = capacity;
    if (count) {
        transfer(r, rd, frames, count, false);
        atomic_store_explicit(&r->read_cursor, rd + (uint32_t)count,
                              memory_order_release);
    }
    return count;
}
uint32_t pcm_ring_available(const struct pcm_ring *r)
{
    uint32_t rd = atomic_load_explicit(&r->read_cursor, memory_order_relaxed);
    return atomic_load_explicit(&r->write_cursor, memory_order_acquire) - rd;
}
