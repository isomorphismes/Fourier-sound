# Agent instructions

## Movie assembly

For Fourier Voice/Fourier-sound movies, this repository owns sound-time
alignment, analysis windows, transforms, mathematical construction, visual
state, and still rendering. Write a numbered still sequence beginning at frame
zero, such as `frame-000000.png` or `frame-000000.ppm`.

Use `isomorphisms/kitchen/tasks/movie-from-stills/build.sh` to assemble those stills into MP4, passing the desired
audio file when the movie should carry sound. Do not add or copy a project-local
`movie.py`, raw-RGB-to-FFmpeg wrapper, or custom movie encoder. If ordinary
movie assembly needs to change, change and test the canonical Kitchen task
instead.

Keeping the still sequence, the finished movie, or both in this repository is
allowed. The frame-to-sample clock and all Fourier/q-series/rendering choices
remain here; Kitchen knows only the numbered stills, frame rate, and optional
audio.
