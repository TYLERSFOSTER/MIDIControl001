# Phase IV Execution Plan (Incremental Test-First)

This document specifies the correct order of implementation and testing for VoiceDopp in **Phase IV**.  
Each stage must **pass its associated tests before proceeding** to the next.

---

## Action 1 — Kinematic API (Initialization)

### Implement:
- `setListenerControls(speedNorm, headingNorm)`
- `getListenerPosition()`
- `getListenerTimeSeconds()`

### Required Tests:
- Voice activation does not crash kinematic subsystem.
- HeadingNorm = 0.5 → θ = 0.
- SpeedNorm = 1.0 → v = v_max.
- Time remains 0 when `render()` is never called.

---

## Action 2 — Time Accumulator

### Implement:
- Per-sample or per-block time increment based on sample rate.
- `timeSeconds_` internal accumulator.

### Required Tests:
- Time increases by exactly 1.0 second when rendering 48,000 samples at 48 kHz.
- Drift < 1e−6 after long runs.
- Resetting voice resets timeSeconds_.

---

## Action 3 — Heading and Speed Mapping

### Implement:
- Heading mapping:  
  θ = 2π * headingNorm – π  
- Speed mapping:  
  v = v_max * speedNorm  
- Unit vector u(θ)

### Required Tests:
- headingNorm=0.5 → θ=0.
- headingNorm=0 → θ=−π.
- headingNorm=1 → θ=π−ε.
- speedNorm=0 → no movement.
- speedNorm=1 → max speed movement.

---

## Action 4 — Listener Trajectory Integration

### Implement:
- x_L(t+Δt) = x_L(t) + v(t) u(θ(t)) Δt  
  (sample or block level)

### Required Tests:
- Straight-line +x motion.
- Straight-line +y motion.
- Diagonal motion.
- Consistency across variable block sizes.
- Zero speed = no movement.

---

## Action 5 — Emitter Lattice Construction

### Implement:
- Emitter coordinates:  
  x_{k,m} = k Δ⊥ n(φ) + m Δ∥ b(φ)
- Orientation vectors n(φ), b(φ)
- Δ⊥ rules for density ρ

### Required Tests:
- For φ=0, lattice aligns with axes.
- For φ=π/2, lattice rotates 90 degrees.
- ρ=0 → only k=0 line is produced.
- Distances between emitters = Δ∥ and Δ⊥.

---

## Action 6 — Distance + Retarded Time

### Implement:
- r_i(t) = ‖x_i – x_L(t)‖
- t_ret = t – r_i/c

### Required Tests:
- t_ret <= t always.
- Approaching emitter → r decreases → t_ret increases.
- Receding emitter → r increases → t_ret decreases.
- Zero listener speed matches baseline r logic.

---

## Action 7 — Source Functions at Retarded Time

### Implement:
- A_env(t_ret)
- A_field(t_ret)
- Carrier: sin(2π f t_ret + φ)

### Required Tests:
- Carrier frequency correct via phase derivative tests.
- Field modulation waveform matches analytic sin curve.
- ADSR stage switching correct for many random t_ret.

---

## Action 8 — Predictive Scoring

### Implement:
- Horizon set τ ∈ {0, H/2, H}
- Predictive positions x̃_L
- Score_i = max over horizons

### Required Tests:
- Higher score when listener moving toward emitter.
- Identical scoring for symmetric geometry.
- Reproducibility across runs.
- Consistent ordering of emitters under identical geometry.

---

## Action 9 — K-Selection with Hysteresis

### Implement:
- Eligibility region r ≤ R_out
- Re-entry region r ≤ R_in
- δ-margin for challenger replacement
- Incumbent protection

### Required Tests:
- Emitters enter correctly when crossing R_in.
- Emitters exit when exceeding R_out.
- Challenger replacement occurs only when score > incumbent + δ.
- Stable K-sized selection.

---

## Action 10 — Crossfades

### Implement:
- g_i ramps:
  - +1/T_xfade for entering
  - −1/T_xfade for leaving

### Required Tests:
- Crossfades monotonic.
- No gain overshoot or undershoot.
- Total gain is smooth across transitions.
- No audio clicks in high-frequency tests.

---

## Action 11 — Audio Rendering

### Implement:
- p(t) = Σ g_i(t) * p_i(t)

### Required Tests:
- No NaNs, infinities, or denormals.
- Stability under long runs.
- Correct attenuation with distance.
- Doppler frequency contours match analytic predictions.

---

## Summary

This plan ensures:

- Mathematically validated behavior at every stage  
- Perfect isolation of failures  
- No propagation of upstream errors into later physics  
- Full reproducibility  
- Physically correct Doppler behavior consistent with the architecture

**Execution rule:**  
Move to the next Action *only after the current Action’s tests pass 100%.  
