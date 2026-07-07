// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
// clang-format on

#include <cmocka.h>

// Pull in memsim internals so static functions can be tested
#include "memsim.c"

// setup & teardown

static int setup_small(void **state) {
  // Replacement policy is irrelevant for (most) of these tests which exercise
  // page table initialisation, frame allocation, VPN mapping. LRU selected as
  // it is useful to test if the head and tail pointers are set to NULL on init.
  mmu *m = create_MMU(4, lru);
  assert_non_null(m);
  *state = m;
  return 0;
}

static int teardown_mmu(void **state) {
  destroy_MMU((mmu *)*state);
  return 0;
}

// create_MMU tests

static void test_create_mmu_returns_non_null(void **state) {
  (void)state;
  mmu *m = create_MMU(4, lru);
  assert_non_null(m);
  destroy_MMU(m);
}

static void test_create_mmu_page_table_initialised(void **state) {
  mmu *m = (mmu *)*state;
  // every page table entry should be -1 (unmapped)
  for (int i = 0; i < (int)NUM_PAGES; i++) {
    assert_int_equal(m->page_table[i], -1);
  }
}

static void test_create_mmu_frame_nodes_initialised(void **state) {
  mmu *m = (mmu *)*state;
  // each frame's embedded node should have its frame field set to its own index
  for (int f = 0; f < m->numFrames; f++) {
    assert_int_equal(m->frame_data[f].node.frame, f);
  }
}

static void test_create_mmu_frame_vpns_initialised(void **state) {
  mmu *m = (mmu *)*state;
  // each frame's vpn should be -1 (no resident page)
  for (int f = 0; f < m->numFrames; f++) {
    assert_int_equal(m->frame_data[f].vpn, -1);
  }
}

static void test_create_mmu_lru_state_empty(void **state) {
  mmu *m = (mmu *)*state;
  assert_null(m->lru_state.head);
  assert_null(m->lru_state.tail);
}

static void test_create_mmu_stores_mode(void **state) {
  (void)state;
  mmu *m1 = create_MMU(4, fifo);
  mmu *m2 = create_MMU(4, lru);
  assert_int_equal(m1->mode, fifo);
  assert_int_equal(m2->mode, lru);
  destroy_MMU(m1);
  destroy_MMU(m2);
}

// has_free_frames tests

static void test_has_free_frames_on_fresh_mmu(void **state) {
  mmu *m = (mmu *)*state;
  assert_int_equal(has_free_frames(m), 1);
}

static void test_has_free_frames_after_all_allocated(void **state) {
  mmu *m = (mmu *)*state;
  for (int i = 0; i < m->numFrames; i++) {
    allocate_frame(m, i);
  }
  assert_int_equal(has_free_frames(m), 0);
}

// allocate_frame tests

static void test_allocate_frame_sequential_pfns(void **state) {
  mmu *m = (mmu *)*state;
  for (int i = 0; i < m->numFrames; i++) {
    int pfn = allocate_frame(m, i);
    assert_int_equal(pfn, i);
  }
}

static void test_allocate_frame_updates_page_table(void **state) {
  mmu *m = (mmu *)*state;
  int vpn = 5;
  int pfn = allocate_frame(m, vpn);
  assert_int_equal(m->page_table[vpn], pfn);
}

static void test_allocate_frame_sets_vpn(void **state) {
  mmu *m = (mmu *)*state;
  int vpn = 5;
  int pfn = allocate_frame(m, vpn);
  assert_int_equal(m->frame_data[pfn].vpn, vpn);
}

static void test_allocate_frame_clean_by_default(void **state) {
  mmu *m = (mmu *)*state;
  int pfn = allocate_frame(m, 5);
  assert_int_equal(m->frame_data[pfn].dirty, 0);
}

static void test_allocate_frame_pushes_to_lru_head(void **state) {
  (void)state;
  mmu *m = create_MMU(4, lru);

  allocate_frame(m, 10);
  assert_int_equal(m->lru_state.head->frame, 0);
  assert_int_equal(m->lru_state.tail->frame, 0);

  allocate_frame(m, 20);
  // most recent allocation is head
  assert_int_equal(m->lru_state.head->frame, 1);
  assert_int_equal(m->lru_state.tail->frame, 0);

  allocate_frame(m, 30);
  assert_int_equal(m->lru_state.head->frame, 2);
  assert_int_equal(m->lru_state.tail->frame, 0);

  destroy_MMU(m);
}

static void test_allocate_frame_no_lru_update_in_fifo_mode(void **state) {
  (void)state;
  mmu *m = create_MMU(4, fifo);

  allocate_frame(m, 10);
  allocate_frame(m, 20);

  // DLL should remain empty in fifo mode
  assert_null(m->lru_state.head);
  assert_null(m->lru_state.tail);

  destroy_MMU(m);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_create_mmu_returns_non_null),
      cmocka_unit_test_setup_teardown(test_create_mmu_page_table_initialised,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_create_mmu_frame_nodes_initialised,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_create_mmu_frame_vpns_initialised,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_create_mmu_lru_state_empty,
                                      setup_small, teardown_mmu),
      cmocka_unit_test(test_create_mmu_stores_mode),
      cmocka_unit_test_setup_teardown(test_has_free_frames_on_fresh_mmu,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_has_free_frames_after_all_allocated,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_allocate_frame_sequential_pfns,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_allocate_frame_updates_page_table,
                                      setup_small, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_allocate_frame_sets_vpn, setup_small,
                                      teardown_mmu),
      cmocka_unit_test_setup_teardown(test_allocate_frame_clean_by_default,
                                      setup_small, teardown_mmu),
      cmocka_unit_test(test_allocate_frame_pushes_to_lru_head),
      cmocka_unit_test(test_allocate_frame_no_lru_update_in_fifo_mode),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
