# Calibration Findings

**Date:** 2026-06-05
**Tool:** marquee-console.exe --calibrate
**Hardware:** Windows 11 (10.0.26200), QPC 10,000,000 ticks/sec, console 120x30

## Refresh Rate Sweep (10–360 Hz, step 10)

| Result | Value |
|--------|-------|
| Maximum stable refresh | 360 Hz |
| Tearing floor | 360 Hz (no tearing detected up to sweep limit) |
| Recommended refresh | 324 Hz (90% of max stable) |

No tearing detected at any rate. The console host handles the full sweep range cleanly.

## Poll Rate Sweep (50–1000 Hz, step 50)

| Result | Value |
|--------|-------|
| Minimum effective poll | 1000 Hz |
| Latency ceiling | 1000 Hz (no latency issues up to sweep limit) |
| Recommended poll | 2000 Hz (2× min effective) |

All poll rates showed p95 latency < 1 ms. No perceptible input lag at any tested rate.

## Validated Defaults

- **Default refresh:** 324 Hz
- **Default poll:** 2000 Hz
- **Subjective quality at defaults:** Smooth, no visible tearing, no input lag.

## Notes

- The busy-wait frame pacing at extreme rates (>1000 Hz refresh) shows ~83% of target FPS, but remains stable and responsive.
- Poll latency at all rates is excellent (sub-millisecond p95).
- These defaults are specific to this hardware configuration; re-run `--calibrate` on other machines.
