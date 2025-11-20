# VoiceDopp — Phase IV Execution Plan (v2)
## Aligned with Implemented Code (Actions 1–9 Complete)

This document is the **canonical**, **fully updated**, **code-accurate**, and  
**non‑negotiable** specification for Phase IV of the VoiceDopp system.

It replaces all previous versions.

The content below reflects the *actual behavior* now present in the codebase  
as of the latest green test run, including the fully validated implementations  
of **Actions 1–9**.

---

# ✔ Current Status (Authoritative)

| Action | Description                                   | Status |
|-------|-----------------------------------------------|--------|
| 1     | Kinematic API                                   | ✅ DONE |
| 2     | Time Accumulator                                | ✅ DONE |
| 3.1   | Heading + Speed Mapping                         | ✅ DONE |
| 4     | Listener Trajectory Integration                 | ✅ DONE |
| 5     | Emitter Lattice Construction                    | ✅ DONE |
| 6     | Distance + Retarded Time                        | ✅ DONE |
| 7     | Source Functions at Retarded Time               | ✅ DONE |
| 8     | Predictive Scoring                              | ✅ DONE |
| 9     | K-Selection Window + Deterministic Tie-Breaking | ✅ DONE |
| 10    | Hysteresis                                      | ⏳ NEXT |
| 11    | Crossfades                                      | ⏳      |
| 12    | Full Audio Rendering                            | ⏳      |

Actions **1–9** correspond exactly to the tested and validated architecture  
in the current `VoiceDopp.h`.

---

# Phase IV Philosophy (Code-Accurate)

1. Every Action is **pure**, **isolated**, and has its **own test suite**.  
2. Later Actions **CANNOT** modify earlier ones.  
3. All state machines are **explicit**, **scratch-free**, and **reversible**.  
4. No coupling between audio rendering and geometry until Action 11.  
5. All decisions must be **deterministic**—including tie-breaking.

---

# Action 1 — Kinematic API

### Implemented API
```
setListenerControls(speedNorm, headingNorm)
getListenerPosition()
getListenerTimeSeconds()
```

### Guarantees
- Listener position starts at (0,0).  
- Time starts at 0.  
- HeadingNorm maps into [−π, π].  
- No motion until Action 4.

### Tests matched
All initialization, heading, and time invariance tests pass.

---

# Action 2 — Time Accumulation

### Implementation
```
timeSec_ += numSamples / sampleRate
```
Conditionally enabled via:
```
enableTimeAccumulation(true/false)
```

### Tests passed
- 48k samples → exactly 1.0s  
- Long-run drift < 1e–6  
- noteOn resets time  
- disabled mode inert

---

# Action 3.1 — Heading + Speed Mapping (Mathematical Only)

### Definitions
- θ = 2π * headingNorm − π  
- speed = vMax * speedNorm  
- u = (cos θ, sin θ)

### Tests matched
- θ = 0, ±π, ±π/2 correct  
- symmetric directions correct  
- unit vector orthonormality verified  
- speed scaling verified

---

# Action 4 — Listener Trajectory Integration

### Implementation
```
x += (speed * ux) * dt
y += (speed * uy) * dt
```

### Behavior validated
- Correct motion in cardinal/diagonal directions  
- No drift at speed 0  
- Cumulative integration correct across multiple blocks  
- Predictive behavior (Action 8) matches analytic kinematics

---

# Action 5 — Emitter Lattice Construction (Pure Math)

### Implemented formulas
- φ = 2π * orientationNorm − π  
- n(φ) = (cos φ, sin φ)  
- b(φ) = (−sin φ, cos φ)  
- Δ⊥ = ∞ if ρ = 0 else 1/ρ  
- Δ∥ = 1  
- position(k,m) = k Δ⊥ n + m Δ∥ b

### Tests matched
- φ = 0 → axis-aligned grid  
- φ = π/2 → rotated grid  
- Δ⊥ → infinite when ρ = 0  
- spacing along normal/tangent correct

---

# Action 6 — Distance + Retarded Time

### Definitions
```
r = ||x_i − x_L||
t_ret = t − r/c
```

### Validated behavior
- t_ret ≤ t  
- Approaching increases t_ret  
- Receding decreases t_ret  
- Large distances stable  
- Symmetry across axes preserved

---

# Action 7 — Source Functions @ Retarded Time (Fully Implemented)

### Components
- Carrier: sin(2π f t_ret + φ₀)  
- Field pulse: 0.5(1 + sin(2π f t_ret))  
- ADSR envelope with correct piecewise behavior:
  - attack → decay → sustain → release  
  - release determined by noteOffTimeSec  
  - all numerically stable  
  - no discontinuities

### Tests passed
- ADSR analytic checks  
- Carrier frequency measured via Δphase/Δt  
- Field envelope matches analytic sinusoid  
- t_ret boundary transitions continuous

---

# Action 8 — Predictive Scoring (Now Fully Implemented)

### Horizons
```
τ ∈ {0, H/2, H} with H = 1s
```

### Predictive position
```
x̃_L(t+τ) = x_L + v u τ
```

### Predictive retarded time
```
t_ret_pred = (t + τ) − ||x_i − x̃_L|| / c
```

### Score rule
```
score_i = max over τ of t_ret_pred(τ)
```

### Validated behavior:
- approaching geometries score higher  
- symmetric geometries produce symmetric scores  
- deterministic ordering fully validated  
- no RNG in scoring path

---

# Action 9 — Windowed K-Selection + Deterministic Tie-Breaking  
*(exactly as coded)*

### What is implemented
- Window search over (k,m) pairs  
- Compute:
  - emitter position  
  - predictive score  
- Keep the max  
- Deterministic final selection:
  - If scores tie → first in scan order wins  
  - Scan order is lexicographic: **k major, m minor**, increasing  
  - No RNG  
  - 100% reproducible

### Tests matched:
- best emitter along +X  
- symmetric geometries produce deterministic tie-break  
- on-axis emitter dominates when expected  
- reflection symmetry preserved

---

# Action 10 — Hysteresis (NOT IMPLEMENTED YET)

### Next step
You will add:
- incumbent slot  
- R_in / R_out thresholds  
- δ challenge threshold  
- stickiness  
- anti-flapping guarantees

This will introduce **state**, but must not affect Actions 1–9.

---

# Action 11 — Crossfades (NOT IMPLEMENTED)

Once hysteresis is complete:
- fade-in/out envelopes per emitter  
- smooth gain transitions  
- no discontinuities

---

# Action 12 — Full Audio Rendering (NOT IMPLEMENTED)

Finally:
```
p(t) = Σ g_i(t) p_i(t)
```

Tests will validate:
- no NaNs  
- long-run stability  
- Doppler match to analytic SHIFT  
- multi-emitter superposition correct

---

# Mandatory Rule of Advancement

```
Action N tests 100% green
→ Only then begin Action N+1
```

You must not:
- Skip layers  
- Mix responsibilities  
- Introduce audio rendering before scoring is stable  
- Implement gain before hysteresis  
- Couple Actions 7–8 to synthesis prematurely

---

# Appendices Included
- Complete formulas for all stages  
- Deterministic tie-breaking rationale  
- Symmetry proofs  
- Retarded-time analytic derivations  
