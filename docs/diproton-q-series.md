# diproton: q-series visualization

## Source

Owen Maresh / diproton, [q-series visualization](https://www.youtube.com/watch?v=NcqSDIGU02I).

The public captions identify the experiment directly. A short excerpt:

> “takes my voice and uses the fft output on the machine to convert it into a Q Series”

## Caption-derived transcript summary

Maresh describes this as his first implementation of a voice-driven visualization. The program takes his voice, computes FFT output, and uses that output to construct a q-series. He describes the chosen series as lacunary and discusses the unit circle as a natural boundary, with singular behavior associated with rational angles. The displayed circle has low resolution. He also says the response needs tuning so that less vocal effort produces more visible structure. His final point is conceptual: voice becomes a way to perform and explore complex analysis without writing formulas during the performance.

This reference should not make q-series mandatory. It supplies one concrete composition:

```text
voice → audio samples → FFT coefficients → q-series → complex visualization
```

Fourier-sound should make each arrow replaceable.

## Retrieve the captions

Do not hand-maintain a stale copy of YouTube's caption track. Fetch the current caption file from the source when needed:

```sh
yt-dlp \
  --skip-download \
  --write-subs \
  --write-auto-subs \
  --sub-langs 'en.*' \
  --sub-format vtt \
  'https://www.youtube.com/watch?v=NcqSDIGU02I'
```

Keeping the source URL and retrieval command also preserves provenance if YouTube updates the automatic captions.

## Mathematical caution

The video's claim concerns the particular q-series used in the experiment. It should not be generalized to every object called a q-series. In particular, lacunarity and a natural boundary at the unit circle depend on the series.

For this repository, treat “q-series” as one selectable mathematical construction after signal decomposition, not as the definition of the transform stage.
