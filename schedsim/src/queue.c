#include "queue.h"

#include <assert.h>
#include <stdlib.h>

/*
 * Circular-buffer queue implementation.
 *
 * We keep explicit size so that head==tail is not ambiguous (could be empty or
 * full otherwise). Indices wrap using modulo cap.
 */

void q_init(queue_t *q, int cap) {
  assert(q != NULL);
  assert(cap > 0);

  q->buf = (int *)malloc((size_t)cap * sizeof(int));
  assert(q->buf != NULL);

  q->cap = cap;
  q->size = 0;
  q->head = 0;
  q->tail = 0;
}

void q_free(queue_t *q) {
  if (q == NULL) {
    return;
  }

  free(q->buf);
  q->buf = NULL;
  q->cap = 0;
  q->size = 0;
  q->head = 0;
  q->tail = 0;
}

int q_empty(const queue_t *q) {
  assert(q != NULL);
  return q->size == 0;
}

void q_push(queue_t *q, int v) {
  assert(q != NULL);
  assert(q->buf != NULL);
  assert(q->size < q->cap); // not full

  q->buf[q->tail] = v;
  q->tail = (q->tail + 1) % q->cap;
  q->size++;
}

int q_pop(queue_t *q) {
  assert(q != NULL);
  assert(q->buf != NULL);
  assert(q->size > 0); // not empty

  int v = q->buf[q->head];
  q->head = (q->head + 1) % q->cap;
  q->size--;
  return v;
}
