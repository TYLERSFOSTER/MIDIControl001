# S6 — Next-Engineer Onboarding (Unified, Current)

## 1. Purpose
This onboarding document brings a new engineer into the **current** architecture of MIDIControl001, reflecting **all work completed through the end of Phase III (Nov 17, 2025)**.

It supersedes older S6 versions and explains:
- the present structure of the plugin  
- what architectural constraints you must respect  
- what Phase III accomplished  
- what design invariants now exist  
- how tests, snapshots, and modes function  
- how to safely extend the system into Phase IV  

It is written so the next engineer can pick up *exactly where the system is now* with zero ambiguity.

---

## 2. Project Principles (Non‑Negotiable)

### 2.1 The Prime Directive
The entire development flow is governed by Tyler’s Prime Directive:
- Human → Directive → Model hierarchy  
- Zero hallucination  
- Never modify files without seeing current content  
- Minimal patches only  
- Block‑synchronous reasoning  
- No speculative rewrites  
- Diagnostics, logs, and tests override all narrative reasoning  

### 2.2 Deterministic DSP
The plugin must always behave deterministically under:
- sample‑accurate conditions  
- different hosts  
- different buffer sizes  
- automated parameter sweeps  
- the entire analyzer suite  

### 2.3 Safety Layer Invariants
The system must never produce:
- NaNs  
- blow‑ups  
- discontinuities  
- runaway RMS  
- DC  

The **VoiceManager + global RMS normalizer** enforce these guarantees.  
Any new voice type must not break them.

---

## 3. Current Architecture (Post‑Phase III)

### 3.1 High‑Level Overview

You are inheriting a **4‑layer architecture**:

1. **APVTS (Host Parameters)**  
2. **ParameterSnapshot (block‑frozen parameters)**  
3. **VoiceManager (voice allocation, CC persistence, mode scaffolding)**  
4. **Voices (BaseVoice + VoiceA + Dopp skeleton)**  

This is a clean, block‑synchronous DSP framework optimized for multiple future voice types.

---

## 3.2 Parameter Flow (Critical Insight)

Every block follows this chain:

```
APVTS → ParameterSnapshot → VoiceManager.startBlock() → Voices
```

This ensures:
- zero race conditions  
- perfect block consistency  
- human‑verifiable behavior  
- analyzable timing  
- deterministic unit tests  

You must **never** bypass this flow.

---

## 3.3 VoiceManager (The Heart of the System)

VoiceManager currently handles:

- **Voice allocation** (idle → active → releasing)  
- **Voice stealing** (quietest‑voice heuristic)  
- **Mode storage** (`mode_`, `lastMode_`)  
- **Mode-change detection + rebuild logic**  
- **Injectable voice factory**  
- **Persistent CC cache**  
- **Per-voice parameter updates**  
- **Global RMS stabilizer** (soft-normalized)  
- **Block‑synchronous orchestration**  

This is the “kernel” of the entire engine.

---

## 3.4 Snapshot Model

`ParameterSnapshot` carries:
- masterVolumeDb  
- masterMix  
- voiceMode (enum)  
- oscFreq  
- envAttack  
- envRelease  
- per‑voice params (`VoiceParams voices[NUM_VOICES]`)  

Snapshots are:
- allocated once per block  
- passed by reference  
- immutable during rendering  
- the only source of voice parameters  

---

## 3.5 Mode System (Phase III Complete)

The mode system now includes:

- `VoiceMode` enum  
- APVTS “Voice Mode” choice parameter  
- `toVoiceMode()` and `toInt()` utilities  
- VoiceManager storing `mode_` + `lastMode_`  
- Rebuild logic: switching modes rebuilds the voice array  
- makeVoiceForMode() scaffold (now supports VoiceA + Dopp skeleton)  

Currently **all modes behave as VoiceA** except for type identity seen in tests.  
But the scaffolding is fully functional.

---

## 3.6 Tests

There are 33 tests covering:

- envelopes  
- oscillators  
- BaseVoice interface  
- VoiceManager polyphony  
- CC behavior (VoiceA)  
- VoiceA ≡ Legacy equivalence  
- snapshot logic  
- parameter roundtrips  
- Dopp skeleton existence  
- mode wiring  
- analyzer integration  

All must pass after any change.

---

## 4. Engineering Culture of the Project

### 4.1 Absolute Rules

You must:

1. Always request current file content before editing.  
2. Produce minimal patches only.  
3. Never assume behavior not grounded in logs or tests.  
4. Reply with single-purpose actions unless explicitly allowed otherwise.  
5. Keep changes reversible and tagged.  
6. Maintain deterministic output.  
7. Maintain block-synchronous invariants.  

### 4.2 Forbidden Actions

You must never:

- rewrite files wholesale  
- introduce mid-block parameter reads  
- add new voice types without updating factory and snapshot logic  
- let voices depend on GUI or APVTS directly  
- break analyzer determinism  
- make multi-file changes without confirmation  

---

## 5. What Phase III Achieved

Phase III completed:

1. **Mode scaffolding**  
2. **Snapshot propagation of voiceMode**  
3. **Voice rebuild system (mode-aware)**  
4. **Injectable factory**  
5. **Persistent CC cache**  
6. **Mode wiring tests**  
7. **VoiceDopp skeleton structure**  
8. **Full system documentation (S3/S5/S6/S7)**  

The plugin is now structurally capable of:
- multiple physics-based voices  
- fully distinct DSP engines  
- mode-aware CC routing  
- block-synchronous mode transitions  

Phase III ended with:
- Full test suite passing  
- Zero sonic changes to VoiceA  
- VoiceDopp skeleton integrated  
- Architecture unified and stabilized  

---

## 6. What You Should Do Next (Entering Phase IV)

Your next tasks depend on what the human requests, but Phase IV generally includes:

### 6.1 Implement VoiceDopp (First Real New Voice Type)
- retarded-time Doppler math  
- velocity trajectory  
- frequency warp  
- phase alignment  
- Dopp-specific CC routing  
- Dopp-specific parameters  
- Dopp analyzer tests  

### 6.2 Implement Mode-Specific CC Maps  
General table:
```
mode → cc → handler
```

### 6.3 Implement Envelope and Oscillator Variants  
- EnvelopeDopp  
- EnvelopeLET  
- OscillatorLET  
- etc.

### 6.4 Strengthen Analyzer  
Add:
- physics invariants  
- per-mode RMS expectations  
- trajectory tracing  

---

## 7. Safety Checklist for Any Future Edit

Before touching ANY file:

### 7.1 Confirm Reality  
- Ask: “Show me `<file>` as it exists now.”  
- Validate content.  
- Only then propose a patch.

### 7.2 Confirm Patch Scope  
- Must be minimal  
- Must be fully reversible  
- Must be labeled (Phase IV A1, etc.)  

### 7.3 Build and Test  
- `cmake --build`  
- Run full test suite  
- Inspect analyzer logs  

### 7.4 Finalize  
- Commit  
- Tag  
- Summarize change to human  

---

## 8. Short Summary for Rapid Onboarding

A new engineer must internalize:
- block-synchronous snapshot architecture  
- mode-aware voice reconstruction  
- CC persistence layer  
- deterministic DSP invariants  
- VoiceManager as single orchestration point  
- BaseVoice contract  
- test suite as ground truth  

Once these are mastered, implementing new voice types is safe and straightforward.

---

## 9. Closing

This unified S6 document presents the exact state of the system at **Phase III completion (Nov 17, 2025)**.

It is binding.

It should be read in conjunction with:
- S3 (Formal Model)
- S5 (Phase III Design)
- S7 (Roadmap)

Welcome to MIDIControl001.  
Proceed with discipline.
