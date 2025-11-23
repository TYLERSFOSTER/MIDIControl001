# VoiceDopp Phase IV Mathematical Specification (Architecture-Aligned)

**Project:** MIDIControl001  
**Module:** `VoiceDopp`  
**Scope:** Physics model reformulated to match the actual plugin architecture (VoiceManager, APVTS, blockwise DSP, CC routing).

---

## 0. Time, Blocks, and Continuous-Discrete Translation

### Continuous Model (conceptual)

All physical quantities are still defined as continuous functions of time:

- Listener trajectory: \( x_L(t) \in \mathbb{R}^2 \)
- Listener velocity: \( v_L(t) = \dot{x}_L(t) \)
- CC streams: \( CC_k(t) \in [0,1] \)
- Envelope, field pulse, and carrier functions evaluated at retarded time

### Plugin Reality (implementation model)

The plugin uses:

- A blockwise DSP callback (`processBlock`) of size \( N \)
- MIDI events timestamped within the block
- Per-voice integrators updated either per-sample or per-block
- CC changes processed when encountered during the block

**Interpretation:**  
All continuous functions are represented as *piecewise constant* (or piecewise linear) over time:

- Listener velocity & heading are constant over each audio block.
- CC values change only at MIDI event timestamps.
- Integration of trajectory is numerical:
  \[
  x_L(t + \Delta t) = x_L(t) + v(t)\,u(	heta(t))\,\Delta t.
  \]
- Retarded time, distances, and emitter scoring use the current block’s approximated state.

This is a mathematically valid discretization of the original model.

---

## 1. Control Mapping (Mode-Dependent)

The plugin architecture routes MIDI CC through VoiceManager, and CC interpretation must depend on **voiceMode**.

### VoiceDopp CC interpretation

When `voiceMode == VoiceDopp`, CC semantics are:

| CC | Meaning | Usage |
|----|---------|-------|
| CC1 | Master Volume | handled in APVTS (global) |
| CC2 | Mix | handled in APVTS (global) |
| CC3 | (reserved or ADSR override if desired) | optional |
| CC4 | Field pulse frequency \($\mu_{\text{pulse}}$\) | sampled at note-on |
| CC5 | Listener speed scalar \($s(t)$\) | blockwise-updated |
| CC6 | Listener heading \($\eta(t)$ \) | blockwise-updated |
| CC7 | Lattice orientation \($\varphi$\) | sampled at note-on |
| CC8 | Lattice density | sampled at note-on |

### Note-On Sampling

For note-on events:

- Lattice parameters \( arphi, 
ho, \mu_{	ext{pulse}} \) are sampled **at the exact MIDI timestamp** inside the block.
- VoiceDopp receives these values via:
  - `noteOn(...)`
  - internal CC state that was updated *before* processing the note-on event.

This matches plugin event ordering.

---

## 2. Listener Motion & Integration (Blockwise)

### Heading and speed

At block start or whenever CC5/CC6 is updated:

\[
v(t) = v_{\max} \cdot s(t), \quad
	heta(t) = 2\pi \cdot CC_6(t) - \pi,
\]
\[
u(	heta) = (\cos	heta,\,\sin	heta).
\]

### Numerical integration inside VoiceDopp

Let \( \Delta t = rac{1}{f_s} \) per sample or \( \Delta T = rac{N}{f_s} \) per block.

Either of these is allowed:

**Per-sample integration**
\[
x_L(t_{n+1}) = x_L(t_n) + v(t_n) u(	heta(t_n)) \Delta t.
\]

**Per-block integration**
\[
x_L(t + \Delta T) = x_L(t) + v(t) u(	heta(t))\,\Delta T.
\]

Both match the continuous model under typical DSP assumptions.

---

## 3. Emitter Geometry (Lattice)

At note-on, VoiceDopp constructs an emitter lattice using parameters:

- Orientation:
  \[
  n(arphi) = (\cosarphi, \sinarphi),\quad
  b(arphi) = (-\sinarphi, \cosarphi)
  \]
- Density/distance:
  \[
  \Delta^\perp =
  egin{cases}
  \infty & 
ho = 0 \
  1/
ho & 
ho > 0
  \end{cases}
  \]
- Along-line spacing \( \Delta^\parallel = 1 \)

Emitters:
\[
x_{k,m} = k\,\Delta^\perp\,n(arphi) + m\,\Delta^\parallel\,b(arphi).
\]

Emitters may be constructed lazily, or on-demand when they enter eligibility range.

---

## 4. Retarded Time Model (Compatible with DSP)

For emitter \(i\):

\[
r_i(t) = \| x_i - x_L(t)\|.
\]

Retarded time:
\[
t^{ret}_i(t) = t - rac{r_i(t)}{c}.
\]

### Implementation Notes

- VoiceDopp tracks global time with a `timeSeconds` accumulator:
  \[
  timeSeconds += rac{1}{f_s} 	ext{ per sample}.
  \]
- Functions evaluated at retarded time use:
  - ADSR envelope for the note
  - Field pulse `A_field(t^{ret})`
  - Carrier signal `s_i(t^{ret}) = \sin(2\pi\lambda_i t^{ret} + \phi_i)`

Since envelope and field functions are lightweight, their evaluation at retarded times is cost-feasible.

---

## 5. Predictive Emitter Selection

VoiceDopp maintains a **finite active emitter set** \( \mathcal{E}_{act} \) of size \( K \).

### Eligibility radius
\[
\mathcal{E}_{elig}(t) = \{ i : r_i(t) \le R_{out} \}.
\]

Re-entry radius:
\[
r_i(t) \le R_{in} \quad (R_{in} < R_{out}).
\]

### Predictive Score

Let:

- Lookahead horizon \( H \)
- Predictor times \( \mathcal{T} = \{ 0, H/2, H \} \)
- Approximate future listener position:
  \[
  	ilde{x}_L(t+	au) = x_L(t) + v(t)u(	heta(t))\,	au
  \]

For each \( 	au \in \mathcal{T} \):

\[
	ilde{r}_{i,	au} = \|\;x_i - 	ilde{x}_L(t+	au)\;\|
\]
\[
	ilde{t}^{ret}_{i,	au} = (t+	au) - rac{	ilde{r}_{i,	au}}{c}
\]

Score:
\[
score_{i,	au} =
w(	ilde{r}_{i,	au})\;
A^{env}_i(	ilde{t}^{ret}_{i,	au})\;
A^{field}(	ilde{t}^{ret}_{i,	au})
\]

Predictive score:
\[
score_i(t) = \max_{	au \in \mathcal{T}} score_{i,	au}.
\]

### Top-K with Soft Margin

Among eligible emitters, pick the top \( K \) by score, but keep incumbents unless a challenger exceeds them by margin \( \delta \).

Crossfade gain:
\[
\dot{g}_i =
egin{cases}
+1/T_{xfade} & i 	ext{ enters} \
-1/T_{xfade} & i 	ext{ exits} \
0 & otherwise
\end{cases}
\]

Final per-block sample:
\[
p(t) = \sum_{i \in \mathcal{E}_{act}(t)} g_i(t)\, p_i(t).
\]

---

## 6. Computational Notes (Plugin-Aligned)

- **Blockwise update of listener state** is recommended (efficient, matches plugin timing).
- **Per-sample retarded time evaluation** gives correct Doppler shift.
- **Lazy emitter activation** prevents infinite sets from being constructed.
- **Emitter scoring & membership changes** only need to occur once per block.
- **Crossfades** may be updated per sample for smoothness.

---

## 7. Parameters Not Managed by APVTS

The following remain internal to VoiceDopp (hardcoded or configurable via CC):

- \( c \) speed of sound
- \( lpha \) absorption
- \( v_{max} \)
- \( R_{in}, R_{out} \)
- \( K \) (active emitters)
- \( H \) horizon
- \( T_{xfade} \)
- \( \Delta^\parallel, \Delta^\perp \)

Architecture does **not** require APVTS exposure for these.

---

## 8. Summary of Architectural Compliance

This updated spec:

- Respects the **blockwise** nature of plugin DSP  
- Matches **CC routing through VoiceManager**  
- Uses **timestamped note-on CC sampling**  
- Defines **per-voice time, trajectory, emitters, and predictive selection**  
- Respects the BaseVoice interface  
- Does not conflict with APVTS or mode switching  
- Allows complete implementation inside `VoiceDopp`  

This is now a precise mathematical + architectural spec for implementing the true VoiceDopp.

---

*End of Phase IV Architecture-Aligned Specification.*
