// by-heart/tests/test_core.cpp — native test runner for the core.
// Half validation / half adversarial, per estate test law. Exit code is
// the gate: 0 = all pass. Build: clang++ -std=c++20 (see justfile).

#include "../src/core.hpp"

#include <cstdio>
#include <cstring>
#include <string>

using namespace byheart;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    g_checks++;                                                         \
    if (!(cond)) {                                                      \
      g_failures++;                                                     \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                   \
  } while (0)

static void steps(Game& g, int n, unsigned inp = 0) {
  for (int i = 0; i < n; i++) g.step(kStepMs, inp);
}

// Advance the teach phase to the first order.
static void through_teach(Game& g) {
  int guard = 0;
  while (g.phase == Phase::kTeach && guard++ < 2000) g.step(kStepMs, input::kConfirm);
  CHECK(g.phase == Phase::kOrder);
}

// Answer the current order correctly through real input.
static void answer_correctly(Game& g) {
  const Order& ord = g.orders[g.order_idx];
  if (ord.plug && !g.armed) {
    g.step(kStepMs, input::kArm);
  }
  int target = 0;
  for (int i = 0; i < 3; i++) {
    if (g.options[i] == ord.pair) target = i;
  }
  int guard = 0;
  while (g.option_cursor != target && guard++ < 4) {
    g.step(kStepMs, g.option_cursor < target ? input::kDown : input::kUp);
  }
  g.step(kStepMs, input::kConfirm);
}

// Answer the current order wrongly through real input.
static void answer_wrongly(Game& g) {
  const Order& ord = g.orders[g.order_idx];
  int wrong = 0;
  for (int i = 0; i < 3; i++) {
    if (g.options[i] != ord.pair) {
      wrong = i;
      break;
    }
  }
  int guard = 0;
  while (g.option_cursor != wrong && guard++ < 4) {
    g.step(kStepMs, g.option_cursor < wrong ? input::kDown : input::kUp);
  }
  g.step(kStepMs, input::kConfirm);
}

static Game play_game(unsigned level, unsigned seed = 42) {
  Game g(seed, false);
  CHECK(g.start_level(level));
  return g;
}

// -- validation ---------------------------------------------------------

static void test_teach_then_orders_flow() {
  Game g = play_game(0);
  CHECK(g.screen == Screen::kPlay);
  CHECK(g.phase == Phase::kTeach);
  through_teach(g);
  CHECK(g.order_total == 8); // 6 recall + 2 chain
}

static void test_correct_answers_complete_a_sitting() {
  Game g = play_game(0);
  through_teach(g);
  int guard = 0;
  while (g.screen == Screen::kPlay && guard++ < 20000) {
    if (g.phase == Phase::kOrder) {
      answer_correctly(g);
    } else {
      g.step(kStepMs, 0);
    }
  }
  CHECK(g.screen == Screen::kVictory);
  CHECK(g.failures == 0);
  CHECK(g.culminated); // the package completed
  CHECK(g.grade() == 3); // flawless: by heart
}

static void test_correction_ladder_retries_then_keeps() {
  Game g = play_game(0);
  through_teach(g);
  long long first_window = g.window_ms_full;
  answer_wrongly(g);
  CHECK(g.phase == Phase::kCorrect);
  CHECK(g.ladder == 1);
  CHECK(g.corrections == 1);
  steps(g, 200, 0); // correction shows; retry begins
  CHECK(g.phase == Phase::kOrder);
  CHECK(g.window_ms_full < first_window); // tightened
  // Recover and finish: grade drops to corrected.
  int guard = 0;
  while (g.screen == Screen::kPlay && guard++ < 20000) {
    if (g.phase == Phase::kOrder) {
      answer_correctly(g);
    } else {
      g.step(kStepMs, 0);
    }
  }
  CHECK(g.screen == Screen::kVictory);
  CHECK(g.grade() == 2);
}

static void test_plug_locks_without_permission() {
  Game g = play_game(1); // THE SPOKEN LAW has plug orders
  through_teach(g);
  // Walk to the first plug order, answering correctly.
  int guard = 0;
  while (!g.orders[g.order_idx].plug && guard++ < 20000) {
    if (g.phase == Phase::kOrder) {
      answer_correctly(g);
    } else {
      g.step(kStepMs, 0);
    }
  }
  CHECK(g.orders[g.order_idx].plug);
  // The right words, unarmed: still a correction.
  const Order& ord = g.orders[g.order_idx];
  int target = 0;
  for (int i = 0; i < 3; i++) {
    if (g.options[i] == ord.pair) target = i;
  }
  while (g.option_cursor != target) g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.phase == Phase::kCorrect);
  CHECK(g.corrections >= 1);
  // Retry; this time speak the phrase first.
  steps(g, 200, 0);
  CHECK(g.phase == Phase::kOrder);
  g.step(kStepMs, input::kArm);
  CHECK(g.armed);
  answer_correctly(g);
  CHECK(g.phase == Phase::kPraise);
}

static void test_window_expiry_climbs_the_ladder() {
  Game g = play_game(0);
  through_teach(g);
  int guard = 0;
  while (g.phase == Phase::kOrder && guard++ < 1000) g.step(kStepMs, 0);
  CHECK(g.phase == Phase::kCorrect);
  CHECK(g.corrections == 1);
}

static void test_on_the_beat_answers() {
  Game g = play_game(0);
  through_teach(g);
  answer_correctly(g); // answered within steps: inside the first half
  CHECK(g.beats == 1);
}

static void test_settings_and_record_bridges() {
  Game g(7, false);
  CHECK((g.window_grace == false && g.text_large == false));
  g.window_grace = true;
  CHECK(g.record_kept == 0);
  g.record_kept = 3;
  g.record_flawless = 2;
  CHECK(g.record_kept == 3 && g.record_flawless == 2);
}

static void test_demo_completes_all_levels() {
  for (unsigned lvl = 0; lvl < kLevelCount; lvl++) {
    Game g(9000 + lvl, true);
    CHECK(g.start_level(lvl));
    int n = 0;
    while (g.screen != Screen::kVictory && g.screen != Screen::kGameOver && n < 30000) {
      g.step(kStepMs, 0);
      n++;
    }
    CHECK(g.screen == Screen::kVictory);
    CHECK(g.failures == 0);
    CHECK(g.culminated);
  }
}

// -- adversarial --------------------------------------------------------

static void test_three_rungs_fail_the_order_never_the_sitting() {
  Game g = play_game(0);
  through_teach(g);
  int first_order = g.order_idx;
  for (int rung = 0; rung < 3; rung++) {
    answer_wrongly(g);
    CHECK(g.phase == Phase::kCorrect);
    steps(g, 200, 0);
  }
  CHECK(g.failures == 1);
  CHECK(g.order_idx == first_order + 1); // moved on, not locked out
  CHECK(g.screen == Screen::kPlay);
}

static void test_sitting_clock_ends_an_idle_sitting() {
  Game g = play_game(0);
  // Idle through teach (auto-advances) and every window: corrections
  // pile up, orders fail, but the sitting only ends on its clock or
  // by completion. Force the clock down and step once.
  g.sitting_ms_left = 16;
  g.step(kStepMs, 0);
  CHECK(g.screen == Screen::kGameOver);
}

static void test_start_level_rejects_out_of_range() {
  Game g(3, false);
  CHECK(!g.start_level(kLevelCount));
  CHECK(!g.start_level(4000000000u));
  CHECK(g.screen == Screen::kGate);
}

static void test_culmination_fires_exactly_once() {
  Game g = play_game(0);
  through_teach(g);
  int culminations = 0;
  int guard = 0;
  while (g.screen == Screen::kPlay && guard++ < 20000) {
    unsigned ev_before = g.event_code;
    (void)ev_before;
    if (g.phase == Phase::kOrder) {
      answer_correctly(g);
      if (g.event_code == event::kCulmination) culminations++;
    } else {
      g.step(kStepMs, 0);
      if (g.event_code == event::kCulmination) culminations++;
    }
  }
  CHECK(g.screen == Screen::kVictory);
  CHECK(culminations == 1);
}

static void test_decoy_is_not_the_law() {
  Game g = play_game(2); // BY HEART carries near-miss decoys
  through_teach(g);
  const Order& ord = g.orders[g.order_idx];
  int decoy_pos = -1;
  for (int i = 0; i < 3; i++) {
    if (g.options[i] == ord.pair + kPairCount) decoy_pos = i;
  }
  CHECK(decoy_pos >= 0); // the near-miss is on the table
  while (g.option_cursor != decoy_pos) g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.phase == Phase::kCorrect); // and it is wrong
}

static void test_determinism_same_seed_same_frame() {
  auto run = []() {
    Game g(777, false);
    g.start_level(2);
    unsigned x = 12345;
    for (int i = 0; i < 3000; i++) {
      x = x * 1103515245u + 12345u;
      g.step(kStepMs, (x >> 16) % 128);
    }
    static char buf[kDrawBufferBytes];
    int n = g.render(buf, kDrawBufferBytes);
    return std::string(buf, buf + n);
  };
  CHECK(run() == run());
}

static void test_draw_lists_well_formed_on_all_screens() {
  auto check = [](Game& g) {
    static char buf[kDrawBufferBytes];
    int n = g.render(buf, kDrawBufferBytes);
    CHECK(n > 0 && n < kDrawBufferBytes);
    int ops = 0;
    for (int i = 0; i < n; i++) {
      if (buf[i] == '\n') {
        ops++;
        CHECK(i + 1 >= n || buf[i + 1] == 'r' || buf[i + 1] == 'c' || buf[i + 1] == 'l' ||
              buf[i + 1] == 'p' || buf[i + 1] == 't');
      }
    }
    CHECK(ops <= kMaxOps);
  };
  Game g(11, false);
  check(g); // gate
  g.step(kStepMs, input::kConfirm);
  check(g); // title
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.screen == Screen::kHowto);
  check(g);
  g.step(kStepMs, input::kBack);
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kDown);
  g.step(kStepMs, input::kConfirm);
  CHECK(g.screen == Screen::kSettings);
  check(g);
  g.start_level(1);
  steps(g, 40, 0);
  check(g); // teach
  through_teach(g);
  check(g); // order
  g.step(kStepMs, input::kPause);
  CHECK(g.screen == Screen::kPause);
  check(g); // pause
}

int main() {
  test_teach_then_orders_flow();
  test_correct_answers_complete_a_sitting();
  test_correction_ladder_retries_then_keeps();
  test_plug_locks_without_permission();
  test_window_expiry_climbs_the_ladder();
  test_on_the_beat_answers();
  test_settings_and_record_bridges();
  test_demo_completes_all_levels();
  test_three_rungs_fail_the_order_never_the_sitting();
  test_sitting_clock_ends_an_idle_sitting();
  test_start_level_rejects_out_of_range();
  test_culmination_fires_exactly_once();
  test_decoy_is_not_the_law();
  test_determinism_same_seed_same_frame();
  test_draw_lists_well_formed_on_all_screens();
  std::printf("by-heart core tests: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
