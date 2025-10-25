(A) Note-On sequence
```mermaid
sequenceDiagram
participant DAW as DAW (Host)
participant PluginProcessor as PluginProcessor.cpp/.h
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h\n+Envelope.h
participant Output as Audio Buffer (JUCE)

Note over DAW,Output: Note-On Path (Voice Allocation + Start)
DAW->>PluginProcessor: MIDI Note On
PluginProcessor->>VoiceManager: allocate Voice for note
VoiceManager->>Voice: initialize Voice (freq, ADSR params)
Voice->>DSP: reset phase + ADSR.noteOn()
DSP-->>Voice: per-sample amplitude envelope
Voice-->>VoiceManager: stream voice audio
VoiceManager-->>PluginProcessor: mix all active voices
PluginProcessor->>Output: write to DAW output buffer
```

(B) Note-Off sequence
```mermaid
sequenceDiagram
participant DAW as DAW (Host)
participant PluginProcessor as PluginProcessor.cpp/.h
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h
participant DSP as Oscillator.h\n+Envelope.h
participant Output as Audio Buffer (JUCE)

Note over DAW,Output: Note-Off Path (Release + Cleanup)
DAW->>PluginProcessor: MIDI Note Off
PluginProcessor->>VoiceManager: find Voice for note
VoiceManager->>Voice: ADSR.noteOff()
DSP-->>Voice: fade to zero during release
Voice-->>VoiceManager: signal inactive (ADSR completed)
VoiceManager-->>PluginProcessor: remove finished voices
```

(C) Parameter Update
```mermaid
sequenceDiagram
participant GUI as PluginEditor.cpp/.h
participant PluginProcessor as PluginProcessor.cpp/.h
participant VoiceManager as VoiceManager.h
participant Voice as Voice.h

Note over GUI,Voice: Parameter Updates (Real-Time Safe)
GUI->>PluginProcessor: parameter change (KnobState)
PluginProcessor->>PluginProcessor: apply SmoothedValue ramp
PluginProcessor->>VoiceManager: propagate parameter to Voices
VoiceManager->>Voice: update oscillator/envelope settings
Note over PluginProcessor,Voice: all updates are thread-safe and smoothed
```

(D) Flowchart View — Signal Path (Top-down)
```mermaid
flowchart TD
DAW["DAW (Host)"]
PluginProcessor["PluginProcessor.cpp/.h"]
VoiceManager["VoiceManager.h"]
Voice["Voice.h"]
DSP["Oscillator.h\n+Envelope.h"]
Output["JUCE Audio Output Buffer"]

DAW -->|MIDI Note On| PluginProcessor -->|allocate Voice| VoiceManager -->|init freq/ADSR| Voice
Voice -->|reset phase + ADSR.noteOn()| DSP -->|per-sample envelope| Voice
Voice -->|audio samples| VoiceManager -->|mix| PluginProcessor -->|write buffer| Output
```