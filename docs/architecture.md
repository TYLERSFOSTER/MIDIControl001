# Sound generation

## Highlevel picture of sound generation via MIDI controller

### Flowchart View — Signal Path (Top-down)
```mermaid
flowchart TD
    MIDI["MIDI Controller"]
    PluginProcessor["PluginProcessor.cpp/.h"]
    VoiceManager["VoiceManager.h"]
    Voice["Voice.h"]
    DSP["Oscillator.h<br/>Envelope.h"]
    Output["JUCE AudioBuffer<float> → Host"]

    MIDI -->|"MIDI Note On/Off events"| PluginProcessor -->|"allocate/release Voice"| VoiceManager -->|"init freq + ADSR params"| Voice
    Voice -->|"reset phase + ADSR.noteOn()/noteOff()"| DSP -->|"per-sample envelope & audio samples"| Voice
    Voice -->|"voice audio"| VoiceManager -->|"mix voices"| PluginProcessor -->|"write to buffer"| Output
```

## `Note On` event: *When key is pressed on MIDI controller*

### A1: `Note On` — External Interface
```mermaid
sequenceDiagram
participant MIDI as MIDI Controller (External)
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
participant Ref as `Note On` Internal Processing
participant Buffer as JUCE AudioBuffer<float>

Note over MIDI,Buffer: External communication — plugin entry/exit
MIDI->>PluginProcessor: MIDI Note On event
PluginProcessor->>Ref: hand-off to internal processing
Note over Ref: continues in A2 (see link below)
Ref-->>PluginProcessor: (see next diagram)
PluginProcessor-->>Buffer: writes processed samples to buffer
```

Continues in: [A2: `Note On` — Internal Processing](#a2-`note on`--internal-processing)

### A2: `Note On` — Internal Processing
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over PluginProcessor,DSP: Internal signal flow — Voice Allocation + Start
PluginProcessor->>VoiceManager: allocate Voice for note
VoiceManager->>Voice: initialize Voice (freq, ADSR params)
Voice->>DSP: reset phase + ADSR.noteOn()
DSP-->>Voice: per-sample amplitude envelope
Voice-->>VoiceManager: stream voice audio
VoiceManager-->>PluginProcessor: mix all active voices
Note over PluginProcessor: returns to A1 (see link below)
```

Returns to: [A1: `Note On` — External Interface](#a1-`note on`--external-interface)

## `Note Off` event: *When key is lifted up on MIDI controller*

### B1: `Note Off` — External Interface
```mermaid
sequenceDiagram
participant MIDI as MIDI Controller (External)
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
participant Ref as `Note Off` Internal Processing
participant Buffer as JUCE AudioBuffer<float>

Note over MIDI,Buffer: External communication — plugin entry/exit
MIDI->>PluginProcessor: MIDI Note Off event
PluginProcessor->>Ref: hand-off to internal processing
Note over Ref: continues in B2 (see link below)
Ref-->>PluginProcessor: (see next diagram)
PluginProcessor-->>Buffer: writes updated samples to buffer
```

Continues in: [B2: `Note Off` — Internal Processing](#b2-`note off`--internal-processing)

### B2: `Note Off` — Internal Processing
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h + Envelope.h

Note over PluginProcessor,DSP: Internal signal flow — Release + Cleanup
PluginProcessor->>VoiceManager: locate Voice for note
VoiceManager->>Voice: ADSR.noteOff()
DSP-->>Voice: fade to zero during release
Voice-->>VoiceManager: signal inactive (ADSR completed)
VoiceManager-->>PluginProcessor: remove finished voices
Note over PluginProcessor: returns to B1 (see link below)
```

Returns to: [B1: `Note Off` — External Interface](#b1-`note off`--external-interface)

# Sound modulation

## Highlevel picture of sound modulation via MIDI controller

### Flowchart View — Parameter Signal Path (Top-down)
```mermaid
flowchart TD
    GUI["PluginEditor.cpp/.h"]
    Processor["PluginProcessor.cpp/.h"]
    Smooth["SmoothedValue<br/>(parameter ramping)"]
    VoiceManager["VoiceManager.h"]
    Voice["Voice.h"]
    DSP["Oscillator.h<br/>Envelope.h"]

    GUI -->|"User changes knob / slider (KnobState)"| Processor
    Processor -->|"updates SmoothedValue targets"| Smooth
    Smooth -->|"outputs smoothed values per block"| Processor
    Processor -->|"propagates parameters"| VoiceManager
    VoiceManager -->|"update per-voice settings"| Voice
    Voice -->|"apply freq, amp, env params"| DSP

```

## `Parameter Update` event: *[...]*

### C1: Parameter Update — External Interface
```mermaid
sequenceDiagram
participant GUI as PluginEditor.cpp/.h (External)
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
participant Ref as Parameter Update Internal Processing
participant Buffer as JUCE AudioBuffer<float>

Note over GUI,Buffer: External control — parameter change from GUI to audio thread
GUI->>PluginProcessor: parameter change (KnobState)
PluginProcessor->>Ref: hand-off to internal smoothing and propagation
Note over Ref: continues in C2 (see link below)
Ref-->>PluginProcessor: (see next diagram)
PluginProcessor-->>Buffer: next audio block uses updated smoothed values
```

Continues in: [C2: Parameter Update — Internal Processing](#c2-parameter-update--internal-processing)

### C2: Parameter Update — Internal Processing
```mermaid
sequenceDiagram
participant PluginProcessor as PluginProcessor.cpp/.h (Interface)
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