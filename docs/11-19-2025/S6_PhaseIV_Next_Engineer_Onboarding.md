# S6 — Phase IV Next-Engineer Onboarding (Unified, Current)

## 1. Purpose
This onboarding document prepares a new engineer—human or LLM—to begin effective work **mid‑Phase IV**, when VoiceDopp is partially complete (Actions 1–9 implemented and tested), the physics engine is active, and the architecture has evolved beyond classical subtractive synthesis.

It ensures:
- zero ambiguity about the current system
- an accurate understanding of VoiceDopp’s mathematical, architectural, and DSP foundations
- safe continuation of engineering work under Tyler’s Prime Directive
- continuity across long multi‑session development cycles

This document supersedes all earlier S6 versions.

---

## 2. Prime Directive: Mandatory Operating Discipline

Every next engineer must fully internalize:

### 2.1 Hierarchy  
**Human → Directive → Model** — no exceptions.

### 2.2 File Access Protocol  
- Never edit a file without first asking:  
  **“Show me `<path>` as it exists right now.”**
- Only propose minimal patches.
- Never emit speculative rewrites.

### 2.3 Single‑Action Responses  
Respond with one atomic engineering action unless explicitly instructed otherwise.

### 2.4 Deterministic DSP  
All code must preserve:
- block‑synchronous behavior  
- snapshot‑consistent parameter propagation  
- no mid-block parameter reads  
- no thread-unsafe state  
- no allocations inside `render()`  

### 2.5 Test Suite Supremacy  
Tests are the source of truth.  
Logs override narrative reasoning.  
Unexpected test failures → **full‑stop diagnostics**, not patch attempts.

---

## 3. Current Architectural State (Phase IV: Actions 1–9 Complete)

You are entering the project at a moment when **VoiceDopp is real**, not theoretical.  
The following layers are fully implemented and validated:

### 3.1 Kinematic Engine
- listener controls  
- heading + speed mapping  
- velocity vector  
- time accumulator  
- block-synchronous motion integration  

### 3.2 Emitter Geometry & Lattice
- orientation angle φ  
- perpendicular spacing Δ⊥  
- along-line spacing Δ∥  
- grid index (k,m) emitter positioning  
- field of stationary emitters  
- geometry tests fully enforce invariants

### 3.3 Distance & Retarded-Time Physics
You must understand:
```
t_ret = t - r/c
r = ||x_emit - x_listener||
```
Retarded‑time correctness is enforced block‑by‑block by Action 6 tests.

### 3.4 Source Functions
- carrier evaluated at retarded time  
- amplitude envelopes at retarded time  
- field envelope correctness  

### 3.5 Predictive Scoring
This is the core of VoiceDopp’s emitter selection system:
- Predictive horizons τ ∈ {0, H/2, H}  
- Predicted listener positions x̃_L(t + τ)  
- Scoring monotonic in approach geometry  
- Symmetry invariants  
- Deterministic ordering  
- Rank stability  

Action 8 establishes the statistical reliability of selection.

### 3.6 K-Selection + Hysteresis (Action 9)
- enter radius R_in  
- exit radius R_out  
- sticky incumbent  
- challenger threshold δ  
- deterministic tie-breaking  

### 3.7 Phase IV Engine State
VoiceDopp is now a **fully functioning emitter‑field synthesis engine**.

It is integrated but **not yet crossfading** (Action 10) and **not yet audio‑rendering** (Action 11).  
You are entering at this midpoint.

---

## 4. Mental Model Required for Working on VoiceDopp

### 4.1 It Is Not an Oscillator  
VoiceDopp does **geometric field evaluation**.  
Your job is not to reason like subtractive synthesis; your job is to think like a physics engine.

### 4.2 Time Is Retarded  
Everything depends on past-time evaluation:
- envelopes  
- phase  
- field amplitude  
- relative geometry

### 4.3 No Stateful Phase Accumulators  
The phase is computed geometrically:
```
phase_i(t) = φ_i + 2π f ( t - r_i(t) / c )
```
Do NOT introduce phasor incrementers.

### 4.4 Listener Motion Defines Everything  
Heading and speed determine:
- v_L  
- x_L  
- r_i  
- t_ret  
- scoring  
- Doppler warp  
- emitter selection

### 4.5 Symmetry = Sacred  
Action 8 + 9 tests enforce that:
- symmetric emitter geometry  
- symmetric listener paths  
- must produce symmetric scores

Any deviation indicates a geometry or floating‑point bias error.

---

## 5. Project Structure You Must Understand

### 5.1 VoiceManager  
This is the orchestrator:
- block-level mode handling  
- parameter propagation  
- persistent CC cache  
- VoiceDopp instantiation  
- future Dopp-specific routing hooks  
- global RMS gain system

### 5.2 ParameterSnapshot  
Now contains:  
- classical parameters  
- Dopp-model kinematic parameters  
- orientation and density  
- lattice configuration  
- predictive horizon  
- Dopp‑mode scalar constants

Snapshot is the immutable DSP state for each block.

### 5.3 VoiceDopp
Implements:
- physics  
- retarded-time evaluation  
- K-selection  
- scoring  
- lattice geometry  
- emitter context  
- predictive motion  

### 5.4 Tests
The growing test suite includes:
- 1–6 classical geometry  
- 7 source functions  
- 8 predictive scoring  
- 9 hysteresis + selection  
- internal consistency / determinism

These are **canonical invariants**.

---

## 6. How to Continue Work Safely

### Step 1 — Request File Context  
Before touching anything, always:
```
Show me <filename>
```

### Step 2 — Confirm Task Boundaries  
Ask:
```
Do you want a minimal patch, a redesign, or diagnostics only?
```

### Step 3 — Perform Atomic Patch  
Produce the smallest safe patch.

### Step 4 — Run Tests (Mandatory)  
You must run:
```
ctest --output-on-failure
```

Any failure → stop, diagnose, never push onward.

### Step 5 — Commit + Tag  
With message in the format:
```
PhaseIV-ActionN: <description>
```

---

## 7. Known Traps (Critical)

### 7.1 Mid-Block Drift  
Never allow:
- listener heading  
- listener speed  
- lattice orientation  
- density  
- predictive horizon  
to change mid‑render.

### 7.2 Hidden State  
Never introduce:
- phasor accumulators  
- per-emitter cached state not validated by geometry  
- temporal drift outside listener integration  

### 7.3 Incorrect Tie-Breaking  
Symmetric Y emitters must not randomly diverge.  
This is enforced by deterministic ordering tests.

### 7.4 Affine Mapping Errors  
Especially with CC ranges or velocity signs.  
A single sign flip can silently corrupt:
- scoring  
- K‑selection  
- predictive horizons  
- symmetry behavior  

### 7.5 Floating-Point Bias  
When iterating across k,m:
- preserve iteration order  
- avoid accumulating asymmetric error  
- do not reorder loops without explicit need  

---

## 8. What Comes Next (Actions 10–11)

You are inheriting VoiceDopp **mid-build**, with two major action groups remaining:

### 8.1 Action 10 — Crossfades  
Implement emitter-gain envelopes:
- fade-in  
- fade-out  
- smoothing  
- ensure C¹ continuity  
- prevent clicks  
- deterministic under variable buffer sizes  

### 8.2 Action 11 — Full Audio Rendering  
The final step:
- summation of p_i(t)  
- attenuation  
- global RMS gain  
- Dopp-specific gain handling  
- real output from geometry  

Your work must fit seamlessly into all earlier invariants.

---

## 9. Rapid Onboarding Summary (TL;DR)

You must internalize:

1. Snapshot → VoiceManager → VoiceDopp pipeline.  
2. All physics is block-synchronous.  
3. VoiceDopp is a geometry engine, not an oscillator.  
4. Retarded time governs everything.  
5. Scoring + hysteresis determine which emitters matter.  
6. Tests define correctness.  
7. All patches must be minimal and reversible.  

If you internalize these, you can safely continue Phase IV.

---

# End of S6 — Phase IV Next-Engineer Onboarding
