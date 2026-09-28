# Harmonic decomposition is the variable

## Tao's short Fourier article

The intended Terence Tao reference is [Fourier Transform](https://www.math.ucla.edu/~tao/preprints/fourier.pdf), his article for *The Princeton Companion to Mathematics*.

The current standalone preprint is five pages, not twelve. The identifying feature matters more than the remembered page count: Tao opens with even/odd decomposition as a prototype Fourier transform.

For a real-valued function,

[
f_+(x)=\frac{f(x)+f(-x)}{2},\qquad
f_-(x)=\frac{f(x)-f(-x)}{2}.
]

The reflection (x\mapsto -x) gives an action of (\mathbf Z/2\mathbf Z). The two pieces are the (+1) and (-1) symmetry types.

This same construction works on finite data once the data carries an involution. For a list indexed by a set with (i\mapsto \bar i),

[
a_i^+=\frac{a_i+a_{\bar i}}2,\qquad
a_i^-=\frac{a_i-a_{\bar i}}2.
]

So the earlier list-of-numbers example and even/odd functions are not analogous by metaphor; they are the same projector construction applied to different objects.

## The next example is already complex

Tao immediately passes from (\mathbf Z/2\mathbf Z) to the cyclic group of (n)-th roots of unity acting on the complex plane. A function can be split into (n) rotational harmonic types. Ordinary even/odd decomposition is the (n=2) case.

That example matters here because Fourier-sound ultimately renders complex fields. The project can move directly from a finite phone sample block to group-action decompositions without treating sine/cosine frequency bins as the only possible meaning of “harmonic.”

## DFT versus FFT

A direct discrete Fourier transform and an FFT compute the same finite transform; they differ as algorithms.

Start with a small direct DFT implementation because it is:

- mathematically transparent;
- easy to port to C, D, Idriç and other languages;
- useful as a reference oracle for optimized implementations;
- independent of radix and power-of-two choices.

Add FFT implementations as interchangeable accelerators. A renderer or q-series constructor should consume transform output without knowing whether a direct DFT, radix-2 FFT, or another decomposition produced it.

## Questions this repository should keep open

Do not freeze the composition

```text
voice → FFT → q-series
```

into the architecture.

Useful comparisons include:

```text
voice → direct DFT → q-series
voice → FFT → q-series
voice → even/odd or C_n decomposition → complex field
voice → wavelet-like decomposition → complex field
voice → features → Fatou/Julia dynamical parameters
accelerometer → the same decomposition interfaces
```

The experiment is partly about discovering which decomposition and which mathematical target expose useful structure.
