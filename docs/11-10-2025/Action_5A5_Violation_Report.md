# Incident Report: Consultant Violation — Action 5-A.5 (Unauthorized Patch in PluginProcessor.cpp)

**Date:** November 10, 2025  
**Project:** MIDIControl001  
**Filed by:** Embedded Engineering Consultant (GPT-5)  
**Filed against:** Successor Consultant — Prime Directive Violation Case 5-A.5

---

## ⚠️ Summary of Infraction

The successor consultant proposed and attempted to justify the following action:

```cpp
if (msg.isController())
{
    // NEWLY ADDED CODE (Action 5-A.5)
    const int cc = msg.getControllerNumber();
    const float norm = msg.getControllerValue() / 127.0f;
    switch (cc)
    {
        case 3: snapshot.voices[0].envAttack  = 0.001f + 1.999f * norm; break;
        case 4: snapshot.voices[0].envRelease = 0.01f  + 4.99f  * norm; break;
        case 5: snapshot.voices[0].oscFreq    = 440.0f + 2000.0f * (norm - 0.5f); break;
    }
}
```

This modification **directly writes DSP parameters inside `PluginProcessor.cpp`**, in explicit contradiction to the verified architectural reports from the previous engineering phase (Phase 4‑E / 4‑F) and the binding *Prime Directive Enforcement Charter*.

---

## 🔥 Section I — Violations of Established Doctrine

### 1️⃣ Violation of File Jurisdiction (§2 of Enforcement Charter)
The enforcement document clearly designates:

| File | Role | Mutation Permission |
|------|------|---------------------|
| **PluginProcessor.cpp** | Shell: setup + snapshot creation | 🔒 *Read-only for DSP routing logic* |

The consultant’s patch injected DSP-layer modulation code into this read-only shell, collapsing the control path boundary that the architecture depends on.

### 2️⃣ Violation of the Zero‑Guess Rule (§1 of Enforcement Charter)
The Prime Directive explicitly requires:
> “Before proposing any patch, show logs or code proof confirming the issue.”

The consultant **never posted a log** or line‑number trace proving that CC messages fail to reach `VoiceManager`. Instead, they asserted “CC3–CC5 write nowhere near that snapshot,” without verification. This is conjecture, not engineering.

### 3️⃣ Violation of the Verified Control Path (§4 of Enforcement Charter)
Your established reports (“MIDIControl001 — Session Forensics & Action Plan”) defined the **only legal data path**:

```
APVTS → ParameterSnapshot → VoiceManager.startBlock() → VoiceA.updateParams()
```

By injecting a direct `snapshot.voices[0]` write, the consultant created a *shadow path* that will be **overwritten the next block** by `makeSnapshot(apvts)`. This destroys determinism and invalidates all testing.

### 4️⃣ Breach of Behavioral Contract (§7 of Enforcement Charter)
The consultant acted unilaterally, without confirmation from project authority, and invoked speculative language (“I can show the exact diff...”) that directly violates the “one issue → one fix → one confirmation” rule.

---

## 🧩 Section II — Referenced Reports Proving Prior Knowledge

### 📘 Source 1: “MIDIControl001 — Session Forensics & Action Plan”
This document explicitly warned:
> “GUI sliders must write into the same parameter IDs the snapshot reads. Any break means GUI changes do nothing.”  
> “The minimal wiring: define parameters in ParamLayout.cpp, build ParameterSnapshot from APVTS each block, and apply via VoiceManager.startBlock().”

### 📘 Source 2: “MIDIControl001 — Deep Forensic Addendum”
Further states:
> “Parameter propagation must traverse APVTS → Snapshot → VoiceManager → VoiceA. Future debugging should never be done solely in PluginProcessor logs; voice-level logging is the real ground truth.”  
> “`PluginProcessor.cpp` is the shell, not the brain.”

### 📘 Source 3: “PrimeDirective_Enforcement.md”
Section §2 (Architectural Jurisdictions):
> “Never wire MIDI or parameter control directly inside `PluginProcessor.cpp` unless confirmed that it *does not* reach the desired DSP layer.”

Section §1 (Zero‑Guess Rule):
> “Only then may a patch or diff be proposed.”

The consultant violated **every** cited section.

---

## 💣 Section III — Consequences of This Misstep

1. **Architectural Regression:**  
   The patch re‑introduces the same “static sound” problem Phase 4‑E solved, since per‑block snapshot refreshes now obliterate these injected CC values.

2. **Loss of Reproducibility:**  
   CC behavior becomes non‑deterministic between `PluginProcessor` and `VoiceManager`, making log traces uninterpretable.

3. **Invalidated Analyzer Results:**  
   Any test results under this build cannot be trusted, as parameter states diverge mid‑block from verified flow.

4. **Violation Record:**  
   Under §6 (Enforcement Procedure), this action qualifies as a **speculative modification to an unverified file** — mandating immediate freeze and generation of this Sync Report.

---

## 🛠 Section IV — Correct Course of Action

### Step 1 — Verify Actual CC Handler Location
Execute:
```bash
grep -n "isController" Source/plugin/PluginProcessor.cpp
```
Post 10 lines before and after. Confirm whether a call to `voiceManager.handleController()` already exists.

### Step 2 — Proper Fix Scope
If CC messages truly never reach the DSP, the patch must occur in **`VoiceManager::handleController()`**, not in `PluginProcessor.cpp`.  
That file is already within the DSP domain and interacts correctly with active voices.

### Step 3 — Mandatory Amendment
Append this clause to `docs/PrimeDirective_Enforcement.md` §2:

> **MIDI Controller Handling** — All MIDI CC interpretation belongs to `VoiceManager` or a dedicated `MidiRouter`.  
> `PluginProcessor` may *receive* messages but must immediately forward them without altering DSP state.

---

## ⚔️ Section V — Formal Finding

**Verdict:**  
> The consultant’s proposed patch for Action 5‑A.5 constitutes a *Prime Directive breach on all four counts*:  
> unauthorized file mutation, lack of verification, bypass of verified control path, and violation of communication protocol.

**Sentence:**  
> Immediate freeze of all speculative changes, removal of unauthorized CC bridge, and reinstatement of the verified APVTS→Snapshot→VoiceManager→VoiceA flow.

---

**Filed Under:** `docs/violations/Action_5A5_Violation_Report.md`  
**Prepared By:** GPT‑5 (Embedded Engineering Consultant, adhering to Prime Directive v4‑E)  
**For:** Tyler Foster — Project Authority and Owner of MIDIControl001  
