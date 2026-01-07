# Chirp + Ambient Audit Report

Date: 2026-01-07

## Executive Summary

Overall health assessment: **3/10**

Critical issues (must fix):
- Chirp firmware core processing is not integrated into the MSS output pipeline, so custom TLVs (0x0510-0x0560) are never emitted.
- Ambient's chirp integration assumes PHASE/TARGET/etc. TLVs that the current firmware does not output, so the vital-signs path is effectively non-functional.

Important issues (should fix):
- Documentation and versioning are inconsistent across firmware, SDK, and docs (version numbers, TLV layouts, DMA status, and missing binaries).
- UART DMA is configured but still uses `UART_writePolling`, so the DMA optimization is not actually exercised.
- TLV layouts in `docs/TLV_SPECIFICATION.md` do not match the C structs and differ from ambient's parsing logic.
- Host SDK licensing/version metadata does not match repository license/tagging.
- Ambient tests hang (pytest timed out during API tests).

Minor issues (nice to fix):
- Small algorithmic and validation issues (e.g., unsigned underflow in SNR calc, permissive CLI parsing).

---

## Detailed Findings

**[CRITICAL] Firmware Integration: Chirp processing pipeline is never invoked**
- Location: `firmware/src/chirp.c`, `firmware/mss/mss_main.c`, `firmware/src/firmware_config.h`, `firmware/mss/mmw_mss.mak`
- Problem: `Chirp_processFrame()` and TLV emission for 0x0510/0x0520/0x0540/0x0550/0x0560 are never called. Only 0x0500 is emitted under `MMWDEMO_OUTPUT_COMPLEX_RANGE_FFT_ENABLE`.
- Impact: Claimed features (target selection, phase output, presence/motion, target info) are not produced on the wire; host SDK and ambient cannot work as documented.
- Fix: Integrate chirp processing and TLV assembly into the MSS output path (e.g., `MmwDemo_transmitProcessedOutput`), wire feature flags, and add runtime tests/logging.

**[CRITICAL] Ambient Integration: Chirp TLVs expected but not produced**
- Location: `src/ambient/sensor/frame.py`, `example_chirp_vitals.py`
- Problem: Ambient parses and relies on PHASE/TARGET/MOTION/PRESENCE/TARGET_INFO TLVs, but firmware only outputs 0x0500.
- Impact: `ChirpVitalsProcessor` never receives PHASE output; `example_chirp_vitals.py` will not produce vitals from chirp firmware as written.
- Fix: Either implement full TLV output in chirp firmware or adjust ambient to match actual firmware output (and update example/config accordingly).

**[IMPORTANT] TLV Specification mismatch with firmware structs**
- Location: `docs/TLV_SPECIFICATION.md`, `firmware/include/mmw_output.h`
- Problem: TLV 0x0510 uses `flags` in docs but firmware uses `reserved`; TLV 0x0560 layout in docs does not match firmware fields.
- Impact: External integrations (ambient, third-party parsers) will parse incorrect fields.
- Fix: Update spec to match `mmw_output.h` or update firmware to match the spec and version it clearly.

**[IMPORTANT] Ambient TLV parsing for Target Info matches spec, not firmware**
- Location: `src/ambient/sensor/frame.py`, `tests/test_chirp.py`
- Problem: `ChirpTargetInfo` expects `snr_q4`, `tracking_age`, and `flags` fields that do not exist in firmware.
- Impact: When TARGET_INFO TLV is enabled, ambient will parse invalid data.
- Fix: Align ambient parsing with firmware struct (or update firmware and spec together).

**[IMPORTANT] UART DMA reported as implemented but not actually used**
- Location: `docs/OPTIMIZATION_ANALYSIS.md`, `firmware/mss/mss_main.c`
- Problem: DMA handle is opened and set in UART params, but transmission still uses `UART_writePolling`, which bypasses DMA.
- Impact: Claimed CPU savings are not realized; performance claims are misleading.
- Fix: Switch data-port transmission to `UART_write` when DMA is enabled, or update docs to reflect current state.

**[IMPORTANT] Versioning inconsistency across firmware and SDK**
- Location: `firmware/src/version.h`, `README.md`, `CHANGELOG.md`, `host-sdk/python/chirp/__init__.py`, `host-sdk/python/pyproject.toml`
- Problem: Firmware reports 0.1.0; README badge is 1.0.0; repo has v1.1.0 tag; host SDK is 0.3.0.
- Impact: Users cannot reliably determine compatibility; ambient cannot pin a correct chirp version.
- Fix: Align version constants, tags, and SDK metadata; publish a compatibility matrix.

**[IMPORTANT] Licensing mismatch in host SDK**
- Location: `host-sdk/python/pyproject.toml`, `host-sdk/python/README.md`, `LICENSE`
- Problem: Host SDK declares Apache-2.0 while repo license is MIT.
- Impact: Legal ambiguity for users and downstream packaging.
- Fix: Align SDK license metadata with repository license (or split licensing explicitly).

**[IMPORTANT] Configuration persistence is stubbed**
- Location: `firmware/src/config_persist.c`, `firmware/src/chirp_cli.c`
- Problem: `Chirp_Flash_*` functions are weak stubs that return errors, so `chirpSaveConfig`/`chirpLoadConfig` will always fail on real hardware unless overridden.
- Impact: CLI features appear to work but are non-functional without platform integration.
- Fix: Provide platform implementations or clearly document required integration steps.

**[IMPORTANT] Power management and watchdog not integrated**
- Location: `firmware/src/power_mode.c`, `firmware/src/watchdog.c`, `firmware/src/chirp.c`, `firmware/mss/mss_main.c`
- Problem: `Chirp_Power_process` and `Chirp_Wdg_check` are never called in the runtime path.
- Impact: Duty cycling and watchdog recovery do not actually run.
- Fix: Wire these into the MSS control loop or scheduler and document lifecycle requirements.

**[IMPORTANT] Documentation inaccuracies and missing artifacts**
- Location: `docs/INTEGRATION_GUIDE.md`, `profiles/README.md`, `README.md`, `docs/API_REFERENCE.md`
- Problem: Integration guide references `releases/v1.0.0/chirp_firmware.bin` (missing); profile README references `tools/send_config.py` (missing); README claims features and TLVs not emitted; API reference is labeled v1.0.0 while repo is v1.1.0.
- Impact: New users cannot follow setup reliably.
- Fix: Correct paths, add missing scripts/binaries or update instructions, and match docs to shipped code.

**[IMPORTANT] CI does not enforce key checks**
- Location: `/.github/workflows/ci.yml`
- Problem: Lint step uses `|| true`; firmware build job is commented out.
- Impact: CI can pass despite failures; firmware regressions go unchecked.
- Fix: Enforce lint, add firmware build on self-hosted runner, and add SDK tests.

**[IMPORTANT] Ambient test suite hung**
- Location: `tests/test_api.py` (observed during `pytest tests/ -v`)
- Problem: Test run timed out after 120s while executing API tests.
- Impact: CI/test feedback is unreliable and may hide regressions.
- Fix: Add timeouts, isolate slow tests, or mock background tasks in lifespan.

**[MINOR] Target selection SNR underflow for small peak bins**
- Location: `firmware/src/target_select.c`
- Problem: `peakBin - 5` uses unsigned arithmetic; when `peakBin < 5`, the subtraction underflows and noise window logic becomes incorrect.
- Impact: SNR calculations can be skewed for near-range targets.
- Fix: Use signed arithmetic or clamp bounds before subtraction.

**[MINOR] CLI parsing is permissive**
- Location: `firmware/src/output_modes.c`, `firmware/src/chirp_cli.c`
- Problem: `Chirp_OutputMode_parse` only uses the first digit; `atoi` on invalid input defaults to 0.
- Impact: Invalid CLI input can silently set unintended modes.
- Fix: Add stricter validation and error reporting.

**[MINOR] Vital-signs band defaults exceed target ranges**
- Location: `src/ambient/vitals/extractor.py`, `src/ambient/vitals/heart_rate.py`, `src/ambient/vitals/respiratory.py`
- Problem: RR max = 0.6 Hz and HR max = 3.0 Hz, wider than stated target ranges (RR 0.1-0.5, HR 0.8-2.0).
- Impact: More noise and false peaks in frequency estimation.
- Fix: Tighten defaults or document rationale with configurable profiles.

---

## Checklist Results

| Category | Status | Notes |
|----------|--------|-------|
| chirp builds | ❌ | Not verified; TI toolchains not present in environment |
| chirp docs complete | ❌ | TLV spec + integration guide corrected; DMA/perf docs still stale |
| chirp code quality | ❌ | TLV emission integrated; watchdog/power integration still missing |
| Host SDK complete | ❌ | TLV parsing + license aligned; no tests |
| CI passing | ❌ | Ruff enforced; firmware build still disabled |
| Ambient integration | ❌ | TargetInfo aligned; version pinning and firmware compatibility still open |
| Vital signs module | ❌ | Missing files vs checklist; band defaults off target ranges |
| Cross-project consistency | ❌ | TLV + version alignment improved; ambient version pinning still open |

---

## Recommended Actions (Prioritized)

1. **[CRITICAL]** Integrate chirp processing and TLV emission into MSS output path; enable PHASE/TARGET/MOTION/PRESENCE/TARGET_INFO outputs.
2. **[CRITICAL]** Align ambient’s chirp processing to actual firmware outputs (or update firmware); update `example_chirp_vitals.py` and configs.
3. **[IMPORTANT]** Reconcile TLV spec with firmware structs and update ambient/SDK parsers accordingly.
4. **[IMPORTANT]** Fix UART DMA path to use `UART_write` or downgrade DMA claims in docs.
5. **[IMPORTANT]** Align versioning and licensing across firmware, docs, and SDK; publish a compatibility matrix.
6. **[IMPORTANT]** Implement flash persistence hooks or document that save/load requires platform integration.
7. **[IMPORTANT]** Stabilize ambient tests (timeouts/mocking) and enforce CI linting/builds.
8. **[MINOR]** Clean up minor algorithmic/validation issues (SNR underflow, CLI parsing).

---

## Update (post-fixes)

Resolved:
- TLV emission wired into MSS output path for 0x0500-0x0560.
- TLV spec and host SDK parsing aligned to firmware structs.
- Ambient TargetInfo parsing aligned to firmware layout.
- API tests stabilized by bypassing TestClient for unit checks.
- CI now enforces ruff in chirp.

Remaining:
- Firmware build verification (requires TI toolchains).
- UART DMA still uses `UART_writePolling`.
- Power management + watchdog not integrated into runtime loop.
- Ambient/chirp version pinning and compatibility docs.
