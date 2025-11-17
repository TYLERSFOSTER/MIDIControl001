# Consultant Understanding Update: VoiceDopp Global Interpretation

This document captures the full, corrected engineering interpretation of VoiceDopp based on the mathematical specification provided. It reflects the Embedded Engineering Consultant’s understanding of the system architecture, design consequences, and multi-layer implications for the MIDIControl001 plugin.

---

# 1. VoiceDopp Is Not “Another Oscillator Mode”

VoiceDopp is a **physics-based acoustic field simulator**, not a subtractive or FM variation. It incorporates:

- geometric acoustics  
- Doppler kinematics  
- retarded-time phase mapping  
- emitter lattices  
- predictive emitter selection  
- spatial attenuation  
- physical crossfades  
- stochastic or deterministic emitter phase assignment  

This places VoiceDopp in a completely different class of synthesizer engines, closer to:

- wave field synthesis  
- modal synthesis  
- granular spatial emitters  
- N-body acoustic fields  

---

# 2. Different DSP Worlds

**VoiceA world:**  
- single oscillator  
- simple ADSR  
- trivial polyphony  
- no geometry  
- param-direct synthesis  

**VoiceDopp world:**  
- 2D spatial geometry  
- moving listener  
- stationary emitter field  
- infinite lattice → finite K selection  
- retarded time evaluation  
- predictive scoring  
- Doppler warping  
- geometric envelopes  
- blockwise crossfades  

These differences make mode-switching equivalent to **changing synthesis engines**, not swapping oscillator variants.

---

# 3. VoiceManager Is the Engine Supervisor

VoiceManager is no longer merely a voice allocator. It becomes:

- the *engine router* for DSP modes  
- the *lifecycle coordinator* for divergent DSP ontologies  
- the *block boundary integrator* for listener kinematics  
- the *parameter distributor* for mode-specific systems  

Your existing architecture already anticipated this, making the evolution correct.

---

# 4. ParameterSnapshot Must Become Mode-Aware

VoiceDopp requires significantly more data:

## Per-Block Time-Varying
- listener heading θ(t)  
- listener speed v(t)  
- integrated position x_L(t)  
- velocity vector v_L(t)  

## Per-Note Static
- orientation φ  
- line density ρ  
- Δ⊥ lattice spacing  
- μ_pulse  
- emitter phase assignment  
- f(k,m) frequency rules  
- possible RNG seed  

## Per-Voice Configuration
- K active emitters  
- R_in, R_out radii  
- δ margin  
- T_xfade  
- horizon H  
- attenuation kernel params (α, r_min)  

Snapshot evolves into a **structured parameter manifold**, not a handful of floats.

---

# 5. OscillatorDopp ≠ Phase Accumulator

Instead of phase += ω dt, it computes:

```
phase = φ_i + 2π λ_i * (t - r_i(t) / c)
```

Meaning:

- no incremental phase  
- no modulated accumulator  
- phase is determined by geometry  
- Doppler warping emerges naturally  

OscillatorDopp is fundamentally a **geometric phase mapper**.

---

# 6. EnvelopeDopp is Purely Retarded-Time Based

EnvelopeA uses t - t_on.  
EnvelopeDopp uses:

```
A_env_i( t_ret )
```

Implications:

- per-emitter envelope is independent  
- discontinuities allowed  
- evaluation done in past-time domain  
- no internal smoothing or stateful integration (unless explicitly designed)  

---

# 7. Predictive Emitter Selection Is a Dedicated Subsystem

Top-K selection with hysteresis and crossfade requires:

- distance computation  
- predictive geometry  
- scoring across horizons {0, H/2, H}  
- clustering logic  
- entry/exit crossfade ramping  

This should be extracted into a dedicated module:

```
EmitterSelector.h/.cpp
EmitterField.h/.cpp
EmitterInstance.h
DoppContext.h
```

Trying to place it all in a single Voice object would create an unmaintainable DSP monolith.

---

# 8. Render Function Is Fundamentally Different

**VoiceA render():**

```
osc → env → output
```

**VoiceDopp render():**

```
for each audio block:
    update listener trajectory
    select active emitters (predictive)
    update crossfade gains
    for each emitter i:
        compute r_i(t)
        compute t_ret
        evaluate env_i(t_ret)
        evaluate A_field(t_ret)
        compute sin at retarded phase
        accumulate with attenuation
```

Completely different pipeline.

---

# 9. This Mode Changes Plugin Identity

The plugin is not “a synth with modes.”  
It is:

- a **multi-engine DSP host**,  
- each engine having its own physics and rendering model,  
- sharing only the APVTS and VoiceManager supervision.  

VoiceDopp becomes one such engine.

---

# 10. All Future Code Must Reflect This Understanding

Everything built going forward must recognize VoiceDopp as:

> **A full acoustic simulation engine inside MIDIControl001.**

This impacts:
- class structure  
- parameter systems  
- mode routers  
- rendering architectures  
- performance envelopes  
- per-voice allocation  
- CPU budgeting  

---

# Next Step Readiness

This document captures the clarified global understanding.  
Upon your confirmation, I will:

1. Begin issuing file requests (explicit names only).  
2. Build a correct, minimal VoiceDopp skeleton consistent with this interpretation.  
3. Ensure zero breakage to existing VoiceA tests.  

---

*End of document.*
