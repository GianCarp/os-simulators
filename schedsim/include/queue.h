#ifndef QUEUE_H
#define QUEUE_H

/*
 * Simple FIFO queue of integers (job indices), implemented as a circular
 * buffer. Used by Round robin to track the ready set in O(1) push/pop.
 *
 * Invariants:
 *  - cap > 0
 *  - size in [0, cap]
 *  - head points to the next element to pop
 *  - tail points to the next slot to push
 */

typedef struct {
  int *buf; // pointer to dynamically allocated array (circular buffer storage)
  int cap;  // capacity of the buffer, typically num_jobs
  int size; // current number of elements in the queue
  int head; // index of front element
  int tail; // index of next insertion position
} queue_t;

// Initialise queue with capacity cap, typically num_jobs.
void q_init(queue_t *q, int cap);

// Free queue storage
void q_free(queue_t *q);

// Returns 1 if empty, 0 otherwise.
int q_empty(const queue_t *q);

// Push value to back of queue. Undefined behaviour if queue is full.
void q_push(queue_t *q, int v);

// Pop and return value from front of queue. Undefined behaviour if empty.
int q_pop(queue_t *q);

#endif // QUEUE_H
