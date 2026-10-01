# Florence Nightingale, second rendition (30 July 1890)

Source recording:
- Wikimedia Commons: https://commons.wikimedia.org/wiki/File:Florence_Nightingale_voice_-_1576A_2nd_Rendition.ogg
- Internet Archive source: https://archive.org/details/FlorenceNightingale2ndRendition1890GreetingsToTheDearOldComradesOf
- Commons published SHA-1: `5de570b67b20f4e2932fb960b3e3b0f071981544`
- Commons duration: 59.0164172335601 s
- Public Domain Mark.

Archival transcript used for alignment:

> When I am no longer even a memory, just a name, I hope my voice may
> perpetuate the great work of my life. God bless my dear old comrades of
> Balaclava and bring them safe to shore. Florence Nightingale.

The requested experimental splice keeps only:

1. “When I am no longer even a memory, just a name, I hope my voice may perpetuate”
2. “Florence Nightingale.”

The wording “the sound of my voice” is not in the archival transcription; the
recording is transcribed as “I hope my voice may perpetuate”.

## Verified acoustic boundaries

The complete 59.016417 s source was checked with waveform inspection and
`silencedetect` over several thresholds.

The important boundary after “perpetuate” is unusually clear. At -24 dB the
recording is continuously below threshold from 23.3593 s through 25.5701 s.
At -20 dB the corresponding interval is 23.3372-25.5710 s. That long pause
separates “perpetuate” from “the great work of my life”.

The final spoken name is also isolated. At -36 dB there is a quiet interval
51.0746-51.2330 s before “Florence Nightingale”, and trailing quiet begins at
58.2543 s. The splice keeps a little of those quiet margins rather than cutting
on a consonant.

The committed cut intervals are therefore:

- 0.250000-23.504400 s
- 51.074600-58.493900 s

No denoising, filtering, normalization, crossfade, or time stretching is
applied. The two decoded source intervals are concatenated directly. Expected
duration is about 30.674 s.

See `nightingale-alignment.tsv` for the machine-readable cut map.
