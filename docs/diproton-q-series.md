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

## Maresh source trail

The video belongs to a larger body of Maresh's complex-visualization work rather than an isolated demo.

- [graveolensa/tsungfruve](https://github.com/graveolensa/tsungfruve) describes itself as his mathematics journal/diary with code and links the same [diproton YouTube channel](https://youtube.com/diproton).
- [complex-geography/lacunaries](https://github.com/graveolensa/tsungfruve/tree/master/complex-geography/lacunaries) records a dedicated lacunary-functions area.
- [qpochpoly.py](https://github.com/graveolensa/tsungfruve/blob/master/qpochpoly.py) builds unit-disk complex phase plots from finite products and q-Pochhammer products.
- [borwein_cubic_alternates.py](https://github.com/graveolensa/tsungfruve/blob/master/borwein_cubic_alternates.py) implements Borwein cubic theta functions both as lattice sums and as faster q-Pochhammer expressions.
- Maresh's 2013 note [“lacunary functions and intersecting three dimensional cobordism categories”](https://tsungfruve.wordpress.com/2013/05/07/lacunary-functions-and-intersecting-three-dimensional-cobordism-categories/) says he had been collecting lacunary functions and making phase portraits of them, and gives explicit lacunary products.

This trail strongly connects the video to his earlier lacunary/q-series/phase-portrait work. It does **not** yet identify the exact formula driven by the FFT in video `NcqSDIGU02I`. Keep that as an explicit research question instead of silently substituting one of the older formulas.

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
