# CSOPESY practice cases

Build the emulator once using the project README. From the project folder in PowerShell, start a practice case with its config file.

```powershell
.\rehearsal\Run-Case.ps1 .\rehearsal\fcfs.txt
```

Run the command again with `rr.txt`, `batch.txt`, or `lifecycle.txt` to switch cases. Each launch uses the same `build\csopesy.exe` and copies the chosen file into a fresh run folder as `config.txt`. Type `initialize` in the emulator to load it. If you use `report-util`, the run folder also contains `csopesy-log.txt`. The project’s default config stays in place.

When you receive an official case, put its settings into a text file with one key and value per line, then pass that file to the same launcher. The config controls the algorithm, cores, quantum, batch interval, instruction count range, and execution delay. Run any command sequence specified by the question at the emulator prompt. To launch manually, place the desired `config.txt` in a fresh folder and run the existing executable from there.

## 1. FCFS on one core

Use `fcfs.txt`.

1. Enter `initialize`, then `screen -ls`. Expect 0% utilization and one available core.
2. Enter `screen -s alpha`, then `exit`. Enter `screen -s beta`, then `exit`.
3. Run `screen -ls` and reopen the processes with `screen -r alpha` and `screen -r beta`. Use `process-smi` inside each screen and `exit` to go back.

Each process has 1,000 instructions. At most one process is running at a time. FCFS has no quantum-based rotation. A generated `SLEEP` can still release the core and let the other process run.

## 2. Round Robin on one core

Use `rr.txt`. Repeat case 1 with the same process names and commands. Only the scheduler setting differs, and the RR quantum is two ticks.

Take several `screen -ls` snapshots or reopen both processes and use `process-smi`. With both runnable, both should advance before either finishes. A screen snapshot can miss a switch because the quantum is short. Exact per-tick order is covered by the deterministic scheduler tests, while the CLI uses random workloads.

## 3. Four cores, batch generation, and file report

Use `batch.txt`.

1. Enter `initialize`, then `scheduler-start`. Let it run for about two seconds, then enter `scheduler-stop`.
2. Run `screen -ls`. Under load, expect four used cores and 100% utilization. Check that the running rows have distinct core numbers.
3. Open the first generated process with `screen -r p01` while it is active. Enter `process-smi`, then `exit` to return to the main menu.
4. Run `report-util` and open the printed file path. The file has the same sections as `screen -ls`. Counts and progress can change between commands because execution continues after batch generation stops.

## 4. Process lifecycle and command modes

Use `lifecycle.txt`.

1. Before initialization, enter `screen -ls`. Expect `Run initialize first.`
2. Enter `initialize`, then `screen -s demo`. In the process screen, enter `process-smi`. Repeat until it shows `Finished!`.
3. Enter `exit`, then `screen -r demo`. Expect `Process demo not found.`
4. Run `screen -ls` and `report-util`. The finished section should contain `demo` with `60 / 60`, and the single core should be available. Enter `exit` again to quit the emulator.
