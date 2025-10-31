<p align="left">
  <picture>
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: dark)">
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: light)">
    <img src="assets/MPK_Mini.jpg" alt="tonnetzB" width="400">
  </picture>
</p>

# MIDI Control #1
MIDI synth voice plugin using the JUCE C++ framework

See the full design document in [architecture.md](docs/architecture.md).

## Next design steps

### Current symptom summary
#### Issue 1.a — Keyboard notes not changing pitch

- Observation: Every MPK Mini key produces the same oscillator frequency.

- Expected: Each MIDI note should map to frequency = 440 × 2^((note − 69)/12).

**Likely failure layer:** the note number argument (e.g. midiNoteNumber) isn’t being used in VoiceA::noteOn() or is being overwritten / ignored after conversion.

**Possible secondary cause:** envelope triggers but oscillator frequency never updated per note.

#### Issue 1.b — Knobs send no control changes

- Observation: Turning MPK Mini knobs doesn’t affect parameters like gain or attack.

- Expected: CC messages should reach AudioProcessor::processBlock() or a MIDI listener and map to APVTS parameters.

**Likely failure layer:** CC→parameter mapping not implemented (no handleController hook, or wrong CC numbers).

**Alternate possibility:** Standalone host’s MIDI-input routing is restricted to note messages only.

#### Issue 2 — Duplicate / inert sliders in the GUI

- Observation: Standalone GUI shows three identical slider sections; most sliders do nothing, only top “gain” and maybe one other respond.

- Expected: One coherent parameter group (voiceA_*) bound to a single APVTS.

**Likely failure layer:** Multiple parameter groups registered to APVTS (duplicate IDs), so only the first copy is live.

**Or** PluginEditor constructs multiple voice panels instead of one active VoiceA_GUI.

**Or** sliders are created but never attached (SliderAttachment missing or destroyed).