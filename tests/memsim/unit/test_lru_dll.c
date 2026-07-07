// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

// Pull in memsim internals so static functions can be tested
#include "memsim.c"

// setup & teardown

static int setup(void **state) {
  lru_tracker *lru = calloc(1, sizeof(lru_tracker));
  *state = lru;
  return 0;
}

static int teardown(void **state) {
  free(*state);
  return 0;
}

// tests

static void test_push_into_empty_list(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node node = {.prev = NULL, .next = NULL, .frame = 0};

  lru_new_node_push_head(lru, &node);

  assert_ptr_equal(lru->head, &node);
  assert_ptr_equal(lru->tail, &node);
  assert_null(node.prev);
  assert_null(node.next);
}

static void test_push_second_node(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node first = {.prev = NULL, .next = NULL, .frame = 0};
  lru_node second = {.prev = NULL, .next = NULL, .frame = 1};

  lru_new_node_push_head(lru, &first);
  lru_new_node_push_head(lru, &second);

  // second is head, first is tail
  assert_ptr_equal(lru->head, &second);
  assert_ptr_equal(lru->tail, &first);
  // links between them
  assert_ptr_equal(second.next, &first);
  assert_ptr_equal(first.prev, &second);
  assert_null(second.prev);
  assert_null(first.next);
}

static void test_move_head_to_head_is_noop(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node first = {.prev = NULL, .next = NULL, .frame = 0};
  lru_node second = {.prev = NULL, .next = NULL, .frame = 1};

  lru_new_node_push_head(lru, &first);
  lru_new_node_push_head(lru, &second);

  // second is already head, moving it should change nothing
  lru_move_existing_node_head(lru, &second);

  assert_ptr_equal(lru->head, &second);
  assert_ptr_equal(lru->tail, &first);
  assert_ptr_equal(second.next, &first);
  assert_ptr_equal(first.prev, &second);
}

static void test_move_tail_to_head_two_nodes(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node first = {.prev = NULL, .next = NULL, .frame = 0};
  lru_node second = {.prev = NULL, .next = NULL, .frame = 1};

  lru_new_node_push_head(lru, &first);
  lru_new_node_push_head(lru, &second);

  // move first (tail) to head
  lru_move_existing_node_head(lru, &first);

  assert_ptr_equal(lru->head, &first);
  assert_ptr_equal(lru->tail, &second);
  assert_ptr_equal(first.next, &second);
  assert_ptr_equal(second.prev, &first);
  assert_null(first.prev);
  assert_null(second.next);
}

static void test_move_tail_to_head_three_nodes(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node a = {.prev = NULL, .next = NULL, .frame = 0};
  lru_node b = {.prev = NULL, .next = NULL, .frame = 1};
  lru_node c = {.prev = NULL, .next = NULL, .frame = 2};

  lru_new_node_push_head(lru, &a); // [a]
  lru_new_node_push_head(lru, &b); // [b, a]
  lru_new_node_push_head(lru, &c); // [c, b, a]

  // move a (tail) to head: [a, c, b]
  lru_move_existing_node_head(lru, &a);

  assert_ptr_equal(lru->head, &a);
  assert_ptr_equal(lru->tail, &b);
  assert_ptr_equal(a.next, &c);
  assert_ptr_equal(c.prev, &a);
  assert_ptr_equal(c.next, &b);
  assert_ptr_equal(b.prev, &c);
  assert_null(a.prev);
  assert_null(b.next);
}

static void test_move_middle_to_head_three_nodes(void **state) {
  lru_tracker *lru = (lru_tracker *)*state;
  lru_node a = {.prev = NULL, .next = NULL, .frame = 0};
  lru_node b = {.prev = NULL, .next = NULL, .frame = 1};
  lru_node c = {.prev = NULL, .next = NULL, .frame = 2};

  lru_new_node_push_head(lru, &a); // [a]
  lru_new_node_push_head(lru, &b); // [b, a]
  lru_new_node_push_head(lru, &c); // [c, b, a]

  // move b (middle) to head: [b, c, a]
  lru_move_existing_node_head(lru, &b);

  assert_ptr_equal(lru->head, &b);
  assert_ptr_equal(lru->tail, &a);
  assert_ptr_equal(b.next, &c);
  assert_ptr_equal(c.prev, &b);
  assert_ptr_equal(c.next, &a);
  assert_ptr_equal(a.prev, &c);
  assert_null(b.prev);
  assert_null(a.next);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_push_into_empty_list, setup,
                                      teardown),
      cmocka_unit_test_setup_teardown(test_push_second_node, setup, teardown),
      cmocka_unit_test_setup_teardown(test_move_head_to_head_is_noop, setup,
                                      teardown),
      cmocka_unit_test_setup_teardown(test_move_tail_to_head_two_nodes, setup,
                                      teardown),
      cmocka_unit_test_setup_teardown(test_move_tail_to_head_three_nodes, setup,
                                      teardown),
      cmocka_unit_test_setup_teardown(test_move_middle_to_head_three_nodes,
                                      setup, teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
