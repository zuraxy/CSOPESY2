"""Run the real waiting intervals in Detailed-Specs.pdf, Questions 4-8.

This is an executable acceptance check, not a video recording. Question 6 uses
the explicitly separate runner requested by the user. Question 7 captures a
genuine idle snapshot before the first arrival, followed by an occupied core;
its parameter range cannot guarantee recurring idle periods under sustained load.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import json
import shutil
import time
import traceback

from cli_tests import Session, counts, utilization, values, PROMPT


def prepare(args, name, case):
    directory = args.output / name
    directory.mkdir()
    config = directory / "config.txt"
    shutil.copyfile(args.app.parent / "configs" / f"case-{case}.txt", config)
    return directory, config


def capture(directory, session):
    session.close()
    (directory / "transcript.txt").write_text(session.output, encoding="utf-8")


def case4(args):
    directory, config = prepare(args, "case-4", 4)
    session = Session(args.app, config)
    try:
        session.command("initialize", "Initialized.")
        session.command("scheduler-start", "Batch generation started.")
        session.duration(5)
        report = session.report()
        result = counts(report)
        assert utilization(report) == 100 and result["running"] == 4, result
        assert result["running"] + result["ready"] + result["sleeping"] > result["finished"], result
        return {"utilization": utilization(report), **result}
    finally:
        capture(directory, session)


def case5(args):
    results = []
    for run in range(1, 3):
        directory, config = prepare(args, f"case-5-run-{run}", 5)
        session = Session(args.app, config, "alternating")
        try:
            session.command("initialize", "Initialized.")
            session.command("screen -s proc-01", "Process name: proc-01")
            session.command("exit", "Welcome to CSOPESY Emulator!")
            session.report()
            start = session.send("screen -r proc-01")
            session.wait(lambda text: "Process name: proc-01" in text, start)
            session.duration(15)
            observed = values(session.output[start:])
            assert len(observed) > 1 and observed[0] == 0, observed[:5]
            assert all(1 <= b - a <= 10 for a, b in zip(observed, observed[1:])), observed[-5:]
            session.command("exit", "Welcome to CSOPESY Emulator!")
            session.send("exit")
            results.append({"run": run, "print_count": len(observed),
                            "first": observed[0], "last": observed[-1]})
        finally:
            capture(directory, session)
    return {"restarts": results}


def case6(args):
    directory, config = prepare(args, "case-6", 6)
    session = Session(args.runner, config)
    try:
        session.command("initialize", "Initialized.")
        session.command("scheduler-test", "Batch generation started.")
        session.duration(10)
        session.command("scheduler-stop", "Batch generation stopped.")
        session.duration(30)
        report = session.report()
        result = counts(report)
        assert utilization(report) == 0, result
        assert result["finished"] > 0 and not (result["running"] or result["ready"] or result["sleeping"]), result
        return {"executable": args.runner.name, "drain_tick_ms": 1,
                "utilization": utilization(report), **result}
    finally:
        capture(directory, session)


def case7(args):
    directory, config = prepare(args, "case-7", 7)
    session = Session(args.app, config)
    try:
        session.command("initialize", "Initialized.")
        # Send these consecutively to capture the actual pre-arrival idle window.
        # No scheduler counter, utilization value, or process state is overridden.
        start = session.send("scheduler-test\nscreen-ls")
        initial = session.wait(lambda text: "Sleeping processes:\n" in text
                               and PROMPT in text.split("Sleeping processes:\n", 1)[1], start)
        readings = [utilization(initial)]
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline and 100 not in readings:
            session.duration(0.05)
            readings.append(utilization(session.report()))
        assert 0 in readings and 100 in readings, readings
        return {"batch_process_freq": 30, "delay_per_exec": 1,
                "snapshots": readings, "idle_window": "before first batch arrival"}
    finally:
        capture(directory, session)


def case8(args):
    directory, config = prepare(args, "case-8", 8)
    session = Session(args.app, config)
    try:
        session.command("initialize", "Initialized.")
        session.command("scheduler-test", "Batch generation started.")
        session.duration(10)
        snapshots = []
        for index in range(5):
            if index:
                session.duration(5)
            report = session.report()
            row = {"utilization": utilization(report), **counts(report)}
            assert row["utilization"] == 100 and row["running"] == 16 and row["finished"] < 10, row
            snapshots.append(row)
        session.command("scheduler-stop", "Batch generation stopped.")
        session.command("report-util", "Report generated at ")
        # Wait for the terminating prompt to ensure close()/flush completed.
        saved = (directory / "csopesy-log.txt").read_text(encoding="utf-8")
        final = {"utilization": utilization(saved), **counts(saved)}
        assert final["utilization"] == 100 and final["running"] == 16 and final["finished"] < 10, final
        return {"delay_per_exec": 5, "snapshots": snapshots, "saved_report": final}
    finally:
        capture(directory, session)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True,
                        help="New directory for real transcripts, configs, report and results.json")
    args = parser.parse_args()
    args.app = args.app.resolve()
    args.runner = args.runner.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    results = {}

    def run(number, function):
        started = time.monotonic()
        try:
            result = {"passed": True, **function(args)}
        except Exception:
            result = {"passed": False, "error": traceback.format_exc()}
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        return number, result

    with ThreadPoolExecutor(max_workers=5) as pool:
        futures = [pool.submit(run, number, function) for number, function in
                   [(4, case4), (5, case5), (6, case6), (7, case7), (8, case8)]]
        for future in as_completed(futures):
            number, result = future.result()
            results[str(number)] = result
            print(f"Question {number}: {'PASS' if result['passed'] else 'FAIL'} "
                  f"({result['elapsed_seconds']:.2f}s)", flush=True)
    summary = {"source": "Detailed-Specs.pdf, Questions 4-8",
               "question6_policy": "Separate runner: 5 ms/tick during generation, 1 ms/tick after stop",
               "question7_limit": "Idle captured before the first arrival; recurring idle is not guaranteed",
               "cases": dict(sorted(results.items()))}
    (args.output / "results.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print("Results:", args.output / "results.json", flush=True)
    return 0 if all(row["passed"] for row in results.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
