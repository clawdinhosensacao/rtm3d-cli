# RTM Reference Notes (initial baseline)

Goal: keep this document short and practical. It captures implementation patterns to guide `rtm3d-cli` refactors and benchmark upgrades.

## 1) Practical references (3–5)

1. **Devito seismic tutorials/examples (acoustic modeling + RTM patterns)**
   - https://github.com/devitocodes/devito/tree/master/examples/seismic
   - Why relevant: clear separation of forward/adjoint operators, acquisition geometry objects, imaging condition examples.

2. **Devito paper (DSL for finite differences)**
   - Louboutin et al., 2019, *The Devito DSL for automated finite differences in geophysics*.
   - https://doi.org/10.5194/gmd-12-1165-2019
   - Why relevant: good design references for maintainable/stencil-centric implementations.

3. **Classical RTM foundations**
   - Baysal, Kosloff, Sherwood (1983), *Reverse time migration*.
   - Whitmore (1983), *Iterative depth migration by backward time propagation*.
   - Why relevant: conceptual baseline for forward wavefield + backward receiver wavefield + imaging condition.

4. **SEP / Stanford Exploration Project notes on imaging condition and RTM practice**
   - https://sepwww.stanford.edu/
   - Why relevant: practical trade-offs for stability, illumination, and artifacts.

5. **Open seismic processing ecosystems (for implementation ideas)**
   - Madagascar RSF: http://www.ahay.org/
   - Why relevant: reproducible processing flows and migration-related tooling patterns.

> Note: this is the initial list; expand with repo-specific code links during later cycles.

## 2) Baseline implementation checklist for `rtm3d-cli`

### Numerics/stencil
- [ ] Explicitly document PDE and stencil order in code/docs.
- [ ] Keep FD step kernel isolated and testable.
- [ ] Confirm CFL-related parameter checks (dt vs dx/dz/dy and velocity bounds).

### Boundary conditions / absorbing layers
- [ ] Keep damping/PML generation in dedicated module.
- [ ] Add tests for boundary damping profile monotonicity and edge behavior.
- [ ] Parameterize damping strength separately from width (pml cells).

### Source model
- [ ] Keep wavelet generation isolated (`ricker_wavelet` already separated).
- [ ] Support configurable source type and source depth/position policy.

### Acquisition geometry
- [ ] Separate geometry builder (shot + receiver layout) from propagation loops.
- [ ] Add multi-shot geometry abstraction (current: single-shot center source).

### Receiver backpropagation (adjoint)
- [ ] Keep injection and stepping as separate functions (in progress).
- [ ] Add tests for receiver injection indexing/consistency.

### Imaging condition
- [ ] Keep cross-correlation imaging condition isolated.
- [ ] Add optional illumination compensation hooks.
- [ ] Prepare extension point for alternative imaging conditions.

### Data/IO + metadata
- [ ] Persist enough metadata to reproduce run (geometry, wavelet, seed, numerics).
- [ ] Keep synthetic generator and migration config versioned.

## 3) Immediate engineering moves (next cycles)

1. Split RTM internals into focused translation units:
   - `src/rtm/Propagation.cpp`
   - `src/rtm/Geometry.cpp`
   - `src/rtm/Imaging.cpp`
   - keep `RtmEngine.cpp` as orchestration.

2. Introduce lightweight internal structs:
   - `ShotGeometry`, `ReceiverGeometry`, `Wavefields`.

3. Add static-analysis gate in routine:
   - keep `make static` (clang-tidy/cppcheck/fanalyzer fallback).

4. Add multi-shot planning doc + first API surface (even if implementation remains single-shot initially).
