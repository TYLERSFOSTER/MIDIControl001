# Engineering Continuity and Alignment Report
**Project:** MIDIControl001 — VoiceDopp Implementation Phase  
**Prepared for:** Next LLM Consultant Engineer  
**Prepared by:** ChatGPT (Consultant Engineer, session closeout)  
**Date:** November 13, 2025  

---

## I. Current State of the Project

### 1. Baseline Status

- The **baseline voice architecture** (`VoiceA / OscillatorA / EnvelopeA`) is fully stable and verified:
  - No clicks, correct amplitude buildup and decay.
  - GUI knobs mapped correctly through the JUCE `AudioProcessorValueTreeState` (APVTS).
  - Persistent control states across notes working properly via `VoiceManager`.
  - Analyzer and test logs confirm stable output and correct RMS alignment.

### 2. Next Target Module: `VoiceDopp`

- Objective: Implement a physically based *Doppler-field voice type* simulating a moving listener traversing an infinite or quasi-infinite lattice of sound emitters.
- This represents a major shift from monophonic source synthesis toward spatial field synthesis, requiring coordinated control of emitter placement, attenuation, and predictive selection.
- Mathematical specification (in `/docs/VoiceDopp_MathSpec.md`) defines the full continuous model using retarded-time propagation and predictive emitter selection.

### 3. State of Documentation

| Document | Purpose |
|-----------|----------|
| `README.md` | General repo overview and build instructions. |
| `docs/architecture.md` | Full modular JUCE voice architecture (PluginProcessor → VoiceManager → VoiceX → OscillatorX/EnvelopeX). |
| `docs/VOICE_TYPES_ROADMAP.md` | Conceptual roadmap for VoiceA → VoiceDopp → VoiceLET → VoiceEns evolution. |
| `docs/VoiceDopp_MathSpec.md` | Full physical and mathematical specification (this defines the *truth model* for VoiceDopp). |
| `docs/VoiceDopp_RetardedTime_Correction.md` | Clarification on retarded-time semantics for physical correctness. |

All documents are consistent and correctly cross-referenced. Figures referenced in `assets/` are used for conceptual visualization only.

---

## II. Engineering Context and Intent

### 1. Core Architectural Principles

VoiceDopp **inherits** from `BaseVoice` just like VoiceA.  
It overrides `render()` and defines its own emitter generation and time-domain evaluation pipeline.

```
PluginProcessor → VoiceManager → VoiceDopp → (OscillatorDopp, EnvelopeDopp)
```

| Layer | Responsibility |
|-------|----------------|
| **PluginProcessor** | Reads MIDI CCs, builds `ParameterSnapshot`. |
| **VoiceManager** | Allocates/reuses voices, passes snapshot per audio block. |
| **VoiceDopp** | Maintains emitter lattice, computes Doppler-shifted signals. |
| **OscillatorDopp** | Generates per-emitter carrier signals. |
| **EnvelopeDopp** | Shapes global amplitude per note. |

All changes must remain JUCE real-time safe: no dynamic allocation inside `processBlock()`, no mutexes, no unbounded loops.

### 2. Physical Model Summary

VoiceDopp implements a *classical Doppler field*:
- Listener position: x_L(t), velocity v_L(t) derived from CC5 (speed) and CC6 (heading).
- Emitter lattice: defined at note-on by CC7 (plane angle) and CC8 (density).
- Each emitter emits a carrier s_i(t) = sin(2π λ_i t + φ_i) modulated by:
  - Distance attenuation w(r_i(t)) = e^(-α r_i) / r_i,
  - ADSR envelope A_i^{env}(t_i^{ret}),
  - Field pulse envelope A^{field}(t_i^{ret}).
- The received pressure signal is the sum over selected emitters evaluated at **retarded time**
  t_i^{ret} = t - r_i(t)/c.

---

## III. Control Mapping (MIDI CC)

| CC# | Function | Notes |
|-----|-----------|-------|
| **CC1–CC3** | Volume, Mix, Envelope params | Retained from baseline synth |
| **CC4** | Pulse rate (Hz) | Sampled at note-on; controls μ_pulse in [0.1, 8] Hz |
| **CC5** | Listener speed | Maps to Euclidean norm of velocity (|v|) |
| **CC6** | Listener direction | Angle in radians, defines heading vector |
| **CC7** | Plane normal angle | Defines emitter field orientation (sampled at note-on) |
| **CC8** | Lattice density | Defines spacing of emitter lines (sampled at note-on) |

Knobs CC4, CC7, CC8 are *sampled at note-on*, meaning emitter geometry and pulse rate are frozen for that note’s lifetime. CC5–6 are *continuous* global controls affecting all active notes in real time.

---

## IV. Mathematical Model (High-Level Summary)

Received pressure (per note):
p(t) = Σ_{i∈E_act(t)} g_i(t) w(r_i(t)) A^{env}_i(t_i^{ret}) A^{field}(t_i^{ret}) s_i(t_i^{ret})

where:
- t_i^{ret} = t - r_i(t)/c
- r_i(t) = |x_i - x_L(t)|
- λ_i^{heard}(t) = λ_i / (1 - (v_L·r̂_i)/c) (Doppler shift)

Predictive subset selection: A finite subset of emitters E_act(t) is chosen dynamically by a hysteretic scoring rule considering distance, amplitude, and lookahead projection.

---

## V. Open Mathematical and Engineering Questions

1. Attenuation kernel finiteness: For α=0, integral of 1/r diverges over infinite lattice. Current solution: finite subset E_act(t) with hysteresis + exponential falloff.  
2. Retarded-time consistency: Confirm that envelope and pulse functions are evaluated at t_i^{ret}, not at emission time. Correction doc already integrated.  
3. Blockwise causality: Real-time implementation will approximate continuous t_i^{ret} by block sample indices.  
4. Stereo/panning model: TBD; likely use projection of r̂_i onto left/right axes.  
5. Emitter caching and reallocation: Use std::vector or fixed-size pool; avoid new/delete.

---

## VI. Immediate Next Steps (Engineering Plan)

1. Define new parameter group for VoiceDopp CC mappings in ParamLayout.cpp / ParameterIDs.h.  
2. Implement VoiceDopp class skeleton: inherit BaseVoice, add emitter pool struct.  
3. Integrate CC5–CC8 handling (continuous vs. sampled).  
4. Implement retarded-time evaluation logic.  
5. Implement predictive selection (R_in/R_out, top-K, hysteresis).  
6. Add unit tests for Doppler correctness.  
7. Register VoiceDopp in InstrumentRegistry.h.  
8. Write engineering summary commit under docs/11-13-2025/VoiceDopp_Implementation_Report.md.

---

## VII. Consultant Behavioural Directives

The next LLM consultant must operate under Tyler Foster’s Prime Directive and Consultant Mode rules:

1. Conversation discipline: one issue → one fix → one confirmation.  
2. Never speculate or generate code without verified context.  
3. File modification protocol: explicit paths, backups, diffs only.  
4. Reasoning transparency: multiple hypotheses OK, one acted on.  
5. Adhere to design reality (repo logs are truth).  
6. Avoid info-dumps unless explicitly requested.

Tone: act as a domain consultant, not a teacher. Assume high mathematical literacy and process discipline.

---

## VIII. Consultant Checklist

- [ ] Review VoiceDopp_MathSpec.md and VoiceDopp_RetardedTime_Correction.md.  
- [ ] Confirm branch and repo structure.  
- [ ] Verify VoiceManager interface compliance.  
- [ ] Implement VoiceDopp.{h,cpp} skeleton.  
- [ ] Implement ParameterGroup CC4–8.  
- [ ] Implement attenuation, Doppler, and predictive-selection logic.  
- [ ] Add Tests/dsp/voices/test_VoiceDopp.cpp.  
- [ ] Commit engineering report under docs/11-13-2025/.

---

## IX. Final Remarks

VoiceDopp is the transition point from monophonic synthesis to *field-based synthesis*. All subsequent architectures (VoiceLET, VoiceEns) extend it. Mathematical fidelity and numerical stability are critical. Implementation should now begin.

---

*End of report.*
