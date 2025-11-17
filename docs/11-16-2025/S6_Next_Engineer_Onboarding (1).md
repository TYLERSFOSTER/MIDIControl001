# S6 — Next-Engineer Onboarding (Full Version)

## 1. Purpose of This Document
This onboarding document is designed to bring a new engineer—LLM or human—into the MIDIControl001 codebase with total clarity, minimal ramp-up time, and full continuity with decisions made across Phase I, Phase II, and Phase III (in progress). The goal is to ensure zero ambiguity about architecture, intentions, constraints, Prime Directive rules, file structure, DSP flow, and engineering culture of the project.

This is not a high-level summary.  
This is the **definitive onboarding resource** explaining:

- how the plugin works now  
- why it works that way  
- how the system evolved  
- what architectural constraints are non‑negotiable  
- how new voice types must be integrated  
- how the ParameterSnapshot/APVTS/VoiceManager pipeline works  
- how to avoid catastrophic regression  
- what Phase III intends to accomplish  
- how Phase IV and V will build atop it  

You (the incoming engineer) must treat this document as binding.

---

## 2. Core Principles of the Project

### 2.1 The Prime Directive
The entire engineering workflow is governed by the **Prime Directive**, which establishes:

- Human → Directive → Model hierarchy  
- No hallucinatory edits  
- Every modification must reference **actual file context**  
- Every design action requires reality-grounding  
- Diagnostics, logs, and tests are absolute truth  
- Patches must be minimal, reversible, and step-labeled  
- No multi-step reasoning unless explicitly requested  
- No intrusive refactors without human approval  
- No speculative rewrites  
- Backup discipline, branch discipline, and merge hygiene  

Violating this breaks collaboration.  
You are required to operate within these rules.

### 2.2 Determinism and Repeatability
All DSP behavior must be deterministic across:

- Test suite runs  
- Different machines  
- plugin hosts  
- DAWs  
- Parameter automation conditions  

This is a foundational requirement.

### 2.3 Safety Against Audio Catastrophe
The plugin must *never* emit:

- DC spikes  
- NaN propagation  
- unbounded RMS  
- discontinuities  
- clicks from mode-switching  
- buffer out-of-range reads  
- uncontrolled polyphony bursts  

VoiceManager and MixNormalizer enforce safety guarantees.  
Any new voice type must honor these invariants.

---

## 3. The Current Architecture (What You’re Inheriting)

### 3.1 The Four-Layer Structure

**Layer 1 — Host Parameters (APVTS)**  
All front-facing parameters live in ParamLayout.cpp and ParameterIDs.h.

**Layer 2 — ParameterSnapshot (per-block freeze)**  
During each audio block, the PluginProcessor builds a full immutable snapshot.

**Layer 3 — VoiceManager**  
Holds:
- global voice array  
- per-block snapshot propagation  
- smoothing  
- CC persistence  
- mode-aware voice construction (Phase III scaffolding)  
- clickless polyphony gain  

**Layer 4 — BaseVoice and VoiceA**  
BaseVoice defines the interface.  
VoiceA is the canonical implementation.

### 3.2 The Parameter Flow

```
APVTS → ParameterSnapshot (block freeze) → VoiceManager → Voices
```

This ensures:

- no mid-block parameter race conditions  
- perfectly consistent block processing  
- test determinism  
- reproducibility of diagnostics  

### 3.3 Audio Flow

```
Voices → block summation → MixNormalizer/PeakGuard (future) → output buffer
```

### 3.4 VoiceManager Responsibilities

- manage up to 32 voices  
- allocate, steal, and free voices  
- store global mode state  
- detect mode changes (Phase III scaffolding)  
- rebuild voices upon mode shift  
- propagate snapshots  
- smooth poly gain  
- dispatch CC  
- handle per-voice updates  
- apply persistent CC cache  

### 3.5 Invariants You Must Preserve
**You may never violate these:**

1. `startBlock()` must always be called exactly once per block.  
2. All voices must share the same snapshot pointer during a block.  
3. Voices must never allocate memory during audio rendering.  
4. All smoothing must be inside Voice or VoiceManager—not in Processor.  
5. No DSP must depend on GUI-thread timing.  
6. APVTS must remain the single source of truth for user parameters.  
7. No parameter updates may occur mid-block except CC message paths.  
8. VoiceManager must stay the *only* voice allocator.  

---

## 4. Projects Rules You Must Respect (Non-Negotiable)

### 4.1 File Access Rules
Before modifying any file you must:

1. Ask for the current version.  
2. Confirm it matches your expectations.  
3. Perform a minimal diff-based patch.  

No skipping steps.

### 4.2 Git Discipline
Every change must be:

- branch‑isolated  
- commit‑atomic  
- tagged  
- reversible  

Never push directly to main.  
All Phase III work must occur under `phase3-*` branches.

### 4.3 Logging and Diagnostics
All debug logging must:

- follow the established pattern  
- remain minimal in DSP paths  
- avoid allocations  
- support test analyzers  

### 4.4 Test Suite Continuity

30+ tests enforce:

- RMS correctness  
- voice stealing determinism  
- CC mappings  
- phase-reset invariants  
- baseline-vs-VoiceA equivalence  

Any failure = stop immediately.

---

## 5. Understanding Phase III (Where You Are Entering)

Phase III introduces **voice modes** and multiple voice types.

BUT:  
Phase III must introduce **zero behavioral change** until a new voice type is actually implemented.

### 5.1 What Phase III Adds
- VoiceMode enum  
- APVTS parameter “voiceMode”  
- toVoiceMode() / toInt()  
- Processor → Snapshot → VoiceManager propagation  
- Mode change detection  
- Voice rebuild upon mode shift  
- Injectable VoiceFactory  
- applyModeConfiguration() hook  
- Fully inert scaffolding verified by test suite  

### 5.2 Why This Layer Exists
Future voice types include:

- **VoiceDopp** (classical Doppler)  
- **VoiceLET** (relativistic Doppler / LET)  
- **VoiceFM** (FM synthesis voice)  
- **VoiceB / VoiceC variants**  

These require:

- different oscillators  
- different envelopes  
- different per-voice parameters  
- possibly different state variables  
- mode-specific initialization  

Phase III sets the foundation without touching VoiceA behavior.

### 5.3 How to Extend the System
To add a new voice type:

1. Create new Voice class (e.g., VoiceDopp).  
2. Implement BaseVoice interface.  
3. Add constructor path in makeVoiceForMode().  
4. Add APVTS parameters if needed.  
5. Extend snapshot + per-voice param mapping.  
6. Extend mode enum and ParamLayout list.  
7. Extend tests to enforce deterministic behavior.  

---

## 6. The Correct Mental Model (Very Important)

You must think of the plugin as:

- **A block-synchronous DSP system**  
- **All user parameters converted into a pure functional snapshot**  
- **Voices as state machines driven only by snapshots and MIDI**  
- **VoiceManager as the orchestrator and safety layer**  

If you break the data‑flow model, you break deterministic tests and phase correctness.

Your responsibility is to maintain the purity and clarity of this flow.

---

## 7. What the Next Engineer Must Absolutely Avoid

### 7.1 Anti-Patterns
Never:

- change per-block behavior inside Processor  
- let a voice read parameters directly from APVTS  
- add global mutable state outside VoiceManager  
- introduce mid-block allocations  
- merge modes with conditional DSP deep inside Voices  
- add parameters without snapshot integration  
- modify file content without grounding  
- break console/log determinism  

### 7.2 Dangerous Areas of Code
Avoid careless edits in:

- VoiceManager::startBlock()  
- VoiceManager::render()  
- Snapshot builder  
- Voice allocation loops  
- CC dispatch logic  
- SmoothedValue smoothing boundaries  

Missteps here tend to cascade unpredictably.

---

## 8. The Immediate Work You Should Prepare For (Entering Phase III)

As the incoming engineer, your next responsibilities will be:

### 8.1 Implementing VoiceDopp
- retarded-time Doppler model  
- moving listener, stationary source  
- frequency warp  
- phase correction  
- velocity vector dot product with sound propagation  

This will require new state variables.

### 8.2 Implementing VoiceLET
- relativistic Doppler  
- Lorentz-factor γ  
- beta = v/c  
- proper-time parametrization  
- invariance constraints  
- frequency/phase warp formula  

### 8.3 Updating ParameterSnapshot
Each new voice type will introduce:

- new per-voice params  
- new global params  
- new APVTS entries  

### 8.4 Strengthening Tests
Phase III test additions must include:

- mode switching determinism  
- mode-specific voice creation  
- equivalence tests for VoiceA (baseline)  
- stress tests for polyphony under mode-switching  

---

## 9. How to Work Safely on This System

### 9.1 Always Begin With:
```
Show me <file> exactly as it exists now.
```

### 9.2 Never Produce:
- full rewrites  
- speculative patches  
- changes touching multiple files without confirmation  

### 9.3 Always End With:
- run all tests  
- confirm output  
- confirm RMS stability  
- check for clicks  
- re-run analyzer  

### 9.4 Tag every successful checkpoint.

---

## 10. Summary Checklist (Memorize This)

### For every modification:

- [ ] Request file  
- [ ] Confirm content  
- [ ] Produce minimal patch  
- [ ] Justify patch  
- [ ] Insert step tag (Phase III Bx or Cx)  
- [ ] Rebuild  
- [ ] Run full test suite  
- [ ] Inspect diagnostics  
- [ ] Tag and push  

This checklist is mandatory.

---

## 11. Conclusion

If you follow this onboarding guide:

- you will never regress the system  
- you will maintain the safety guarantees  
- you will understand the plugin's architectural logic  
- you will be able to implement new voice types with confidence  
- you will operate in full compliance with the Prime Directive  
- you will maintain test determinism and DSP stability  

If you ignore this guide, you will break the plugin.

This document is binding.  
Welcome to MIDIControl001, Phase III.
