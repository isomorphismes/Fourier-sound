# Nightingale wax-cylinder static overlay experiment

This branch does **not** remove noise from Florence Nightingale's recording.

It asks a narrower visual question: if we later chose to suppress the persistent
wax-cylinder/static component, which Fourier terms and which parts of the current
complex-field picture would be affected?

## Estimate

The source is the complete 30 July 1890 second rendition from Wikimedia Commons,
whose published SHA-1 is `5de570b67b20f4e2932fb960b3e3b0f071981544`.

The experiment:

1. decodes the full recording to mono 44.1 kHz signed-16 PCM;
2. divides it into 4096-sample frames;
3. takes the ten quietest frames as a recording-noise sample;
4. mean-removes and Hann-windows those frames, then averages their Fourier power
   per coefficient;
5. analyzes the loudest frame of the recording through the same transform;
6. computes a *candidate static weight* for every coefficient as
   `min(1, estimated_noise_amplitude / observed_amplitude)`;
7. leaves the original coefficients and original Wegert image untouched;
8. projects only the weighted candidate coefficients through the same current
   polynomial construction;
9. overlays dark gray at up to 72% opacity according to the candidate field's
   local magnitude.

The output pair is:

- `nightingale-baseline.ppm` — ordinary image, unchanged;
- `nightingale-static-overlay.ppm` — same image with the candidate-static
  contribution darkened.

This is deliberately an **annotation**, not a denoiser and not a claim that the
quiet-frame estimate uniquely separates wax noise from Nightingale's voice.
The coefficient-level mask is independent of the temporary polynomial
construction, so a later q-series can reuse the same experiment without
changing the noise estimate.

Run:

```
make nightingale-static-overlay
```
