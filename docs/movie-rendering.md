# Sound-driven movie rendering

Fourier-sound should use the same small downstream movie boundary as the other
visualizers. The sound experiment owns time, analysis and mathematical state;
the renderer owns still images; `tools/movie.py` only turns ordered RGB24
stills into an MP4.

```text
WAV / OGG / microphone
    ↓
decoded or acquired PCM + sample clock
    ↓
framing / decomposition
    ↓
mathematical state
    ↓
complex-field renderer
    ↓
RGB24 still
    ↓
tools/movie.py
    ↓
MP4
```

The movie writer must not know what a Fourier transform, q-series, microphone,
WAV file, or OGG file is.

## Time and audio alignment

Video time is explicit. For frame `k` at `fps`,

```text
t_k = k / fps
sample_k = round(t_k * sample_rate)
```

The experiment chooses the analysis block/window corresponding to that sample
position, runs the selected decomposition, constructs the mathematical object,
and renders one still.

This specifies the clock relationship without freezing window length, overlap,
normalization, interpolation, FFT choice, or q-series construction. Those remain
experiment policy.

## Containers and acquisition

WAV and OGG container details end at decoding. Live microphone details end at
the acquisition boundary. Downstream mathematics receives PCM plus its actual
sample rate/time information.

Source audio or processed audio may later be muxed into the final movie, but
that is downstream packaging. It must not change the still-generation
interface.

## Movie writer

`tools/movie.py` accepts only

```text
ordered RGB24 stills + width + height + fps -> MP4
```

It is intentionally the same small writer used by Pauli. The first actual
Fourier-sound movie experiment should determine whether any additional
sequencing interface is needed; do not create a larger animation API in
advance.
