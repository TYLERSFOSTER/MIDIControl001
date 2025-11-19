# S5 — Phase III Design, Intent, Risks, and Downstream Consequences (Unified, Current)

This document reflects the **current, real architecture** of MIDIControl001 as of the newly completed Phase III checkpoint (commit `4305b74`, tag `v0.3-phase3-complete`).  
It supersedes all earlier S5 documents and aligns with the newly‑generated **S3 Formal Model (Unified)**.

---

# 1. What Phase III Actually Achieved (Ground‑Truth)

Phase III did *not* introduce any new audible behavior.  
It introduced **architecture only**, but *correct*, *safe*, and *clean* architecture that makes multiple voice engines possible.

The real, completed Phase III accomplishments:

### 1.1 Installed a real `VoiceMode` system
- `voiceMode` is an APVTS parameter.
- `ParameterSnapshot` captures the current mode every block.
- `PluginProcessor` passes the mode cleanly into VoiceManager.
- `VoiceManager` stores:  
  `mode_`, `lastMode_`, and rebuilds voices when these differ.

### 1.2 Introduced a deterministic, block‑synchronous voice rebuild model
Rebuilds occur *only* in `startBlock()`, never mid‑render.

This gives:
- No torn arrays
- No cross‑thread races
- Perfectly predictable mode transitions

### 1.3 Phase III added a **mode‑aware voice factory**
`makeVoiceForMode()` now routes by mode (currently returns VoiceA for all modes, but the function is real and used).

### 1.4 Added a **fully integrated VoiceDopp skeleton**
- Header + cpp compile
- Included in factory
- Pure scaffold (no DSP yet)
- Tests confirm existence and correct class wiring

### 1.5 Integrated **persistent CC cache** with correct ordering
- CC3 → envAttack exponential map  
- CC4 → envRelease exponential map  
- CC5 → oscFreq pitch sweep  
- Re-applied every block  
- Tests confirm *no regression in VoiceA behavior*

### 1.6 Added the **mode wiring tests**
These guarantee:
- Mode → snapshot → VoiceManager → voice array plumbing works
- Rebuild triggers when mode changes
- Rebuild does *not* trigger otherwise

### 1.7 All 33 tests passed after Phase III
This confirms:
- No DSP changes
- No regressions
- No mis-wiring
- No CC double-application
- No snapshot inconsistencies

You now have a **real multi‑voice architecture** with no sonic drift.

---

# 2. Purpose of Phase III (Structural, Not Sonic)

Phase III’s purpose was:

## 2.1 Establish a *mode-capable* engine
One plugin → many possible physical models:
- VoiceA (baseline)
- VoiceDopp (classical Doppler)
- VoiceLET (relativistic Doppler)
- VoiceFM (FM voice)
- any future voice types

## 2.2 Ensure mode changes are safe
Mode changes must:
- never occur mid-render  
- never corrupt internal state  
- never produce UB  
- never interact with CC pipelines incorrectly  
- be testable in isolation

Phase III produced this infrastructure.

## 2.3 Ensure deterministic behavior under automation
Hosts may automate voiceMode.  
You must be able to withstand this safely.

## 2.4 Localize all mode-specific logic to a single architectural surface
All mode branching is now in:

- ParamLayout (host-facing names)
- ParameterSnapshot (block freeze)
- VoiceManager::setMode / startBlock
- VoiceManager::makeVoiceForMode
- VoiceManager::rebuildVoices*()

Everything else is mode-agnostic.

This is what makes Phase IV possible.

---

# 3. Design Surfaces Introduced in Phase III

This is the authoritative list of **“the places where Phase III code lives.”**

---

## 3.1 APVTS: user-facing mode selection
File: `ParamLayout.cpp`

- Adds `AudioParameterChoice voiceMode`
- Order determines enum mapping
- Currently: `VoiceA`, `VoiceDopp`, `VoiceLET`, `VoiceFM`

Even though only VoiceA is real, these entries set the future surface.

---

## 3.2 ParameterSnapshot: block-level mode capture
File: `ParameterSnapshot.h`

Snapshot now contains:

```cpp
VoiceMode voiceMode;
```

Every block:
- Processor converts APVTS → enum
- Snapshot captures it
- VoiceManager sees it
- No voice can see host parameters directly

This isolates the DSP from the GUI/host.

---

## 3.3 PluginProcessor: APVTS → Snapshot → VoiceManager
This is the only place where “mode from UI” enters the DSP layer.

This step is critical because it guarantees:
- No mid-block UI reads
- No parameter drift
- No thread-safety violations

---

## 3.4 VoiceManager: the new architecture’s core

### VoiceManager now:
- stores mode
- tracks lastMode
- detects changes
- rebuilds voices on mode change
- exposes a voice factory
- cleansly updates per-block behavior

This is the new backbone of the plugin.

---

## 3.5 VoiceFactory + makeVoiceForMode
`makeVoiceForMode(VoiceMode)` is the primary extension point for Phase IV.

Right now:

```cpp
case VoiceMode::VoiceA: return VoiceA
case VoiceMode::VoiceDopp: return VoiceDopp (skeleton)
default: return VoiceA
```

Later, each case will route to distinct DSP engines.

---

## 3.6 CC persistence (Phase 5-C.4 integrated)
The CC cache is now a permanent architectural layer.

- Correct exponential/log maps
- Per-block propagation
- Continuous behavior under voice rebuilds
- Verified in tests

This ensures real expressive performance on MPK Mini and other controllers.

---

# 4. Risks and Mitigations (Realistic)

Phase III introduces structural risk.  
This section documents them.

---

## Risk 1 — Mode drift
If snapshot.mode != VoiceManager.mode_, behavior becomes undefined.

### Mitigation:
The Phase III pipeline ensures:
- snapshot built first
- mode_ set before rebuild
- rebuild happens in same block
- tests assert determinism

---

## Risk 2 — Stale casts when new voices are added
Future voice types break casting assumptions:

```cpp
dynamic_cast<VoiceA*>(voice)
```

### Mitigation:
VoiceManager now uses a dedicated mode-aware section to update voice parameters.

Future work (Phase IV) will move parameter updates into virtual methods.

---

## Risk 3 — Audio discontinuity during mode change
Mode switching clears voices → silence → new voices start.

This is acceptable for Phase III, because:
- switching physical models mid-note is fundamentally discontinuous
- Phase IV will explore xfade solutions

---

## Risk 4 — Long-term “mode sprawl”
More modes = more complexity.

Phase III centralizes this complexity.

---

## Risk 5 — CC routing divergence
Future voice types may require different CC mappings.

Phase III prepares for this by cleanly separating:
- CC cache
- CC propagation
- per-voice CC handlers

Phase IV will introduce mode-specific routing tables.

---

# 5. Downstream Consequences (What Phase III Enables)

This is the real value of Phase III.

---

## 5.1 Enable physics-based voices
You can now safely implement:

- VoiceDopp (classical Doppler)
- VoiceLET (relativistic Doppler)
- VoiceRel (light-cone emission)
- VoiceFM (Bessel-index modulation)

None of these were architecturally safe before Phase III.

---

## 5.2 Analyzer 2.0
The analyzer can now:
- test mode wiring
- test physical invariants
- benchmark Doppler correctness
- detect mode-specific drift

---

## 5.3 Mode-aware CC maps
Each voice type can have its own CC table.

---

## 5.4 Expanded envelope and oscillator interfaces
Phase III scaffolding is the required precursor to:
- OscillatorLET  
- EnvelopeLET  
- FM-carrier/modulator separation  
- physics-derived envelopes  

---

# 6. What Phase IV Will Build (Brief Preview)

Phase IV is where actual DSP enters the architecture:

### Order of operations (high-level):
1. Implement VoiceDopp DSP  
2. Mode-specific CC routing  
3. Mode-specific per-block pre‑integration  
4. Mode-specific parameter surfaces  
5. New oscillators + envelopes  
6. Analyzer 2.0  
7. LET & Relativistic models  
8. FM voice  

The structure needed for all of this is now complete.

---

# 7. Summary (One Page)

### The plugin is now a **true multi‑voice engine**  
but with **zero sonic change** relative to the baseline.

### Phase III completed:
- Mode parameter  
- Snapshot integration  
- VoiceManager mode wiring  
- Safe rebuild pipeline  
- Voice factory  
- Persistent CC cache  
- VoiceDopp skeleton  
- Tests verifying all behavior  

### Phase III did not do:
- new DSP  
- new envelopes  
- LET/Dopp/FM logic  
- parameter surfaces  
- CC map split  
- analyzer expansion  

### Phase III enables:
**Everything Phase IV needs to introduce new physics safely.**

This document is the authoritative reference for all design consequences of Phase III.

