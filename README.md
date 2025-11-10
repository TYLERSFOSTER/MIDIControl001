<p align="left">
  <picture>
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: dark)">
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: light)">
    <img src="assets/MPK_Mini.jpg" alt="tonnetzB" width="400">
  </picture>
</p>

# MIDI Control #1
MIDI synth voice plugin using the JUCE C++ framework

See the full design document in [architecture.md](docs/architecture.md).

## Next engineering steps

| Step                          | Goal                                                                                                    | Files likely touched                          |
| ----------------------------- | ------------------------------------------------------------------------------------------------------- | --------------------------------------------- |
| **1. Parameter grouping**     | Move these params under a `voiceA_` group in `ParamLayout.cpp` and `ParameterIDs.h`.                    | `Source/params/*`                             |
| **2. Snapshot extension**     | Add `snapshot.voiceA.oscFreq / envAttack / envRelease` fields and copy from APVTS group.                | `ParameterSnapshot.h` & `PluginProcessor.cpp` |
| **3. VoiceManager injection** | Ensure `VoiceManager::startBlock()` and `VoiceA::noteOn()` consume those snapshot values appropriately. | `VoiceManager.h`, `VoiceA.h`                  |
~

## Verification Summary — November 2, 2025  
**Branch:** `(HEAD detached at bc51ced)`  
**Consultant Phase:** Variant A alignment (VoiceManager constructor + snapshot model)

---

### ✅ Build & Compilation
- All production and test targets compile cleanly under CMake 3.29 / JUCE v7.0.9.
- `VoiceManager` constructor and `startBlock()` signatures fully aligned between code and tests.
- `MIDIControl001_tests` target links successfully; no unresolved symbols or ownership mismatches.

---

### ✅ Runtime Test Results
| Subsystem | Result | Notes |
|------------|---------|-------|
| `VoiceManager` basic polyphony | **Passed** | Confirms snapshot injection, note-on scheduling, and per-block updates operate correctly. |
| `VoiceManager` voice stealing | **Passed** | Confirms proper voice-steal selection and render path. |
| Plugin processor integration tests | **Passed** | Confirms APVTS → Snapshot → VoiceManager → Render pipeline stable. |
| Envelope / oscillator unit tests | **Passed** | DSP primitives verified individually. |
| JSON baseline writer (`writeJson`) | **Passed** | `Tests/baseline/voice_output_reference.json` successfully written during test run. |

---

### ⚠️ Known Outstanding Issues (pre-existing)
| Test | Symptom | Likely Source |
|------|----------|----------------|
| `Voice basic lifecycle` | `rms` and `peak` approximately 2× baseline; hash differs. | Amplitude normalization or envelope scaling inside `VoiceA`. |
| `Voice scalecheck lifecycle` | Minor RMS/peak deviation (<10%). | Same class of amplitude-scaling mismatch. |

All other 21 / 23 tests passed.  
`VoiceManager` and integration layers verified non-regressive relative to prior baselines.

---

### ✅ Architectural Validation
- **Constructor semantics:** dependency-injected `SnapshotMaker` ensures deterministic parameter source; prevents undefined state creation.  
- **Block snapshot model:** single per-block pull of parameters (`startBlock()`) guarantees block-coherent DSP updates without heap churn.  
- **Ownership:** `VoiceManager` held as value member inside processor; lifetime equals processor; teardown via `releaseResources()` only.  

---

### 📦 Current State
The system is stable for further DSP calibration work.  
Next focus should be confined to `VoiceA` (gain/envelope normalization and baseline recomputation).

---

**Verification completed:** 2025-11-02  
**Engineer:** Tyler Foster  
**Consultant Phase:** Variant A stabilization — *“Snapshot-per-block architecture verified.”*
