#include "speaker_input.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    struct speaker_input input = {0};
    float source[] = {-1.5f, -0.5f, 0.5f, 1.5f};

    assert(!speaker_input_set(NULL, source, 4, false));
    assert(speaker_input_set(&input, source, 4, false));
    assert(!speaker_input_finished(&input));

    struct audio_properties signed_stereo = {44100, 2, AUDIO_SIGNED16};
    int16_t pcm16[8] = {0};
    size_t produced = 0;
    assert(speaker_input_render(&input, signed_stereo, pcm16, 4, &produced));
    assert(produced == 4);
    assert(pcm16[0] == INT16_MIN && pcm16[1] == INT16_MIN);
    assert(pcm16[2] == -16384 && pcm16[3] == -16384);
    assert(pcm16[4] == 16384 && pcm16[5] == 16384);
    assert(pcm16[6] == INT16_MAX && pcm16[7] == INT16_MAX);
    assert(speaker_input_finished(&input));

    speaker_input_reset(&input);
    struct audio_properties float_mono = {48000, 1, AUDIO_FLOAT32};
    float pcm32[2] = {0};
    assert(speaker_input_render(&input, float_mono, pcm32, 2, &produced));
    assert(produced == 2 && pcm32[0] == -1.0f && pcm32[1] == -0.5f);

    assert(speaker_input_set(&input, source + 1, 2, true));
    float looped[5] = {0};
    assert(speaker_input_render(&input, float_mono, looped, 5, &produced));
    assert(produced == 5);
    assert(looped[0] == -0.5f && looped[1] == 0.5f &&
           looped[2] == -0.5f && looped[3] == 0.5f && looped[4] == -0.5f);
    assert(!speaker_input_finished(&input));

    float invalid[] = {NAN};
    assert(!speaker_input_set(&input, invalid, 1, false));

    puts("PASS speaker input finite validation, clipping, channel duplication, reset and loop");
    return 0;
}
