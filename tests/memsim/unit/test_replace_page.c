// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
// clang-format on

#include <cmocka.h>

#include "memsim.c"

// helpers

// Fill all frames with sequential VPNs starting from base_vpn.
// After this, the MMU has no free frames and every call to replace_page
// will trigger an eviction.
static void fill_frames(mmu *m, int base_vpn) {
  for (int i = 0; i < m->numFrames; i++) {
    allocate_frame(m, base_vpn + i);
  }
}
// FIFO tests

static void test_fifo_evicts_in_insertion_order(void **state) {
  (void)state;
  mmu *m = create_MMU(3, fifo);
  // frames: [VPN 0, VPN 1, VPN 2]
  fill_frames(m, 0);

  // first eviction should evict frame 0 (VPN 0, loaded first)
  replace_result r1 = replace_page(m, 10);
  assert_int_equal(r1.victim.vpn, 0);
  assert_int_equal(r1.new_pfn, 0);

  // second eviction should evict frame 1 (VPN 1)
  replace_result r2 = replace_page(m, 11);
  assert_int_equal(r2.victim.vpn, 1);
  assert_int_equal(r2.new_pfn, 1);

  // third eviction should evict frame 2 (VPN 2)
  replace_result r3 = replace_page(m, 12);
  assert_int_equal(r3.victim.vpn, 2);
  assert_int_equal(r3.new_pfn, 2);

  destroy_MMU(m);
}

static void test_fifo_wraps_around(void **state) {
  (void)state;
  mmu *m = create_MMU(3, fifo);
  fill_frames(m, 0);

  // evict all 3, hand wraps back to 0
  replace_page(m, 10);
  replace_page(m, 11);
  replace_page(m, 12);

  // next eviction should be frame 0 again (now holds VPN 10)
  replace_result r = replace_page(m, 20);
  assert_int_equal(r.victim.vpn, 10);
  assert_int_equal(r.new_pfn, 0);

  destroy_MMU(m);
}

// LRU simple tests

static void test_lru_simple_evicts_oldest_access(void **state) {
  (void)state;
  mmu *m = create_MMU(3, lru_simple);
  fill_frames(m, 0);

  // touch VPN 0 (frame 0) to make it the most recently used
  check_in_memory(m, 0);

  // eviction should pick the lowest access_time, which is VPN 1 (frame 1)
  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 1);
  assert_int_equal(r.new_pfn, 1);

  destroy_MMU(m);
}

// LRU advanced tests

static void test_lru_advanced_evicts_tail(void **state) {
  (void)state;
  mmu *m = create_MMU(3, lru);
  fill_frames(m, 0);
  // DLL order after fill: [frame 2, frame 1, frame 0]
  // tail is frame 0 (VPN 0)

  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 0);
  assert_int_equal(r.new_pfn, 0);

  destroy_MMU(m);
}

static void test_lru_advanced_respects_access_reorder(void **state) {
  (void)state;
  mmu *m = create_MMU(3, lru);
  fill_frames(m, 0);
  // DLL: [frame 2, frame 1, frame 0]

  // touch VPN 0 (frame 0), moves it to head
  // DLL: [frame 0, frame 2, frame 1]
  check_in_memory(m, 0);

  // tail is now frame 1 (VPN 1)
  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 1);
  assert_int_equal(r.new_pfn, 1);

  destroy_MMU(m);
}

static void test_lru_simple_and_advanced_agree(void **state) {
  (void)state;
  mmu *m_simple = create_MMU(3, lru_simple);
  mmu *m_advanced = create_MMU(3, lru);

  fill_frames(m_simple, 0);
  fill_frames(m_advanced, 0);

  // same access pattern on both
  check_in_memory(m_simple, 0);
  check_in_memory(m_advanced, 0);

  replace_result r_simple = replace_page(m_simple, 10);
  replace_result r_advanced = replace_page(m_advanced, 10);

  assert_int_equal(r_simple.victim.vpn, r_advanced.victim.vpn);
  assert_int_equal(r_simple.new_pfn, r_advanced.new_pfn);

  destroy_MMU(m_simple);
  destroy_MMU(m_advanced);
}

// Clock tests

static void test_clock_skips_referenced_frames(void **state) {
  (void)state;
  mmu *m = create_MMU(3, _clock);
  fill_frames(m, 0);
  // all frames have ref=1 from allocate_frame

  // clear ref on frame 2 only
  m->frame_data[2].ref = 0;

  // clock hand starts at 0, should skip frame 0 (ref=1, cleared to 0),
  // skip frame 1 (ref=1, cleared to 0), evict frame 2 (ref=0)
  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 2);
  assert_int_equal(r.new_pfn, 2);

  destroy_MMU(m);
}

static void test_clock_clears_ref_bits_as_it_scans(void **state) {
  (void)state;
  mmu *m = create_MMU(3, _clock);
  fill_frames(m, 0);

  m->frame_data[2].ref = 0;

  replace_page(m, 10);

  // frames 0 and 1 should have had their ref bits cleared during the scan
  assert_int_equal(m->frame_data[0].ref, 0);
  assert_int_equal(m->frame_data[1].ref, 0);

  destroy_MMU(m);
}

// Clean clock tests

static void test_clean_clock_prefers_clean_page(void **state) {
  (void)state;
  mmu *m = create_MMU(3, _clock_clean);
  fill_frames(m, 0);

  // clear all ref bits so pass 1 can find candidates immediately
  m->frame_data[0].ref = 0;
  m->frame_data[1].ref = 0;
  m->frame_data[2].ref = 0;

  // make frame 0 and 1 dirty, frame 2 clean
  m->frame_data[0].dirty = 1;
  m->frame_data[1].dirty = 1;
  m->frame_data[2].dirty = 0;

  // should skip dirty frames 0 and 1,as they are dirty, and evict clean frame 2
  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 2);
  assert_int_equal(r.new_pfn, 2);

  destroy_MMU(m);
}

static void test_clean_clock_falls_through_to_dirty(void **state) {
  (void)state;
  mmu *m = create_MMU(3, _clock_clean);
  fill_frames(m, 0);

  // clear all ref bits
  m->frame_data[0].ref = 0;
  m->frame_data[1].ref = 0;
  m->frame_data[2].ref = 0;

  // all frames dirty, pass 1 finds no clean victim, pass 2 evicts first ref=0
  m->frame_data[0].dirty = 1;
  m->frame_data[1].dirty = 1;
  m->frame_data[2].dirty = 1;

  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.vpn, 0);
  assert_int_equal(r.new_pfn, 0);

  destroy_MMU(m);
}

// Random tests

static void test_random_deterministic_with_seed(void **state) {
  (void)state;

  srand(5);
  mmu *m1 = create_MMU(4, _random);
  fill_frames(m1, 0);
  replace_result r1 = replace_page(m1, 10);

  srand(5);
  mmu *m2 = create_MMU(4, _random);
  fill_frames(m2, 0);
  replace_result r2 = replace_page(m2, 10);

  assert_int_equal(r1.victim.vpn, r2.victim.vpn);
  assert_int_equal(r1.new_pfn, r2.new_pfn);

  destroy_MMU(m1);
  destroy_MMU(m2);
}

// Eviction metadata tests (policy-independent)

static void test_eviction_clears_old_page_table_entry(void **state) {
  (void)state;
  mmu *m = create_MMU(2, fifo);
  fill_frames(m, 0);

  // VPN 0 is in frame 0
  assert_int_equal(m->page_table[0], 0);

  // evict VPN 0
  replace_page(m, 10);

  // VPN 0 should no longer be mapped
  assert_int_equal(m->page_table[0], -1);
  // VPN 10 should now be in frame 0
  assert_int_equal(m->page_table[10], 0);

  destroy_MMU(m);
}

static void test_eviction_reports_dirty_victim(void **state) {
  (void)state;
  mmu *m = create_MMU(2, fifo);
  fill_frames(m, 0);

  mark_dirty(m, 0);

  replace_result r = replace_page(m, 10);
  assert_int_equal(r.victim.dirty, 1);

  destroy_MMU(m);
}

static void test_eviction_resets_frame_metadata(void **state) {
  (void)state;
  mmu *m = create_MMU(2, fifo);
  fill_frames(m, 0);
  mark_dirty(m, 0);

  replace_result r = replace_page(m, 10);

  // new page in the frame should be clean, ref=1
  assert_int_equal(m->frame_data[r.new_pfn].dirty, 0);
  assert_int_equal(m->frame_data[r.new_pfn].ref, 1);
  assert_int_equal(m->frame_data[r.new_pfn].vpn, 10);

  destroy_MMU(m);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      // FIFO
      cmocka_unit_test(test_fifo_evicts_in_insertion_order),
      cmocka_unit_test(test_fifo_wraps_around),
      // LRU simple
      cmocka_unit_test(test_lru_simple_evicts_oldest_access),
      // LRU advanced
      cmocka_unit_test(test_lru_advanced_evicts_tail),
      cmocka_unit_test(test_lru_advanced_respects_access_reorder),
      cmocka_unit_test(test_lru_simple_and_advanced_agree),
      // Clock
      cmocka_unit_test(test_clock_skips_referenced_frames),
      cmocka_unit_test(test_clock_clears_ref_bits_as_it_scans),
      // Clean clock
      cmocka_unit_test(test_clean_clock_prefers_clean_page),
      cmocka_unit_test(test_clean_clock_falls_through_to_dirty),
      // Random
      cmocka_unit_test(test_random_deterministic_with_seed),
      // Eviction metadata (policy-independent)
      cmocka_unit_test(test_eviction_clears_old_page_table_entry),
      cmocka_unit_test(test_eviction_reports_dirty_victim),
      cmocka_unit_test(test_eviction_resets_frame_metadata),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
