# Next-Engineer Orientation: Required Files & Why They Matter

This short document tells you **exactly which parts of the project you must inspect before attempting any Phase III or DSP‑mode work**, and **why each category is essential** for architectural correctness and stable continuation.

---

## 🔥 Core Governance Documents

### `docs/PRIME_DIRECTIVE.md`  
Defines the collaboration protocol between human ↔ LLM engineers.  
Without this, you will misunderstand the required reasoning discipline, update protocol, hypothesis handling, and the strict “single‑action” workflow.

### `docs/engineering_continuity_and_alignment.md`  
Describes how to maintain project coherence across sessions and contributors.  
Critical for avoiding architectural drift and for understanding the phase‑tag system (A1–A7, B1–B3).

### `docs/architecture.md`  
The top‑level system description (snapshot flow, VoiceManager boundaries, APVTS role, DSP subsystem).  
You need this to understand how *any* new voice mode should interact with the existing synth.

### `docs/voice_modes_roadmap.md` (new)  
Defines planned VoiceB / VoiceLET / VoiceDopp behavior.  
Prevents mis‑implementation and ensures Phase III follows the intended design.

---

## 🔥 Build System (CMake)

### `CMakeLists.txt` (root)  
### `CMakeLists.txt` (Tests)  
These show how files are compiled, which tests are included, what libraries are linked, and how JUCE is configured.  
Missing or misunderstood build rules often cause silent linkage errors or incomplete test coverage.

---

## 🔥 Parameter & Snapshot Infrastructure

### `Source/params/ParameterSnapshot.cpp` (if present)  
### `Source/params/ParameterIDs.h`  
### `Source/params/ParamLayout.cpp`  
### `Source/params/...`  
This subsystem defines every control value feeding into DSP.  
All new modes (VoiceB, VoiceLET, etc.) must obey snapshot rules and APVTS layout, or the plugin becomes unstable.

---

## 🔥 DSP Subsystem

You must read all voice‑layer and DSP components:

### `Source/dsp/VoiceLegacy*.*`  
Establishes the baseline sound and constraints—tests compare VoiceA behavior to this.

### `Source/dsp/MixNormalizer*.*`  
### `Source/dsp/PeakGuard*.*`  
### `Source/dsp/InstrumentRegistry*.*`  
### `Source/dsp/SmoothedValue*.*`  
### `Source/dsp/envelopes/*`  
### `Source/dsp/oscillators/*`  
### `Source/utilities` (if exists)  
These files define smoothing, gain behavior, envelopes, oscillators, and registration of voice types—ALL of which are required to safely extend the synth in Phase III.

---

## 🔥 Plugin UI

### `Source/plugin/PluginEditor.h`  
### `Source/plugin/PluginEditor.cpp`  
The UI binds parameters to user interaction.  
Mode‑switching work requires UI alignment; otherwise APVTS/GUI invariants break.

---

## 🔥 Resources

### `assets/*`  
If UI elements or diagrams are used, they must match the architectural docs and planned voice modes.

---

## 🔥 Tests

### `Tests/analyzer/*`  
Analyzer scripts validate RMS alignment, diagnostic logs, and non‑regression of DSP.

### `Tests/dsp/*`  
Low‑level tests ensuring envelopes, oscillators, and voices behave as expected.

### `Tests/params/*`  
Covers parameter existence, snapshot behavior, APVTS round‑trip, and mode plumbing.

These tests form the safety net that Phase III changes must not break.

---

## 🔥 Baseline DSP Reference

### `baseline/voice_output_reference.json`  
Defines the canonical behavior of VoiceA vs. legacy voices.  
Any DSP or mode work **must** preserve this unless explicitly updating benchmarks.

---

## 🔥 State Summary

### `report_summary.txt`  
Contains historical change logs, validation results, or recent analyzer summaries.

---

## 🔥 Full Project Tree

### `tree -I build -I .git`  
Gives the structural overview, ensuring no orphan files or missing directories.

---

## Final Note

**Inspecting all of these files is *mandatory* for anyone continuing engineering work.**  
They provide the architectural, behavioral, and procedural context required to:

- maintain Phase II invariants  
- extend into Phase III correctly  
- avoid regressions  
- and preserve the original intent of the plugin’s design.

You are now prepared to proceed safely.

