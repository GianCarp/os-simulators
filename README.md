# OS Policy Simulators (Virtual Memory + Scheduling)

This repo contains simulators exploring operating system policies:
- Virtual memory page replacement (FIFO, LRU, CLOCK, CLEAN-CLOCK, RAND)
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

## Build and run

```bash
make memsim
./build/memsim <tracefile> <frames> <rand|fifo|lru|clock|clean-clock> <quiet|debug> [seed]
```
Example run:

``` bash
# From the repository root
./build/memsim ./memsim/traces/gcc.trace 50 clock quiet
total memory frames:                 50
events in trace:                1000000
total disk reads:                 70204
total disk writes:                10495
page fault rate (%):             7.0204
seed:                                 1
```



## Documentation

Detailed design notes and implementation commentary for the virtual memory
simulator are available here:

- [`memsim/README.md`](memsim/README.md)

This includes:
- Data structure design (`page_table`, `frame_entry`)
- Page replacement algorithms (LRU, FIFO, CLOCK, CLEAN-CLOCK, RAND)
- Detailed execution flow and implementation rationale


