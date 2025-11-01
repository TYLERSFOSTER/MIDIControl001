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

## Next engineering steps

### Step-by-step plan (no code edits yet)

#### Unify parameter layout
- Delete the inline `createParameterLayout()` from `PluginProcessor.cpp`.
- Use the version from `ParamLayout.cpp`.

#### Expand the snapshot struct
- Add the `std::array<VoiceParams, NUM_VOICES>` to `ParameterSnapshot`.
- Each `VoiceParams` holds `oscFreq`, `envAttack`, `envRelease`.

#### Update `makeSnapshotFromParams()`
- Loop over `NUM_VOICES`.
- Read from prefixed IDs (`voices/voice1/osc/freq`, etc.).
- Fill `snapshot.voices[i]`.

#### VoiceManager `startBlock`
- Store the snapshot.
- For each active voice, call `voice->updateParams(snapshot.voices[index])`.

#### VoiceA
- Accept those parameters in `noteOn()` and `updateParams()`.
- Use the existing hooks in `OscillatorA` and `EnvelopeA`.
