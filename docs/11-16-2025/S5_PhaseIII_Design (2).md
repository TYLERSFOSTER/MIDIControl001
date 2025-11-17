# S5 — Phase III Design, Intent, Risks, and Downstream Consequences

**Project:** MIDIControl001  
**Scope:** Phase III — Voice Mode System, Multi‑Voice Architecture, and Mode‑Aware DSP  
**Author:** ChatGPT (Consultant Engineer) for Tyler Foster  
**Status:** Design locked as of tag `phase3-B9-checkpoint` on `main`

---

## S5.1. Phase III: High‑Level Purpose

Phase III is about turning the plugin from a single‑voice‑type, single‑architecture instrument into a **multi‑voice‑mode synth framework** that can host multiple, structurally distinct voice engines under a shared set of constraints:

1. **Single plugin, multiple physical models**  
   - Today: only `VoiceA` exists (simple, “baseline” subtractive voice).  
   - Phase III: architecture must be ready for additional modes such as:  
     - `VoiceDopp` — classical Doppler voice (retarded time, moving listener).  
     - `VoiceLET` — relativistic Doppler / Lorentz‑Ether‑Theory flavored voice.  
     - `VoiceFM` or other “classical synth” types you might add later.

2. **Hard constraint: Mode changes must be *structurally safe***  
   - No invalid states if the host switches the Voice Mode parameter while audio is running.
   - No dangling pointers, mis‑typed casts, or partial allocations.
   - The design must handle mode changes without requiring a full plugin reload.

3. **Soft constraint: Sonic continuity vs. architectural clarity**  
   - We accept that switching voice modes may cause audible discontinuities (e.g., voices are rebuilt and envelopes restart), at least initially.  
   - What matters most in Phase III is a **clear and safe architecture**, not a perfectly “clickless” morph between fundamentally different voice engines.  
   - Later phases (Phase IV / V) can explore cross‑fading or higher‑level morphing if desired.

4. **Extensibility mandate**  
   - Adding a new voice type should *not* require surgical changes across the codebase.  
   - There must be clearly defined extension points (e.g. factories, interfaces, and documentation) that let “future Tyler” or another engineer implement new modes with minimal surprise.

The Phase III design you have now is the **scaffolding layer** that enforces these constraints without changing any existing audio behavior yet.


---

## S5.2. Ground‑Truth Summary of Where Phase III Stands

As of tag `phase3-B9-checkpoint` (and merged into `main`), the following is true:

1. **`VoiceMode` enumeration exists and is minimal**  
   - Defined in `Source/params/ParameterSnapshot.h` as:

     ```cpp
     enum class VoiceMode : int
     {
         VoiceA = 0,
         // Future: VoiceLET = 1,
         //         VoiceDopp = 2,
         //         ...
     };
     ```

   - Conversion helpers exist:
     - `inline VoiceMode toVoiceMode(int raw)` clamps everything to `VoiceMode::VoiceA`.
     - `inline int toInt(VoiceMode m)` returns `static_cast<int>(m)`.

   - The **key point**: Right now, **only `VoiceA` is a valid, supported runtime mode**. All other potential values are forced back to `VoiceA` by design.

2. **ParameterLayout has a “Voice Mode” parameter**  
   - `Source/params/ParamLayout.cpp` defines an `AudioParameterChoice` for `ParameterIDs::voiceMode` with a `StringArray` containing mode names ordered to match the eventual `VoiceMode` enumeration.  
   - At present, only `"voiceA"` is effective; additional names are architectural scaffolding rather than fully active behavior.

3. **`ParameterSnapshot` carries `VoiceMode`**  
   - `ParameterSnapshot` includes:

     ```cpp
     VoiceMode voiceMode = VoiceMode::VoiceA;
     ```

   - The snapshot for each audio block includes `masterVolumeDb`, `masterMix`, `voiceMode`, and global/voice parameters.

4. **`PluginProcessor` converts parameter → `VoiceMode` → `ParameterSnapshot`**  
   - Inside `Source/plugin/PluginProcessor.cpp`, the Voice Mode parameter’s raw value is read from the APVTS, converted to an integer, then to a `VoiceMode` via `toVoiceMode`, and stored in the `ParameterSnapshot` for that block.

5. **`VoiceManager` stores & reacts to `VoiceMode` structurally (but inertly)**  
   - `Source/dsp/VoiceManager.h` now has:
     - `VoiceMode mode_` — the **current** mode.  
     - `VoiceMode lastMode_` — the **previous** mode, for detecting changes.  
     - A `setMode(VoiceMode)` / `getMode()` pair.  
     - A `VoiceFactory` injection point (`std::function<std::unique_ptr<BaseVoice>(VoiceMode)>`) and `makeVoiceForMode(VoiceMode)` fallback.
     - A `rebuildVoicesForMode()` helper that re‑allocates and prepares all voices for the current mode.
     - A `rebuildVoicesIfModeChanged()` helper called in `startBlock()`.

   - Crucially, **`makeVoiceForMode()` still always returns a `VoiceA`**, and `toVoiceMode` still clamps to `VoiceA`. So **all tests and sonic behavior remain exactly as before**.

6. **Tests confirm zero behavioral drift**  
   - The full battery of 30 tests (EnvelopeA, OscillatorA, VoiceManager, VoiceA vs legacy, APVTS roundtrips, analyzer bridge, etc.) all pass.  
   - No tests yet assert behavior about multiple modes; they only assert that the new scaffolding doesn’t break the current `VoiceA` world.

In short: **Phase III scaffolding for voice modes is fully installed and stable, but only “VoiceA” is active**. All multi‑mode complexity is contained in a small, explicit surface area (enum, layout, snapshot, VoiceManager factory & rebuild logic).


---

## S5.3. Design Surfaces Introduced in Phase III

This section spells out the *exact points* where Phase III touches the codebase—these are the places a future engineer must understand before altering modes or adding new voice types.

### S5.3.1. `VoiceMode` Enumeration & Conversions

**File:** `Source/params/ParameterSnapshot.h`

- Holds the canonical enumeration of voice modes.  
- Hard requirement: **Ordering and values must match the `AudioParameterChoice` options in `ParamLayout.cpp`**.

**Responsibilities:**

1. Provide a canonical list of modes:
   ```cpp
   enum class VoiceMode : int
   {
       VoiceA = 0,
       // Future: VoiceLET = 1,
       //         VoiceDopp = 2,
       //         VoiceFM   = 3,
       //         ...
   };
   ```

2. Provide safe conversion from `int` (APVTS index) to `VoiceMode`:
   ```cpp
   inline VoiceMode toVoiceMode(int raw)
   {
       switch (raw)
       {
           case 0:
           default:
               return VoiceMode::VoiceA;
       }
   }
   ```

3. Provide `VoiceMode → int`:
   ```cpp
   inline int toInt(VoiceMode m)
   {
       return static_cast<int>(m);
   }
   ```

**Phase III intent:**  
- For now, **all non‑zero indices are treated as `VoiceA`**.  
- When new modes are made real, `toVoiceMode` can be safely expanded with a `switch` that explicitly recognizes each new mode while still clamping unknown values to a default (likely `VoiceA`).


### S5.3.2. Parameter Layout: Host‑Facing Voice Mode Parameter

**File:** `Source/params/ParamLayout.cpp`

- Introduces or extends `AudioParameterChoice` for `voiceMode`.  
- The `StringArray` of names (e.g. `"voiceA"`, `"VoiceDopp"`, `"VoiceLET"`, `"VoiceFM"`, etc.) must **mirror the enum index order**.  
- This is the **only way** the host knows which mode is which, and how to display them in the UI.

**Key invariants:**

- Index 0 must always correspond to `VoiceMode::VoiceA`.  
- When you add new modes, you append to the list in a **forward‑compatible** way—avoid reordering.



### S5.3.3. `ParameterSnapshot`: Block‑Level Mode Capture

**File:** `Source/params/ParameterSnapshot.h`

- `ParameterSnapshot` is the representation of “what the APVTS parameters were for this audio block, in a form that can be cheaply copied and passed around”.  
- The addition here is `VoiceMode voiceMode`.  
- That value is set once per block in `PluginProcessor` and then passed by reference into `VoiceManager` and eventually `BaseVoice` / `VoiceA` implementations.

**Why this is crucial:**

- It ensures that **all voices in a block see the same mode**.  
- We avoid reading directly from APVTS inside voice processing, which is both thread‑unsafe and conceptually messy.


### S5.3.4. `PluginProcessor`: Parameter → Snapshot → VoiceManager

**File:** `Source/plugin/PluginProcessor.cpp`

High‑level logic (simplified):

1. Read `voiceMode` raw from APVTS (`getRawParameterValue` / `getParameter` etc.).  
2. Convert to `int` index and then to `VoiceMode` via `toVoiceMode`.  
3. Populate `ParameterSnapshot` with that `VoiceMode`.  
4. Pass that snapshot to `VoiceManager` via the `SnapshotMaker` callback.

The main design principle:  
> The plugin processor knows about the APVTS and host, the `VoiceManager` knows about snapshots and modes, and the voices know nothing about the host.


### S5.3.5. `VoiceManager`: Mode‑Aware Allocation and Rebuild

**File:** `Source/dsp/VoiceManager.h`

New responsibilities introduced in Phase III:

1. **Store the current mode:**
   ```cpp
   VoiceMode mode_     = VoiceMode::VoiceA;
   VoiceMode lastMode_ = VoiceMode::VoiceA;
   ```

2. **Expose `setMode` and `getMode`:**
   ```cpp
   void setMode(VoiceMode m) { mode_ = m; }
   VoiceMode getMode() const noexcept { return mode_; }
   ```

3. **Optional injected factory:**
   ```cpp
   using VoiceFactory = std::function<std::unique_ptr<BaseVoice>(VoiceMode)>;

   explicit VoiceManager(SnapshotMaker makeSnapshot,
                         VoiceFactory voiceFactory = {});
   ```

4. **Internal factory fallback (`makeVoiceForMode`):**
   ```cpp
   std::unique_ptr<BaseVoice> makeVoiceForMode(VoiceMode mode) const
   {
       juce::ignoreUnused(mode);
       return std::make_unique<VoiceA>();
   }
   ```

5. **Centralized voice rebuild helper:**
   ```cpp
   void rebuildVoicesForMode()
   {
       voices_.clear();
       voices_.reserve(maxVoices);

       auto makeVoice = [this](VoiceMode m)
       {
           if (voiceFactory_)
               return voiceFactory_(m);
           return makeVoiceForMode(m);
       };

       for (int i = 0; i < maxVoices; ++i)
       {
           auto v = makeVoice(mode_);
           v->prepare(sampleRate_);
           voices_.push_back(std::move(v));
       }
   }
   ```

6. **Mode change detection in `startBlock`:**
   ```cpp
   void startBlock()
   {
       static ParameterSnapshot snapshot;
       snapshot = makeSnapshot_();

       rebuildVoicesIfModeChanged(); // Phase III B8

       applyModeConfiguration();     // currently inert

       // Reapply CC cache, update per‑voice params, etc.
   }
   ```

7. **Mode change tracker:**
   ```cpp
   void rebuildVoicesIfModeChanged()
   {
       if (mode_ == lastMode_)
           return;

       lastMode_ = mode_;
       rebuildVoicesForMode();
   }
   ```

This is the **core of the Phase III scaffolding**: a safe pattern for atomically switching from one family of voice types to another at block boundaries.


---

## S5.4. Intent: Why This Design and Not Something Else?

Here is the explicit design rationale and trade‑off analysis, turned into a series of “why not X?” questions.

### 1. Why not just let the voices read the mode parameter directly?

Because that would scatter host‑facing concern across the DSP layer:

- Voices would have to know how to map APVTS values to an enum.  
- You’d risk per‑sample parameter reads, thread safety issues, and unpredictable timing.  
- Tests would become fragile because any change to parameter naming or layout could break deep DSP code unexpectedly.

**Snapshot + VoiceManager** ensures a clear dependency direction:
- APVTS → Processor → Snapshot → VoiceManager → Voices.


### 2. Why rebuild the entire voice array on mode change?

Because different voice modes can require **fundamentally different internal state**, e.g.:

- Additional oscillators, envelopes, delay lines, state filters.  
- Different notion of what “time” even means (`VoiceDopp` with retarded time vs. a simple ADSR voice).  
- Different controller mappings, modulation paths, sample buffers, etc.

Attempting to mutate an existing `VoiceA` into a `VoiceDopp` in place would:
- Lead to complicated transitional states and “half‑mutated” objects.  
- Force all voice types to share a giant mega‑class with an explosion of conditionals.

By contrast, **full rebuild on mode change** is:

- Simple to reason about.  
- Safe: no stale members or dangling pointers.  
- Localized: one function, `rebuildVoicesForMode`, handles the whole transformation.

The cost is that **current notes are dropped** during the transition. For Phase III, we accept this cost in exchange for clarity and safety.


### 3. Why not embed each voice type as a member of `VoiceManager` and just switch a pointer?

That would make `VoiceManager` a grab‑bag container of different concrete voice arrays (e.g. vectors for `VoiceA`, `VoiceDopp`, etc.) and add complexity at every call site (which list do we route to right now?).

Instead, the design uses:

- A single `std::vector<std::unique_ptr<BaseVoice>> voices_`.  
- A simple factory that returns `std::unique_ptr<BaseVoice>` based on `VoiceMode`.  

This keeps the processing loop generic:
```cpp
for (auto& v : voices_)
    v->render(buffer, numSamples);
```

and moves all complexity to controlled extension points.


### 4. Why is `rebuildVoicesIfModeChanged` called in `startBlock` and not e.g. inside `setMode`?

Because API calls like `setMode` may happen on the **message thread**, while `startBlock` is called on the **audio thread**.

You do **not** want to rebuild voices on the message thread while the audio thread is potentially mid‑render:

- It introduces race conditions and torn state between threads.  
- It’s harder to reason about whether a given rebuild is safely “visible” to processing.

By restricting the rebuild to `startBlock`:

- You ensure voice transitions are aligned to block boundaries.  
- The same thread that processes audio (or its associated callback) handles the voice array mutation.

This is a major safety property of the Phase III design.


---

## S5.5. Risks and Failure Modes

Here are the key risks introduced by Phase III, along with how the current design handles (or at least localizes) them.

### Risk 1: Mode inconsistency between host, snapshot, and voices

**Scenario:**  
- Host switches the Voice Mode parameter, but due to a bug:
  - `ParameterSnapshot.voiceMode` is not updated correctly, or  
  - `VoiceManager::setMode` isn’t called, or  
  - `rebuildVoicesIfModeChanged` isn’t invoked.

**Consequence:**  
- Voices may run in a mode that doesn’t match what the UI or host thinks.  
- Automation and presets may behave strangely.

**Mitigation provided by design:**

- All mode‑related behavior flows through a **narrow interface**:
  - APVTS → `voiceMode` param → integer index → `toVoiceMode` → snapshot → `VoiceManager::setMode` → `rebuildVoicesIfModeChanged`.  
- Mis‑wiring is easier to detect via targeted tests on exactly this flow.  
- `toVoiceMode` currently clamps everything to `VoiceA`, which essentially gives you a “safe default” even if the host sends unexpected indices.

### Risk 2: Runtime crashes due to incorrect casting or missing factory cases

**Scenario (future):**  
- You add `VoiceDopp` and update the factory in `VoiceManager`.  
- Somewhere, you still `dynamic_cast<VoiceA*>` and then dereference without checking null.  
- Or you forget a `case VoiceMode::VoiceDopp:` in `makeVoiceForMode`, returning a `nullptr` or wrong type.

**Consequence:**  
- Crashes, UB, or silent misbehavior when switching modes.

**Mitigation provided by design:**

- Currently, `startBlock` only `dynamic_cast`s to `VoiceA` because `VoiceA` is the only real voice type.  
- Once new types exist, the plan is to:
  - Move common parameter updates into `BaseVoice` virtual methods where possible, and/or  
  - Use mode‑specific code paths that explicitly handle only the appropriate type.

**Action item for future work:**  

- Introduce tests that simulate mode changes and verify that voice arrays are valid and safe after each transition.  
- Replace `dynamic_cast<VoiceA*>` patterns with polymorphic parameter update APIs when additional modes go live.

### Risk 3: Dropouts and audible discontinuities on mode changes

**Scenario:**  
- Host automates Voice Mode mid‑phrase.  
- `rebuildVoicesForMode` gets called in `startBlock`, clearing all voices.  
- All envelopes and state are reset; audio may jump or drop to silence for a block.

**Consequence:**  
- Audible pops, gaps, or unnatural transitions when automating Voice Mode.

**Mitigation / design stance:**  

- For Phase III, this is considered **acceptable** and **expected**, because mode changes correspond to **fundamental architecture changes**, not subtle tone tweaks.  
- If future design aims for smooth morphs, this will be addressed in a later phase (Phase IV/V) with cross‑fades or dual‑mode overlap windows.

### Risk 4: Long‑term maintenance: “modes sprawl”

**Scenario:**  
- Over time, you add `VoiceDopp`, `VoiceLET`, `VoiceFM`, maybe others.  
- Each has unique parameter requirements, CC mappings, test suites, etc.  
- If not carefully managed, code becomes harder to understand and reason about.

**Mitigation in the current scaffolding:**

- Phase III centralizes all mode‑related branching in a **small, identifiable set of locations**:
  - `VoiceMode` enum and `toVoiceMode` / `toInt`.  
  - `ParamLayout` names & ordering.  
  - `VoiceManager` factory and `rebuildVoicesForMode`.  
- Additional documentation (S3, S6, S7) already starts to outline how to extend and test new modes.

**Future recommendation:**  

- Maintain a **Mode Registry section** in docs for every new voice type:
  - What it does.  
  - What its parameters are.  
  - How it behaves under CC control.  
  - How it differs from `VoiceA`.  
- Add explicit tests per mode.


---

## S5.6. Downstream Consequences for Phase IV and Beyond

Phase III’s scaffolding has several explicit downstream consequences:

### Consequence 1: Voice types become first‑class citizens

- The plugin is no longer “a single voice type that might change a few internal flags”.  
- Instead, each voice type is a full, self‑contained implementation of `BaseVoice` with a distinct physical or algorithmic model.

This enables:

- Experimental physics‑based modes without fear of contaminating the “baseline” voice logic.  
- Clear mapping between conceptual modes (Doppler, relativistic, FM, etc.) and the actual code structure.


### Consequence 2: The analyzer pipeline and tests can be made mode‑aware

- You already have tests like “VoiceA matches VoiceLegacy output”, analyzer bridges, RMS alignment checks, etc.  
- With the mode scaffolding in place, you can extend these pipelines to:

  - Compare `VoiceDopp` to an analytic Doppler reference.  
  - Compare `VoiceLET` output to simulated Lorentz‑boosted signals.  
  - Assert invariants like “when mode is X, the output envelope behaves as Y”.

Phase III is the prerequisite for this; without a `VoiceMode` surface, tests would have to hack parameters or rely on structural details rather than declarative “mode identity”.


### Consequence 3: UI and preset systems can now express structural choices

- Once additional names in `AudioParameterChoice` are *backed by actual voice types*, hosts and preset systems can express choices like:
  - “Use the relativistic Doppler voice for this patch.”  
  - “Switch from classical to relativistic for the chorus.”

Even before full implementation, **the parameter is already present and stable**, so presets that store a Voice Mode value are forward‑compatible with future versions that make those modes real.


### Consequence 4: Device integration and external control (e.g., MPK Mini CCs) can branch based on mode

- In the future, CC mappings don’t have to be identical across all modes.  
- You might, for instance:
  - Make CC3/4/5 control envelope on `VoiceA`.  
  - Make them drive Doppler velocity, acceleration, or observer position on `VoiceDopp`.  
  - Use them for rapid morphing of relativistic parameters for `VoiceLET`.

Phase III gives you the architectural leverage to *define those behaviors cleanly* in mode‑specific code paths.


---

## S5.7. Concrete Examples of Extending the Phase III Design

To make the design more “graspable”, here are some concrete, worked‑through examples of how the Phase III scaffolding would be used to add new functionality.

### Example 1: Adding `VoiceDopp` as a second mode (high‑level sketch)

Steps (conceptual, not actual patch):

1. **Define the new enum case:**
   ```cpp
   enum class VoiceMode : int
   {
       VoiceA   = 0,
       VoiceDopp = 1,
   };
   ```

2. **Update `toVoiceMode`:**
   ```cpp
   inline VoiceMode toVoiceMode(int raw)
   {
       switch (raw)
       {
           case 0: return VoiceMode::VoiceA;
           case 1: return VoiceMode::VoiceDopp;
           default: return VoiceMode::VoiceA;
       }
   }
   ```

3. **Add `"VoiceDopp"` to the `AudioParameterChoice` list** in `ParamLayout.cpp` in the correct index position (1).

4. **Implement `VoiceDopp` deriving from `BaseVoice`:**
   - With its own `prepare`, `noteOn`, `render`, `handleController`, etc.  
   - Possibly using the mathematical spec you wrote for retarded‑time Doppler.

5. **Extend `makeVoiceForMode`:**
   ```cpp
   std::unique_ptr<BaseVoice> makeVoiceForMode(VoiceMode mode) const
   {
       switch (mode)
       {
           case VoiceMode::VoiceDopp:
               return std::make_unique<VoiceDopp>();

           case VoiceMode::VoiceA:
           default:
               return std::make_unique<VoiceA>();
       }
   }
   ```

6. **Optionally adjust `applyModeConfiguration`** to set up global DSP paths or shared state specific to `VoiceDopp` (e.g., global Doppler reference frame).

7. **Add tests:**
   - Verify that when Voice Mode is set to `VoiceDopp`, `VoiceManager` allocates `VoiceDopp` instances.  
   - Add an analyzer test comparing `VoiceDopp` output to a known Doppler reference.  

Because Phase III has already done the hard work (mode enum, snapshot, mode‑aware `VoiceManager`), each of these steps is local and contained.


### Example 2: Adding a registry/”mode descriptor” structure

In a further evolution, you might define something like:

```cpp
struct VoiceModeDescriptor
{
    VoiceMode mode;
    juce::String name;
    juce::String shortDescription;
    juce::String longDescription;
};
```

and keep a small static table, which both:

- `ParamLayout.cpp` and  
- The documentation / onboarding material

can read from. But even without this, Phase III’s scaffolding is already aligned with that direction.


---

## S5.8. Summary of Phase III Design Decisions

1. **Voice modes are explicit, finite, and centrally defined** via `VoiceMode`.  
2. **The host sees modes via `AudioParameterChoice` in `ParamLayout.cpp`**, tied to that enum’s ordering.  
3. **Per‑block snapshots carry `VoiceMode` into `VoiceManager`**, which owns all mode‑aware allocation and transition logic.  
4. **`VoiceManager` rebuilds its voice array when `mode_` changes**, using either an injected factory or `makeVoiceForMode`.  
5. **All multi‑mode behavior is currently inert**—everything still produces `VoiceA`—so Phase III introduces *no sonic or behavioral changes*, only architecture.  
6. **Risks are contained and documented**, with a clear path to testing and extension for future modes.

From here, **Phase IV and beyond** can safely implement advanced voice types (`VoiceDopp`, `VoiceLET`, `VoiceFM`, etc.) on top of this scaffold, knowing that:

- The host→APVTS→Snapshot→VoiceManager→Voice chain is stable.  
- Mode changes are handled at well‑defined boundaries.  
- Tests can be grown as each new mode becomes real, without rewriting the core architecture.

This is the core content that future engineers—or “future Tyler”—must internalize when thinking about Phase III and everything that follows it.
