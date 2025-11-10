# Prime Directive Enforcement Charter — MIDIControl001

**Date:** November 10, 2025  
**Purpose:** Make the operational discipline *enforceable*.  
This document defines non-negotiable behavioral and architectural rules for any consultant or engineer working on `MIDIControl001`.  
It exists to prevent the kind of drift that led to “three-line patch in `PluginProcessor.cpp`” violations.

---

## 1️⃣ Zero-Guess Rule: No Fix Without Verification

Before proposing **any patch or code change**, the consultant **must**:

1. **Open the relevant file** and locate the suspected entry point.  
2. **Describe it in plain text** (file path + line range + observed behavior).  
3. **Show log or runtime proof** confirming the issue.  
4. **Wait for human confirmation**.  
5. **Only then** may a patch or diff be proposed.

> ✅ “I can show logs proving `updateParams()` overwrites active voice freq.” → Patch allowed.  
> ❌ “I can show a 3-line patch to route CC3–5 in PluginProcessor.cpp.” → Forbidden speculation.

---

## 2️⃣ Architectural Jurisdictions

| Layer | Role | Mutation Permission |
|--------|------|---------------------|
| **PluginProcessor.cpp** | Shell only: handles APVTS setup, `processBlock`, and snapshot creation | 🔒 *Read-only* for DSP routing logic |
| **ParameterSnapshot.cpp** | Captures current APVTS values each block | ✅ Can evolve if new parameters are added |
| **VoiceManager.cpp** | Core DSP coordinator: distributes per-block snapshot values to voices | ✅ Primary control-edit zone |
| **VoiceA.cpp** | Per-voice DSP + envelope/osc control | ✅ Secondary control-edit zone |
| **PluginEditor.cpp** | GUI + SliderAttachments | ✅ Edit for visual or interactive controls only |

**Never** wire MIDI or parameter control directly inside `PluginProcessor.cpp` unless confirmed that it *does not* reach the desired DSP layer.

---

## 3️⃣ “Minimal ≠ Blind” Rule

Short patches are good **only after verification**.  
A “minimal diff” that bypasses architectural flow is still a Prime-Directive violation.

> ✅ “3-line patch after confirming Snapshot ignores param ID.”  
> ❌ “3-line patch because it looks simpler.”

---

## 4️⃣ The Verified Control Path (Mandatory Reference)

All GUI and MIDI controls must flow through the following chain:

```
APVTS (parameter tree)
 → ParameterSnapshot (rebuilt every block)
 → VoiceManager.startBlock()
 → VoiceA.updateParams()
```

Breaking or bypassing any stage invalidates test results and voids consultant authority for that change.

---

## 5️⃣ Definition of Verification

A proposal is *verified* only if at least one of these exists:

- A matching log line proving the observed behavior.
- A code snippet with line numbers from current file context.
- A terminal trace (DBG / analyzer output) showing cause and effect.

If none exist → stop, document, request confirmation.

---

## 6️⃣ Enforcement Procedure

If any consultant produces speculative patches or modifies unverified files:

1. Freeze all proposed code.  
2. Generate a **Sync Report (.md)** documenting where reality diverged.  
3. Only after that report is reviewed may engineering resume.

---

## 7️⃣ Communication Contract

- **One issue → one fix → one confirmation.**  
- Never propose multi-branch “maybe” solutions.  
- Emotional signals (e.g. “I just want this to WORK”) are **sync alerts**, not noise.  
  They mean the engineer must stop proposing and realign reality through artifact proof.

---

## 8️⃣ Summary

The goal is not just correctness — it’s **traceable correctness**.  
A patch is valid only if it can be tied to a verified observation in logs, code, or runtime output.  
All future consultants must treat this document as binding project law.

---

**File:** `docs/PrimeDirective_Enforcement.md`  
**Owner:** Tyler Foster (Project Authority)  
**Amendments:** Only Tyler may revise behavioral or structural permissions.  
