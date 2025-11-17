# S3 — Formal Model of the Current Architecture (Fully Expanded)

## Overview

This document provides a rigorous, formal, engineering-grade representation of the current plugin architecture of **MIDIControl001** after completion of Phase II and the Phase III B1–B9 scaffolding steps. It is designed so that any engineer—human or LLM—can recover the entire mental model of the system from first principles, without relying on tacit memory or prior conversational context.

The purpose is to distill:  
- the *state machine*,  
- the *dataflow*,  
- the *control flow*,  
- the *DSP graph*,  
- the *parameter system*,  
- the *voice lifecycle*,  
- and the *mode system scaffolding*  
into a single authoritative reference.

---

# 1. Global System Model

The plugin is a **polyphonic, voice-based synthesizer architecture** with:

- **APVTS** (AudioProcessorValueTreeState) for parameter management  
- **ParameterSnapshot** for per-block immutable parameter state  
- **VoiceManager** coordinating up to 32 voices  
- **BaseVoice** polymorphic interface  
- **VoiceA** as the only concrete voice implementation  
- **MixNormalizer**, **PeakGuard**, **EnvelopeA**, **OscillatorA** for DSP  
- **Phase III mode scaffolding** (currently non-functional, but structurally correct)

The system is structured around a strict block-based update cycle:

```
host → APVTS → ParameterSnapshot → VoiceManager.startBlock → voices.render
```

Each block (approx 48–512 samples depending on the host) is rendered with **fixed** parameters, guaranteeing deterministic and race-free behavior.

---

# 2. Parameter System Model

## 2.1 APVTS Parameter Topology

APVTS currently exposes the following groups:

### Global Parameters
| ID | Meaning | Range | Notes |
|----|---------|--------|-------|
| masterVolumeDb | Master gain | [-60, +12] dB | Applied post-sum |
| masterMix | Wet/dry ratio | [0,1] | Reserved for future FX |
| voiceMode | Voice algorithm selection | {0,1,2,3...} | Currently only 0 active |

### Per-Voice Parameters  
Each voice receives its own snapshot struct:

```
VoiceParams {
    float oscFreq;
    float envAttack;
    float envRelease;
}
```

These are embedded into:

```
ParameterSnapshot {
    float masterVolumeDb;
    float masterMix;
    VoiceMode voiceMode;
    float oscFreq;
    float envAttack;
    float envRelease;
    std::array<VoiceParams, NUM_VOICES> voices;
}
```

The snapshot is constructed **once per block** in PluginProcessor and passed into VoiceManager.

---

# 3. Dataflow Model

## 3.1 Snapshot Flow

```
APVTS → ParameterSnapshot.make() → VoiceManager.startBlock() →
voice.updateParams(snapshot.voices[i])
```

A single snapshot is shared across all voices.

Snapshots are:
- **immutable** after creation (per block)
- **consistent** across all voices
- **deterministic and race-free**

## 3.2 Controller Flow

MIDI CC messages travel:

```
PluginProcessor → VoiceManager.handleController → each voice.handleController
```

Additionally, CC3–5 are captured in a **persistent CC cache** that is re-applied every block:

```
snapshot.envAttack  = ccCache.envAttack
snapshot.envRelease = ccCache.envRelease
snapshot.oscFreq    = ccCache.oscFreq
```

This guarantees:
- CC values persist across new notes,
- CC changes inside a block will not cause inconsistent results,
- per-block snapshot always reflects the most recently received CC state.

---

# 4. DSP Graph Model

Each block follows:

```
buffer = 0
for voice in voices:
    if voice.active:
        voice.render(buffer)
apply global RMS-based gain normalization
apply masterVolumeDb
```

The DSP graph is *flat*—no buses, no spatialization, no modulation routing yet.

---

# 5. Voice Lifecycle Model

### States

A voice is in one of:

```
Idle → Active → Releasing → Idle
```

The transition rules:

- **noteOn**: Idle → Active  
- **noteOff**: Active → Releasing  
- **env reaches zero**: Releasing → Idle  
- **voice stealing**: Active → Active (new note with reset envelope)

### Allocation

On noteOn:

1. Find first inactive voice  
2. Else steal quietest active voice  
3. newVoice.noteOn(snapshot, midiNote, velocity)

### Rendering

Each active voice:

```
OscillatorA → EnvelopeA → sum into buffer
```

EnvelopeA determines activity state.

---

# 6. Mode System (Phase III Scaffolding)

### 6.1 VoiceMode Enum

```
enum class VoiceMode {
    VoiceA = 0,
    // future: VoiceDopp, VoiceLET, VoiceFM, ...
};
```

### 6.2 Mode Storage

VoiceManager stores two values:

```
VoiceMode mode_;      // current target mode
VoiceMode lastMode_;  // previous mode, for change detection
```

### 6.3 Mode Change Detection

At `startBlock()`:

```
if (mode_ != lastMode_)
    rebuildVoicesForMode();
```

### 6.4 Rebuild Process

```
voices_.clear();
for i in 0..31:
    voices_.push_back( factory(mode_) )
```

### 6.5 makeVoiceForMode()

Currently:

```
return std::make_unique<VoiceA>();
```

Later:

```
switch(mode_) {
    case VoiceA:   return make VoiceA
    case VoiceDopp:return make VoiceDopp
    case VoiceLET: return make VoiceLET
}
```

### 6.6 Behavioral State

**Currently 100% inert.**  
Mode changes rebuild all voices but since all modes return VoiceA, the result is structurally identical.

---

# 7. Failure Modes and Invariants

### Invariant: Snapshot consistency  
Snapshots must never reflect mid-block CC or APVTS changes.

### Invariant: Voice reallocation only on mode change  
Voices should only be destroyed/constructed when `voiceMode` parameter changes.

### Invariant: deterministic RMS-based gain  
GlobalGain must use a stable ramp for smoothness and predictability.

### Risk: accidental mid-render rebuild  
This is prevented by ensuring rebuild only happens in `startBlock()`.

---

# 8. Mathematical Model of Timing

Let:
- B = block size (samples)
- Fs = sample rate
- tₙ = block times n·B/Fs

Then:

- ParameterSnapshot(t) = constant for t ∈ [tₙ, tₙ₊₁)
- Voice state vᵢ(t) evolves under the DSP equations using snapshot(tₙ)
- Voices are reconstructed only on block boundaries

This makes the system **piecewise-stationary** with crisp transition points and mathematically clean evolution.

---

# 9. Architectural Consequences

### Positive
- deterministic block-level behavior  
- perfectly race-free parameter access  
- scalable to arbitrarily complex voice types  
- trivial to port to JUCE’s internal render pipelines  
- makes future modes easy to add

### Negative / Risks
- rebuilding 32 voices every mode switch is expensive  
- snapshot size may grow with future modes  
- per-voice params currently duplicated redundantly across all voices  
- future polyphonic FX (filters, routing) will require refactoring

---

# 10. Readiness for Phase III

The architecture is now at an ideal position for Phase III:

- **mode system exists**  
- **mode-change detection exists**  
- **voice reallocation pipeline exists**  
- **factory injection exists**  
- **tests confirm no behavior change**  
- **snapshot is stable and extensible**  
- **clean per-block model**  

Phase III can now add:
- VoiceDopp  
- VoiceLET  
- VoiceFM  
- Mode-specific DSP  
- Per-mode parameter schemas  
- Correct state transitions  

without rewriting core systems.

---

# End of S3 — Formal Model of the Current Architecture
