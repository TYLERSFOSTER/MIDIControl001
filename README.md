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


## Next design steps: Individual voices/UIs

| Phase                                 | Objective                                                                                                                                                     | Deliverable                                                              |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| **1. Stabilize DSP hierarchy**        | Promote `Voice.h` → `BaseVoice`; implement concrete `VoiceA`. Add minimal registry/factory for `VoiceManager::makeVoice("voiceA")`. Maintain identical sound. | `BaseVoice`, `VoiceA`, updated `VoiceManager`, `InstrumentRegistry` stub |
| **2. Build dynamic GUI shell**        | Give `PluginEditor` a `std::unique_ptr<BaseVoiceGUI>`. Create abstract GUI interface (`attachToAPVTS`, `paint`, `resized`). Implement swap logic per C3.      | `BaseVoiceGUI`, modified `PluginEditor`                                  |
| **3. Implement first instrument GUI** | Build `VoiceA_GUI` attached to `voiceA_*` parameters; self-contained and modular.                                                                             | `VoiceA_GUI`                                                             |
| **4. Build standalone shell**         | Enable `Standalone` target in CMake. Use same `PluginEditor`. Confirm MPK Mini works for MIDI input.                                                          | `Standalone` build target                                                |
| **5. Add visualizations**             | Introduce lock-free visualization channel; each `VoiceX_GUI` subscribes.                                                                                      | `VisualizerBus` / `LockFreeBuffer`, visualization interfaces             |
| **6. Add new instruments later**      | Register `VoiceB` + `VoiceB_GUI`, create new APVTS group (`voiceB_*`), automatically hot-swap GUI/DSP.                                                        | Multi-instrument support active                                          |

### Incremental sequence
| Step  | Action                                                                                                                                    | Test milestone                                                                                                                              | Validation                                       |
| ----- | ----------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| **1** | Snapshot current branch, run all existing `Tests/dsp/test_VoiceManager.cpp` and `test_Voice.cpp`.                                         | Baseline regression: save current test outputs (waveform hashes, RMS values, etc.)                                                          | Ensures a known-good state before any change.    |
| **2** | Create `BaseVoice` header by renaming old `Voice.h`, but keep the old `Voice` implementation temporarily as `VoiceLegacy` for comparison. | Add a small `test_BaseVoice_interface.cpp` to assert pure-virtual contract (compile-time check only).                                       | Verifies interface layer compiles cleanly.       |
| **3** | Move DSP code from `VoiceLegacy` into `VoiceA`.                                                                                           | Duplicate the existing `test_Voice.cpp` as `test_VoiceA_equivalence.cpp` that feeds identical input and compares sample output (RMS ±1e-6). | Confirms that `VoiceA` produces identical sound. |
| **4** | Update `VoiceManager` to use `BaseVoice` pointers, but still hard-create `VoiceA`.                                                        | Extend `test_VoiceManager.cpp` with polymorphic instantiation test (ensure virtual dispatch works).                                         | Prevents allocation or type-casting regressions. |
| **5** | Add `InstrumentRegistry` with `"voiceA"` entry.                                                                                           | Add `test_InstrumentRegistry.cpp` verifying that `makeVoice("voiceA")` returns a working instance.                                          | Locks down factory behavior early.               |
| **6** | Remove `VoiceLegacy`. All old tests should pass.                                                                                          | Run full `ctest`.                                                                                                                           | Confirms parity with pre-refactor output.        |
