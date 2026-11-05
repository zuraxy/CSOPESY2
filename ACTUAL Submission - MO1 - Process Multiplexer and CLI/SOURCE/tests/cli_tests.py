"""CLI regression checks. Uses pipes to verify the actual executables, including
live output while no new input arrives. Unit tests do not depend on this clock.
"""
import argparse
import codecs
import os
from pathlib import Path
import re
import subprocess
import tempfile
import threading
import time
import unittest

PROMPT = "root:\\> "


def temporary_directory():
    return tempfile.TemporaryDirectory(prefix="csopesy_cli_", dir=os.environ.get("CSOPESY_TEST_TMP"))


def config_file(directory, *, cores=1, quantum=20, frequency=1,
                minimum=100000, maximum=100000, delay=0):
    path = Path(directory) / "config.txt"
    path.write_text(
        f'num-cpu {cores}\nscheduler "rr"\nquantum-cycles {quantum}\n'
        f'batch-process-freq {frequency}\nmin-ins {minimum}\nmax-ins {maximum}\n'
        f'delay-per-exec {delay}\n', encoding="utf-8")
    return path


class Session:
    def __init__(self, app, config=None, workload=None, cwd=None):
        command = [str(app)]
        if config:
            command += ["--config", str(config)]
        if workload:
            command += ["--workload", workload]
        self.process = subprocess.Popen(command, cwd=cwd, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.output = ""
        self.condition = threading.Condition()
        self.eof = False
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()
        self.wait(lambda text: PROMPT in text)

    def _read(self):
        decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        try:
            while True:
                chunk = os.read(self.process.stdout.fileno(), 65536)
                if not chunk:
                    break
                with self.condition:
                    self.output += decoder.decode(chunk).replace("\r", "")
                    self.condition.notify_all()
        finally:
            with self.condition:
                self.eof = True
                self.condition.notify_all()

    def wait(self, predicate, start=0, timeout=8):
        deadline = time.monotonic() + timeout
        with self.condition:
            while not predicate(self.output[start:]):
                if self.eof or time.monotonic() >= deadline:
                    raise AssertionError("Expected CLI output was not received:\n" + self.output[-4000:])
                self.condition.wait(max(0, deadline - time.monotonic()))
            return self.output[start:]

    def duration(self, seconds):
        # This is an explicit elapsed-time requirement in the assessment, not
        # a substitute for synchronization. Output expectations use wait().
        deadline = time.monotonic() + seconds
        with self.condition:
            while time.monotonic() < deadline:
                if self.eof:
                    raise AssertionError("CLI ended during the required waiting interval")
                self.condition.wait(deadline - time.monotonic())

    def send(self, command):
        with self.condition:
            start = len(self.output)
        self.process.stdin.write((command + "\n").encode("utf-8"))
        self.process.stdin.flush()
        return start

    def command(self, command, expected, timeout=8):
        start = self.send(command)
        return self.wait(lambda text: expected in text, start, timeout)

    def report(self, command="screen-ls"):
        start = self.send(command)
        return self.wait(lambda text: "Sleeping processes:\n" in text
                         and PROMPT in text.split("Sleeping processes:\n", 1)[1], start)

    def close(self):
        if self.process.poll() is None:
            self.process.stdin.close()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
                raise AssertionError("CLI did not stop after input EOF")
        self.reader.join(timeout=5)
        self.process.stdout.close()
        if self.process.returncode:
            raise AssertionError(f"CLI exit code {self.process.returncode}:\n{self.output[-2000:]}")

    def __enter__(self):
        return self

    def __exit__(self, kind, value, traceback):
        self.close()


def utilization(text):
    return int(re.search(r"CPU utilization: (\d+)(?:\.\d+)?%", text).group(1))


def counts(text):
    return {
        "running": len(re.findall(r"\tCore: \d+\t", text)),
        "finished": len(re.findall(r"\tFinished\t", text)),
        "ready": len(re.findall(r"\tReady\t", text)),
        "sleeping": len(re.findall(r"\tSleeping\t", text)),
    }


def values(text):
    return [int(value) for value in re.findall(r'"Value from: (\d+)"', text)]


class CliTests(unittest.TestCase):
    app = None
    runner = None

    def test_default_config_is_executable_relative(self):
        with temporary_directory() as directory:
            result = subprocess.run([str(self.app)], cwd=directory,
                                    input="screen-ls\ninitialize\nexit\n", text=True,
                                    capture_output=True, timeout=8)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("Run initialize first.", result.stdout)
            self.assertIn("Initialized.", result.stdout)
            self.assertNotIn("Could not open", result.stdout)

    def test_aliases_and_report_location(self):
        with temporary_directory() as directory:
            config = config_file(directory)
            with Session(self.app, config) as session:
                session.command("initialize", "Initialized.")
                first = session.report("screen-ls")
                second = session.report("screen -ls")
                self.assertEqual(utilization(first), 0)
                self.assertEqual(utilization(second), 0)
                session.command("report-util", "Report generated at ")
                saved = (Path(directory) / "csopesy-log.txt").read_text()
                self.assertEqual(counts(saved), counts(second))
                self.assertEqual(utilization(saved), 0)

    def test_variable_logs_stream_without_process_smi_and_restart_at_zero(self):
        with temporary_directory() as directory:
            config = config_file(directory)
            for unused in range(2):
                with Session(self.app, config, "alternating") as session:
                    start = session.send("initialize\nscreen -s proc-01")
                    text = session.wait(lambda text: len(values(text)) >= 12, start, timeout=5)
                    observed = values(text)
                    self.assertEqual(observed[0], 0)
                    self.assertTrue(all(1 <= b - a <= 10 for a, b in zip(observed, observed[1:])))
                    self.assertIn("Lines of code: 100000", text)
                    # Main-menu commands cannot leak into the attached screen.
                    session.command("scheduler-start", "Available commands: process-smi, exit")
                    session.command("exit", "Welcome to CSOPESY Emulator!")
                    session.command("screen -r proc-01", "Process name: proc-01")

    def test_finished_screen_cannot_be_resumed(self):
        with temporary_directory() as directory:
            config = config_file(directory, minimum=1, maximum=1)
            with Session(self.app, config, "alternating") as session:
                session.command("initialize", "Initialized.")
                session.command("screen -s done", "Process name: done")
                session.wait(lambda text: "Finished!" in text)
                session.command("exit", "Welcome to CSOPESY Emulator!")
                session.command("screen -r done", "Process done not found.")
                self.assertEqual(counts(session.report())["finished"], 1)

    def test_question7_initial_idle_and_later_busy_are_real(self):
        with temporary_directory() as directory:
            config = config_file(directory, quantum=10, frequency=30, minimum=30, maximum=45, delay=1)
            with Session(self.app, config) as session:
                session.command("initialize", "Initialized.")
                start = session.send("scheduler-test\nscreen-ls")
                initial = session.wait(lambda text: "Sleeping processes:\n" in text
                                       and PROMPT in text.split("Sleeping processes:\n", 1)[1], start)
                self.assertEqual(utilization(initial), 0)
                deadline = time.monotonic() + 3
                while time.monotonic() < deadline:
                    session.duration(0.05)
                    if utilization(session.report()) == 100:
                        break
                else:
                    self.fail("No occupied-core snapshot in Question 7")

    def test_separate_runner_announces_policy(self):
        with Session(self.runner) as session:
            self.assertIn("5 ms/tick while generating; 1 ms/tick after scheduler-stop", session.output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True)
    arguments, remaining = parser.parse_known_args()
    CliTests.app = arguments.app.resolve()
    CliTests.runner = arguments.runner.resolve()
    unittest.main(argv=["cli_tests.py"] + remaining)
