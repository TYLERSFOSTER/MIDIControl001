# Plugin Design / Architecture

## ?

### Runtime Context — Host Environments

JUCE plugins always run *inside a host*, which owns the real-time audio loop and the message (GUI) thread.  
Depending on how the plugin is built or launched, the *host* can be:

1. a **DAW** such as FL Studio or Logic (VST3/AU),
2. the **JUCE Standalone wrapper** (self-hosted executable),
3. a **custom CLI runner** that instantiates the DSP directly.

All three call the same `PluginProcessor` and optional `PluginEditor`,  
but differ in *who owns the main loop* and *who calls `processBlock()`*.

#### Host responsibilities

| Environment | Who owns main loop | What JUCE does | Typical entry point |
|--------------|--------------------|----------------|---------------------|
| **DAW Host (e.g. FL Studio)** | The DAW’s engine | JUCE runs *inside* the plugin process, handling `AudioProcessor` + GUI layers | `processBlock()` called by host |
| **JUCE Standalone** | JUCE itself | JUCE provides audio + MIDI devices and runs its own threads | `StandaloneFilterWindow` |
| **CLI Runner (Custom)** | Your `main()` | You instantiate `PluginProcessor` manually, feed MIDI/buffers, render to file | `PluginProcessor::processBlock()` |

#### Conceptual layering

```text
       [DAW Host] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
[Standalone Host] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
     [CLI Runner] → calls → [JUCE Plugin Framework] → [PluginProcessor → DSP]
```

### Module Dependencies

```text
Host (DAW / Standalone / CLI)
 ├─ calls → PluginProcessor.cpp  ← (real-time audio loop)
 │             ├─ owns AudioProcessorValueTreeState (APVTS — global State Tree)
 │             ├─ uses VoiceManager.h        (Factory / Pool)
 │             │     ├─ injects per-voice parameter views from APVTS
 │             │     └─ creates modular Voice subclasses (e.g. VoiceA, VoiceB, VoiceC)
 │             │           ├─ uses Oscillator*.h (Concrete Strategies)
 │             │           └─ uses Envelope*.h   (Concrete Strategies)
 │             ├─ uses SmoothedValue (Decorator for per-parameter smoothing)
 │             ├─ updates APVTS from MIDI CC, Host automation, and GUI events
 │             └─ exposes state changes to PluginEditor.cpp and VoiceGUI modules (Observers)
 │
 └─ calls → PluginEditor.cpp  ← (GUI thread / Active Object)
               ├─ observes APVTS via parameter attachments (Observer)
               ├─ spawns per-Voice GUI panels bound to APVTS subtrees
               ├─ reflects current parameter values in GUI controls (knobs, sliders, meters)
               └─ sends user interactions → APVTS (State Tree) → PluginProcessor (two-way binding)
```

## Sound generation


### `Note On` event: *When key is pressed on MIDI controller*

#### A1: `Note On` — External Interface
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

#### A2: `Note On` — Internal Processing (DSP)
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h
participant ValueTree as State store
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over PluginProcessor,DSP: Internal processing (DSP) — Voice allocation + start
Note over VoiceManager,DSP: Audio/Synthesis Engine
Note over PluginProcessor,ValueTree: Audio/Synthesis Engine Controller
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

### `Note Off` event: *When key is lifted up on MIDI controller*

#### B1: `Note Off` — External Interface
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

#### B2: `Note Off` — Internal Processing (DSP)
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h
participant ValueTree as State store
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Envelope.h + Oscillator.h

Note over PluginProcessor,DSP: Internal processing (DSP) — release + cleanup
Note over VoiceManager,DSP: Audio/Synthesis Engine
Note over PluginProcessor,ValueTree: Audio/Synthesis Engine Controller
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

## Sound modulation

### `Parameter Update` event: *Triggered by GUI, MIDI CC, or Host Automation*

Parameter updates may originate from:
- user interaction in a **Voice-specific GUI** (e.g. `FMVoiceGUI`, `GranularVoiceGUI`),
- incoming **MIDI controller data**, or
- **DAW automation envelopes**.

All of these sources converge in the **AudioProcessorValueTreeState** (APVTS, *State Store*),  
which the `PluginProcessor` owns. The APVTS synchronizes parameter values  
across threads and provides smoothed transitions for the DSP engine.

Each Voice module and its corresponding GUI read/write parameters through  
a *namespaced parameter group* in the APVTS (e.g. `fm_attack`, `granular_density`).  
This allows multiple voice types and GUIs to coexist modularly without interfering  
with one another.

---

#### C1: Parameter Update — External Interface
```mermaid
sequenceDiagram
participant GUI as VoiceGUI
participant MIDI as MIDI Controller
participant Host as DAW Automation
participant APVTS as State Store
participant Processor as PluginProcessor.cpp/.h
participant Buffer as JUCE AudioBuffer

Note over GUI,Buffer: Parameter updates from GUI, MIDI CC, or Host automation
GUI->>APVTS: user changes control (Attachment)
MIDI->>APVTS: CC message mapped to parameter
Host->>APVTS: automation event
APVTS->>Processor: share updated value
Processor->>Buffer: to next audio block
Note over APVTS,Processor: notifies Processor in C2
```

At this point, the Processor has received updated parameter values from the APVTS. See [C2: Parameter Update — Internal Processing](#c2-parameter-update--internal-processing) for how those values are applied within the DSP engine.

#### C2: Parameter Update — Internal Processing
```mermaid
sequenceDiagram
participant Processor as PluginProcessor.cpp/.h
participant APVTS as State store
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over Processor,DSP: Internal flow — smoothing + parameter propagation
Processor->>APVTS: read current values
Processor->>VoiceManager: propagate active parameter groups
VoiceManager->>Voice: update per-voice
Voice->>DSP: apply per sample
DSP-->>Voice: updated values
Voice-->>VoiceManager: voice stream
VoiceManager-->>Processor: mix all active voices
Note over Processor: steady state til update

```

Returns to: [C1: Parameter Update — External Interface](#c1-parameter-update--external-interface)

#### C3: Voice GUI Switching — Dynamic Binding Between GUI and DSP
```mermaid
sequenceDiagram
participant User as User
participant GUIHost as PluginEditor.cpp/.h
participant VoiceGUI as VoiceGUI
participant APVTS as State Store
participant Processor as PluginProcessor.cpp/.h
participant VoiceManager as VoiceManager.h

Note over GUIHost,VoiceManager: Dynamic reconfiguration — switch active voice type

User->>GUIHost: select new voice
GUIHost->>VoiceGUI: kill current
GUIHost->>APVTS: detach parameters
GUIHost->>Processor: request VoiceManager to instantiate new voice type
Processor->>VoiceManager: create new subclass
VoiceManager-->>Processor: new voice registered
GUIHost->>APVTS: attach parameters
GUIHost->>VoiceGUI: create new
VoiceGUI->>APVTS: bind controls
APVTS-->>Processor: param's sync'ed
Processor-->>VoiceManager: DSP using new voice
Note over GUIHost,Processor: GUI and DSP now share state through State store
```

## GUI

##
```mermaid
flowchart TD
    User["User Interaction (Knobs, Sliders)"]
    GUI["PluginEditor.cpp/.h<br/>(GUI Thread)"]
    APVTS["AudioProcessorValueTreeState<br/>(State Tree)"]
    Processor["PluginProcessor.cpp/.h<br/>(Audio Thread)"]
    DSP["VoiceManager / DSP Engine"]

    User -->|"G1: Param change"| GUI
    GUI -->|"APVTS.setParameterNotifyingHost"| APVTS
    APVTS -->|"G1: notify"| Processor
    Processor -->|"G3: smoothed params"| DSP
    DSP -->|"G3: write analysis data"| GUI
    APVTS -->|"G2: reflected change"| GUI
```

#### G1: Parameter Update (User → Processor)

When the user touches a knob, the GUI sends the change safely to the processor via the AudioProcessorValueTreeState (APVTS), which synchronizes with automation and the real-time thread.

```mermaid
sequenceDiagram
participant User as Knob/Slider
participant GUI as PluginEditor.cpp/.h
participant APVTS as State store
participant Processor as PluginProcessor.cpp/.h
participant DSP as Audio Engine

Note over GUI,APVTS: GUI thread — user interaction
User->>GUI: adjust parameter
GUI->>APVTS: set param. + notify Host
Note over APVTS,Processor: APVTS syncs safely
APVTS-->>Processor: param. change callback
Processor->>DSP: update target
DSP-->>Processor: next audio block
```

➡ This is the standard control path — it’s what makes knobs automate and record properly in the host.

#### G2: Parameter Reflection (Processor → GUI)

When automation or MIDI CC changes a parameter on the audio thread, the GUI must reflect the new value. JUCE’s APVTS handles this automatically if controls are attached via SliderAttachment, ButtonAttachment, etc.

```mermaid
sequenceDiagram
participant Host as DAW Automation
participant Processor as PluginProcessor.cpp/.h
participant APVTS as State store
participant GUI as PluginEditor.cpp/.h

Note over Host,Processor: Automation / Audio thread
Host->>Processor: set parameter
Processor->>APVTS: updates parameter
Note over APVTS,GUI: APVTS notifies attachments
APVTS-->>GUI: set value from state
GUI->>GUI: redraw control with new value
```

➡ This is the reverse flow — parameters change in the engine, and the GUI follows.

#### G3: Visualization Refresh (DSP → GUI)

This process handles continuous data visualization (waveform preview, envelopes, levels, etc.).
Because the GUI thread can’t directly read DSP memory, this requires a shared lock-free buffer or atomic handoff.

```mermaid
sequenceDiagram
participant DSP as Audio Engine
participant Shared as LockFreeBuffer / Atomic<float>
participant GUI as PluginEditor.cpp/.h (Visualizer)

Note over DSP,GUI: Real-time safe visualization update
DSP->>Shared: write current level / waveform snapshot
GUI->>Shared: read on timer callback (non-realtime)
Shared-->>GUI: latest display data
GUI->>GUI: repaint visualization
```

➡ This is custom logic, separate from the APVTS — used for meters, scopes, oscillators, etc.
