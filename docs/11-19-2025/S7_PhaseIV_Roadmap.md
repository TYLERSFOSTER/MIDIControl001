# S7 — Phase IV Roadmap (Acoustic Engines, Multi‑Emitter Physics, Deterministic DSP)

## 0. Purpose

This S7 document defines the **authoritative roadmap for Phase IV** of the
MIDIControl001 project. It supersedes all earlier S7 versions and reflects:

- the *actual* codebase and architecture as it exists today  
- the *real* VoiceDopp mathematical specification provided by the human  
- the *true* execution model and block‑synchronous DSP rules  
- the *non-negotiable* development methodology (Action‑based, test‑gated)  

This roadmap is the **only valid forward plan**.  
All engineers must adhere to it strictly.

---

# 1. Phase IV Overview

Phase IV transforms MIDIControl001 from:

> “A subtractive baseline synth with a mode scaffold”

into:

> “A **multi-engine acoustic DSP environment** with physics-based emitters,
>  relativistic and classical Doppler engines, granular field simulations,
>  and mode-specific CC routing.”

Phase IV is the first phase where:

- audio output becomes **mode-dependent**  
- real physical models enter the DSP layer  
- per-voice rendering pipelines diverge  
- block-level physics computation appears  
- crossfades, predictive geometry, retarded time, and K-selection become real  

Phase IV is **not optional**: it is the direct realization of the architecture built in Phases II & III.

---

# 2. Phase IV Structure (Actions 7–11)

These actions correspond exactly to the canonical Phase IV implementation plan:

| Action | Layer | Description | Output |
|-------|--------|--------------|---------|
| 7 | Source Physics | Retarded-time source evaluation | VoiceDopp partial audio |
| 8 | Predictive Scoring | Ranking emitters via geometric prediction | Stable scoring subsystem |
| 9 | K-selection + Hysteresis | Top-K active emitters with stable selection | K-set per block |
| 10 | Crossfades | Smooth transitions between emitter sets | Zero-clicking |
| 11 | Audio Rendering | Full acoustic field rendering | First real Dopp audio |

Everything beyond 11 is Phase V.

---

# 3. Detailed Action Roadmap (Canonical)

## Action 7 — Source Functions at Retarded Time

**Goal:** implement all per-emitter source evaluations based on retarded time.

### Requirements
- Envelope evaluated at `t_ret`
- Field amplitude evaluated at `t_ret`
- Carrier evaluated using geometric phase:
  ```
  phase = φ_i + 2π f * (t - r_i(t)/c)
  ```
- Must not break block synchrony
- Must not use incremental oscillator state (this is geometry-driven)

### Tests Required
- Carrier Δphase gives correct frequency shift
- Envelope(t_ret) matches ideal ADSR
- No discontinuities at envelope corner cases
- Deterministic behavior across block boundaries

### Output of Action 7
A **per-emitter DSP kernel** that returns instantaneous audio
for a single emitter, ignoring K-selection.

---

## Action 8 — Predictive Scoring

**Goal:** use forward-predicted listener positions to evaluate emitter desirability.

### Concepts
For horizons `τ ∈ {0, H/2, H}`:
```
x_L_pred(τ)  — predicted listener position
t_ret(τ)     — predicted retarded time
A_field(t_ret(τ)) — predicted field strength
score = maxτ A_field(...)
```

### Requirements
- Must be symmetric under geometric reflection
- Must provide deterministic ordering
- No randomization unless explicitly seeded
- Numerical stability under high velocity

### Tests Required
- Approaching emitter scores higher
- Symmetric geometry = symmetric score
- Identical scores → deterministic tie-break
- Scoring invariant under block size variation

### Output of Action 8
A **stable scalar scoring function** for every emitter in the lattice window.

---

## Action 9 — K-Selection with Hysteresis

**Goal:** determine the set of active emitters per block.

### Mechanism
- Region entry: emitters within radius `R_in`
- Region exit: removed when `r ≥ R_out`
- Challenger logic:
  ```
  if score_challenger > score_incumbent + δ → replace
  ```
- Hysteresis prevents flapping

### Requirements
- Absolutely stable ranking across many blocks
- Deterministic under near-equal scores
- Works for large lattices

### Tests Required
- Correct entry at R_in boundary
- Correct exit at R_out
- No flapping under jittered velocity
- Ranked K-set consistent with Action 8

### Output of Action 9
A **stable, per-block set of K active emitters**.

---

## Action 10 — Crossfade Envelope System

**Goal:** smooth transitions when emitter sets change.

### Mechanism
For an emitter i:
- fade-in rate = `+1 / T_xfade`
- fade-out rate = `−1 / T_xfade`
- gains must be continuous (no steps)
- global sum must be at least C¹

### Requirements
- No clicks  
- No amplitude spikes  
- Crossfades deterministic across blocks  

### Tests Required
- Monotonic fade-in/out
- No overshoot
- Audio continuity verified for jittered blocks
- Sum gain envelope has no discontinuities

### Output of Action 10
A **per-emitter gain envelope system**.

---

## Action 11 — Full Audio Rendering

**Goal:** merge all subsystems (7–10) into final DSP output.

### Mechanism
For each sample in block:
```
p(t) = Σ_i g_i(t) * source_i(t)
```

### Additional Logic
- distance-based attenuation  
- optional safety smoothing  
- optional beam-width adjustment  
- Dopp → audio buffer mixing  
- global RMS normalizer integration  

### Tests Required
- No NaNs  
- No blowups under extreme velocity  
- Doppler shift matches analytic ground truth  
- Long-run stability (10 min synthetic stress)  

### Output of Action 11
**The first audible VoiceDopp engine**.

---

# 4. Structural Work Required in Phase IV

Phase IV requires restructuring beyond the bare Actions.

## 4.1 New Subsystem Files

Recommended minimal file layout:

```
Source/dsp/voices/dopp/
    DoppContext.h/.cpp
    EmitterField.h/.cpp
    EmitterSelector.h/.cpp
    EmitterInstance.h
    DoppSource.h/.cpp
    DoppCrossfade.h/.cpp
```

## 4.2 Clean Interfaces

- `OscillatorDopp` (geometry-phase engine)
- `EnvelopeDopp` (retarded-time envelopes)
- `EmitterSelector` (Action 8–9)
- `CrossfadeEngine` (Action 10)
- `DoppRenderKernel` (Action 11)

These interfaces must remain **block-synchronous**.

## 4.3 ParameterSnapshot Expansion

Snapshot gains additional fields:

- listener speed  
- listener heading  
- position  
- velocity vector  
- lattice orientation  
- lattice density  
- predictive horizon H  
- thresholds R_in, R_out  
- δ hysteresis margin  
- crossfade time  

All must be frozen per block.

---

# 5. Risk Map

### Risk 1 — Parameter Drift  
Mitigation: freeze values in snapshot; never read APVTS mid-block.

### Risk 2 — CPU Spikes  
Mitigation: limit K; precompute per-block geometry; vectorize operations.

### Risk 3 — Non-deterministic scoring  
Mitigation: strict ordering; deterministic tie-breaking.

### Risk 4 — Audio Clicks at K-boundaries  
Mitigation: Action 10 fade model.

### Risk 5 — Incorrect Doppler shift  
Mitigation: Action 11 analytic frequency tests.

### Risk 6 — Phase discontinuity  
Mitigation: analytic geometric phase; never accumulate phase incrementally.

---

# 6. Phase IV Completion Criteria

Phase IV is complete when **every item below is green**:

- [ ] VoiceDopp renders audible classical Doppler  
- [ ] Predictive scoring stable across 1000-block sequences  
- [ ] K-set evolution stable with zero flapping  
- [ ] Crossfades clickless under jittered host buffers  
- [ ] Deterministic across runs (bit-identical)  
- [ ] Full test suite extended to 150+ tests  
- [ ] VoiceA remains bit-identical to baseline  
- [ ] ParameterSnapshot extended safely  
- [ ] Analyzer 2.0 validates Doppler invariants  

Only then do we open Phase V (multi-mode morphing, LET, FM, hybrid engines).

---

# 7. Long-Term Track (Beyond Phase IV)

Phase V–VII include:

- multimode blending (LET ↔ Dopp)  
- emitter-field animation  
- relativistic acoustic models  
- FM/Dopp hybrids  
- stochastic emitter fields  
- GUI spatial renderer  

These are strictly beyond Phase IV.

---

# 8. Summary (One Page)

Phase IV builds:

- the **first real physics-based voice**  
- the **multi-emitter acoustic field engine**  
- the **predictive selection and stability layer**  
- the **crossfade architecture**  
- the **retarded-time evaluation path**  
- the **deterministic multi-emitter DSP core**

This roadmap is binding.  
All engineers must follow it strictly.

