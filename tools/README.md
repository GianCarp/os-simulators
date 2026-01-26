# Workload Generator notes

`workload_generator.c` is a command line tool for generating synthetic workloads for `schedsim`. The generator produces text files in the format expected by the scheduler:

```bash
<id> <arrival_time> <run_time>
```

Each line in the file represents a single job, where:
- `id` is a job id which starts at 1. It is analogous to a PID and is used  for deterministic tie-breaking when jobs are otherwise equivalent under a  scheduling policy.
- `arrival_time` is the time at which the job becomes ready.
- `run_time` is the total CPU time required by the job.

Generated workload files can be fed directly into `schedsim` to compare the  behaviour of different scheduling policies.
## Build and run

The workload generator is built alongside the other project binaries using the top level Makefile:

```bash
make workload_gen
```

Which produces the binary:

```bash
build/workload_gen
```

The generator is invoked as:

```
./build/workload_gen -n N [flags] > workload.txt
```

The `-n` flag is required and specifies the number of jobs to generate. The generator uses output redirection rather than explicit file I/O. This keeps the implementation simple, supports piping and output inspection, and matches classic Unix tool behaviour.

**Optional command line flags:**

- `--seed s` the seed for the random number generator, using the same seed will reproduce the same workload. Default seed is current time. 
- `--arrival {uniform|burst}` sets the arrival time distribution.  Default is `burst`
	- `uniform` arrival times are chosen uniformly at random in the range `[0, max-arrival]`
	- `burst` jobs arrive in clusters, used to simulate high periods of contention. 
- `--max-arrival T` maximum arrival time, default is 50.
- `--min-run R` the minimum CPU run time for a job, default is 1
- `--max-run R` the maximum CPU run time for a job, default is 20
- `--burst-prob P` in burst mode, the probability that consecutive jobs share the same arrival time. Higher values produce more dense arrival clusters. Valid range `[0, 100]`, default 35.
- `-h` display usage information.

## Examples

Generate a small bursty workload with 10 jobs:

```bash
./build/workload_gen -n 10 > schedsim/workloads/bursty_10.txt
```

Generate a uniform arrival workload with a fixed seed:

```bash
./build/workload_gen -n 20 --arrival uniform --seed 42 > schedsim/workloads/uniform_20.txt
```

Generate a workload with longer-running jobs:

```bash
./build/workload_gen -n 15 --min-run 5 --max-run 30 > schedsim/workloads/long_jobs.txt
```

These files are then passed directly to schedsim, e.g:

```bash
./build/schedsim schedsim/workloads/bursty_10.txt --all --time-slice 4
```

## Design notes

The generator produces deterministic workloads and has the following  properties:

- Workloads can be regenerated exactly using a fixed random seed.
- Arrival patterns and run-time ranges can be adjusted from the command line.
- Generated workloads conform to the assumptions enforced by `schedsim`  (sequential job IDs, non-negative arrival times, positive run times).

The generator is designed to produce small, focused workloads that make differences between scheduling policies easy to observe and analyse.

## Arrival patterns

The generator supports two arrival patterns:

- **uniform**:  jobs arrive at random times within a fixed window
- **bursty**: jobs arrive in clusters, simulating alternating periods of high and low contention

## Argument parsing 

The generator performs simple, explicit command line parsing by iterating over `argv` and matching each argument against the supported flags. Each flag updates internal configuration parameters (e.g. number of jobs, arrival pattern, run-time bounds) that are later used during workload generation.

Numeric arguments are parsed using a small helper function (`parse_int`) rather than `atoi`. This ensures that arguments are fully validated (the entire string must represent a valid integer) and allows malformed input to be detected early, rather than being silently accepted.

This approach keeps the generator implementation straightforward, avoids hidden behaviour, and makes the relationship between command-line flags and generation parameters explicit.

## Arrival time generation

Arrival times are generated using a small helper function,  `rand_inclusive(lo, hi)`, which returns a pseudo-random integer in the inclusive  range `[lo, hi]`.   This helper function is used, rather than `%` arithmetic throughout the code, to centralises the logic for bounded random number generation, make the intended range explicit, and avoid repeated inline arithmetic throughout the generator.

In uniform mode, each job’s arrival time is generated independently using  `rand_inclusive(0, max-arrival)`.

In burst mode, arrival times are generated incrementally using a shared  `current_arrival` value. The first job is assigned a random starting arrival  time. For each subsequent job, a percentage roll determines whether the job arrives at the same time as the previous one (forming a burst) or whether the  arrival time advances by a small random step. The likelihood of remaining in the same burst is controlled by the `--burst-prob` parameter.

This approach keeps the arrival-generation logic simple and readable, while making the relationship between configuration parameters and workload behaviour easy to reason about when comparing scheduling policies.
