
# S7 — Phase III → Phase IV Roadmap (Voice System Evolution)

This document provides the *full, long‑form engineering roadmap* for the MIDIControl001 voice‑system evolution from **Phase III** (mode scaffolding, multi‑voice architecture) into **Phase IV** (actual new synthesis types, Doppler/LET/Relativistic behaviors, FM, cross‑modulation, CC topologies, and unified DSP pipelines).

This is written for *expert incoming engineers*, assuming deep DSP, JUCE, and systems‑engineering experience.

---

# 0. High-Level Summary

Phase III establishes:
- A stable *VoiceMode layer*
- Deterministic *voice reconstruction logic*
- Block‑level mode‑reactivity (B8/B9)
- Test‑anchored behavioral invariants
- Zero functional change to baseline VoiceA

Phase IV will:
- Introduce actual **new voice types**  
- Expand the DSP architecture into a *multi‑physics* voice framework  
- Formalize **per‑mode CC maps**, **per‑mode parameter surfaces**, and **per‑mode envelopes**  
- Expand the analyzer pipeline to catch voice‑type failures  
- Ensure reversibility + testability at every step  

This roadmap expresses all foreseeable paths.

---

# 1. Roadmap Track A — Voice‑Type Expansion

This track introduces new synthesizer “physics models.”  
The order is chosen to minimize architectural turbulence.

## A1 — VoiceDopp (Classical Doppler Voice)  
**Goal:** Introduce time‑varying pitch based on relative velocity of listener ↔ source, using retarded‑time approximation.

**Changes:**
- `VoiceDopp.h/.cpp` implementing:
  - classical Doppler formula `f' = f * c / (c ± v)`
  - pre‑render trajectory integration  
  - velocity curve → pitch modulation → oscillator frequency  
- Add voice‑specific CC routings:
  - CC3 = radial velocity  
  - CC4 = acceleration  
  - CC5 = “perspective mix” (near/far weighting)
- Unit tests:
  - frequency shift correctness  
  - continuous pitch behavior under acceleration  
  - RMS/peak invariants  
  - compatibility with polyphonic mixing  

**Risks:**
- aliasing under high acceleration  
- discontinuities when velocity goes through zero  
- phase-reset artifacts

---

## A2 — VoiceLET (Relativistic Doppler Voice)
**Goal:** Extend VoiceDopp into relativistic Doppler:  
`f' = f * sqrt((1 - β) / (1 + β))`.

**Changes:**
- Add Lorentz-boosted time coordinate
- include `γ = 1 / sqrt(1 - β²)`  
- Add “time dilation envelope modifier”  
- New CC mappings:
  - CC3 = β (normalized velocity)  
  - CC4 = dilation factor  
  - CC5 = worldline curvature  

**Risks:**
- domain restrictions β < 1  
- numerical instability as β → 1  
- smoothing required for γ

---

## A3 — VoiceRel (Full Relativistic Emission Model)
**Goal:** Introduce:
- retarded‑time emission
- light‑cone intersection solver
- variable propagation medium

This reprises prior work you already prototyped with me in earlier sessions.

**Tests:**
- Reproduce prior analytic curves  
- Compare with simulation mode  
- Validate no drift across 512‑sample blocks

**Risks:**
- Block boundaries cause subtle time shifts  
- Requires sub-sample phase alignment

---

## A4 — VoiceFM (Two‑Operator FM Voice)
**Goal:** Introduce FM architecture but keep it stable + minimal.

**Design:**
- Carrier oscillator + mod oscillator  
- CC mapping:
  - CC3 = FM index  
  - CC4 = mod frequency ratio  
  - CC5 = mod depth curve  

**Tests:**
- Periodicity preservation under ratio = 1, 2, 3…  
- Index sweep continuity  
- Compare against analytic sine-FM reference (Bessel decomposition)

---

# 2. Roadmap Track B — Parameter & CC Architecture

## B1 — Per‑Mode Parameter Surfaces
Every voice mode requires different:
- default values  
- ranges  
- smoothing times  
- CC meaning  

Create:
- `ModeParamMap.h`  
- static tables mapping `(VoiceMode → ParamSurface)`

**Tests:**
- APVTS surface loads correctly  
- All modes round‑trip through save/restore  
- Snapshot→voice alignment invariant under switching

---

## B2 — Per‑Mode CC Routing Tables
A unified compile‑time structure:

```cpp
struct CCRouting {
    std::function<void(Voice&, float)> handlers[128];
};
```

Mode‑indexed:

```
CCRoutings[VoiceMode::A]
CCRoutings[VoiceMode::Dopp]
CCRoutings[VoiceMode::LET]
...
```

**Tests:**
- dispatch correctness  
- no undefined CC access  
- no X‑mode leakage into Y‑mode

---

## B3 — Mode‑Aware Envelope Generators
Some modes (LET, Rel) require:
- dilation  
- time‑warping  
- post‑Lorentz rescaling  

Introduce:
- `EnvelopeLET`  
- `EnvelopeRel`  

Test:
- continuity  
- monotonicity  
- attack/release stability

---

# 3. Roadmap Track C — DSP Pipeline Unification

This track ensures all voices plug into a consistent DSP sequencing layer.

## C1 — Split Envelope/Oscillator from Voice Core
Goal: Per‑mode choices of env/osc without rewriting Voice code.
Introduce:
- `OscillatorInterface`
- `EnvelopeInterface`

And per‑mode implementations:
- `OscillatorA`, `OscillatorDopp`, `OscillatorLET`, `OscillatorFM`
- `EnvelopeA`, `EnvelopeLET`, etc.

---

## C2 — Introduce Mode‑Aware Pre‑Render Pass
All future voices require a *pre‑block integration* step.

For block N:
- compute trajectory  
- update frequency curves  
- bake envelope constants  
- check time‑domain constraints  

**Tests:**
- pre‑block invariance under sample rate changes  
- deterministic behavior under render() repetition  
- analyzer integration tests

---

## C3 — Polyphonic Consistency & Gain Model Revision
Future voices require per‑voice levels that vary wildly  
(e.g., relativistic dilation, FM index shifts).

We introduce:
- per‑mode “energy model”  
- normalization pre‑mix  
- revised global gain controller  

Tests:
- no clipping  
- RMS stability  
- voice‑stealing under high index FM still safe

---

# 4. Roadmap Track D — Analyzer and Safety Systems

## D1 — Analyzer 2.0 (Mode‑Aware)
Add:
- per‑mode expected RMS  
- “physics invariants”  
- new exit codes  

Examples:
- LET: energy dilation monotonicity  
- Dopp: velocity→pitch mapping monotonic  
- FM: index→spectrum complexity increases consistently  

---

## D2 — Diagnostic Channels
Add file outputs:
- frequency trajectories  
- envelope dilation traces  
- modulation index curves  
- debug waveforms  

All optional but critical for debugging Mode C, D.

---

# 5. Roadmap Track E — Phase IV+ Speculative Extensions

## E1 — VoiceMesh (Multi‑Emitter Geometry)
You proposed experiments where multiple emitters interact.  
Could introduce:
- geometric configuration file  
- “mesh voice” combining many subvoices  

## E2 — VoiceField (Field‑Line Oscillation Model)
Nonlocal oscillation based on differential equations:
- u_tt = c² u_xx over synthetic geometry  
- solved approximately per block  
- used to shape timbre dynamically  

## E3 — Machine‑Learning‑Driven Voice Parameterization
Your ML background makes this a natural extension:
- learned oscillator curves  
- learned FM index envelope  
- learned Doppler curves trained on real recordings

---

# 6. Final Integration Plan

**Phase III Goal:**  
All mode‑switching, factory logic, rebuild logic is stable, deterministic, and test‑guarded.

**Phase IV Goal:**  
Introduce *actual new DSP*, with full architectural support.

**Sequence for Phase IV Start:**

1. Create `VoiceDopp` class (A1)  
2. Register in factory + rebuild tests  
3. Add CC map for Dopp  
4. Extend analyzer to catch Dopp physics invariants  
5. Validate basic polyphony  
6. Only then: LET → Rel → FM

We never break the existing baseline VoiceA tests.  
Every new mode goes through:
- constructor tests  
- render tests  
- analyzer tests  
- polyphony tests  
- CC routing tests  
- identity‑under-default tests

---

# 7. Completion Criteria

Phase IV is complete when:
- 3+ new voice types coexist safely  
- all modes have stable CC maps  
- analyzer 2.0 can detect failures  
- upstream documents (Prime Directive, Orientation, Architecture) reflect new DSP subsystems  
- no regressions in VoiceA or baseline poly chain

---

# 8. Appendix — Glossary

**VoiceMode:**  
Enum representing synthesis models.

**Mode Surface:**  
The set of default parameters and ranges unique to each mode.

**Physics Invariant:**  
A mathematical truth the DSP must enforce (e.g., relativistic dilation).

**Analyzer 2.0:**  
Extended diagnostic system detecting DSP regressions across modes.

---

End of S7 Roadmap.
