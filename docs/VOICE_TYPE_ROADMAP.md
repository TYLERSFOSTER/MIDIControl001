# VOICE TYPES ROADMAP

The triple **`VoiceA` / `OscillatorA` / `EnvelopeA`** constitutes the baseline modular voice type for this plugin **MIDIControl001**. Architectural details can be found in [docs/architecture.md](../docs/architecture.md). The present document outlines the direction for more advanced **`Voice` / `Oscillator` / `Envelope`** combinations—voice architectures designed to explore richer physical and psychoacoustic behaviors while maintaining the same pluggable infrastructure.

---

## 1. `VoiceDopp`: Classical Doppler, Moving Through Fields of Emitters

*For further development and more extensive details of the `VoiceDopp`, please see [`docs/VoiceDopp_MathSpec.md`](../docs/VoiceDopp_MathSpec.md) 

### 1.1 Conceptual Overview

The `VoiceDopp` type models a *field of micro sources*, that is, hundreds of discrete oscillators positioned in 3-dimensional space, each representing a small emitter of sound. The listener moves through this field, and `VoiceDopp` simulates what this listener hears. As the sounds reach the moving listener's ear, the listener perceives the cumulative Doppler shifts and intensity falloff of the sound coming from these emitters. MIDI controller knobs change the listener's direction and velocity, and MIDI controller keys trigger various emitters in the field.
<div align="center">
  <img src="../assets/VoiceDopp_concept.png" alt="VoiceDopp Concept Diagram" width="90%">
  <p><b>Figure 1.</b> Concept sketch of the VoiceDopp modular voice architecture.</p>
</div>
This simulates the physics of *motion through a dynamic soundscape*: as the listener changes velocity and direction, the perceived pitch and amplitude of each emitter are continuously modulated according to classical Doppler and distance laws.

---
### 1.2 Implementation
#### 1.2.1 Physical Basis / Underlying Mathematics

Each emitter, indexed with subscript $i$, has position $x_{i}(t)$ and velocity $v_{i}(t)$. The listener has position $x_{L}(t)$ and velocity $v_{L}(t)$. If $f_{i}(t)$ denotes the frequency produced by emitter $i$, the instantaneous perceived frequency $f'_{i}(t)$ coming from emitter $i$, as perceived by the moving listener, is given by the classical [*Doppler relation*](https://en.wikipedia.org/wiki/Doppler_effect):

$$
f'_i(t)=f_i(t)\frac{c + v_L \cdot \widehat{r}_i(t)}{c - v_i \cdot \widehat{r}_i(t)},
$$

where $\hat{r}_i(t) = \frac{x_i(t) - x_L(t)}{\|x_i(t) - x_L(t)\|}$ and $c$ is the speed of sound (≈343 m/s).

Amplitude scales as inverse square of distance:

$$
A_i(t)=\frac{A_0}{\|x_i(t) - x_L(t)\|^2}.
$$

These computations are lightweight enough to run per block, with emitter states updated at audio‑rate interpolation.

---

#### 1.2.2 Parameter Mapping (MIDI / CC)

Each MIDI key spawns a local field of emitters with unique base frequencies. The global listener velocity (CC6) applies across all active notes, so pitch shifts are *coherent across the soundscape*.
<div align="center">
  <img src="../assets/MPK_Mini_labelled.jpg" alt="MPK Mini" width="70%">
  <p><b>Figure 2.</b> Akai MPK Mini MIDI controller with knob labels.</p>
</div>

| CC# | Control | Range / Meaning | DSP Effect |
|----|----------|----------------|-------------|
| CC1–CC5 | *Global volume, global mix, voice attack, voice release, & voice pitch* | Retained from baseline synth | Affects overall mixdown and dry/wet blend, as well as ADSR envelope and keyboard tuning |
| CC6 | *Signed velocity of listener* | 0.0 → $+-v^{\text{max}}$ (reverse), 0.5 → 0 (stationary), 1.0 → $+v^{\text{max}}$ (forward) | Determines relative Doppler shift of all emitters |
| CC7 | *Emitter field density* | 0.0 → sparse (few emitters), 1.0 → dense (hundreds) | Controls number of emitters per note and their spatial distribution |

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

#### 1.2.4 Integration with Modular Architecture

| Component | Role | Connection |
|------------|------|------------|
| `PluginProcessor` | reads CC6/CC7 from APVTS | builds `ParameterSnapshot` |
| `VoiceManager` | distributes snapshot per block | updates all `VoiceDopp` instances |
| `VoiceDopp` | per‑note engine | spawns & updates emitters |
| `OscillatorDopp` | per‑emitter generator | applies Doppler shift |
| `EnvelopeDopp` | per‑voice amplitude shaping | matches perceptual dynamics |

This architecture remains fully compatible with the existing **VoiceX** model: adding new voice types involves subclassing and parameter‑group registration, with no changes to `PluginProcessor` or `VoiceManager` fundamentals.

### 1.3 Knob (`CC#`) Behavior Amendment — Extended `VoiceDopp` Control Specification

This amendment supersedes the original section **1.2.2 (Parameter Mapping)** in the *Voice Types Roadmap*.  
It defines the **finalized control semantics** for `VoiceDopp` after November 2025 design revisions, integrating all current physical, temporal, and architectural constraints.

---

#### 1.3.1 Updated CC Mapping Overview

| CC | Control | Range / Mapping | Sampling Behavior | Scope | Description |
|----|----------|----------------|------------------|--------|--------------|
| `CC1` | Global volume | [0–1] | per block | global | Controls overall gain applied at mixdown. |
| `CC2` | Global mix (wet/dry) | [0–1] | per block | global | Crossfades between dry signal and synthesized output. |
| `CC3` | Envelope attack | [0–1] | per block | per-voice continuous | Retains baseline ADSR behavior. |
| `CC4` | **Emitter-field pulse rate (Hz)** | [0–1] → 0.1–8 Hz (log map) | **sampled at Note On** | per-voice fixed | Defines rhythmic modulation speed for emitter amplitude. |
| `CC5` | **Listener speed magnitude** | [0–1] → 0–vₘₐₓ | per block | global continuous | Sets Euclidean magnitude of listener velocity. |
| `CC6` | **Listener direction (heading angle)** | [0–1] → 0–2π radians | per block | global continuous | Determines direction of motion vector. |
| `CC7` | **Emitter-line normal angle** | [0–1] → 0–2π radians | **sampled at Note On** | per-voice fixed | Rotates emitter lattice orientation. |
| `CC8` | **Emitter-line density** | [0–1] → spacing = 1 / (ρₘₐₓ·CC8 + ε) | **sampled at Note On** | per-voice fixed | Controls distance between parallel emitter lines (0 → single line, 1 → dense 2D lattice). |

---

#### 1.3.2 Temporal Semantics

1. **Per-Block Continuous Controls (CC1–CC3, CC5–CC6):**  
   - Updated once per audio block through the standard `APVTS → ParameterSnapshot → VoiceManager` chain.  
   - Affect all active voices coherently.  
   - Provide global motion and global tone behavior.  

2. **Per-Note Sample-and-Hold Controls (CC4, CC7, CC8):**  
   - Values captured once at `noteOn()` and stored immutably in that voice instance.  
   - Mid-note changes do **not** affect existing emitters.  
   - New notes adopt the current CC4/7/8 state, allowing overlapping voices with distinct pulse rates and lattice geometries.  

This guarantees deterministic behavior, prevents per-block geometry churn, and enables polyphonic spatial independence.

---

#### 1.3.3 Mathematical Model Summary

##### 
***Listener Motion.***
Listener velocity vector defined each block by:
$$
v_L = v_{\text{max}} 
\begin{bmatrix}
\cos(2π·\textsf{CC6}) \\
\sin(2π·\textsf{CC6})
\end{bmatrix}
·\textsf{CC5}.
$$
This vector drives Doppler frequency modulation for all emitters in all active voices.

#####
***Emitter Geometry.***
Define $θ_N := 2π·\textsf{CC7}$.
At note-on, each voice instantiates its emitter lattice according to:
$$
x_i = m·n + k·t,
$$
where
$$
n = (\cos θ_N, \sin θ_N)\quad\text{and}\quad
t = (−\sin θ_N, \cos θ_N).
$$
Here
- `CC8` controls line spacing as $d = 1 / (ρ_{max} · \textsf{CC8} + ε)$.  
- `CC8` → 0 produces one emitter line; `CC8` → 1 yields a dense quasi-2D field.  
- Geometry is frozen until `noteOff()`.

##### 
***Field Pulse Modulation.***
Each note has a field-wide amplitude LFO driven by CC4:
$$
A_{\text{field}}(t) = \tfrac{1}{2}\Big(1 + \sin\big(2π\;f_{\text{pulse}}(t)\big)\Big)
$$
where $f_{\text{pulse}}$ is the linear map between real intervals
$$
f_{\mathrm{pulse}} : \underset{\text{knob value}}{\underbrace{[0\,,\;1\,]}} \longrightarrow\!\!\!\! \underset{\text{pulse frequency (Hz)}}{\underbrace{[\,0.1,\;8.0\,]}}
$$
given by the formula
$$
f_{\mathrm{pulse}}(\textsf{CC4}) = 0.1 + 7.9\cdot\textsf{CC4}
$$
This amplitude multiplier modulates all emitters in that note simultaneously, creating rhythmic “breathing” of the sound field. Different notes may pulse asynchronously, forming complex polyrhythms.

---

#### 1.3.4 Implementation Guidance for Future Consultants

1. **Snapshot Capture:** `ParameterSnapshot` retains all eight CCs per block.  
2. **Voice Spawning:** `VoiceManager::noteOn()` forwards the snapshot; `VoiceDopp::noteOn()` extracts CC4, CC7, CC8 → immutable fields.  
3. **Real-Time Safety:** `VoiceDopp::updateParams()` must skip geometry and pulse updates while active (`active_ == true`).  
4. **Render Loop:** Continually applies updated motion vector (`CC5–CC6`) while preserving frozen geometry and pulse rate.  
5. **Pulse Implementation:** Use either a small internal oscillator or `EnvelopeDopp` subcomponent; maintain phase continuity using block-based increment Δφ = 2π fₚₗₛ Δt.  
6. **Test Recommendations:**  
   - Add regression test verifying per-voice immutability of CC4–CC8 post–noteOn.  
   - Verify Doppler shift consistency under varying CC5/6 motion updates.  

All modifications remain compliant with the **Prime Directive Enforcement Charter**, maintaining file jurisdiction boundaries and deterministic runtime behavior.

---

#### 1.3.5 Behavioral Synopsis

> *`VoiceDopp` now supports independent per-note emitter geometry and pulse rhythm (CC4/7/8, sampled at note-on) combined with globally coherent motion control (CC5/6, continuous), achieving multi‑layer Doppler lattices that remain fully real‑time safe, polyphonic, and architecturally compliant.*


---
## 2. `VoiceLET`: *Relativistic* Doppler, Moving Through Fields of Emitters

### 2.1 Conceptual Overview

`VoiceLET` extends `VoiceDopp` into the relativistic regime.  Whereas `VoiceDopp` obeys the *classical Doppler law*, `VoiceLET` models the [*relativistic Doppler effect*](https://en.wikipedia.org/wiki/Relativistic_Doppler_effect) and [*Lorentz time dilation*](https://en.wikipedia.org/wiki/Lorentz_transformation), describing what a moving listener hears when traversing a dense field of micro-emitters at velocities approaching the speed of sound’s analog “c” (the propagation speed in the simulated medium).

This architecture explores perceptual distortions that occur when the relative velocity between listener and emitters becomes significant enough that classical assumptions break down — a *nonlinear* mapping between motion and perceived pitch.

<div align="center">
  <img src="../assets/Classical_versus_relativistic.gif" alt="VoiceLET Concept Diagram" width="70%">
  <p><b>Figure 3.</b> Concept GIF sketch of the VoiceLET modular voice architecture.</p>
</div>

---
### 2.2 Implementation
#### 2.2.1 Physical Basis

For each emitter $i$, with emitted frequency $f_i(t)$ and instantaneous radial velocity $v_{\text{rad}}$ (positive if approaching the listener), the **relativistic Doppler shift** is given by:

$$
f'_i(t) = f_i(t) \sqrt{\frac{1 + v_{\text{rad}}/c}{1 - v_{\text{rad}}/c}},
$$

where $c$ is the propagation speed constant of the medium (analogous to the speed of sound or light).

Amplitude falloff and spatial panning follow the same inverse-square and vector-projection logic as in `VoiceDopp`, but the **time parameterization** now includes Lorentz scaling:

$$
t' = \frac{t - v_{\text{rad}} x / c^2}{\sqrt{1 - (v/c)^2}}.
$$

This produces a perceptual compression of time and frequency content as relative speed increases.

---

#### 2.2.2 Parameter Mapping (MIDI / CC)

| CC | Control | Range / Meaning | DSP Effect |
|----|----------|----------------|-------------|
| CC1–CC5 | *Global volume, global mix, voice attack, voice release, & voice pitch* | Retained from baseline synth | Affects overall mixdown and dry/wet blend, as well as ADSR envelope and keyboard tuning |
| CC6 | *Relativistic velocity ratio* | 0.0 → stationary, 1.0 → \(v = c\) | Controls Lorentz factor γ and relativistic Doppler scaling |
| CC7 | *Emitter field density* | 0.0 → sparse, 1.0 → dense | Controls number of emitters per note (same as `VoiceDopp`) |

---

#### 2.2.3 Implementation Sketch

- `VoiceLET` inherits from `VoiceDopp` and overrides the frequency update logic.  
- The `OscillatorLET` computes per-emitter frequencies using the relativistic Doppler law instead of the classical one.  
- The `EnvelopeLET` optionally scales its internal timebase by the Lorentz factor γ, creating perceptual “slowdowns” or “compressions” depending on velocity.  
- A global parameter \( v/c \) (normalized from CC6) is stored in the `ParameterSnapshot` and applied to all active emitters.  
- To maintain stability, the update loop clamps \( v/c < 0.99 \).

```cpp
void OscillatorLET::setRelativisticFreq(float baseFreq, float v_{\text{rad}}_over_c) {
    const float gamma = std::sqrt((1 + v_{\text{rad}}_over_c) / (1 - v_{\text{rad}}_over_c));
    currentFreq_ = baseFreq * gamma;
}
```
---

## 3 `VoiceEns` / `FreqBandEns` / `EnvelopeEns`

[...]

a
- Introduce ModMatrix UI
- Implement parameter morphing API
