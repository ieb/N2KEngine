#!/usr/bin/env python3
"""
Decoder and analyser for nmeabridge engine-YYYY-MM-DD.bin per-day history files.

Format (mirrors nmeabridgeapp/doc/history-log-format.md and EngineProtocol.kt):
  14-byte header:
    0..6   ASCII "navdata"
    7..10  u32 LE  startTimeSec (UTC seconds at file's 00:00:00)
    11     u8      streamType (2 == ENGINE)
    12     u8      recordSize (27)
    13     u8      secondsPerRecord (1)
  Body: contiguous fixed-size records, slot N at startTimeSec + N*spr.

Engine record (27 bytes, little-endian, magic 0xDD at offset 0):
  off  size  field            scaling
   1    u16  rpm              * 0.25
   3    u32  engine hours     * 1 s
   7    u16  coolant K        * 0.01,  -273.15 -> C
   9    u16  alternator K     * 0.01,  -273.15 -> C
  11    u16  alternator V     * 0.01
  13    u16  oil bar          * 0.001
  15    u16  exhaust K        * 0.01,  -273.15 -> C
  17    u16  engine room K    * 0.01,  -273.15 -> C
  19    u16  engine batt V    * 0.01
  21    u16  fuel %           * 0.004
  23    u16  status1          bitmask
  25    u16  status2          bitmask

Sentinel handling: NMEA-2000 reserves the top of the unsigned range as
"not available / out of range / reserved". Treat raw values >= 0xFFFD as None.
"""

from __future__ import annotations

import argparse
import csv
import os
import struct
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Iterator, Optional

HEADER_SIZE = 27 # set by file: read from header anyway
MAGIC = b"navdata"
STREAM_ENGINE = 2
ENGINE_RECORD_SIZE = 27
ENGINE_MAGIC = 0xDD
RESERVED_U16_MIN = 0xFFFD
RESERVED_U32_MIN = 0xFFFFFFFD

# status1 bits (from N2KEngine/lib/enginesensors/enginesensors.h)
STATUS1_BITS = {
    0x0001: "CHECK_ENGINE",
    0x0002: "OVERTEMP",
    0x0004: "LOW_OIL_PRES",
    0x0008: "LOW_OIL_LEVEL",
    0x0010: "LOW_FUEL_PRESS",
    0x0020: "LOW_SYSTEM_VOLTAGE",
    0x0040: "LOW_COOLANT_LEVEL",
    0x0080: "WATER_FLOW",
    0x0100: "WATER_IN_FUEL",
    0x0200: "CHARGE_INDICATOR",
    0x0400: "PREHEAT",
    0x0800: "HIGH_BOOST",
    0x1000: "REV_LIMIT",
    0x2000: "EGR",
    0x4000: "THROTTLE_POS",
    0x8000: "EMERGENCY_STOP",
}
STATUS2_BITS = {
    0x0001: "WARN_1",
    0x0002: "WARN_2",
    0x0004: "POWER_REDUCTION",
    0x0008: "MAINTENANCE",
    0x0010: "ENGINE_COMM_ERROR",
    0x0020: "SUB_THROTTLE",
    0x0040: "NEUTRAL_START_PROTECT",
    0x0080: "ENGINE_SHUTTING_DOWN",
}


def _u16_or_none(v: int) -> Optional[int]:
    return None if v >= RESERVED_U16_MIN else v


def _u32_or_none(v: int) -> Optional[int]:
    return None if v >= RESERVED_U32_MIN else v


def _bits(mask: int, table: dict) -> str:
    if mask == 0:
        return ""
    return "|".join(name for bit, name in table.items() if mask & bit)


@dataclass
class EngineSample:
    ts: int  # UTC epoch seconds
    rpm: Optional[float]
    hours_s: Optional[int]
    coolant_c: Optional[float]
    alternator_c: Optional[float]
    alternator_v: Optional[float]
    oil_bar: Optional[float]
    exhaust_c: Optional[float]
    room_c: Optional[float]
    engine_batt_v: Optional[float]
    fuel_pct: Optional[float]
    status1: Optional[int]
    status2: Optional[int]

    @property
    def running(self) -> bool:
        # The firmware treats RPM > 800 as running. Use 500 here so the
        # leading/trailing edges around start/stop are visible in the data.
        return self.rpm is not None and self.rpm > 500

    def status1_names(self) -> str:
        return _bits(self.status1 or 0, STATUS1_BITS)

    def status2_names(self) -> str:
        return _bits(self.status2 or 0, STATUS2_BITS)


def open_engine_file(path: str):
    """Yield (start_ts_utc_sec, generator-of-EngineSample) for the file."""
    with open(path, "rb") as f:
        header = f.read(14)
        if len(header) < 14 or header[:7] != MAGIC:
            raise ValueError(f"{path}: bad magic / short header")
        start_ts, stream_type, rec_size, spr = struct.unpack("<IBBB", header[7:14])
        if stream_type != STREAM_ENGINE:
            raise ValueError(f"{path}: streamType {stream_type} != ENGINE (2)")
        if rec_size != ENGINE_RECORD_SIZE:
            raise ValueError(f"{path}: recordSize {rec_size} != 27")
        body = f.read()

    def _scale(raw, factor, offset=0.0):
        v = _u16_or_none(raw)
        return None if v is None else v * factor + offset

    def gen() -> Iterator[EngineSample]:
        n = len(body) // rec_size
        for i in range(n):
            buf = body[i * rec_size : (i + 1) * rec_size]
            if buf[0] != ENGINE_MAGIC:
                continue  # gap / bad frame
            (rpm, hrs, cool, altt, altv, oil, exh, room, ebv, fuel, s1, s2) = struct.unpack(
                "<HIHHHHHHHHHH", buf[1:27]
            )
            ts = start_ts + i * spr
            yield EngineSample(
                ts=ts,
                rpm=_scale(rpm, 0.25),
                hours_s=_u32_or_none(hrs),
                coolant_c=_scale(cool, 0.01, -273.15),
                alternator_c=_scale(altt, 0.01, -273.15),
                alternator_v=_scale(altv, 0.01),
                oil_bar=_scale(oil, 0.001),
                exhaust_c=_scale(exh, 0.01, -273.15),
                room_c=_scale(room, 0.01, -273.15),
                engine_batt_v=_scale(ebv, 0.01),
                fuel_pct=_scale(fuel, 0.004),
                status1=_u16_or_none(s1),
                status2=_u16_or_none(s2),
            )

    return start_ts, gen()


def _fmt(v: Optional[float], fmt: str = "{:.1f}") -> str:
    return "" if v is None else fmt.format(v)


def cmd_csv(args: argparse.Namespace) -> int:
    w = csv.writer(sys.stdout)
    w.writerow([
        "utc",
        "rpm",
        "coolant_c",
        "exhaust_c",
        "alternator_c",
        "room_c",
        "alternator_v",
        "engine_batt_v",
        "oil_bar",
        "fuel_pct",
        "hours_s",
        "status1",
        "status2",
    ])
    for path in args.files:
        _, samples = open_engine_file(path)
        for s in samples:
            if args.running_only and not s.running:
                continue
            w.writerow([
                datetime.fromtimestamp(s.ts, tz=timezone.utc).isoformat(),
                _fmt(s.rpm, "{:.0f}"),
                _fmt(s.coolant_c),
                _fmt(s.exhaust_c),
                _fmt(s.alternator_c),
                _fmt(s.room_c),
                _fmt(s.alternator_v, "{:.2f}"),
                _fmt(s.engine_batt_v, "{:.2f}"),
                _fmt(s.oil_bar, "{:.2f}"),
                _fmt(s.fuel_pct, "{:.0f}"),
                "" if s.hours_s is None else s.hours_s,
                s.status1_names(),
                s.status2_names(),
            ])
    return 0


def _running_segments(samples, merge_gap_s=120, min_duration_s=60):
    """Group running samples into segments. Short gaps (<merge_gap_s) of
    non-running samples between two running stretches are bridged so a
    transient rpm dropout does not chop one run into many. Segments shorter
    than min_duration_s are dropped (firmware bring-up jitters, brief
    cranking)."""
    raw = []
    seg = []
    for s in samples:
        if s.running:
            seg.append(s)
        else:
            if seg:
                raw.append(seg)
                seg = []
    if seg:
        raw.append(seg)

    # Merge segments separated by a short gap.
    merged = []
    for seg in raw:
        if merged and (seg[0].ts - merged[-1][-1].ts) <= merge_gap_s:
            merged[-1].extend(seg)
        else:
            merged.append(seg)

    for seg in merged:
        if seg[-1].ts - seg[0].ts >= min_duration_s:
            yield seg


def _stats(values):
    vs = [v for v in values if v is not None]
    if not vs:
        return None
    return {
        "min": min(vs),
        "max": max(vs),
        "mean": sum(vs) / len(vs),
        "n": len(vs),
    }


def _stat_str(st):
    if st is None:
        return "—"
    return f"min {st['min']:.1f}  mean {st['mean']:.1f}  max {st['max']:.1f}  (n={st['n']})"


def cmd_summary(args: argparse.Namespace) -> int:
    for path in args.files:
        print(f"\n=== {os.path.basename(path)} ===")
        try:
            start_ts, gen = open_engine_file(path)
        except ValueError as e:
            print(f"  skip: {e}")
            continue
        samples = list(gen)
        if not samples:
            print("  (no records)")
            continue
        first = datetime.fromtimestamp(samples[0].ts, tz=timezone.utc)
        last = datetime.fromtimestamp(samples[-1].ts, tz=timezone.utc)
        n_running = sum(1 for s in samples if s.running)
        print(f"  span: {first.isoformat()}  ->  {last.isoformat()}  "
              f"({len(samples)} samples, {n_running}s running)")

        for idx, seg in enumerate(_running_segments(samples), start=1):
            t0 = datetime.fromtimestamp(seg[0].ts, tz=timezone.utc)
            t1 = datetime.fromtimestamp(seg[-1].ts, tz=timezone.utc)
            dur = seg[-1].ts - seg[0].ts
            print(f"\n  -- run {idx}: {t0.strftime('%H:%M:%S')}-{t1.strftime('%H:%M:%S')}  "
                  f"({dur//60}m{dur%60:02d}s, n={len(seg)})")
            print(f"     rpm        : {_stat_str(_stats([s.rpm for s in seg]))}")
            print(f"     coolant C  : {_stat_str(_stats([s.coolant_c for s in seg]))}")
            print(f"     exhaust C  : {_stat_str(_stats([s.exhaust_c for s in seg]))}")
            print(f"     alt C      : {_stat_str(_stats([s.alternator_c for s in seg]))}")
            print(f"     room C     : {_stat_str(_stats([s.room_c for s in seg]))}")
            print(f"     alt V      : {_stat_str(_stats([s.alternator_v for s in seg]))}")
            print(f"     batt V     : {_stat_str(_stats([s.engine_batt_v for s in seg]))}")

            # gap (coolant - exhaust) at warmed-up sustained-rpm region
            warm = [s for s in seg if s.coolant_c and s.coolant_c > 60 and s.rpm and s.rpm > 1200]
            if warm:
                gaps = [s.coolant_c - s.exhaust_c for s in warm if s.exhaust_c is not None]
                gst = _stats(gaps)
                if gst:
                    print(f"     coolant-exhaust gap (warm, rpm>1200): {_stat_str(gst)}")

            alarm_set = set()
            for s in seg:
                if s.status1:
                    alarm_set.update(_bits(s.status1, STATUS1_BITS).split("|"))
                if s.status2:
                    alarm_set.update(_bits(s.status2, STATUS2_BITS).split("|"))
            alarm_set.discard("")
            if alarm_set:
                print(f"     alarms seen: {','.join(sorted(alarm_set))}")
    return 0


def cmd_trace(args: argparse.Namespace) -> int:
    """Print every sample within ±N seconds of the highest exhaust temperature
    seen across the supplied files. Useful for inspecting the lead-in to a
    failure even when you do not know the exact wall-clock minute.
    """
    all_samples: list[EngineSample] = []
    for path in args.files:
        _, gen = open_engine_file(path)
        all_samples.extend(gen)
    all_samples.sort(key=lambda s: s.ts)

    # Find peak coolant or peak exhaust depending on flag
    key = (lambda s: s.exhaust_c if s.exhaust_c is not None else -1) if args.peak == "exhaust" \
          else (lambda s: s.coolant_c if s.coolant_c is not None else -1)
    peak = max(all_samples, key=key)
    peak_dt = datetime.fromtimestamp(peak.ts, tz=timezone.utc)
    print(f"# peak {args.peak}: {peak_dt.isoformat()}  "
          f"coolant={_fmt(peak.coolant_c)}C  exhaust={_fmt(peak.exhaust_c)}C  rpm={_fmt(peak.rpm,'{:.0f}')}")

    lo, hi = peak.ts - args.window, peak.ts + args.window
    print(f"{'utc':<20}  {'rpm':>5}  {'cool':>5}  {'exh':>5}  {'gap':>5}  {'alt':>5}  {'rm':>5}  status")
    for s in all_samples:
        if s.ts < lo or s.ts > hi:
            continue
        dt = datetime.fromtimestamp(s.ts, tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        gap = (s.coolant_c - s.exhaust_c) if (s.coolant_c is not None and s.exhaust_c is not None) else None
        st = (s.status1_names() + ("/" + s.status2_names() if s.status2_names() else ""))
        print(f"{dt}  {_fmt(s.rpm,'{:.0f}'):>5}  {_fmt(s.coolant_c):>5}  "
              f"{_fmt(s.exhaust_c):>5}  {_fmt(gap):>5}  "
              f"{_fmt(s.alternator_c):>5}  {_fmt(s.room_c):>5}  {st}")
    return 0


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    pc = sub.add_parser("csv", help="Decode files to CSV on stdout")
    pc.add_argument("files", nargs="+")
    pc.add_argument("--running-only", action="store_true",
                    help="Only emit rows where engine is running (rpm>500)")
    pc.set_defaults(func=cmd_csv)

    ps = sub.add_parser("summary", help="Per-run statistics")
    ps.add_argument("files", nargs="+")
    ps.set_defaults(func=cmd_summary)

    pt = sub.add_parser("trace", help="Dump samples around the peak temperature")
    pt.add_argument("files", nargs="+")
    pt.add_argument("--peak", choices=["coolant", "exhaust"], default="coolant")
    pt.add_argument("--window", type=int, default=180,
                    help="seconds either side of peak to show (default 180)")
    pt.set_defaults(func=cmd_trace)

    args = p.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
