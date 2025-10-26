# Plugin Design / Architecture

## Runtime Context — Host Environments

JUCE plugins always run *inside a host*, which owns the real-time audio loop and the message (GUI) thread.  
Depending on how the plugin is built or launched, the *host* can be:

1. a **DAW** such as FL Studio or Logic (VST3/AU),
2. the **JUCE Standalone wrapper** (self-hosted executable),
3. a **custom CLI runner** that instantiates the DSP directly.

All three call the same `PluginProcessor` and optional `PluginEditor`,  
but differ in *who owns the main loop* and *who calls `processBlock()`*.

### Host responsibilities

| Environment | Who owns main loop | What JUCE does | Typical entry point |
|--------------|--------------------|----------------|---------------------|
| **DAW Host (e.g. FL Studio)** | The DAW’s engine | JUCE runs *inside* the plugin process, handling `AudioProcessor` + GUI layers | `processBlock()` called by host |
| **JUCE Standalone** | JUCE itself | JUCE provides audio + MIDI devices and runs its own threads | `StandaloneFilterWindow` |
| **CLI Runner (Custom)** | Your `main()` | You instantiate `PluginProcessor` manually, feed MIDI/buffers, render to file | `PluginProcessor::processBlock()` |

### Conceptual layering

```text
       [DAW Host] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
[Standalone Host] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
     [CLI Runner] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
```

## Module Dependencies

```text
Host (DAW/Standalone)
 ├─ calls → PluginProcessor.cpp  ← (real-time audio loop)
 │             ├─ owns AudioProcessorValueTreeState (shared parameter state)
 │             ├─ uses VoiceManager.h        (Factory / Pool)
 │             │     └─ uses Voice.h         (Strategy interface)
 │             │           ├─ uses Oscillator.h (Concrete Strategy)
 │             │           └─ uses Envelope.h   (Concrete Strategy)
 │             ├─ uses SmoothedValue (Decorator for per-param smoothing)
 │             ├─ updates ValueTree from MIDI CC / Host automation
 │             └─ notifies PluginEditor.cpp via State store attachments (Observer)
 │
 └─ calls → PluginEditor.cpp     ← (GUI thread / Active Object)
               └─ observes ValueTree via parameter attachments
```

# Sound generation


## `Note On` event: *When key is pressed on MIDI controller*

### A1: `Note On` — External Interface
```mermaid
sequenceDiagram
participant Buffer as JUCE AudioBuffer
participant MIDI as MIDI Controller
participant PluginProcessor as PluginProcessor.cpp/.h
participant Ref as Processor Core

Note over Buffer,PluginProcessor: External communication — plugin entry/exit
Note over PluginProcessor,Ref: Internal process. (DSP) — Voice alloc. + start
MIDI->>PluginProcessor: MIDI Note On event
PluginProcessor->>Ref: hand-off to internal processing
Note over Ref: continues in A2
Ref-->>PluginProcessor: (returns voice audio)
PluginProcessor-->>Buffer: writes processed samples to buffer
```

Continues in: [A2: `Note On` — Internal Processing (DSP)](#a2-note-on--internal-processing-dsp)

### A2: `Note On` — Internal Processing (DSP)
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h
participant ValueTree as State store
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over PluginProcessor,ValueTree: Audio/Synthesis Engine Controller
Note over PluginProcessor,DSP: Internal processing (DSP) — Voice allocation + start
Note over ValueTree,DSP: Processor Core
Note over VoiceManager,DSP: Audio/Synthesis Engine
PluginProcessor->>ValueTree: read ADSR parameters
PluginProcessor->>VoiceManager: allocate Voice for note (Factory)
VoiceManager->>Voice: init. w/ ADSR params
Voice->>DSP: note on + phase reset
DSP-->>Voice: per-sample amplitude
Voice-->>VoiceManager: stream voice audio
VoiceManager-->>PluginProcessor: mix all active voices
Note over PluginProcessor: returns to A1
```

Returns to: [A1: `Note On` — External Interface](#a1-note-on--external-interface)

## `Note Off` event: *When key is lifted up on MIDI controller*

### B1: `Note Off` — External Interface
```mermaid
sequenceDiagram
participant Buffer as JUCE AudioBuffer
participant MIDI as MIDI Controller
participant PluginProcessor as PluginProcessor.cpp/.h
participant Ref as Processor Core

Note over Buffer,PluginProcessor: External communication — plugin entry/exit
Note over PluginProcessor,Ref: Internal processing (DSP) — release + cleanup
MIDI->>PluginProcessor: MIDI Note Off event
PluginProcessor->>Ref: hand-off to internal processing
Note over Ref: continues in B2
Ref-->>PluginProcessor: (returns silence / tail samples)
PluginProcessor-->>Buffer: writes updated samples to buffer
```

Continues in: [B2: `Note Off` — Internal Processing (DSP)](#b2-note-off--internal-processing-dsp)

### B2: `Note Off` — Internal Processing (DSP)
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h
participant ValueTree as State store
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Envelope.h + Oscillator.h

Note over PluginProcessor,ValueTree: Audio/Synthesis Engine Controller
Note over PluginProcessor,DSP: Internal processing (DSP) — release + cleanup
Note over ValueTree,DSP: Processor Core
Note over VoiceManager,DSP: Audio/Synthesis Engine
PluginProcessor->>ValueTree: read release parameter
PluginProcessor->>VoiceManager: locate Voice for note
VoiceManager->>Voice: trigger note release
Voice->>DSP: ADSR gets fade/sample
DSP-->>Voice: output decaying waveform
Voice-->>VoiceManager: signal inactive
VoiceManager-->>PluginProcessor: remove finished voice
Note over PluginProcessor: returns to B1
```

Returns to: [B1: `Note Off` — External Interface](#b1-note-off--external-interface)

# Sound modulation

## `Parameter Update` event: *Triggered by GUI, MIDI CC, or Host Automation*

Parameter updates may originate from:
- user interaction in the `PluginEditor`,
- incoming MIDI controller data, or
- DAW automation envelopes.

All of these updates converge in the **AudioProcessorValueTreeState** (State store),  
which the `PluginProcessor` owns. The State store synchronizes parameter values  
across threads and provides smoothed transitions for DSP consumption.

### C1: Parameter Update — External Interface

```mermaid
sequenceDiagram
participant GUI as PluginEditor.cpp/.h (GUI Thread)
participant MIDI as MIDI Controller
participant Host as DAW Automation
participant State store as AudioProcessorValueTreeState (Shared State)
participant PluginProcessor as PluginProcessor.cpp/.h (Audio Thread)
participant Buffer as JUCE AudioBuffer

Note over GUI,Buffer: Parameter updates (GUI, MIDI, or Host automation)
GUI->>State store: knob or slider change (Attachment)
MIDI->>State store: CC message mapped to parameter
Host->>State store: automation event
State store->>PluginProcessor: notify of updated values
PluginProcessor->>Buffer: next audio block uses smoothed params
```

Continues in: [C2: Parameter Update — Internal Processing](#c2-parameter-update--internal-processing)

### C2: Parameter Update — Internal Processing
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over PluginProcessor,DSP: Internal signal flow — smoothing and parameter propagation
PluginProcessor->>PluginProcessor: apply SmoothedValue ramp (thread-safe)
PluginProcessor->>VoiceManager: propagate parameters to Voices
VoiceManager->>Voice: update oscillator/envelope settings
Voice->>DSP: adjust frequency / gain / ADSR targets
DSP-->>Voice: apply smoothed parameters per sample
Voice-->>VoiceManager: updated audio stream reflects new parameters
VoiceManager-->>PluginProcessor: mixed audio with parameter changes applied
Note over PluginProcessor: returns to C1 (see link below)
```

Returns to: [C1: Parameter Update — External Interface](#c1-parameter-update--external-interface)