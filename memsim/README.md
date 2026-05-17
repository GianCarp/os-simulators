
# Memory sim notes

This simulator models a simplified virtual memory system with a linear page table and multiple page replacement policies supported - FIFO, simple LRU, advanced LRU, clock, clean-clock, and rand. All simulator state is owned by a single Memory Management Unit (MMU) struct, which represents the global memory subsystem.

## Build and run

```bash
# From the repository root build with:
make all OR make memsim
./build/memsim 
# Usage:
  ./build/memsim <tracefile> <num_frames> <policy> [-debug] [-seed N]

Policies:
  fifo | lru-simple | lru-advanced | rand | clock | clean-clock

Options:
  -debug      Print per-access trace (default: quiet)
  -seed N     Set RNG seed for rand policy (default: 1)
  -h          Show this help message

```
Where:

- **tracefile**: file containing the memory access, of the form `<address> <R|W>`. Tracefiles provided in `/memsim/traces`
- **frames**: the number of physical frames available, as desired by the user 
- **policy**: page replacement policy, one of the options as listed above

Options:
- **debug mode**: `quiet` for summary only `debug` for logging every event
- **seed**: optional seed for the random replacement policy

Example run:

``` bash
# From the repository root
./build/memsim ./memsim/traces/gcc.trace 50 clock
total memory frames:                 50
events in trace:                1000000
total disk reads:                 70204
total disk writes:                10495
page fault rate (%):             7.0204
seed:                                 1
```

## Implementation notes

### Structs
The simulator uses a mix of public and internal structs to achieve a clean API and hide implementation details from callers. The split enforces an opaque design, where callers are only able to interact with the simulator state through the API functions; i.e. using a pointer to a heap allocate `mmu` struct as opposed to creating the object on the stack and having access to internal fields directly. 

#### mmu struct
```c
typedef struct mmu mmu;
```
`mmu` is declared as an incomplete type in `memsim.h`, with its definition being in `memsim.c`. Callers obtain a `mmu *` returned by `create_MMU()` and pass it to every API function. It is not possible for callers to access fields directly. All simulator state lives in this struct, it owns the heap allocated `page_table` and `frame_data`.

```c
struct mmu {
  int numFrames;
  int *page_table;       // VPN -> PFN, -1 if not resident
  frame_entry *frame_data; // per-frame metadata array
  int time;              // time used in simple LRU 
  int clock_hand;        // current position for clock algorithms
  int fifo_hand;         // current position for FIFO
  int next_frame;        // next free frame index during initial fill
  lru_tracker lru_state; // head/tail pointers for the DLL used in advanced LRU
  enum repl mode;        // active replacement policy
};
```
`mode` is stored within the struct so that `check_in_memory()` and `allocate_frame()` only update the relevant per-frame metadata without needing the policy passed as a parameter on every call. 

In a real OS, the `page_table` and `frame_data` are separate kernel managed structures. The page table is a per-process data structure stored in the kernel space of a processes address space, while the frame metadata (`mem_map`) is a global structure used by the paging policy to track which pages are in which frames, along with required metadata. As this simulator is for a single process, both of these are contained in `mmu` for simplicity.

#### frame entry

```c 
typedef struct {
  int vpn;   // which page is in this frame, -1 if free
  int dirty; // if the frame has been written to, 0 or 1 for clean or dirty
  int access_time; // simple LRU timestamp
  int ref;         // clock ref bit
  lru_node node;   // each frame has a DLL node
} frame_entry;
```
`frame_entry` is a hidden struct that represents a physical frame (PFN) in RAM. This struct also contains the metadata (state) for a given physical frame. The simulator has fields for the metadata required for all policies, e.g. `access_time` for simple LRU or `ref` for clock variants. The `mode` field within `mmu` is used to ensure that only the relevant fields are updated. A real OS would have a single paging policy in place, and therefore only have the fields required.

By having `lru_node node` within `frame_entry`, the DLL node (for advanced LRU) is embedded directly into the frame metadata as opposed to some other allocation. This means that the `lru_node` for frame `f` is obtained through `mmu->frame_data[f].node`. So, given a PFN the node in the DLL for advanced LRU is reachable in O(1).

`mmu->frame_data` is indexed by PFN, it is the inverse of `mmu->page_table`.
- `page_table[vpn]` answers "is this virtual page in memory, and if so what frame?"
- `frame_data[pfn]` answers "what page is in this frame, and what is its state?"

#### LRU node

```c 
typedef struct lru_node {
  struct lru_node *prev;
  struct lru_node *next;
  int frame; // PFN this node belongs to
} lru_node;
```
Each node represents one physical frame in the LRU DLL. The `frame` field stores the PFN so that when the eviction path reaches the tail node, it can obtain the victim PFN in O(1) without a reverse lookup. Nodes are not separate heap allocations, each `lru_node` is embedded directly inside its corresponding `frame_entry` meaning that the node and its frame metadata are co-located.  


#### LRU tracker

```c 
typedef struct {
  lru_node *head; // most recently used
  lru_node *tail; // least recently used, evicted first
} lru_tracker;

```

Owns the head and tail pointers for the DLL. This list is maintained in access order, with head being the MRU frame and tail being the LRU. Eviction always occurs from the tail. A hit moves the node from its current position to head and re-links the list as appropriate. These are both O(1) operations.

#### evicted page

```c 
typedef struct {
  int vpn;   // VPN of the evicted page
  int dirty; // 1 if the page was dirty, 0 if clean
} evicted_page;
```
`evicted_page` is a record of a page that has just been evicted from physical memory. It is a field within `replace_result` that is returned by `replace_page()`. `dirty` tells the caller whether a disk write is required; a dirty eviction increments `disk_writes`, a clean one is simply discarded.

#### replace result

```c 
typedef struct {
  int new_pfn;
  evicted_page victim;
} replace_result;
```

`replace_result` is the return type of `replace_page`. This struct bundles the PFN that the incoming page is loaded into with the metadata of the page which was just evicted.

### API 

#### create_mmu()

```c 
mmu * create_MMU(int frames, enum repl mode);
```
`create_MMU` allocates and initialises all simulator state for a machine with `frames` physical frames which uses the replacement policy specified by `mode`. Returns a `mmu *` on success and `NULL` on failure. The returned pointer is to be passed to `destroy_MMU()` by the caller when the simulation is complete. 

`create_MMU()` uses `memset()` to zero initialise the `mmu` struct, then explicitly set fields that require a non-zero initial value:
- `page_table` entries are set to `-1` to indicate that each PFN has no virtual page initially. 
- `frame_data[f].vpn` is set to `-1`, as above all frames are initially free.
- `frame_data[f].node.frame` - each embedded `lru_node` stores its own PFN so that eviction can recover the PFN from the node in O(1) without a reverse lookup.
- `mode` is stored so that `check_in_memory()` and `allocate_frame()` updates only the relevant metadata per access without needing the policy passed as an argument on every call. 

`malloc()` is used for `page_table` rather than `calloc()` as all entries are immediately overwritten with `-1`. `calloc()` is used for `frame_data` as most fields correctly start with `0`; only `vpn` and `node.frame` need to be initialised explicitly.

If any allocation fails, memory is freed appropiately before returning `NULL`, ensuring no partial heap allocation. 

The simulator models a 32-bit virtual address space with 4 KB pages, so there are $2^{20}$ possible virtual pages. `page_table` allocates all of these entries upfront in a linear array.

#### destroy_mmu()

```c 
void destory_MMU()(mmu *mmu_ptr);
```

Frees all heap memory owned by the `mmu`, i.e. `frame_data`, `page_table`, and the `mmu` struct itself. Both pointer fields are set to `NULL` after freeing as a defensive measure against use-after-free. The caller should not access mmu_ptr after this call.

#### check_in_memory()

``` c
int check_in_memory(mmu *mmu_ptr, int vpn);
```
Checks if the virtual page is in physical memory, returning the PFN if present or `-1` on a page fault. This is the first step of every memory access.

On hit, updates the per-frame metadata relevant to the policy:
- `lru_simple` updates `access_time` for the frame. 
- `lru_advanced` moves the frames embedded `lru_node` to the head of the DLL via `lru_move_existing_node_head()`. 
- `_clock` and `_clean_clock` sets the reference bit to `1`.
- `fifo` and `random` has no metadata to update on hit. 

On a miss, the caller is responsible for servicing the fault by calling either `allocate_frame()` if free frames remain or `replace_page()` if memory is full. 

#### allocate_frame()

```c 
int allocate_frame(mmu *mmu_ptr, int vpn);
```
Places `vpn` into the next free (sequentail) physical frame and returns the PFN that was assigned. Only valid while free frames remain. `has_free_frames()` should be called first to verify this. Once all frames are occupied, `replace_page()` should be used instead.

Frames are assigned sequentially from `0` to `numFrames - 1` via `next_frame`, which is incremented on each call. This works as physical memory starts empty and frames are filled in order before any eviction occurs.

On allocation the following metadata is initialised for the frame:

- `vpn` set to the incoming virtual page
- `dirty` set to 0, the page is clean on load
- `access_time` set to the current timestamp for `lru_simple`
- `ref` set to 1, the page is considered recently used for clock variants
- `lru_node` pushed to the head of the DLL via `lru_new_node_push_head()`, if the active policy is `lru_advanced`

The return PFN is used by the caller to mark the frame dirty if the access that triggered the allocation was a write. 

#### replace_page()

```c 
replace_result replace_page(mmu *mmu_ptr, int vpn);
```

Called when physical memory is full so a page must be evicted from memory to enable the new page to be loaded. Victim is selected in accordance to the policy chosen for this invocation of the process. This page is evicted with the latest VPN access being loaded into the frame. Returns`replace_result` that contains the PFN that the page was loaded into and metadata of the evicted page.

`victim_frame` is initialised to `-1` as a guard. If the `victim_frame` value is still set to `-1` after executing the policy an error has occurred. Simulator calls `exit()` with an appropriate error message. This should never occur in correct execution.

Regardless of the policy, after selecting a victim: 
1. The evicted page's `vpn` and `dirty` status are recorded into the returned `replace_result`.
2. The victim's `vpn` is marked as not present in memory, i.e. `page_table[victim_vpn] = -1`
3. Incoming `vpn` is mapped into the victim frame, i.e. `page_table[incoming_vpn] = victim_frame`
4. Frame metadata is reset.

##### LRU simple
Scans all frames to find the page with the lowest `access_time`. O(n) per eviction. Simple to implement, but scales poorly with frame count.

##### LRU advanced

Uses a DLL to store frames in access order, with the head being the MRU frame and the tail being the LRU frame. So the tail is always the eviction candidate, its frame field gives the victim PFN directly in O(1). The tail is then unlinked. 

Rather than freeing the victim node and allocating a new one for the incoming page, the same node is reused in place. The incoming vpn is written into the victim frame and the node is pushed to the head via `lru_new_node_push_head()`, representing the newly loaded page as the MRU. This avoids any heap allocation during eviction.

On hit, `lru_move_existing_node_head` moves the accessed frame's node from its current position to the head, maintaining correct access order. The node is unlinked from its current position and relinked at the head. This is O(1) because each node has a `prev` and `next` pointer so relinking doesn't require scanning. 


##### Random 

Random needs to pick a victim frame uniformly at random from within  `[0, numFrames-1]`.Every frame needs to have the same probability of selection for it to be truly random.

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

##### FIFO

Evicts the frame pointed to by fifo_hand and advances the hand by one. The hand wraps circularly over [0, numFrames - 1]. This works correctly because frames are filled sequentially during initial allocation, so the oldest resident page is always at the current hand position.

##### Clock

Clock is an approximation of LRU that avoids maintaining a data structure for full access order. Recently used pages are given a second chance before eviction, via a reference bit for each frame. The bit is set to 1 whenever the page is accessed, and cleared by the algorithm when it searches for a page to remove. 

Clock is implemented similarly to FIFO in that a pointer acts as an iterator over the frames in a circular fashion, and operates as follows:

1. The algorithm inspects the frame pointed to by `clock_hand`
2. If the frames reference bit is set (1), clear it and advance to next frame, repeating until a frame with reference bit of 0 is found.
3. At worst, a full cycle has occurred. Regardless, we have found the frame which contains the page to evict.

Pages accessed recently are protected from eviction as their reference bit is set to 1. Worst-case time per eviction is O(numFrames) but average behavior is efficient. This behavior closely approximates LRU at significantly lower implementation cost.

##### Clean clock


Clean clock is an extension of clock, that prefers to evict clean pages over dirty when selecting a victim. The goal of this is to reduce the number of writes back to disk. The basic clock implementation would evict the first page that is found with reference bit of 0, even if it was dirty - which has the overhead of writing to disk. 

Clean clock may require two passes:

**Pass 1** scans frames, clearing reference bits as needed, and evicts the first frame encountered with `ref = 0` and `dirty = 0`.

**Pass 2** is a safety mechanism, in the case where all frames are dirty repeat the simple clock, evict the first page with ref bit set to 0.

#### has_free_frames()

```c 
int has_free_frames(mmu *mmu_ptr);
```
Returns `1` if free frames remain, comparing `next_frame` against `numFrames`. Returns `-1` if there are no free frames. Called in the loop before each page fault to determine if `allocate_frame()` or `replace_page()` is to be used.

#### mark_dirty()

```c 
void mark_dirty(mmu *mmu_ptr, int pfn);
```

Sets the `dirty` bit to `1` for the frame at `pfn`. Called after any write access.

#### lru_new_node_push_head()

```c 
static void lru_new_node_push_head(lru_tracker * lru, lru_node *node);
```

Inserts a node at the head of the DLL. Only safe to call when the node is not already in the list. Calling on an existing node would corrupt the list as it would not unlink the node from its current position. Used in two places:
- `allocate_frame()` when a page is loaded into a free frame for the first time.
- `replace_page()` when a victim node is reused for the incoming page, after eviction.

The new node's `prev` is set to `NULL` as it is the head. Its `next` is set to the previous head, which may be `NULL` if the list was empty. If the previous head was not `NULL`, its `prev` is updated to point to the new node. `lru->head` is then updated to the new node. If `lru->tail` is `NULL` the list was previously empty, so `lru->tail` is set to the new node making head and tail the same; i.e. the DLL has only one element.

#### lru_move_existing_node_head()

```c 
static void lru_move_existing_node_head(lru_tracker * lru, lru_node *node);
```

Moves an existing node from its current position in the DLL to head. Called by `check_in_memory()` on every cache hit for advanced LRU to maintain correct access order. Safe to call only if the node is already in the list. Handles three cases:

1. Already at head. If `lru->head == node` then return immediately. 
2. At the tail. If the node has `prev` but no `next` (i.e. `NULL`) unlink by setting `lru->tail` to `node->prev` and clearing its `next` pointer, then insert at head.
3. In the middle. Unlink by stitching the surrounding nodes together, then insert at head. 

In the non-trivial cases, after unlinking, the node is inserted at the head by setting `node->prev` to `NULL` `node->next` to the current head, updating `lru->head->prev` to the node, and lastly setting `lru->head` to the node.



### memsim main execution

**1. Argument parsing**

`main()` expects the command line arguments as shown in the example below, and detailed at the top of this README.

`./memsim <tracefile> <frames> <policy> [-debug] [seed]
`

**2. Simulator initialisation**

- `create_MMU()` called, allocates all simulator data structures on the heap. If allocation fails, program exits

**3. Trace loop**
For each memory access:
1. Extract VPN from address
2. Check residency with `check_in_memory()`
3. On page fault:
   - Allocate a free frame, or
   - Replace a page using the selected policy
4. If the access is a write, mark the frame dirty
5. Update counters and optional debug output
 
### Change log
The following list details the major changes made:
- `mmu` struct made opaque, implementation details hidden in `memsim.c`
- `create_MMU()` refactored to return `mmu *` (heap allocated) rather than taking a pointer parameter
- replace_page() returns a replace_result struct containing both the new PFN and evicted page metadata, replacing the previous design which mixed a return value for the victim page with an out-parameter for the new frame number
- Added `has_free_frames()` and `mark_dirty()` accessor functions to support the opaque type

