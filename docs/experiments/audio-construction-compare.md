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
