<p align="left">
  <picture>
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: dark)">
    <source srcset="assets/MPK_Mini.jpg" media="(prefers-color-scheme: light)">
    <img src="assets/MPK_Mini.jpg" alt="tonnetzB" width="400">
  </picture>
</p>

# MIDI Control #001
MIDI synth voice plugin using the JUCE C++ framework

# Next design steps: Individual voices/UIs

| Phase                                 | Objective                                                                                                                                                     | Deliverable                                                              |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| **1. Stabilize DSP hierarchy**        | Promote `Voice.h` → `BaseVoice`; implement concrete `VoiceA`. Add minimal registry/factory for `VoiceManager::makeVoice("voiceA")`. Maintain identical sound. | `BaseVoice`, `VoiceA`, updated `VoiceManager`, `InstrumentRegistry` stub |
| **2. Build dynamic GUI shell**        | Give `PluginEditor` a `std::unique_ptr<BaseVoiceGUI>`. Create abstract GUI interface (`attachToAPVTS`, `paint`, `resized`). Implement swap logic per C3.      | `BaseVoiceGUI`, modified `PluginEditor`                                  |
| **3. Implement first instrument GUI** | Build `VoiceA_GUI` attached to `voiceA_*` parameters; self-contained and modular.                                                                             | `VoiceA_GUI`                                                             |
| **4. Build standalone shell**         | Enable `Standalone` target in CMake. Use same `PluginEditor`. Confirm MPK Mini works for MIDI input.                                                          | `Standalone` build target                                                |
| **5. Add visualizations**             | Introduce lock-free visualization channel; each `VoiceX_GUI` subscribes.                                                                                      | `VisualizerBus` / `LockFreeBuffer`, visualization interfaces             |
| **6. Add new instruments later**      | Register `VoiceB` + `VoiceB_GUI`, create new APVTS group (`voiceB_*`), automatically hot-swap GUI/DSP.                                                        | Multi-instrument support active                                          |
