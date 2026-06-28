# Engine cooling-failure analysis — May & June 2026 incidents

Analysis of two raw-water-flow / overheat events captured by the
`nmeabridgeapp` per-day binary history, used to validate the water-flow
alarm-logic changes in `lib/enginesensors/`.

Source data: `nmeabridgeapp/datasets/engine-YYYY-MM-DD.bin` (one record per
second, format documented in `nmeabridgeapp/doc/history-log-format.md` and
`EngineProtocol.kt`).

Decoder: `tools/engine_log.py` — pure-Python, stdlib only. Subcommands:

```
python3 tools/engine_log.py summary <files>           # per-run statistics
python3 tools/engine_log.py csv     <files>           # per-second CSV
python3 tools/engine_log.py trace   --peak coolant <files>   # window around peak
```

## Healthy baseline (D2-40, all good runs across 30–31 May, 20–21 Jun 2026)

| metric | typical warmed-up range |
|---|---|
| coolant | 80 – 92 C |
| **exhaust elbow** | **31 – 39 C** (mean 34–36 C) |
| coolant – exhaust gap (rpm > 1200) | **45 – 55 C** (means 48–52 C) |
| alternator | 60 – 85 C |
| engine room | 40 – 65 C |
| alternator V / battery V | 13.7–14.5 / 13.5–14.0 |

Confirms the firmware comment that "normal water flow on the elbow is 37C".
The alarm constants — 45 C absolute trip, 35 C rise-anchor baseline,
8 C rise-in-30 s — are all comfortably outside healthy variation.

## Why no coolant-convergence trigger

An earlier version of the firmware included a coolant-vs-exhaust convergence
trigger (fire if `coolant - exhaust < 30 C` once coolant > 70 C). It was
removed after the warm-restart sequence on run 2 of 21 Jun was inspected:
the silencer (where the exhaust probe is mounted) is thermally isolated from
the engine block, so on a warm restart the block is at ~85 C while the
silencer has cooled toward ambient. At 09:30:45 — engine at 1380 rpm, all
healthy — coolant was 82.0 C and exhaust 51.4 C: gap 30.6 C, **0.5 C from
tripping the convergence alarm**. Exhaust then fell monotonically for ~5 min
as raw water cooled the silencer faster than exhaust gas heated it, before
diverging into the normal ~50 C steady-state gap. Convergence is therefore
a false-positive risk on every warm restart, while contributing no
detectable lead time on either captured failure mode (it does not fire at
all on the sudden-detachment case and is the *last* of the three triggers
to fire on the slow-degradation case). Removed.

## Note on dating

The README incident headed *"21 July 2026"* is in fact recorded in
`engine-2026-06-21.bin` at 09:09 UTC (10:09 BST). There is no
`engine-2026-06-22.bin`. Both this document and the README's narrative
refer to the same 21 June 2026 event.

## Incident A — 2026-05-31, slow flow degradation

Run starts 10:03:35 UTC at ~1500 rpm, coolant ~62 C from a previous warmup.
The exhaust elbow climbs roughly linearly from start: about +1 C every
5 s once past 30 C, i.e. ~12 C/min — well above the new rate-of-rise
threshold (8 C in 30 s).

| time (UTC) | rpm | coolant | exhaust | gap | event |
|---|---|---|---|---|---|
| 10:03:44 | 1348 | 64.0 | 27.1 | 36.9 | start, gap normal |
| 10:04:34 | 1522 | 66.1 | 31.3 | 34.8 | exhaust climbing fast for a cold engine |
| 10:05:54 | 1664 | 79.6 | 45.7 | 33.9 | exhaust > 45 C — abs trip |
| 10:06:09 | 1675 | 78.7 | 50.5 | 28.2 | gap < 30 C — convergence trip |
| 10:07:39 | 1333 | 96.6 | 66.2 | 30.4 | coolant > 95 C |
| 10:08:39 | 1341 | 91.6 | 74.1 | **17.5** | gap collapsed |
| 10:09:11 | 1350 | **101.2** | 77.5 | 23.7 | peak coolant |
| 10:09:14 | 759 | 98.2 | 78.0 | 20.2 | rpm cut to idle |
| 10:09:24 | 767 | 99.1 | 78.4 | 20.7 | CHECK_ENGINE / MAINTENANCE asserted |
| 10:09:39 | 764 | 99.1 | 79.7 | 19.4 | OVERTEMP asserted |
| 10:09:54 | 769 | 100.7 | 80.0 | 20.7 | last sample, engine off shortly |

When the new alarm logic would trip (replayed against the data):

| trigger | first fires at | lead time vs OVERTEMP (10:09:39) |
|---|---|---|
| rate-of-rise (8 C / 30 s) | ~10:05:30 (cool 73 C) | **~4 min earlier** |
| absolute > 45 C | 10:05:54 (cool 80 C) | ~3 min 45 s earlier |

The old code (45 C absolute, no persistence) also tripped at 10:05:54, but
OVERTEMP was the only bit eventually set in the historical record — and
that didn't happen until 4 minutes later. **This is the failure mode where
the new rate-of-rise trigger adds real, measurable detection time.**

## Incident B — 2026-06-21, sudden hose detachment

Warm restart at 09:30 BST, stable cruise at ~2200 rpm. Coolant locks at
86–87 C, exhaust at 37–38 C, gap at ~49 C for ~9 minutes — textbook
healthy operation. Then a hose detachment causes the failure described in
README §"Engine overheat data from 21 July 2026" (date typo in README).

| time (UTC) | rpm | coolant | exhaust | gap | event |
|---|---|---|---|---|---|
| 09:08:45 | 2235 | 87.1 | 38.4 | 48.7 | normal |
| 09:09:00 | 2235 | 90.0 | 40.2 | 49.8 | hose detaches; exhaust climbing |
| 09:09:15 | 1480 | 92.7 | 47.1 | 45.6 | exhaust > 45 C; skipper throttles back |
| 09:09:30 | 1632 | 95.8 | 51.9 | 43.9 | coolant > 95 C |
| 09:09:45 | 1480 | 97.6 | 57.9 | 39.7 | — |
| 09:09:55 | 833 | 98.3 | 62.6 | 35.7 | engine going to idle / shutdown |
| 09:09:58 | 833 | — | 62.6 | — | coolant signal lost (engine off) |

Exhaust rose **+24 C in 55 s** (~26 C/min). No alarm bits were set by
the firmware in service that day — the only status1/2 flag seen on 21 Jun
across the whole file was `CHARGE_INDICATOR` from an unrelated earlier
run. Consistent with the README: the in-service threshold was 80 C, peak
exhaust was 62.6 C, no alarm fired and no display showed exhaust temp.

When the new alarm logic would trip:

| trigger | first fires at | notes |
|---|---|---|
| rate-of-rise (8 C / 30 s) | 09:09:15 (+8.7 C since 09:08:45) | tied with abs |
| absolute > 45 C | 09:09:15 (47.1 C) | tied with rate |

Rate-of-rise and absolute fire **simultaneously** here. The value of the
new logic in this incident is that it actually fires (the in-service
80 C threshold did not), with the absolute + 5 s persistence reaching
EMERGENCY_STOP at ~09:09:20.

## Conclusions

1. Healthy baseline data validates the alarm constants — they sit
   well outside normal-operation variation in both warmed-up cruise and
   warmup transients.
2. The May incident (slow degradation) is where the rate-of-rise trigger
   adds several minutes of detection time over an absolute-only check.
3. The June incident (sudden detachment) is caught at the same instant
   by absolute and rate-of-rise. The improvement here is over the
   original 80 C threshold that was actually in service, which never
   tripped.
4. The 5 s EMERGENCY_STOP persistence and engine-running gate are
   defence-in-depth against bad samples and hot-soak / hot-restart false
   positives. Neither incident exhibited those failure modes, but the
   cost of including them is negligible.
5. A coolant-vs-exhaust convergence trigger was considered and removed:
   the silencer (where the exhaust probe is mounted) is thermally
   isolated from the block, so on a warm restart the coolant–exhaust
   gap shrinks to within ~0.5 C of the trip margin under entirely
   healthy conditions before re-diverging. See the warm-restart section
   above.
