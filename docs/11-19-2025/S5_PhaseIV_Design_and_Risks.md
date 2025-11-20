# S5 — Phase IV Design, Intent, Risks, and Downstream Consequences  
### (Authoritative, Current, Post‑Action 9)

This S5 document fully specifies the *design philosophy*, *architectural commitments*, *risk model*, and *downstream consequences* of **Phase IV** of the MIDIControl001 engine—specifically the emergence of **VoiceDopp** as the plugin’s first physics‑based DSP engine.

It supersedes all earlier S5 documents.

---

# 0. Purpose of Phase IV
Phase IV is the first phase where the plugin ceases being a “polyphonic sine synth” and becomes a **multi‑engine DSP host** capable of:
- geometric acoustics  
- retarded‑time emission  
- Doppler shift  
- predictive emitter selection  
- crossfades  
- multi‑emitter summation  

Phase IV introduces *real DSP differences between voice types* for the first time.

It must be executed with extreme discipline.

---

# 1. Phase IV Core Deliverables

Phase IV introduces three categories of deliverables:

## 1.1 Mathematical Engines
- **VoiceDopp** (classical Doppler, emitter lattice, retarded time)  
- Future: VoiceLET (relativistic), VoiceFM (2‑op FM)

Only **VoiceDopp** is in-scope for the opening of Phase IV.

## 1.2 Structural Supports
To support VoiceDopp correctly, Phase IV requires:
- Listener kinematics (Actions 1–4)  
- Emitter lattice (Action 5)  
- Retarded time solver (Action 6)  
- Source evaluation (Action 7)  
- Predictive scoring (Action 8)  
- K-selection & hysteresis (Action 9)  
- Crossfades (Action 10)  

**All Actions 1–9 are now fully implemented and validated.**

Remaining structural support for Action 10 (crossfades) and Action 11 (audio rendering) must follow strictly from the S3 spec.

## 1.3 Validation Systems
A physics-within-DSP voice requires far more validation:
- geometric invariants  
- time monotonicity  
- deterministic scoring  
- stable crossfades  
- no spatial discontinuities  
- no Doppler blow-ups  
- stable long-run RMS behavior  

Phase IV must leave a *verified and safe* VoiceDopp engine.

---

# 2. Design Intent (What Phase IV Is Supposed to Build)

Phase IV intends to produce:

## 2.1 A Fully Functional Acoustic Emitter Engine
VoiceDopp simulates:
- a moving listener  
- a stationary infinite emitter lattice  
- finite active window via k-selection  
- retarded-time phase mapping  
- amplitude & field envelopes  
- Doppler frequency warp  
- attenuation kernels  
- multi-emitter summation  

It is an *acoustic field simulator*, not a synthesizer oscillator.

## 2.2 Deterministic, Block-Synchronous Execution
Every DSP decision must occur at block boundaries:
- listener updates  
- emitter window selection  
- predictive scoring  
- hysteresis winner  
- crossfade gains  

**Render() must contain no control flow that depends on future state.**

## 2.3 Zero Sonic Regression for VoiceA
VoiceA must remain **bit-identical** to its Phase III version.

Phase IV must never alter:
- EnvelopeA  
- OscillatorA  
- VoiceManager’s behavior with VoiceA  
- RMS behaviors  
- globalGain behavior  

VoiceA is sacred.

## 2.4 A Clean Engine Surface for Future Modes
VoiceDopp is the “pattern” for future physics engines:
- VoiceLET (relativistic)  
- VoiceREL (spacetime dilation)  
- VoiceFM (Bessel-frequency engine)  

Phase IV must build VoiceDopp in a way that generalizes.

---

# 3. Architectural Surfaces Modified by Phase IV

Phase IV touches five major architectural surfaces:

---

## 3.1 BaseVoice Contract Expansion
VoiceDopp requires additions:
- per-voice geometry state  
- per-voice emitter selection state  
- per-voice crossfade ramp state  
- per-block precomputation hook  
- retarded-time evaluation utilities  

This will expand BaseVoice by *adding virtual preBlock()*, not modifying render() semantics.

---

## 3.2 VoiceManager Extensions
VoiceManager must orchestrate new behaviors:
- block-synchronous listener trajectory updates  
- snapshot → Dopp-specific params  
- preBlock invocation for all voices  
- mode-specific CC routing (future)  
- parameter surfaces (future)  

VoiceManager must remain deterministic.

---

## 3.3 ParameterSnapshot Expansion
VoiceDopp requires:
- listener velocity vector  
- orientation φ  
- density ρ  
- Δ⊥ spacing  
- speedOfSound  
- horizon H  
- K-window bounds  
- R_in, R_out hysteresis radii  
- δ challenger margin  

Snapshot must remain immutable per block.

---

## 3.4 New Subsystems (Mandatory)
Phase IV introduces new subsystems:

### A. EmitterField  
Responsible for:
- n(φ), b(φ) basis  
- Δ⊥ spacing  
- emitter coordinate generation

### B. EmitterSelector  
Handles:
- predictive scoring  
- challenger thresholding  
- hysteresis  
- choice of K window  

### C. DoppContext  
Holds:
- listener kinematics  
- time t, position, velocity  
- sound speed  
- global modeling constants  

### D. CrossfadeController  
Manages:
- fade-in and fade-out ramps  
- gain smoothing  
- window-to-window stabilization  

These must be implemented incrementally.

---

# 4. Dependencies Between Actions (Critical)

Actions 1–9 are now implemented and passing.

Action 10 (crossfades) depends directly on:
- Action 9 K-selection  
- Action 8 scoring  
- Action 7 source evaluation  
- Action 6 retarded time  

Action 11 (audio rendering) depends on:
- all of the above  
- crossfade stability  
- proper gain smoothing  
- no degenerate windows  

Therefore:

```
Action 10 must be implemented before Action 11.
Action 11 cannot skip or “partially borrow” from future systems.
```

---

# 5. Detailed Risk Model

Phase IV is mathematically sensitive.  
These are the failure risks and their mitigations.

---

## 5.1 Risk: Retarded Time Instability
**Issue:** t_ret = t − r/c may approach negative or flip sign.

**Mitigation:**  
- clamp minimum r to r_min > 0  
- enforce t_ret <= t  
- use double precision for distance  
- forbid v >= c  

---

## 5.2 Risk: Hysteresis Flapping
**Issue:** Without correct δ and R_in/R_out, selection will oscillate.

**Mitigation:**  
- unit-tested entry/exit rules  
- deterministic scanning of emitter window  
- enforce exact tie-breaking order (scan order)  

---

## 5.3 Risk: Crossfade Clicks
Without monotone ramps, block-boundary clicks occur.

**Mitigation:**  
- ramp-in/out must be C¹ smooth  
- avoid re-entry into same emitter with different gain states  
- no instantaneous gain changes  

---

## 5.4 Risk: Deterministic Drift
Predictive scoring must be reproducible.

**Mitigation:**  
- forbid RNG  
- forbid floating-point comparisons without Approx() margin  
- lock orientation, velocity, density to snapshot  

---

## 5.5 Risk: Breaking VoiceA
VoiceA must remain bit-exact.

**Mitigation:**  
- all VoiceDopp code lives in new files  
- no changes to EnvelopeA or OscillatorA  
- VoiceManager must avoid mode leakage  
- tests run VoiceA baseline diff  

---

## 5.6 Risk: Multi-Emitter Summation Blow-Up
Summing many emitters risks excessive RMS.

**Mitigation:**  
- cap active emitters  
- attenuation kernel must limit distant emitters  
- RMS normalizer must remain stable  

---

## 5.7 Risk: Excessive CPU Load
Dopp is heavier than VoiceA.

**Mitigation:**  
- limit K-window size  
- reduce emitter field density  
- allow compile-time tuning  

---

# 6. Downstream Consequences of Phase IV

## 6.1 Plugin Identity Changes
MIDIControl001 becomes:
- a physics DSP simulator  
- not just a subtractive voice  

This affects:
- documentation  
- marketing  
- GUI  
- code structure  

## 6.2 Analyzer 2.0 Required
Physics-based DSP requires:
- invariants  
- monotonicity checks  
- distance-based expectations  

This is a long-term requirement.

## 6.3 Future Modes Become Easy
VoiceLET and VoiceFM can reuse:
- snapshot expansions  
- preBlock  
- crossfade system  
- emitter selection  

## 6.4 Long-Term Engine Evolution
Phase IV establishes:
- stable BaseVoice interface  
- stable Manager orchestration  
- stable snapshot semantics  
- modular physics engines  

This yields decades-long maintainability.

---

# 7. Phase IV Acceptance Criteria

Phase IV is **complete** only when all of the following pass:

- VoiceDopp Action 10 (crossfade) tests  
- VoiceDopp Action 11 (audio) tests  
- no regressions in VoiceA  
- hysteresis stable under stress tests  
- no clicks, pops, or discontinuities  
- retarded time monotonicity holds  
- RMS never exceeds safety bounds  
- predictive scoring remains deterministic  
- new S3/S5/S6/S7 are fully updated  

Only then can Phase V begin.

---

# 8. Final Summary

Phase IV is the **first DSP-divergent phase**.  
It introduces VoiceDopp as a full physics engine.

It requires:
- precision  
- determinism  
- strict sequencing  
- zero regression  

This S5 document defines the exact constraints, risks, surfaces, and consequences.

Phase IV must be executed with absolute discipline.

