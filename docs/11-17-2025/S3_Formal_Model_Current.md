# S3 — Formal Model of the Current Architecture (Unified, Up-to-Date)

## 0. Purpose

This document is the **authoritative, engineering-grade formal model** of the current state of the MIDIControl001 plugin *as of the end of the present design session (Phase III complete, VoiceDopp skeleton built, mode plumbing active, CC persistence installed)*.

It replaces all previous S3 documents and fully supersedes earlier architectural descriptions.

It is designed so that **any incoming engineer—human or LLM—can reconstruct the entire architecture from first principles**, without depending on conversational archives.

It expresses:

- the DSP graph  
- the program state machine  
- the per‑block dataflow  
- the parameter system  
- the multi-voice system  
- the mode system (VoiceA + VoiceDopp skeleton)  
- the CC persistence controller  
- the test‑level invariants  
- the formal timing model  
- the object interaction graph  

This S3 is the *single source of truth* for the plugin's current architecture.


---

# 1. Global Architectural Overview

The system is a **polyphonic synthesizer** implemented in JUCE with:

- **APVTS** → Host parameter store  
- **ParameterSnapshot** → Immutable per-block state  
- **VoiceManager** → Polyphonic orchestration, mode-awareness, CC persistence  
- **BaseVoice** → Abstract DSP voice interface  
- **VoiceA** → Fully functional baseline voice  
- **VoiceDopp** → Skeleton class; no DSP yet, but fully wired into allocation  
- **Mixing subsystem** → RMS-normalized clickless summation  
- **Global mode system** → Phase III scaffolding complete  

The core principle is:

> ***All parameter reads are snapshot-locked per audio block.***  
> ***All voices render using the same immutable snapshot.***  
> ***VoiceManager is the single point of orchestration.***

This gives us deterministic DSP, guaranteed thread safety, and perfect reproducibility in tests.


---

# 2. Parameter System — Formal Definition

## 2.1 Parameter Topology

There are three layers:

### Layer 1 — Master Parameters  
- `masterVolumeDb ∈ [-60, +0]`  
- `masterMix     ∈ [0, 1]`

### Layer 2 — Global Voice Parameters  
- `voiceMode ∈ {VoiceA, VoiceDopp, VoiceLET, VoiceFM} (UI list)`  
  - currently clamped so that VoiceA is the only active mode  
- `oscFreq`  
- `envAttack`  
- `envRelease`

### Layer 3 — Per‑Voice Parameters  
For each of `NUM_VOICES = 3`:
```
voice[i].oscFreq
voice[i].envAttack
voice[i].envRelease
```

These are stored in the APVTS and transferred into a block-level `ParameterSnapshot`.

---

## 2.2 ParameterSnapshot Formal Definition

A `ParameterSnapshot` is a **pure data structure**, containing:

```
masterVolumeDb : float
masterMix      : float
voiceMode      : VoiceMode
oscFreq        : float
envAttack      : float
envRelease     : float
voices         : array[NUM_VOICES] of VoiceParams
```

Properties:

1. **Immutable** within an audio block  
2. **Allocated once per block** in `PluginProcessor::processBlock`  
3. **Shared by all voices**  
4. **Cannot reference APVTS** after creation  

This snapshot is the **canonical DSP configuration** for the current block.

---

# 3. DSP Dataflow — Formal Block Diagram

```
┌────────────────────┐
│     Host (DAW)     │
└─────────┬──────────┘
          │ parameters, automation
          ▼
┌──────────────────────────────┐
│ APVTS (ValueTree)            │
└─────────┬────────────────────┘
          │ read once / block
          ▼
┌──────────────────────────────┐
│ ParameterSnapshot (immutable)│
└─────────┬────────────────────┘
          │ passed by Ptr
          ▼
┌──────────────────────────────┐
│ VoiceManager                 │
│  - mode handling             │
│  - CC persistence            │
│  - voice allocation          │
│  - voice stealing            │
│  - RMS gain stabilizer       │
└─────────┬────────────────────┘
          │ updateParams()
          ▼
┌──────────────────────────────┐
│ Voices[0..31] (BaseVoice*)   │
│  VoiceA / VoiceDopp skeleton │
└─────────┬────────────────────┘
          │ render()
          ▼
┌──────────────────────────────┐
│ Mono Mix Buffer              │
└─────────┬────────────────────┘
          │ gain, mix, masterVolume
          ▼
┌──────────────────────────────┐
│ Output (stereo)              │
└──────────────────────────────┘
```

---

# 4. MIDI Flow — Formal Model

### Raw flow:
```
PluginProcessor → VoiceManager.handleController() → voice.handleController()
```

### CC3/4/5 persistent flow:
```
handleController(cc,norm) 
    → update CC cache
    → -> snapshot.envAttack/envRelease/oscFreq overwritten per block
```

### Note flow:
```
NoteOn:
    find inactive voice
    else steal quietest
    voice.noteOn(snapshot, note, velocity)

NoteOff:
    voice.noteOff()
    update globalGain target if polyphony ends
```

---

# 5. VoiceManager — Formal State Machine

## 5.1 Internal State

```
voices_: array[0..31] of unique_ptr<BaseVoice>
mode_: VoiceMode
lastMode_: VoiceMode
ccCache: { envAttack, envRelease, oscFreq }
globalGain_: SmoothedValue<float>
currentSnapshot_: pointer to snapshot
sampleRate_: float
```

---

## 5.2 VoiceManager Lifecycle

### prepare(sampleRate):
- resets smoothing  
- syncs `lastMode_ = mode_`  
- calls `rebuildVoicesForMode()`  
- prepares all voices  

### startBlock():
1. snapshot ← makeSnapshot()  
2. applyModeRouting (currently inert)  
3. rebuildVoicesIfModeChanged()  
4. applyModeConfiguration()  
5. apply persistent CC cache into snapshot  
6. update per-voice parameters

### handleNoteOn()
- steals or allocates voice  
- triggers noteOn(snapshot, …)  
- sets globalGain target to 1.0  

### handleNoteOff()
- releases all matching voices  
- if none remain active → globalGain target = 0.0  

### handleController()
- logs  
- updates CC cache  
- forwards to each voice  

### render()
- mix all voices into scratch buffer  
- calculate block RMS  
- update globalGain  
- apply smoothed gain  
- write results  

---

# 6. Voice Subsystem — Formal Model

## 6.1 BaseVoice Interface (Abstract)
Each voice must implement:

```
prepare(sampleRate)
updateParams(VoiceParams)
handleController(cc, norm)
noteOn(snapshot, midiNote, velocity)
noteOff()
render(float* buffer, int numSamples)
isActive()
getNote()
getCurrentLevel()
```

## 6.2 VoiceA — Fully Implemented
Implements:
- sine oscillator  
- linear ADSR envelope  
- per-voice parameters  
- controller mapping for CC3/4/5  

Guaranteed invariants:
- no allocations inside render  
- no APVTS access  
- deterministic envelope evolution  

## 6.3 VoiceDopp — Skeleton
Currently:

- present in code  
- constructed when mode switches  
- inherits BaseVoice  
- empty DSP (no oscillation, no envelope)  
- passes compilation and presence tests  

This is the landing zone for Phase IV classical Doppler physics.

---

# 7. Mode System — Formal Compositional Model

## 7.1 Mode Enumeration

```
VoiceA
VoiceDopp
VoiceLET
VoiceFM
```

Currently, only **VoiceA** is active; others route to VoiceA or skeleton.

## 7.2 Mode Change Rule

Mode change occurs when:

```
snapshot.voiceMode != lastMode_
```

Which triggers:

```
rebuildVoicesForMode()
```

Guarantee:
- All mode transitions are atomic at block boundaries  
- DSP never sees mixed voice types within a block  

## 7.3 Allocation Rule

```
voices_[i] = voiceFactory_(mode) or makeVoiceForMode(mode)
```

Factory injection allows dependency injection for tests or prototypes.

---

# 8. CC Persistence — Formal Model

Let:

```
ccCache = {
    envAttack  : float,
    envRelease : float,
    oscFreq    : float
}
```

Normalized CC values → mapped into physical values:

- Attack: exponential range 0.001s → 2s  
- Release: exponential 0.02s → 5s  
- Pitch: linear offset around 440 Hz  

At `startBlock()`:

```
snapshot.envAttack  ← ccCache.envAttack
snapshot.envRelease ← ccCache.envRelease
snapshot.oscFreq    ← ccCache.oscFreq
```

This ensures *new notes always inherit last CC values*.

---

# 9. Formal Timing Model

Let:

- block index: `n`  
- block size: `B`  
- sample rate: `Fs`

Then:

```
t_n = n * (B / Fs)
```

Snapshot creation time is:

```
snapshot(t) = snapshot_n  for t ∈ [t_n, t_{n+1})
```

Properties:

1. **Snapshot is piecewise-constant**  
2. **No mid-block parameter drift**  
3. All voices see the **same** snapshot at a given block  

This yields a mathematically clean “block-time” DSP model.

---

# 10. Test-Level Invariants

The architecture must satisfy:

### Invariant 1 — Snapshot consistency  
Snapshot must never reflect mid-block parameter/CC changes.

### Invariant 2 — Voice type consistency  
All voices must be rebuilt together upon a mode change.

### Invariant 3 — No allocations inside render  
All heap operations must occur:
- inside prepare  
- inside rebuild  
- never inside render  

### Invariant 4 — RMS normalization holds  
GlobalGain must remain bounded:
```
0.25 ≤ gain ≤ 4.0
```

### Invariant 5 — Deterministic voice stealing  
Quietest active voice must be chosen.

### Invariant 6 — All tests must pass  
Baseline VoiceA output equivalence is sacred.

---

# 11. Current Architecture Summary

At the end of this design session, the plugin architecture is now:

- fully mode-aware  
- safe under mode switching  
- contains VoiceDopp skeleton  
- contains persistent CC mapping  
- maintains deterministic DSP  
- 100% backward-compatible with VoiceA-only behavior  
- test suite fully green  

This is the formal ground truth of the system.



