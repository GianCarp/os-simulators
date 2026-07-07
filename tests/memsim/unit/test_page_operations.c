// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
// clang-format on

#include <cmocka.h>

// Pull in memsim internals so static functions can be tested
#include "memsim.c"

// setup & teardown

// Creates a 4-frame MMU with LRU and pre-allocates VPNs 10, 20, 30 into
// frames 0, 1, 2. Frame 3 is left free. LRU is selected as some tests
// verify that check_in_memory updates the DLL head on a hit, which only
// happens in LRU mode. Tests that need a different mode manage their own MMU.
static int setup_with_pages(void **state) {
  mmu *m = create_MMU(4, lru);
  assert_non_null(m);
  allocate_frame(m, 10);
  allocate_frame(m, 20);
  allocate_frame(m, 30);
  *state = m;
  return 0;
}

static int teardown_mmu(void **state) {
  destroy_MMU((mmu *)*state);
  return 0;
}

// check_in_memory tests

static void test_check_in_memory_unmapped_vpn(void **state) {
  mmu *m = (mmu *)*state;
  // VPN 15 was never allocated
  assert_int_equal(check_in_memory(m, 15), -1);
}

static void test_check_in_memory_mapped_vpn(void **state) {
  mmu *m = (mmu *)*state;
  // VPN 10 was allocated to frame 0
  assert_int_equal(check_in_memory(m, 10), 0);
  // VPN 20 was allocated to frame 1
  assert_int_equal(check_in_memory(m, 20), 1);
  // VPN 30 was allocated to frame 2
  assert_int_equal(check_in_memory(m, 30), 2);
}

static void test_check_in_memory_updates_lru_head(void **state) {
  mmu *m = (mmu *)*state;
  // After setup, DLL order is [frame 2, frame 1, frame 0]
  assert_int_equal(m->lru_state.head->frame, 2);

  // Accessing VPN 10 (frame 0) should move it to head
  check_in_memory(m, 10);
  assert_int_equal(m->lru_state.head->frame, 0);
}

static void test_check_in_memory_no_lru_update_in_fifo(void **state) {
  (void)state;
  mmu *m = create_MMU(4, fifo);
  allocate_frame(m, 10);
  allocate_frame(m, 20);

  // DLL should be empty in fifo mode
  assert_null(m->lru_state.head);

  check_in_memory(m, 10);

  // Still empty after a hit
  assert_null(m->lru_state.head);

  destroy_MMU(m);
}

// set_dirty / get_dirty tests

static void test_dirty_default_is_clean(void **state) {
  mmu *m = (mmu *)*state;
  assert_int_equal(m->frame_data[0].dirty, 0);
}

static void test_mark_dirty_and_read_back(void **state) {
  mmu *m = (mmu *)*state;
  mark_dirty(m, 0);
  assert_int_equal(m->frame_data[0].dirty, 1);
}

// get_vpn tests

static void test_vpn_returns_correct_vpn(void **state) {
  mmu *m = (mmu *)*state;
  assert_int_equal(m->frame_data[0].vpn, 10);
  assert_int_equal(m->frame_data[1].vpn, 20);
  assert_int_equal(m->frame_data[2].vpn, 30);
}

static void test_vpn_unallocated_frame(void **state) {
  mmu *m = (mmu *)*state;
  assert_int_equal(m->frame_data[3].vpn, -1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_check_in_memory_unmapped_vpn,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_check_in_memory_mapped_vpn,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_check_in_memory_updates_lru_head,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test(test_check_in_memory_no_lru_update_in_fifo),
      cmocka_unit_test_setup_teardown(test_dirty_default_is_clean,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_mark_dirty_and_read_back,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_vpn_returns_correct_vpn,
                                      setup_with_pages, teardown_mmu),
      cmocka_unit_test_setup_teardown(test_vpn_unallocated_frame,
                                      setup_with_pages, teardown_mmu),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
