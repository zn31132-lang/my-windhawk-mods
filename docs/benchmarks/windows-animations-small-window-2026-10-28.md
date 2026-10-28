# Small-window restore pacing: local observations and optional CPU fallback

This report accompanies an opt-in source-area threshold for GPU restore/launch
selection. It is a single-machine investigation, not a replacement for the
existing matched benchmarks or evidence for changing the default GPU route.

## Environment and scope

- Windows 11, build 26300.
- Intel Core i9-12900H; NVIDIA GeForce RTX 3060 Laptop GPU.
- Active display mode: 3840 x 2160 at 144 Hz. DWM reported a refresh ratio of
  10000000/69445, approximately 144 Hz.
- Runtime tests used installed Windows Animations 1.3.7 and a local 1.3.7
  prototype that selected CPU for restores below 1,500,000 source pixels.
- Source sizes reported by telemetry: 1182 x 751 for the ordinary window and
  3582 x 1891 for the large window.
- Minimize/restore shuffling and launch animations were enabled. Base duration
  was 200 ms; each effect's own duration adjustment remained active. Closing
  used Thanos, 5 px blocks and 630 ms. Both GPU toggles were initially enabled.
- The accompanying implementation is rebased onto development version 1.4.0.
  It has been compiled for x86 and x64, including the routing static assertions,
  but has **not** been runtime-benchmarked on 1.4.0. New 1.4.0 effects were not
  part of the runtime measurements below.

## Procedure and data

Six runs each created a test window, minimized/restored it ten times using a
900 ms timer, and closed it. The resulting 132 animation summaries all reported
`status=ok`. The runs were sequential, with default mixed GPU routing, forced CPU,
and the local area-based prototype at each window size.

The accompanying [CSV](windows-animations-small-window-2026-10-28.csv) contains
the parsed summaries, including individual render and pacing costs. It omits
process/window identifiers and local paths. Run names:

| Run | Configuration |
| --- | --- |
| `baseline` | 1.3.7, ordinary window, hybrid GPU enabled |
| `cpu-small` | 1.3.7, ordinary window, minimize/restore GPU toggle disabled |
| `gpu-large` | 1.3.7, large window, hybrid GPU enabled |
| `cpu-large` | 1.3.7, large window, minimize/restore GPU toggle disabled |
| `adaptive-small` | local 1.3.7 prototype, ordinary window |
| `adaptive-large` | local 1.3.7 prototype, large window |

Effects were shuffled, so per-effect sample counts and order differ. There was
no controlled warm-up protocol or repeated matched per-effect trial set.
Background system/driver load was not controlled. These are exploratory
observations, not statistically established speedups. `status=ok` only reports
successful animation completion; it does not measure visual quality.

## Observations

For several ordinary-window restores, GPU drawing itself averaged roughly
0.3-1.1 ms, while the reported pacing wait averaged approximately 17-22 ms.
The CPU route often produced more submitted frames in the same target duration:

| Effect | Target duration | Baseline GPU submitted frames | Prototype CPU submitted frames |
| --- | ---: | ---: | ---: |
| Ink | 260 ms | 15, 17 | 40, 40 |
| Splinter | 300 ms | 20 | 46 |
| Windows 10 | 280 ms | 15 | 39 |
| Mirage | 220 ms | 12, 31 | 34, 34 |
| Genie | 200 ms | 29, 22 | 27, 27 |

These are the mod's submission counts, possibly including bootstrap/endpoint
frames. They are **not measured displayed FPS**. Total elapsed time in the CSV
also includes setup/finish work, so it is not an isolated animation-loop duration.
Some GPU trials already paced well; the wait's underlying cause remains unproven.

For large windows, forcing CPU was not consistently better. For example, Scorch
submitted 20 frames in the GPU run and 10 in the forced-CPU run at 200 ms; its
mean CPU frame render cost was 22.930 ms. Large GPU results also varied, and the
area-based prototype did not eliminate that variability.

## Proposed setting

`minimize_restore.gpu_restore_min_source_pixels` defaults to `0`, preserving the
existing selection. A positive threshold selects CPU for smaller restore and
launch surfaces, retaining the GPU route for surfaces at or above the threshold
when otherwise supported, enabled, warm and compatible.

`1500000` reproduces the local prototype's routing decision. It is an example,
not a measured optimal crossover: only two window sizes were compared on one
machine. Minimize routing, including the large Hexagon special case, close
routing, compatibility checks and renderer retention during rapid reversals
remain unchanged. More devices and matched tests on 1.4.0 are needed before
considering a nonzero default or automatic adaptation.

## Reproduction harness

The [probe source](windows-animations-small-window-probe.cpp) only manipulates
its own test window and listens for Windows Animations debug messages through
DBWIN. Close other DBWIN/debug-output collectors before using it. Run it in a
separate directory so its `probe.log` does not overwrite a previous run.

Example compilation with Windhawk's compiler:

```powershell
clang++.exe windows-animations-small-window-probe.cpp -DUNICODE -D_UNICODE -O2 -static -o probe.exe -luser32 -lgdi32 -ldwmapi
```

1. Enable the mod's performance telemetry and logging temporarily.
2. Record the mod version, settings and active monitor refresh rate.
3. Run `probe.exe` for the ordinary window, or `probe.exe large` for the large
   window. The latter uses the same large-window dimensions as the original test.
4. Save `probe.log` after each run and compare matched effects/durations.
5. Disable temporary telemetry/logging when done.

The original measured runs used separate executables with these two fixed window
sizes; the supplied reproduction source consolidates that choice into an argument.
It is compiled for reproducibility, not claimed to be a runtime-tested 1.4.0 suite.
