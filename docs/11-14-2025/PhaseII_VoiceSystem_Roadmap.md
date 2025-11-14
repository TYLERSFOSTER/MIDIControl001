# Phase II — Voice System Roadmap  
**Project:** MIDIControl001  
**Document:** PhaseII_VoiceSystem_Roadmap.md  
**Prepared For:** Next LLM Consultant Engineer  
**Date:** 2025-11-14

---

# 0. Purpose of This Document

This file provides a *complete engineering continuity map* for the next consultant stepping into **Phase II**, where the plugin transitions from a single-voice architecture (**VoiceA**) to a modular, pluggable multi‑voice architecture that supports:

- **Voice mode switching** (A, Dopp, LET, Ens, etc.)  
- **New voice classes** (`VoiceDopp`, `VoiceLET`)  
- **Configurable emitter-geometry and physics systems**  
- **GUI voice-mode selector**  
- **Updated APVTS layout**  
- **Incremental test-driven integration**

It also documents *why* each change is safe, where it plugs into the architecture, what tests validate the step, and what constraints are imposed by Phase‑I’s stability guarantees.

---

# 1. Architectural Baseline (Phase I Recap)

The MIDIControl001 plugin currently contains:

```
PluginProcessor  
  → VoiceManager  
      → VoiceA  
          → OscillatorA  
          → EnvelopeA
```

### 1.1 What We Know Is Stable
- VoiceA has complete test coverage (render, lifecycle, envelope, controller mapping).
- VoiceManager is stable and passes polyphony + voice stealing tests.
- APVTS parameter layout is mature.
- Analyzer diagnostic bridge is stable.
- GenericAudioProcessorEditor works for now.

**Goal:** Do not break *any* of this while adding modular voice architectures.

---

# 2. Phase II Goals

Phase II introduces:

## 2.1 Global Voice Mode
A new APVTS parameter:  
```
voice/mode : { voiceA, voiceDopp, voiceLET }
```

Used by:
- PluginProcessor: selects which voice factory the VoiceManager uses.
- GUI: drop‑down menu.
- Tests: ensures mode loads/saves.

## 2.2 Pluggable Voice Factory  
`InstrumentRegistry` becomes the dispatcher:

```
voiceMode == A    → make VoiceA()
voiceMode == Dopp → make VoiceDopp()
voiceMode == LET  → make VoiceLET()
```

## 2.3 VoiceDopp Integration (classical Doppler)  
Fully defined in `VoiceDopp_MathSpec.md`.  
Implements:
- Emitter lattice
- Retarded‑time evaluation
- Predictive emitter selection
- Classical Doppler frequency law

## 2.4 VoiceLET (relativistic Doppler)
Extension of VoiceDopp:
- Uses relativistic Doppler shift
- Optional Lorentz‑scaled envelope timing

---

# 3. Phase II Step Breakdown (A → D → B → C)

This is the exact sequence you approved.

---

# **STEP A — Add Global Voice Mode Parameter**

### A.1 Modify ParameterIDs.h  
Add:
```
voiceMode = "voice/mode"
```

### A.2 Modify ParamLayout.cpp  
Add a new `AudioParameterChoice`.

### A.3 Update PluginProcessor::makeSnapshotFromParams  
Add new field:
```
snapshot.voiceMode
```

### A.4 Constraints
- MUST NOT break existing tests.
- Default must be “voiceA”.
- No DSP changes.

### A.5 Tests that Validate Step A
- `test_params.cpp` — parameter existence  
- `test_processor_smoke.cpp` — APVTS default load  
- `test_processor_state` — state save/restore  
- `test_Snapshot.cpp` — new Snapshot fields stable  

**No DSP tests should change**.

---

# **STEP D — Update VoiceManager to Support Mode Switching**

VoiceManager is currently:

```
VoiceManager(std::function<ParameterSnapshot()>)
```

Phase II upgrade:

```
void VoiceManager::setMode(VoiceMode mode);
```

VoiceManager internally:
- clears voices on mode switch
- repopulates pool using InstrumentRegistry::makeVoice
- preserves independence of existing voices until they finish (option B)

### D.1 Why Step D safely follows A
- `voiceMode` now exists in APVTS
- Snapshot includes mode
- No new voice types yet → zero runtime impact
- Tests for VoiceManager do not reference voice types

### D.2 Tests that Validate Step D
- `test_VoiceManager.cpp` (polyphony + envelope decay)
- `test_BaseVoice_interface.cpp`
- `test_processor_integration.cpp` (after adding a single line to set mode)

None of these tests assume VoiceA specifically.

---

# **STEP B — Register New Voice Types Into Registry**

`InstrumentRegistry.h` becomes:

```
makeVoice("voiceA")   → new VoiceA
makeVoice("voiceDopp")→ new VoiceDopp
makeVoice("voiceLET") → new VoiceLET
```

### B.1 Why Step B Occurs After A & D

- A introduces mode in APVTS.
- D allows mode switching.
- B adds new types but does **not activate** them yet until the GUI or user sets voiceMode.

### B.2 Test Safety
- No existing test instantiates voiceDopp or voiceLET.
- Registry test only checks VoiceA.
- No breakage until rendering begins.

---

# **STEP C — GUI Integration (Standalone Dropdown)**

Modify `PluginEditor.cpp`:

Add:
```
ComboBox voiceModeSelector;
```

This selector reads/writes APVTS param "voice/mode".

### C.1 Why GUI changes last
- GUI has no tests.
- APVTS parameter must exist first (Step A).
- VoiceManager must handle switching (Step D).
- Registry must have voice classes (Step B).
- GUI then becomes a minimal patch.

---

# 4. How the New Layers Fit Together

```
[APVTS] ← voice/mode (choice param)
      ↓
[PluginProcessor]
      ↓ passes snapshot.voiceMode each block
[VoiceManager]
      ↳ if (mode changed) → rebuild voice pool
      ↳ render using selected voice class
[InstrumentRegistry]
      ↳ makeVoice("voiceA")
      ↳ makeVoice("voiceDopp")
      ↳ makeVoice("voiceLET")
```

VoiceDopp and VoiceLET are “drop-in” because:

- They subclass BaseVoice.
- They implement identical method signatures.
- VoiceManager does not know or care what physics a voice uses.

That’s the entire reason the plugin can scale from a mono synth to a relativistic Doppler field engine without changing the PluginProcessor or APVTS core.

---

# 5. Test Matrix Across Steps A → D → B → C

| Step | Category | Expected Tests | Risk |
|------|----------|----------------|-------|
| **A** Add voice/mode param | APVTS | test_params, test_Snapshot | LOW |
| **D** VoiceManager mode switching | DSP / Manager | test_VoiceManager, test_processor_integration | LOW |
| **B** Registry adds VoiceDopp/LET | Registry | test_InstrumentRegistry | LOW |
| **C** GUI Dropdown | None | Manual only | ZERO |

No test in the suite introspects oscillator internals or voice classes beyond `VoiceA` basics.

---

# 6. VoiceDopp Implementation Guidance

### Core Components

| Component | Purpose |
|----------|----------|
| `VoiceDopp` | Per-note engine with emitter lattice |
| `OscillatorDopp` | Frequency = classical Doppler law |
| `EnvelopeDopp` | EnvelopeA reused; field pulse added |
| `Emitter struct` | position, base frequency, phase |

### Predictive Selection
- Emitter pool infinite → select top‑K using hysteresis + scoring.
- K is script variable (compile-time constant or APVTS param).

### Retarded-Time Evaluation
For each emitter:
```
t_ret = t - r(t)/c
```
Evaluate:
- carrier
- ADSR
- field pulse amplitude

---

# 7. Why the Plugin Architecture Supports This Expansion

## 7.1 BaseVoice contract is stable and minimal  
All voice types implement only:

```
prepare(sr)
noteOn(snapshot,noteNumber,velocity)
noteOff()
render(buffer,N)
isActive()
getCurrentLevel()
```

This allows arbitrary physics.

## 7.2 VoiceManager decouples DSP from PluginProcessor
VoiceManager handles:
- polyphony
- CC forwarding
- mono mixdown
- stealing
- lifecycle

The DSP layer never touches APVTS.

## 7.3 APVTS snapshots isolate GUI from DSP  
Snapshot is a pure data bag extracted once per block.

---

# 8. Future Consultant Onboarding

To continue where this document leaves off:

1. Implement Step A patch (add voice/mode param).  
2. Implement Step D (VoiceManager supports it).  
3. Implement Step B (register new classes).  
4. Implement Step C (GUI dropdown).  
5. Begin VoiceDopp scaffold (`VoiceDopp.h`/`.cpp`).  
6. Add tests for Doppler correctness (FFT-based or instantaneous frequency checks).

This roadmap ensures Phase II is **incremental, test-safe, and architecture-consistent**.

---

# 9. End of Document
