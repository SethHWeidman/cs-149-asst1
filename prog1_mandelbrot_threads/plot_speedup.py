#!/usr/bin/env python3

"""
Measure mandelbrot speedup for both tiling strategies (block and interleaved) across thread counts.

Run this after building `mandelbrot`; it sweeps 1..8 threads for each schedule and stores
per-schedule CSVs plus a combined Matplotlib plot under `speedup_plots/`.
"""

import dataclasses
import pathlib
import re
import subprocess
import sys

import matplotlib.pyplot as plt

SCHEDULES = ("block", "interleaved")
# Regex to extract speedup factor and thread count from output.
SPEEDUP_RE = re.compile(
    r"\((?P<speedup>[0-9.]+)x speedup from (?P<threads>\d+) threads\)"
)
# Regex to extract the execution time of the serial Mandelbrot run.
SERIAL_RE = re.compile(r"\[mandelbrot serial\]:\s*\[(?P<ms>[0-9.]+)\] ms")
# Regex to extract the execution time of the threaded Mandelbrot run.
THREAD_RE = re.compile(r"\[mandelbrot thread\]:\s*\[(?P<ms>[0-9.]+)\] ms")


@dataclasses.dataclass
class RunResult:
    threads: int
    speedup: float
    serial_ms: float
    thread_ms: float
    schedule: str


def run_case(
    workdir: pathlib.Path, binary: pathlib.Path, threads: int, schedule: str
) -> RunResult:
    cmd = [str(binary), "-t", str(threads), "-s", schedule]
    print("Running " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=workdir, check=True, capture_output=True, text=True)
    serial_ms = extract_value(SERIAL_RE, proc.stdout, "serial time")
    thread_ms = extract_value(THREAD_RE, proc.stdout, "thread time")
    speedup = extract_value(SPEEDUP_RE, proc.stdout, "speedup")
    return RunResult(
        threads, float(speedup), float(serial_ms), float(thread_ms), schedule
    )


def extract_value(regex: re.Pattern[str], text: str, label: str) -> float:
    match = regex.search(text)
    if not match:
        snippet = text.splitlines()[-10:]
        raise RuntimeError(f"Unable to find {label} in output:\n" + "\n".join(snippet))
    return float(match.group(1))


def collect_data(
    workdir: pathlib.Path, binary: pathlib.Path
) -> dict[str, list[RunResult]]:
    results: dict[str, list[RunResult]] = {schedule: [] for schedule in SCHEDULES}
    for schedule in SCHEDULES:
        for threads in range(1, 9):
            results[schedule].append(run_case(workdir, binary, threads, schedule))
    return results


def write_csv(
    schedule: str, data: list[RunResult], output_dir: pathlib.Path
) -> pathlib.Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    csv_path = output_dir / f"speedup_{schedule}.csv"
    with csv_path.open("w", encoding="utf-8") as fh:
        fh.write("threads,speedup,serial_ms,thread_ms\n")
        for entry in data:
            fh.write(
                f"{entry.threads},{entry.speedup:.2f},{entry.serial_ms:.3f},{entry.thread_ms:.3f}\n"
            )
    print(f"Wrote {csv_path}")
    return csv_path


def plot_combined(
    results: dict[str, list[RunResult]], output_dir: pathlib.Path
) -> pathlib.Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    fig, ax = plt.subplots(figsize=(8.5, 4.8))
    colors = {"block": "#c62828", "interleaved": "#1565c0"}
    # Extract thread counts from the first available schedule's data
    threads = [entry.threads for entry in next(iter(results.values()))]
    for schedule in SCHEDULES:
        data = results[schedule]
        speedups = [entry.speedup for entry in data]
        ax.plot(
            threads,
            speedups,
            marker="o",
            linewidth=2.5,
            label=schedule,
            color=colors.get(schedule),
        )
        for x, y in zip(threads, speedups):
            ax.annotate(
                f"{y:.2f}×",
                (x, y),
                textcoords="offset points",  # Interpret xytext as an offset in points
                # Offset text 0 horizontally, 14 points upwards from the data point
                xytext=(0, -14),
                ha="center",  # Center the text horizontally
                fontsize=10,
            )
    ax.set_xticks(threads)
    ax.set_xlabel("Threads")
    ax.set_ylabel("Speedup (x)")
    ax.set_title("Block vs. interleaved tiling speedup")
    ax.grid(True, axis="y", linestyle="--", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    out_path = output_dir / "speedup_comparison.png"
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"Wrote {out_path}")
    return out_path


def main() -> None:
    workdir = pathlib.Path(__file__).resolve().parent
    binary = workdir / "mandelbrot"
    if not binary.exists():
        raise FileNotFoundError(f"Expected binary at {binary}. Run `make` first.")
    data = collect_data(workdir, binary)
    output_dir = workdir / "speedup_plots"
    for schedule, entries in data.items():
        write_csv(schedule, entries, output_dir)
    plot_combined(data, output_dir)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        print(exc.stdout)
        print(exc.stderr, file=sys.stderr)
        sys.exit(exc.returncode)
