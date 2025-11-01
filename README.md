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

| Step                          | Goal                                                                                                    | Files likely touched                          |
| ----------------------------- | ------------------------------------------------------------------------------------------------------- | --------------------------------------------- |
| **1. Parameter grouping**     | Move these params under a `voiceA_` group in `ParamLayout.cpp` and `ParameterIDs.h`.                    | `Source/params/*`                             |
| **2. Snapshot extension**     | Add `snapshot.voiceA.oscFreq / envAttack / envRelease` fields and copy from APVTS group.                | `ParameterSnapshot.h` & `PluginProcessor.cpp` |
| **3. VoiceManager injection** | Ensure `VoiceManager::startBlock()` and `VoiceA::noteOn()` consume those snapshot values appropriately. | `VoiceManager.h`, `VoiceA.h`                  |
