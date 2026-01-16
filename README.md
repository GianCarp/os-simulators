# OS Policy Simulators (Virtual Memory + Scheduling)

This repo contains simulators exploring operating system policies:
- Virtual memory page replacement (FIFO, LRU, CLOCK, RAND)
- Thread scheduling (planned)
- Interactive driver to run experiments (planned)

## Background

The virtual memory simulator began as part of an Operating Systems course
assignment, where LRU, CLOCK, and RAND page replacement policies were
implemented and analysed using real memory traces. FIFO and clean-clock added later as an extension.

After completing the assignment, the simulator was
refactored and extended to improve correctness, performance, structure,
and reproducibility. The repository is being expanded to include a thread
scheduling simulator and a unified driver for interactive experimentation.

## Build

```bash
make memsim
./build/memsim <tracefile> <frames> <rand|fifo|lru|clock> <quiet|debug> [seed]
```
Example:

```bash
make memsim
./build/memsim memsim/traces/gcc.trace 64 lru quiet
```
## Documentation

Detailed design notes and implementation commentary for the virtual memory
simulator are available here:

- [`memsim/README.md`](memsim/README.md)

This includes:
- Data structure design (`page_table`, `frame_entry`)
- Page replacement algorithms (LRU, FIFO, CLOCK, Clean-CLOCK, RAND)
- Detailed execution flow and implementation rationale


