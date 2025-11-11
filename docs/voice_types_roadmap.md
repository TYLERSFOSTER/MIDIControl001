# VOICE TYPES ROADMAP

The triple **`VoiceA` / `OscillatorA` / `EnvelopeA`** constitutes the baseline modular voice type for this plugin **MIDIControl001**.  
This document outlines the direction for more advanced **`VoiceX` / `OscillatorX` / `EnvelopeX`** combinations—voice architectures designed to explore richer physical and psychoacoustic behaviors while maintaining the same pluggable infrastructure.

---

## 1. `VoiceDopp` / `OscillatorDopp` / `EnvelopeDopp`

### 1.1 Conceptual Overview

The `VoiceDopp` type models a *field of micro-sources*—hundreds of discrete oscillators positioned in 3‑D space—each representing a small emitter of sound. The listener moves through this field, perceiving the cumulative Doppler shifts and intensity falloff of these emitters.

This simulates the physics of *motion through a dynamic soundscape*: as the listener changes velocity and direction, the perceived pitch and amplitude of each emitter are continuously modulated according to classical Doppler and distance laws.

<figure align="center">
  <img src="../assets/VoiceDopp_concept.png" alt="VoiceDopp Concept Diagram" width="90%">
  <figcaption><b>Figure 1.</b> Concept sketch of the VoiceDopp modular voice architecture.</figcaption>
</figure>

---
### 1.2 Implmentation
#### 1.2.1 Physical Basis

Each emitter `i` has position $\mathbf{x}_{i}(t)$ and velocity $\mathbf{v}_{i}(t)$. The listener has position $\mathbf{x}_{L}(t)$ and velocity $\mathbf{v}_{L}(t)$.  
The instantaneous perceived frequency is given by the classical Doppler relation:

$$
f'_i(t) = f_i(t) \frac{c + \mathbf{v}_L \cdot \hat{\mathbf{r}}_i(t)}{c - \mathbf{v}_i \cdot \hat{\mathbf{r}}_i(t)},
$$
where $\hat{\mathbf{r}}_i(t) = \frac{\mathbf{x}_i(t) - \mathbf{x}_L(t)}{\|\mathbf{x}_i(t) - \mathbf{x}_L(t)\|}$
and $c$ is the speed of sound (≈343 m/s).

Amplitude scales as inverse square of distance:
$$
A_i(t) = A_0 / \|\mathbf{x}_i(t) - \mathbf{x}_L(t)\|^2.
$$

These computations are lightweight enough to run per block, with emitter states updated at audio‑rate interpolation.

---

#### 1.2.2 Parameter Mapping (MIDI / CC)

Each MIDI key spawns a local field of emitters with unique base frequencies.  
The global listener velocity (CC6) applies across all active notes, so pitch shifts are *coherent across the soundscape*.

<p align="left">
  <picture>
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: dark)">
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: light)">
    <img src="assets/MPK_Mini.jpg" alt="MIDIControl001" width="400">
  </picture>
</p>

| CC | Control | Range / Meaning | DSP Effect |
|----|----------|----------------|-------------|
| **CC6** | *Signed velocity of listener* | 0.0 → $+-\mathbf{v}^{\operatorname{max}}$ (reverse), 0.5 → 0 (stationary), 1.0 → $+\mathbf{v}^{\operatorname{max}}$ (forward) | Determines relative Doppler shift of all emitters |
| **CC7** | *Emitter field density* | 0.0 → sparse (few emitters), 1.0 → dense (hundreds) | Controls number of emitters per note and their spatial distribution |
| **CC1–CC2** | *Global volume / mix* | Retained from baseline synth | Affects overall mixdown and dry/wet blend |

---

#### 1.2.3 Implementation Sketch

- `VoiceDopp` inherits from `BaseVoice` and overrides `render()` to manage a small emitter pool.  
- Each emitter is an instance of a lightweight `DoppEmitter` struct holding phase, base frequency, position, and velocity.  
- `OscillatorDopp` computes per‑emitter waveforms (sine or band‑limited) with frequency offsets from Doppler equations.  
- `EnvelopeDopp` modulates the overall amplitude envelope for the note cluster (attack/decay).  
- The `VoiceManager` provides `CC6` and `CC7` via `ParameterSnapshot` so that all active voices receive updated motion and density parameters per block.  
- All emitters mix to a mono or stereo buffer with simple distance‑based panning.

```cpp
struct DoppEmitter {
    juce::Vector3D<float> pos, vel;
    float baseFreq, phase;
};

void VoiceDopp::render(float* buffer, int numSamples) {
    for (auto& e : emitters_) {
        float dopplerHz = e.baseFreq * (c + vL.dot(rhat(e))) / (c - e.vel.dot(rhat(e)));
        oscDopp_.setFrequency(dopplerHz);
        for (int n = 0; n < numSamples; ++n)
            buffer[n] += envDopp_.nextSample() * oscDopp_.nextSample();
    }
}
```

---

### 1.5 Integration with Modular Architecture

| Component | Role | Connection |
|------------|------|------------|
| `PluginProcessor` | reads CC6/CC7 from APVTS | builds `ParameterSnapshot` |
| `VoiceManager` | distributes snapshot per block | updates all `VoiceDopp` instances |
| `VoiceDopp` | per‑note engine | spawns & updates emitters |
| `OscillatorDopp` | per‑emitter generator | applies Doppler shift |
| `EnvelopeDopp` | per‑voice amplitude shaping | matches perceptual dynamics |

This architecture remains fully compatible with the existing **VoiceX** model: adding new voice types involves subclassing and parameter‑group registration, with no changes to `PluginProcessor` or `VoiceManager` fundamentals.

---
