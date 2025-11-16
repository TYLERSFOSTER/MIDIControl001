
# Phase III — Engineering Continuity and Alignment Report  
**Project:** MIDIControl001  
**Prepared for:** Tyler Foster  
**Prepared by:** ChatGPT (Embedded Engineering Consultant)  
**Date:** Phase III Kickoff (Post–Phase II A7 Freeze)

---

# I. PURPOSE OF THIS REPORT

This **Phase III Engineering Continuity Report** is designed to:

1. Give *the next LLM consultant* (or any human developer) a **complete, high‑bandwidth understanding** of the system state after **Phase II A7 — Stable voiceMode plumbing**.
2. Establish a **stable operational baseline** for future work (VoiceLET, VoiceB, VoiceDopp, multi‑mode engines).
3. Document **architectural invariants**, **engineering principles**, **failure history**, and **rules for safe collaboration**.
4. Provide a **Phase III roadmap** with bounded surface area, tightly scoped stages, and clear success criteria.
5. Ensure **Prime‑Directive compliance**, **reasoning discipline**, and **single‑action development** as the project evolves.

This document is intentionally exhaustive:  
It is meant to replace “tribal knowledge” with **explicit, stable engineering state**.

---

# II. CONTEXT: WHERE WE ARE IN THE PROJECT

## 1. What Phase II Achieved (Real Achievements, Not Hallucinated)

Phase II produced **tangible, correct, test‑verified architecture**:

### ✔ Added global `voiceMode` parameter to APVTS  
### ✔ Added snapshot wiring (`ParameterSnapshot.voiceMode`)  
### ✔ Processor now forwards snapshot mode → `voiceManager_.setMode()`  
### ✔ `VoiceManager` stores an internal `mode_`  
### ✔ Tests verify snapshot updates + safe forwarding  
### ✔ Build is stable, 30/30 tests passing  
### ✔ Phase II A7 tagged cleanly in Git (`phase2-A7-voiceMode-stable`)

Nothing in Phase II changed DSP behavior.  
All changes were **architectural plumbing**, not sonic.

---

## 2. What Phase II DID NOT Attempt (Correctly)

Phase II did *not* attempt:

- Adding VoiceB, VoiceLET, or VoiceDopp
- Changing voice allocation logic
- Introducing multithreading  
- Changing parameter ranges  
- Modifying the VoiceA DSP path  
- Adding UI components  
- Adding runtime voice switching  
- Interacting with editor state or persistent UI settings  
- Implementing any Doppler or LET physics  
- Poly-mode switching

This restraint was *correct*.

Phase II’s job was to **prepare the ground**, not build the house.

---

# III. ROOT-CAUSE ANALYSIS OF EARLIER FAILURES

Phase II encountered repeated failures that revealed deeper architectural and process issues.

Here they are **in explicit engineering language**, not emotional gloss:

---

## 1. **Private Member Access Violations in Tests**

**Root cause:**  
Tests were written against *private internals* (`proc.voiceManager_`) instead of against **public system behavior**.

**Architectural violation:**  
Private fields → untestable without friend classes or public accessors.

**Corrective outcome:**  
We removed all test access to private internals.  
Tests now assert behavior solely through APVTS + processBlock.

---

## 2. **Hallucinated Test Expectations**

Issue:  
Earlier AI responses assumed nonexistent APIs or files.

Impact:  
Introduced cascading confusion and corrections.

Solution:  
We introduced strict rules:

- **No references to lines without naming the file.**  
- **No proposing patches without asking for actual file content.**  
- **Always verify existence before relying on a function/file.**

---

## 3. **Git workflow breakage**

Problem:  
Some changes were proposed without corresponding `git add` steps, leading to:

- Untagged work
- Inconsistent commits
- Lost or orphaned changes

Outcome:  
We established:

### ✔ Every engineering action must include explicit `git add` statements  
### ✔ Every freeze must include a tag  
### ✔ No partial staged areas during architectural transitions

---

## 4. **Assumed Modes/Voices Not in Architecture**

AI earlier assumed:

- Mode‑switching DSP paths  
- Alternate voices that do not exist  
- Components that were planned but not built

Phase II grounded all future mode work in **explicit, real architecture**, not conceptual sketches.

---

# IV. AUTHORITATIVE DESCRIPTION OF CURRENT ARCHITECTURE (POST‑A7)

This is the **ground truth** for Phase III.

---

## 1. Top-Level Architecture Overview

```
MIDIControl001AudioProcessor
 ├── APVTS                           (owned)
 ├── ParameterSnapshot (building helper)
 ├── VoiceManager                    (owned)
 │    ├── setMode()
 │    ├── getMode()
 │    ├── prepare()
 │    ├── startBlock()
 │    ├── handleNoteOn/Off()
 │    ├── handleController()
 │    └── render()
 │
 └── monoScratch buffer
```

VoiceManager currently always instantiates **VoiceA**.

---

## 2. Parameter Flow (Full Path)

```
APVTS
  ↓ makeSnapshotFromParams()
ParameterSnapshot
  ↓ processor.processBlock()
VoiceManager.setMode(mode)
  ↓
VoiceManager.startBlock()
  ↓
VoiceManager.updateParams() → VoiceA
  ↓
VoiceA.render() → mixer → monoScratch → stereo fan-out
```

---

## 3. Global Mode Invariants (As of A7)

- `mode_` exists but affects **no DSP logic yet**.  
- All voices are still **VoiceA**.  
- Processor always forwards `voiceMode` on every block.  
- Snapshot building is the **source of truth** for system state.

This means Phase III can safely implement alternate voices/DSP paths.

---

# V. PHASE III OBJECTIVES (HIGH-LEVEL)

Phase III consists of three major sequences:

## **Sequence 1 — Strengthen mode architecture**
- Introduce runtime voice-type switching (factory or registry)
- Wire modes to actual voice selection
- Maintain DSP stability and voice recycling invariants

## **Sequence 2 — Add new voice types**
- VoiceLET (relativistic Doppler)
- VoiceDopp (classical Doppler)
- VoiceB (alternate synthesis)

## **Sequence 3 — Add per-mode behavior and editor exposure**
- UI mode selector
- Safe voice reset operations
- Mode‑dependent parameter groups

---

# VI. PHASE III — TECHNICAL ROADMAP (FULL DETAIL)

This is the **authoritative engineering plan** moving forward.

---

## **Step III.A — Voice Registry + Construction Layer**

### Goals:
- Decouple VoiceManager from VoiceA
- Introduce clean factory for future voices

### Requirements:
- Must not break Phase II snapshot behavior
- Must remain test‑coverable
- Must not alter VoiceA DSP until VoiceLET is ready

---

## **Step III.B — Multi‑mode VoiceManager**

Extend `VoiceManager` so that:

- `mode_ == 0` ⇒ VoiceA  
- `mode_ == 1` ⇒ VoiceLET  
- `mode_ == 2` ⇒ VoiceDopp  
- Possibly `mode_ == 3` ⇒ VoiceB  

This requires:

- Per‑mode prepare()
- Consistent render() behavior
- Zero‑click voice reinitialization
- No partial destruction of voices mid‑block

---

## **Step III.C — VoiceLET (Relativistic Doppler Voice)**

### Design:
- Based on the previous non‑synth version you built with ChatGPT  
- Must plug into BaseVoice interface  
- Needs:
    - world velocity vector  
    - relative source/listener transform  
    - time dilation → pitch shift  
- Must remain performant in polyphonic contexts

---

## **Step III.D — VoiceDopp (Classical Doppler)**

This is the mode referenced in several earlier conversations.  
Not specific to Phase III, but Phase III builds the required infrastructure.

Requirements:
- Frequency shift proportional to v/c  
- Smooth handling of approaching vs receding  
- Maintain consistent envelope behavior

---

## **Step III.E — UI Mode Selector**

Add a simple, non-graphical (GenericEditor) exposed parameter:

```
voiceMode: { VoiceA / VoiceLET / VoiceDopp / VoiceB }
```

Tests must validate:
- selection → snapshot  
- snapshot → VoiceManager  
- no crash  
- correct voice instantiation

---

# VII. PHASE III — TESTING STRATEGY

We adopt **zero tolerance** for regression:

### 1. Pre‑merge test run
Run full suite after every atomic action.

### 2. Snapshot invariants
- Ensure all modes have consistent serialization
- ParameterSnapshot must remain stable

### 3. Voice instantiation tests
- VoiceManager creates correct type  
- VoiceLET and VoiceDopp compile and render  
- No DSP asserts, NaNs, denorms

### 4. Integration tests
- processBlock stable across mode changes  
- voices stop cleanly  
- no memory leaks

---

# VIII. ARCHITECTURAL VERIFICATION SWEEP (Phase II → Phase III Transition)

This sweep ensures the baseline is safe.

Verification checks include:

### ✔ No private-member dependency in tests  
### ✔ All tests reference public behavior  
### ✔ APVTS → snapshot → mode → VoiceManager path stable  
### ✔ No races or unsafe concurrent access  
### ✔ No DSP change in Phase II  
### ✔ prepare/startBlock/render pipeline intact  
### ✔ No legacy modes lingering  
### ✔ Git repository now clean and tagged  

Result:  
**System is stable and safe for Phase III development.**

---

# IX. STABLE‑STATE SNAPSHOT SUMMARY (POST‑A7)

**Commit tag:** `phase2-A7-voiceMode-stable`  
**Tests:** 30/30 passing  
**Architecture:** Consistent  
**Next work:** Phase III mode system

---

# X. APPENDIX — ENGINEERING PRINCIPLES GOING FORWARD

These are *mandatory* for Phase III.

1. **Never reference lines without naming the file.**  
2. **Never propose code without confirming actual file content.**  
3. **One action per instruction.**  
4. **Every action must come with failure hypotheses.**  
5. **Every engineering change must include:**
   - Explicit file paths  
   - Diff  
   - git add commands  
6. **Never test private internals.**  
7. **No DSP changes without explicit approval.**  
8. **Never invent functionality or files.**  
9. **Snapshot is always the source of truth.**  
10. **You (the model) must treat Tyler as the project owner and final authority.**

---

# XI. END OF REPORT

Phase III now has a:

- clean foundation  
- known architecture  
- verified tests  
- correct mode plumbing  
- explicit, non‑speculative roadmap  
- strong Prime‑Directive discipline  

Future LLM consultants can begin Phase III safely from here.
