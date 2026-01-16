
# Memory sim notes

## Build and run

```bash
make memsim
./build/memsim <tracefile> <frames> <rand|fifo|lru|clock> <quiet|debug> [seed]
```

## implementation notes

### evicted_page struct

The `evicted_page` struct is used as a record of an evicted page.

```c
typedef struct {
	int vpn;
	int dirty;
} evicted_page;
```

This is not relating to a page in memory, but rather a page that was in memory but has since been evicted. It is returned via `replacePage()` to inform the caller of what has occurred as a result of the page swap.

`replacePage()` needs to return two pieces of information:
1. which page was evicted - used for debug mode and observability  
2. if it was a dirty page - to update `disk_writes` 

### frame_entry struct

The `frame_entry` struct is an object that describes the state of a single physical frame in memory.

```c
typedef struct {
	int vpn; // which page is in this frame, -1 if free
	int dirty; // 0 or 1 for clean or dirty
	int access_time; // LRU timestamp
	int ref; // clock ref bit
} frame_entry;
```

This represents a physical frame (PFN) in RAM, and all the metadata associated with it, which is useful for the replacement algorithm. The simulator tracks metadata needed by multiple policies (LRU uses  `access_time`, CLOCK uses `ref`, etc.). A real OS would not necessarily track  
all of this simultaneously if only one policy were in use.

In addition to this struct, memory is modeled via `page_table` which is a linear page table, that maps VPN to PFN. 

The index of `frame_entry` and `page_table` are different:

- The `page_table` index is a VPN, with the content being them physical frame. `page_table[VPN] = PFN` (or `-1`)

- `frame_entry` index into is the physical frame, and the content is metadata about that frame. 

- `page_table` used for : I have this VPN, what frame is it in?
- `frame_entry frame_data[PFN].vpn` used for: for this physical frame, what is in it?

We need `page_table` to see if a page is in memory or not and we need the `frame_table `as replacement policies evict a page from a frame - so need to track metadata with a given frame.

### createMMU()

`createMMU()` initialise simulator state for a machine with 'frames' physical frames. Returns 0 on success, -1 on allocation failure. Calling `createMMU()` allocates memory on the heap. This should be freed during normal shutdown and is essential if the code is reused in a long running process or driver that runs multiple simulations.

```c
int createMMU(int frames) {
	numFrames = frames;
	
	page_table = (int *)malloc(NUM_PAGES * sizeof(int));
	frame_data = (frame_entry *)calloc(frames, sizeof(frame_entry));
	
	if (!page_table || !frame_data) {
		return -1;
	}
	
	for (int i = 0; i < NUM_PAGES; i++) {
		page_table[i] = -1;
	}
	
	for (int f = 0; f < frames; f++) {
		frame_data[f].vpn = -1;
	}
	return 0;
}
```

The simulator models a 32-bit virtual address space with fixed 4 KB pages. So, there are  $2^{20}$ pages. Therefore, allocate all of these possible mappings in the linear page table. Initialise the value of each page table entry to -1 as no pages are in memory initially.

Similarly, set the vpn in `frame_data` array to -1, with other fields within frame_data being set to zero by calloc - as needed. `malloc()` used for `page_table` as after initiliasing immediately setting all values to -1, no need to initialise to 0 with `calloc()`.

`createMMU()` is called once, at the start of `main()`, once all command line arguments have been successfully parsed. 

### Design choice, frame_entry and createMMU() refactor

Initially, didn't have the `frame_entry` struct, as such was maintaining a separate array for each of the fields within `frame_entry`.


```c
// OLD IMPLEMENTATION
int createMMU(int frames) {
	numFrames = frames;
	
	page_table = (int *)calloc(NUM_PAGES, sizeof(int));
	frame_table = (int *)calloc((size_t)frames, sizeof(int));
	dirty_bit = (int *)calloc((size_t)frames, sizeof(int));
	access_time = (int *)calloc((size_t)frames, sizeof(int));
	recently_used = (int *)calloc((size_t)frames, sizeof(int));
	
	if (!page_table || !frame_table || !dirty_bit || !access_time ||
		!recently_used) {return -1;}
	
	for (int i = 0; i < NUM_PAGES; i++) {
		page_table[i] = -1;
	}
	
	for (int f = 0; f < frames; f++) {
		frame_table[f] = -1;
	}
	
	return 0;
}
```

### checkInMemory()

`checkInMemory()`  determines if a given VPN is in memory or not. It returns the corresponding PFN if present, or -1 if not. 

On a hit, it updates per-frame metadata used by replacement policies:
- LRU: updates the last-access timestamp
- clock: sets the reference bit

```c
int checkInMemory(int vpn) {
	int result = page_table[vpn];
	if (result != -1) {
		frame_data[result].access_time = _time++; // LRU	
		frame_data[result].ref = 1; // clock
	}	
	return result;
}
```

`checkInMemory()` is called as the first step of each memory access. 

### allocateFrame()

In this simulator, we assume physical memory starts empty. So, we allocate free frames sequentially (PFN: 0, 1, 2, …) until physical memory is full, i.e. use `allocateFrame()` until memory is full and then  we need to start using the specified page replacement policy. 

```c
int allocateFrame(int vpn) {
	int pfn = next_frame++;
	page_table[vpn] = pfn;
	
	frame_data[pfn].vpn = vpn;
	frame_data[pfn].dirty = 0;
	frame_data[pfn].access_time = _time++;
	frame_data[pfn].ref = 1;
	
	return pfn;
}
```


`next_frame` is a global integer, initiliased to 0, that is used to place the pages contiguously while `allocated` is less than `frames` specified by the command line argument. In the main loop:

```c
// ...
if (allocated < frames) {
	pfn = allocateFrame(vpn);
	allocated++;
} else {
	Pvictim = replacePage(vpn, replace, &pfn);
// ....
```

`allocateFrame()` simply places the vpn into the next free frame, and updates the metadata within `frame_data`. 

`allocateFrame()` returns the pfn that was just used to store the vpn, so that after calling we can inspect if we have performed a write operation on the page. If so we mark the page as dirty so that we know we have to save it to disk when replacing the page.

```c
if (pfn != -1 && rw == 'W') {
	frame_data[pfn].dirty = 1;
}
```

Note that `pfn != -1` part is a defensive guard. `pfn == -1` indicates a page fault, which should have already been serviced via `allocateFrame()` in the case where there are still free frames in memory, or by `replacePage()` in the case where we need to swap a page. For execution to reach this part of the code with a `pfn == -1` is a bug we are protecting against here.

Regardless:

- If the first access after loading is a read, the page is still identical to what’s on disk so it’s clean, so if you evict it later you can just discard it.

- If the first access after loading is a write, you’ve modified the in-memory copy so it’s dirty, so if you evict it later you must write it back, or we would lose the change

### replacePage()

`replacePage()` perform page replacement when memory is full:
- selects a victim frame based on the specified replacement mode
- evicts a page from the frame, and places the new vpn into that frame. 
- returns metadata about the evicted page
- writes the pfn for the new page into new_frame

`victim_frame` is the pfn that we are swapping a page into. Initialise to -1 as a guard. Before doing any page swaps if `victim_frame < 0` throw an error. 

#### LRU

LRU victim selection is implemented by scanning frames for the minimum timestamp, this is O(numFrames), which is fine for this simulator where performance isn't required - this is the reason LRU is approximate with clock anyway in real systems? 

#### Random

RAND needs to pick a victim frame uniformly at random from within  `[0, numFrames-1]`.  Every frame needs to have the same probability of selection for it to be truly random.

Initially had:

```c
victim_frame = rand() % numFrames;
```

However this is not random, and actually introduces modulo bias. We are taking all possible values `rand()` can produce and grouping them into buckets using `%`. This is only fair if each bucket ends up with the same number of values - which will not be the case. 

**Example:**

`val = rand() % 4;`

If rand can produce 10 values, `[0,9]` and we want to chose from 4 frames we would have something like this:

```bash
0 % 4 = 0
1 % 4 = 1
2 % 4 = 2
3 % 4 = 3
4 % 4 = 0
5 % 4 = 1
6 % 4 = 2
7 % 4 = 3
8 % 4 = 0
9 % 4 = 1
```

Which produces the following distribution: 

| Frame | Values that map to it | Count |
| ----- | --------------------- | ----- |
| 0     | 0, 4, 8               | 3     |
| 1     | 1, 5, 9               | 3     |
| 2     | 2, 6                  | 2     |
| 3     | 3, 7                  | 2     |
So frames 0 and 1 are 50% more likely to be selected than 2 and 3, as more values within the acceptable range have a remainder that corresponds to this frame.

Essentially, need to realise that `%` doesn't create randomness, it groups numbers into a distribution that likely doesn't have equal probabilities. 

So, in practice to ensure an even distribution we reduce our range. Specifically, we reduce it to be one less than largest multiple of the operand on the right of `%` that is less than the size of our range. So for the above example, the largest multiple of 4 less than or equal to the number of possible values (10) is 8. So we make our range `[0,7]` which results in the following, even, distribution: 

| Frame | Values that map to it | Count |
| ----- | --------------------- | ----- |
| 0     | 0, 4                  | 2     |
| 1     | 1, 5                  | 2     |
| 2     | 2, 6                  | 2     |
| 3     | 3, 7                  | 2     |

Back to the implementation within memsim!

1. Firstly, define the amount of buckets we want with  `unsigned span = (unsigned)numFrames;` could use `numFrames` directly but this is trying to decouple the number of frames within the system and the logic of removing modulo bias. Also supports refactor easily, by changing span directly in one place.

2. `unsigned limit = (RAND_MAX + 1u) - ((RAND_MAX + 1u) % span);` `RAND_MAX+1u` is the total number of values `rand()` can produce. `(RAND_MAX + 1) % span` are the leftover values that don’t fit evenly into `span`. So now we have the range `[0, limit-1]` which is all divisible by `span`, i.e. modulo operations will produce even buckets.

3. Keep using `rand()` until we get a value that falls within our desired range, at which point determine the victim frame with modulo operation. 

**RAND summary:** The policy selects a victim frame uniformly at random. Rather than using `rand() % numFrames`, which introduces modulo bias when the random number range is not evenly divisible, the implementation uses rejection sampling. Random values greater than or equal to a computed `limit` are discarded, ensuring that the remaining values map evenly onto frame indices and produce a truly uniform distribution.

#### FIFO

Evict the page that has been resident in memory the longest.  `fifo_hand` is a circular pointer over frames. It always points to the next frame to evict, it wraps back to 0 once we have hit `numFrames`. This works because we are sequentially storing pages in frames. 

#### Clock 

Clock is an approximation of LRU that avoids maintaining a data structure for full access order. Recently used pages are given a second chance before eviction, via a reference bit for each frame. The bit is set to 1 whenever the page is accessed, and cleared by the algorithm when it searches for a page to remove. 

Clock is implemented similarly to FIFO in that a pointer acts as an iterator over the frames in a circular fashion.

```c
else if (mode == _clock) {
	while (frame_data[clock_hand].ref == 1) {
		frame_data[clock_hand].ref = 0;
		clock_hand = (clock_hand + 1) % numFrames;
	}
	victim_frame = clock_hand;
	clock_hand = (clock_hand + 1) % numFrames;
}
```

1. The algorithm inspects the frame pointed to by `clock_hand`
2. If the frames reference bit is set (1), clear it and advance to next frame, repeating until a frame with reference bit of 0 is found.
3. At worst, a full cycle has occurred. Regardless, we have found the frame which contains the page to evict.

Pages accessed recently are protected from eviction as their reference bit is set to 1. Worst-case time per eviction is O(numFrames)  but average behavior is efficient.This behavior closely approximates LRU at significantly lower implementation cost.

#### Clean clock

Clean clock is an extension of clock, that prefers to evict clean pages over dirty when selecting a victim with the goal of reducing writes back to disk. The basic clock implementation would evict the first page that is found with reference bit of 0, even if it was dirty - which has the overhead of writing to disk. 

Clean clock may require two passes:

**Pass 1** scans frames, clearing reference bits as needed, and evicts the first frame encountered with `ref = 0` and `dirty = 0`.

**Pass 2** is a safety mechanism, in the case where all frames are dirty repeat the simple clock, evict the first page with ref bit set to 0.

#### Execution after selecting the victim frame

Regardless of what policy is selected, as mentioned above a check occurs first to ensure that a valid frame has been selected. Then the following occurs:

1. Populate a `victim` object with the vpn and dirty status from the selected frame, to track disk writes. Currently not using the return vpn from replacePage() anywhere except for debug mode.
2. Mark the victim vpn as not being present in memory, i.e. set page table value at vpn index to -1.
3. Map the incoming vpn into the selected frame
4. Reset the meta data for the frame

The last thing we do is:

```c
if (new_frame) {
	*new_frame = victim_frame;
}
```

Firstly, the use of the if statement is so `replacePage()` can  safely be called without requesting the PFN of the newly loaded page, preventing NULL dereference and making the API more flexible. 

The purpose of updating `*new_frame` is so that after the page has been placed into a new frame we can set this frames bit to dirty if we have a write, i.e.:

```c
if (pfn != -1 && rw == 'W') {
	frame_data[pfn].dirty = 1;
}
```

where `pfn` is passed in the argument as `*new_frame` 

## memsim main execution

**1. Argument parsing**

`main()` expects the following command line arguments:

`./memsim <tracefile> <frames> <policy> <quiet|debug> [seed]
`
where:

- **tracefile**: file containing the memory access, of form `<address> <R|W>`
- **frames**: the number of physical frames available 
- **policy**: page replacement policy one of LRU, FIFO, clock, clock-clean, and rand
- **debug mode**: 'quiet' for summary only 'debug' for every event
- **seed**: optional seed for the random replacement policy


**2. Simulator initialisation**

- `createMMU()` called, allocates all simulator data structures on the heap. If allocation fails, program exits

**3. Trace loop**
For each memory access:
1. Extract VPN from address
2. Check residency with `checkInMemory()`
3. On page fault:
   - Allocate a free frame, or
   - Replace a page using the selected policy
4. If the access is a write, mark the frame dirty
5. Update counters and optional debug output
 






