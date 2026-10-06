# Audio construction comparison

This experiment holds the sound analysis and rendering pipeline fixed while
changing only the map from Fourier coefficients to a complex field.

For each fixture both constructions use the same loudest 4096-sample block,
mean removal, periodic Hann, normalized radix-2 FFT, viewport, and canonical
Wegert coloring.

The dense control uses the same first twelve coefficients as the sparse candidate:
`f(q)=sum_(k=0)^11 c[k] q^k`.

The sparse candidate uses those same first twelve coefficients with exponents
`0,1,2,4,8,16,32,64,128,256,512,1024`.

This dyadic schedule is an explicit candidate experiment, **not** an assertion
about Owen Maresh's missing voice-to-q-series mapping. His public code establishes
relevant q-series/lacunary phase-portrait work but does not expose that driver.

Each fixture writes `.dense.ppm` and `.dyadic.ppm`. Both must be nonconstant,
and more than 20% of pixels must differ, so the test directly measures whether
the construction changes the visual result while sound analysis stays fixed.

## First controlled corpus result

GitHub Actions run `36892722076` rendered all six real-audio fixtures with
the controlled 12-coefficient comparison. The dense and dyadic images were
visually nontrivial and materially different.

| fixture | pixels changed | mean absolute RGB difference / channel |
| --- | ---: | ---: |
| thunder/rain | 99.38% | 28.53 |
| iceberg contact | 96.59% | 24.56 |
| Dvořák Largo | 94.87% | 12.39 |
| Bartók Sonatina | 80.92% | 3.73 |
| Russolo Corale | 91.89% | 10.72 |
| Russolo Serenata | 90.77% | 6.74 |

The lower differences for Bartók and Serenata are consistent with an important
limitation of this particular comparison: twelve coefficients at 44.1 kHz with
a 4096-sample FFT cover only bins 0 through 11, roughly 0–118 Hz. This experiment
isolates exponent-schedule sensitivity; it does not yet establish a good
coefficient-selection policy for music.
