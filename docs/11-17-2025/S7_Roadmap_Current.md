# S7 — Phase III → Phase IV Roadmap (Unified, Current)

## 0. Status Summary

You are entering the system **after full completion of Phase III**, including:
- Mode scaffolding (VoiceMode enum, APVTS choice param)
- Snapshot → VoiceManager → voice pipeline finalized
- Mode-change detection (rebuildVoicesIfModeChanged)
- VoiceDopp skeleton integrated
- CC persistence system implemented
- Full test suite passing and validated
- Architecture fully documented

The system is now ready for **Phase IV: true multi‑mode DSP**.

This document explains where we are now, what Phase IV requires, and how future engineers must proceed to avoid breaking invariants.

---

## 1. Current Architecture Capabilities

### 1.1 Stable Mode Layer
- VoiceMode enumeration exists and is propagated block‑synchronously.
- VoiceManager honors mode changes safely (block boundaries only).
- Voice rebuilding is deterministic and safe for audio thread.

### 1.2 VoiceManager Rebuild Pipeline
- `setMode()` stores user intent.
- `startBlock()` detects changes → rebuilds voices.
- `makeVoiceForMode()` chooses implementation.
- Supports injected factory for testing/new modes.

### 1.3 CC Persistence & Parameter Coherence
- CC3/4/5 cached → reapplied every block.
- Ensures stable behavior across note boundaries.
- Tests confirm no drift.

### 1.4 VoiceDopp Skeleton
- Exists as a valid BaseVoice subtype.
- All unit tests confirming structural correctness.
- No DSP yet — ready for Phase IV physics.

---

## 2. Phase IV — The Real Work

Phase IV transforms the plugin from “multi-mode ready” into **multi-mode actual**.

### 2.1 Deliverables of Phase IV
- Fully implemented **VoiceDopp** (classical Doppler)
- Implement **VoiceLET** (relativistic Doppler)
- Implement **VoiceFM** (2‑op FM synthesis)
- Add mode‑specific CC routing tables
- Add per-mode parameter surfaces
- Add per-mode analyzers
- Add tests for each mode
- Expand GUI if needed

This is the first phase where sonic output will *change* based on mode.

---

## 3. Roadmap Track A — New Voice Types

### A1 — VoiceDopp (Classical Doppler, full implementation)
**Goal:** real physical Doppler model.

Requirements:
- Retarded-time approximation
- Moving listener coordinate update per block
- Frequency warp `f' = f * c/(c ± v)`
- Proper handling of phase accumulation
- Velocity smoothing
- CC bindings:
  - CC3 = radial velocity
  - CC4 = acceleration
  - CC5 = distance-mix curve

Tests:
- Frequency shift monotonic in velocity
- Phase continuous across blocks
- No aliasing at high v

---

### A2 — VoiceLET (Relativistic Doppler)
**Goal:** include special relativity.

Model:
- β = v/c
- γ = 1/sqrt(1−β²)
- Relativistic Doppler formula:
  `f' = f * sqrt((1 - β)/(1 + β))`

New parameters:
- dilation factor
- proper-time envelope scaling

Tests:
- γ monotonic in β
- β < 1 clamp enforcement
- smooth transitions

---

### A3 — VoiceFM (Two-Operator FM)
**Goal:** classical FM voice.

Model:
- carrier oscillator
- modulator oscillator
- mod index curve
- modulation depth smoothing

Tests:
- harmonicity for integer ratios
- Bessel-based spectrum correctness
- continuity under CC modulation

---

## 4. Roadmap Track B — Parameter Surfaces & CC Routing

### B1 — Mode-Specific Parameter Surfaces
Each mode has:
- Different defaults
- Different smoothing times
- Different ranges
- Different semantics

Action:
- Create ModeParamSurface registry
- Add mapping in ParameterSnapshot
- Update ParamLayout as needed

---

### B2 — CC Routing Tables
Global `handleController()` dispatch should become:

```
CCRoutings[activeMode][cc](voice, norm);
```

Tests must ensure:
- No leakage between modes
- No undefined CC access
- Deterministic handling per block

---

## 5. Roadmap Track C — DSP Pipeline Unification

### C1 — Common Interfaces
Introduce:

```
OscillatorInterface
EnvelopeInterface
```

Then per-voice implementations:
- OscillatorA, OscillatorDopp, OscillatorLET…
- EnvelopeA, EnvelopeLET, EnvelopeRel…

### C2 — Pre-Block Integration Step
Future voices require precomputations:
- trajectory solve
- dilation bake
- FM index envelope generation

This must occur in `prepare()` or `startBlock()`, not `render()`.

---

## 6. Roadmap Track D — Analyzer Expansion

Add Analyzer 2.0:
- Mode-specific RMS expectations
- Physics invariants (e.g. Doppler monotonicity)
- FM spectral signature checks
- Proper-time envelope continuity tests

---

## 7. Roadmap Track E — Phase IV Completion Criteria

Phase IV is complete when:

- [ ] VoiceA, VoiceDopp, VoiceLET, VoiceFM all produce stable audio  
- [ ] CC routing is mode-specific and verified  
- [ ] Analyzer 2.0 detects physics violations  
- [ ] Tests cover mode switching, allocation, DSP correctness  
- [ ] GUI reflects mode-specific controls if needed  
- [ ] Baseline VoiceA remains bit‑identical to legacy tests  

Only then do we move to Phase V (cross‑mode morphing, physics blending, multi‑emitter geometry).

---

## 8. Appendix — Glossary

**Mode Surface** — set of parameter ranges & semantics per mode  
**Physics Invariant** — mathematical constraint enforced in DSP  
**CCRoutings** — mode-indexed array of CC handlers  
**Retarded-time** — past light-cone solution for classical Doppler  
**β** — v/c, normalized velocity  
**γ** — Lorentz factor  

---

# End of S7 — Unified Roadmap
