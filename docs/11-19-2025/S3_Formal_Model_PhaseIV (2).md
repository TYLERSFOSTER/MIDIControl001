# S3 — Formal Model of the VoiceDopp Architecture (Phase IV, Current Truth)

## 0. Purpose
This S3 document is the **authoritative, engineering‑grade formal model** for the **VoiceDopp classical Doppler engine** as implemented and structured during **Phase IV** of the MIDIControl001 project.  
It fully supersedes all earlier S3 documents.

The goal is to give any incoming engineer—human or LLM—the complete ability to reconstruct:

- the mathematical model  
- the DSP model  
- the software architecture  
- the timing rules  
- the per‑block invariants  
- the unit‑test contract  
- the full system ontology  

without reading any prior conversation.

This is the **single source of truth** for the entire VoiceDopp subsystem as of Phase IV.

---

## 1. Global Architectural Overview

VoiceDopp is not an oscillator variation. It is a **geometric acoustic emitter system** embedded into a JUCE polyphonic synthesizer architecture.

### VoiceDopp integrates:

- 2D listener kinematics  
- stationary infinite emitter lattices  
- retarded‑time phase mapping  
- geometric amplitude fields  
- block‑synchronous time integration  
- predictive emitter scoring  
- top‑K hysteretic emitter selection  
- crossfading of emitter sets  
- per‑emitter source evaluation  

All of this runs under the global `VoiceManager` orchestration layer and uses the same block‑synchronous snapshot architecture as every other voice type.

---

## 2. Mathematical Model (Formal)

### 2.1 Listener Kinematics

The listener has:

- position `x_L(t) ∈ ℝ²`
- heading angle θ(t)
- speed v(t)
- velocity vector `v_L(t) = v(t) u(t)` where `u(t) = (cos θ, sin θ)`
- integrated in discrete block time

Discrete update:

```
x_L(t + Δt) = x_L(t) + v_L(t) Δt
```

### 2.2 Emitter Lattice

The emitter field is an oriented, infinite 2D lattice:

```
φ = 2π orientationNorm − π
n = (cos φ, sin φ)
b = (−sin φ, cos φ)
Δ∥ = 1
Δ⊥ = 1 / ρ      if ρ > 0
Δ⊥ = ∞          if ρ = 0
```

Emitter coordinates:

```
x_{k,m} = k Δ⊥ n + m Δ∥ b
```

### 2.3 Distance + Retarded Time

For each emitter i:

```
r_i(t) = || x_i − x_L(t) ||
t_ret = t − r_i(t) / c
```

where `c = 343 m/s`.

### 2.4 Source Functions

Carrier:

```
p_i(t) = sin( 2π f t_ret + φ_i )
```

Field envelope:

```
A_field(t_ret)
```

Amplitude envelope:

```
A_env(t_ret)
```

Final emitter contribution:

```
s_i(t) = A_env(t_ret) A_field(t_ret) p_i(t)
```

### 2.5 Predictive Scoring

For horizon set τ ∈ {0, H/2, H}:

```
x̃_L(t + τ) = x_L(t) + v_L(t) τ
```

Score per horizon:

```
s_i(τ) = f_scoring( x̃_L(t+τ), x_i )
```

Final predictive score:

```
Score_i = max_τ s_i(τ)
```

### 2.6 Top-K Selection With Hysteresis

Emitter is active if:

```
r_i ≤ R_out
```

Emitter enters if:

```
r_i ≤ R_in
```

Challenger replaces incumbent if:

```
Score_challenger > Score_incumbent + δ
```

### 2.7 Crossfades

Each emitter i has gain g_i(t) satisfying:

```
dg_i/dt = ± 1/T_xfade
```

bounded by [0,1].

Final voice output:

```
p(t) = Σ_i g_i(t) s_i(t)
```

---

## 3. Software Architecture (Formal)

### 3.1 C++ Object Model (Top-Level)

```
VoiceDopp : public BaseVoice
```

Internally holds:

- `DoppContext` (physics state shared across block)
- `EmitterField` (lattice construction)
- `EmitterSelector` (predictive scoring + hysteresis)
- `EmitterInstance[K_max]` (active emitters)
- utility structures:
  - `ListenerState`
  - `BlockTime`
  - `DoppParams`
  - `CrossfadeEnvelope`
  - `SourceEvaluator`
  - `RetardedTimeSolver`
  - `PredictiveHorizon`
```

### 3.2 Block Synchronous Flow

For each block:

```
startBlock(snapshot):
    update params
    update listener trajectory
    build/rotate emitter lattice
    compute predictive scores
    select active emitters (with hysteresis)
    update crossfade envelopes
    prepare per-emitter source context
```

In `render()`:

```
for each sample:
    for each emitter:
        compute r_i(t), t_ret
        evaluate envelopes + carrier
        accumulate contribution
```

---

## 4. Parameter Surfaces

### 4.1 Host-Normalized → Physical Map

- speedNorm ∈ [0,1]  
- headingNorm ∈ [0,1]  
- densityNorm ∈ [0,1]  
- orientationNorm ∈ [0,1]  

Mappings:

```
θ = 2π headingNorm − π
v = v_max * speedNorm
φ = 2π orientationNorm − π
ρ = densityNorm
```

### 4.2 VoiceDopp Parameter Group

Per block parameters:

```
struct DoppParams {
    float speedNorm;
    float headingNorm;
    float densityNorm;
    float orientationNorm;
    float fCarrier;
    float T_xfade;
    float R_in;
    float R_out;
    float deltaScore;
    float horizon;
};
```

---

## 5. Timing Model (Formal)

### 5.1 Global Block Time

Block index n, block size B, sample rate Fs:

```
Δt_block = B / Fs
t_n = n * Δt_block
snapshot(t) = snapshot_n for t ∈ [t_n, t_{n+1})
```

### 5.2 Listener Timing

Listener time `t_L` is accumulated only if time accumulation enabled:

```
t_L += Δt_block
```

This ensures deterministic behavior for static tests.

---

## 6. Unit Test Contract

### 6.1 Kinematics (Actions 1–4)

- heading→angle mapping  
- speed→velocity mapping  
- trajectory integration  
- block-time freeze invariants  

### 6.2 Emitter Field (Action 5)

- correct basis vectors  
- Δ⊥ rules  
- orientation semantics  
- lattice coordinate correctness  

### 6.3 Retarded Time (Action 6)

- t_ret ≤ t  
- monotonic correctness under approach/recede  
- stability for large distances  

### 6.4 Source Functions (Action 7)

- accurate sinusoidal retarded phase  
- envelope continuity  
- frequency correctness via Δphase/Δt tests  

### 6.5 Predictive Scoring (Action 8)

- correct horizon geometry  
- deterministic ordering  
- symmetry preservation  
- reproducibility under identical geometry  

### 6.6 K-Selection + Hysteresis (Action 9)

- correct entry/exit logic  
- correct δ-margin handling  
- no flapping  
- stable K-set  

### 6.7 Crossfades (Action 10)

- monotonic gain ramps  
- no overshoot  
- smooth gain sum  

---

## 7. Deterministic Constraints

VoiceDopp must satisfy:

1. No allocations inside render  
2. No mid-block parameter reads  
3. All geometry evaluated from snapshot  
4. Block-synchronous trajectory and predictive solve  
5. Deterministic tie-break ordering  
6. No denormals, infs, or NaNs  

---

## 8. Summary

This S3 captures the **complete formal ontology** of VoiceDopp Phase IV:

- exact math  
- exact DSP  
- exact C++ architectural surfaces  
- timing rules  
- invariants  
- unit test contract  

Nothing else defines the system.

This is the canonical model for all future work.
