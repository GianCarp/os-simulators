# OS Simulators

This repository contains a collection of simulators for exploring
core operating system policies in controlled environments. Each simulator
isolates a specific OS mechanism, i.e. CPU scheduling or virtual memory replacement,
and exposes the policies behaviour through reproducible workloads and detailed performance metrics.

The goal of the project is not to emulate a full operating system, but to make
individual policy decisions explicit, observable, and comparable, while keeping
implementations simple enough to reason about and extend.

## Project layout

```
os-simulators/
├── memsim/ # Virtual memory page replacement simulator
├── schedsim/ # Single-core CPU scheduling simulator
├── tools/ # Supporting tools (i.e. workload generator)
├── build/ # Build outputs (ignored by git)
├── makefile # Top-level build orchestration
└── README.md # This file
```

Each subdirectory contains its own README with detailed design notes and usage instructions.

## Components

### `memsim/` — Virtual Memory Simulator

`memsim` simulates page replacement policies using real memory traces. It is
designed to explore how different page replacement strategies behave under realistic
access patterns.

Supported policies include:

- FIFO
- Random
- Clock
- Clean-clock
- LRU simple - O(n) scan
- LRU advanced - O(1) implementation

The simulator reports page fault rates, disk reads, and disk writes, and supports
deterministic replay via a fixed RNG seed.

See [`memsim/README.md`](memsim/README.md) for full details.

---

### `schedsim/` — CPU Scheduling Simulator

`schedsim` is a single-core CPU scheduling simulator used to compare the
behaviour of different scheduling policies under controlled workloads.

Currently supported policies:

- First Come First Served (FCFS)
- Shortest Job First (SJF)
- Round Robin (RR)

For a given invocation of the program, each policy is run against the same immutable workload. The simulator reports standard scheduling metrics such as response time, turn-around time, waiting time, and makespan.

The implementation focuses on:

- explicit policy boundaries
- private per-policy state
- clear, explainable scheduling decisions

See [`schedsim/README.md`](schedsim/README.md) for architecture, metrics, and
policy details.

---

### `tools/` — Supporting Utilities

The `tools` directory contains helper programs used to generate inputs for the
simulators.

#### `workload_gen`

A command-line workload generator for `schedsim` that produces synthetic job
arrival patterns and run-time distributions.

Features:

- deterministic output via fixed seeds
- uniform and bursty arrival patterns
- configurable arrival windows and run-time ranges
- Unix-style output via stdout (supports piping and inspection)

See [`tools/README.md`](tools/README.md) for usage and implementation notes.

---

## Building the project

All components are built from the repository root using the top-level Makefile.

To build everything:

```bash
make
```
This produces the following binaries in build/:

``` bash
build/memsim
build/schedsim
build/workload_gen
```

Individual components can also be built explicitly:

```bash
make memsim
make schedsim
make workload_gen
```

## Workflow

A common workflow when exploring scheduling behaviour is:

1. Generate a workload: `./build/workload_gen -n 20 > schedsim/workloads/example.txt` 
2. Run the scheduler: `./build/schedsim schedsim/workloads/example.txt --all --time-slice 4`
3. Compare metrics across policies and reason about trade-offs.

Similarly, memsim can be run directly against real memory traces provided in `memsim/traces/`

## Future work
Planned extensions include:
- Multi-Level Feedback Queue (MLFQ) scheduling
- Ticket-based scheduling (lottery / stride)
- Multi-core scheduling support
- Shared analysis tooling across simulators

