// by-heart/src/core.cpp — implementation. See core.hpp for the design.

#include "core.hpp"

namespace byheart {

// ---------------------------------------------------------------------------
// Small helpers (freestanding: no libc assumptions beyond the buffer)
// ---------------------------------------------------------------------------

static int str_len(const char* s) {
  int n = 0;
  while (s[n] != '\0') n++;
  return n;
}

static void shuffle_ints(Rng& rng, int* a, int n) {
  for (int i = n - 1; i > 0; i--) {
    int j = rng.range(0, i);
    int t = a[i];
    a[i] = a[j];
    a[j] = t;
  }
}

// ---------------------------------------------------------------------------
// Game: lifecycle
// ---------------------------------------------------------------------------

Game::Game(unsigned seed, bool demo_mode) : rng(seed), demo(demo_mode) {}

void Game::set_screen(Screen s) {
  screen = s;
  screen_ms = 0;
  menu_cursor = 0;
}

void Game::set_caption(const char* text, long long ms) {
  caption = text;
  caption_ms = ms;
}

bool Game::start_level(unsigned idx) {
  if (idx >= kLevelCount) return false;
  const Level& lvl = kLevels[idx];
  level_idx = idx;
  order_total = lvl.order_count + lvl.chain_count;
  // Recall orders: taught pairs drawn at random, never twice in a row.
  int prev = -1;
  for (int i = 0; i < lvl.order_count; i++) {
    int p = rng.range(0, lvl.pair_count - 1);
    if (p == prev && lvl.pair_count > 1) p = (p + 1) % lvl.pair_count;
    prev = p;
    orders[i] = Order{p, false, false};
    if (lvl.plug_orders && i % 3 == 2) orders[i].plug = true;
  }
  // The culmination package: a closing chain at speed.
  for (int i = 0; i < lvl.chain_count; i++) {
    int p = rng.range(0, lvl.pair_count - 1);
    if (p == prev && lvl.pair_count > 1) p = (p + 1) % lvl.pair_count;
    prev = p;
    orders[lvl.order_count + i] = Order{p, false, true};
  }
  order_idx = 0;
  failures = 0;
  corrections = 0;
  beats = 0;
  ladder = 0;
  culminated = false;
  sitting_ms_left = lvl.sitting_ms;
  teach_idx = 0;
  phase = Phase::kTeach;
  phase_ms = 0;
  set_caption("SHE SPEAKS THE LAW. KEEP IT BY HEART.", 3000);
  set_screen(Screen::kPlay);
  return true;
}

void Game::begin_order() {
  const Level& lvl = kLevels[level_idx];
  const Order& ord = orders[order_idx];
  // Options: the true response, plus decoys. On the By Heart level one
  // decoy is the near-miss of the same pair — the cruelest option.
  int opts[3];
  int n = 0;
  opts[n++] = ord.pair;
  if (lvl.decoys) opts[n++] = ord.pair + kPairCount;
  while (n < 3) {
    int d = rng.range(0, lvl.pair_count - 1);
    bool dup = (d == ord.pair);
    for (int i = 0; i < n; i++) {
      if (opts[i] == d) dup = true;
    }
    if (!dup) opts[n++] = d;
  }
  shuffle_ints(rng, opts, 3);
  for (int i = 0; i < 3; i++) options[i] = opts[i];
  option_count = 3;
  option_cursor = 0;
  // The window: grace setting widens it; the ladder tightens it; the
  // culmination chain runs it fast.
  long long base = lvl.window_ms + (window_grace ? 1500 : 0);
  if (ord.chain) base = base * 3 / 5;
  if (ladder == 1) base = base * 3 / 4;
  if (ladder >= 2) base = base / 2;
  window_ms_full = base;
  window_ms_left = base;
  armed = false;
  phase = Phase::kOrder;
  phase_ms = 0;
  event_code = event::kOrderIssued;
}

void Game::answer(int option_pos) {
  const Order& ord = orders[order_idx];
  int chosen = options[option_pos];
  if (ord.plug && !armed) {
    fail_order("LOCKED WITHOUT PERMISSION. ASK FIRST.");
    return;
  }
  if (chosen == ord.pair) {
    if (window_ms_left * 2 >= window_ms_full) {
      beats++;
      event_code = event::kOnTheBeat;
      set_caption("ON THE BEAT. WELL KEPT.", 1500);
    } else {
      event_code = event::kCorrect;
      set_caption("WELL KEPT.", 1500);
    }
    if (ord.chain && order_idx == order_total - 1 && !culminated) {
      // The package completes — exactly once, by construction.
      culminated = true;
      event_code = event::kCulmination;
      set_caption("THE PACKAGE COMPLETES. SHE ANSWERS.", 2600);
    }
    phase = Phase::kPraise;
    phase_ms = 0;
  } else {
    fail_order("NOT THE LAW. AGAIN.");
  }
}

void Game::fail_order(const char* why) {
  corrections++;
  ladder++;
  event_code = event::kCorrection;
  set_caption(why, 2200);
  if (ladder >= 3) failures++;
  phase = Phase::kCorrect;
  phase_ms = 0;
}

void Game::advance_order() {
  ladder = 0;
  order_idx++;
  if (order_idx >= order_total) {
    const Level& lvl = kLevels[level_idx];
    finish_sitting(failures <= lvl.allowed_failures);
    return;
  }
  begin_order();
}

void Game::finish_sitting(bool won) {
  if (won) {
    event_code = event::kVictory;
    set_screen(Screen::kVictory);
  } else {
    event_code = event::kGameOver;
    set_screen(Screen::kGameOver);
  }
}

int Game::grade() const {
  if (screen != Screen::kVictory) return 0;
  if (failures > 0) return 1;      // drilled, kept
  if (corrections > 0) return 2;   // corrected, kept
  return 3;                        // by heart
}

// ---------------------------------------------------------------------------
// Game: stepping
// ---------------------------------------------------------------------------

void Game::step(unsigned dt_ms, unsigned raw_input) {
  long long dt = dt_ms > 50 ? 50 : static_cast<long long>(dt_ms);
  event_code = event::kNone;
  unsigned inp = demo ? demo_policy() : raw_input;
  screen_ms += dt;
  if (caption_ms > 0) caption_ms -= dt;
  switch (screen) {
    case Screen::kGate:
      if (inp & (input::kConfirm | input::kTap)) set_screen(Screen::kTitle);
      break;
    case Screen::kTitle:
      step_title(inp);
      break;
    case Screen::kHowto:
      if (inp & (input::kBack | input::kConfirm)) set_screen(Screen::kTitle);
      break;
    case Screen::kPlay:
      step_play(dt, inp);
      break;
    case Screen::kPause:
      step_pause(inp);
      break;
    case Screen::kSettings:
      step_settings(inp);
      break;
    case Screen::kVictory:
    case Screen::kGameOver:
      if (inp & (input::kConfirm | input::kTap)) set_screen(Screen::kTitle);
      break;
  }
}

void Game::step_title(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 3 ? menu_cursor + 1 : 3;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      start_level(title_level);
    } else if (menu_cursor == 1) {
      set_screen(Screen::kHowto);
    } else if (menu_cursor == 2) {
      set_screen(Screen::kSettings);
    } else {
      title_level = (title_level + 1) % kLevelCount;
    }
  }
}

void Game::step_pause(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 2 ? menu_cursor + 1 : 2;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & input::kPause) {
    set_screen(Screen::kPlay);
    return;
  }
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      set_screen(Screen::kPlay);
    } else if (menu_cursor == 1) {
      start_level(level_idx);
    } else {
      set_screen(Screen::kTitle);
    }
  }
}

void Game::step_settings(unsigned inp) {
  if (inp & input::kDown) menu_cursor = menu_cursor < 2 ? menu_cursor + 1 : 2;
  if (inp & input::kUp) menu_cursor = menu_cursor > 0 ? menu_cursor - 1 : 0;
  if (inp & input::kBack) {
    set_screen(Screen::kTitle);
    return;
  }
  if (inp & (input::kConfirm | input::kTap)) {
    if (menu_cursor == 0) {
      window_grace = !window_grace;
    } else if (menu_cursor == 1) {
      text_large = !text_large;
    } else {
      set_screen(Screen::kTitle);
    }
  }
}

void Game::step_play(long long dt, unsigned inp) {
  if (inp & input::kPause) {
    set_screen(Screen::kPause);
    return;
  }
  const Level& lvl = kLevels[level_idx];
  sitting_ms_left -= dt;
  if (sitting_ms_left <= 0) {
    sitting_ms_left = 0;
    finish_sitting(false);
    return;
  }
  phase_ms += dt;
  switch (phase) {
    case Phase::kTeach: {
      int cards = lvl.pair_count + (lvl.plug_orders ? 1 : 0);
      bool adv = (inp & (input::kConfirm | input::kTap)) != 0 || phase_ms >= 2600;
      if (adv) {
        teach_idx++;
        phase_ms = 0;
        if (teach_idx >= cards) begin_order();
      }
      break;
    }
    case Phase::kOrder: {
      const Order& ord = orders[order_idx];
      if ((inp & input::kArm) && ord.plug && !armed) {
        armed = true;
        event_code = event::kArmed;
        set_caption("THE PHRASE IS SPOKEN. THE PLUG ARMS.", 1800);
        break;
      }
      if (inp & input::kDown) option_cursor = option_cursor < 2 ? option_cursor + 1 : 2;
      if (inp & input::kUp) option_cursor = option_cursor > 0 ? option_cursor - 1 : 0;
      if (inp & (input::kConfirm | input::kTap)) {
        answer(option_cursor);
        break;
      }
      window_ms_left -= dt;
      if (window_ms_left <= 0) {
        window_ms_left = 0;
        fail_order("THE WINDOW CLOSED. AGAIN.");
      }
      break;
    }
    case Phase::kPraise:
      if (phase_ms >= 900) advance_order();
      break;
    case Phase::kCorrect:
      if (phase_ms >= 1800) {
        if (ladder >= 3) {
          advance_order(); // failed, recorded — and the sitting goes on
        } else {
          begin_order(); // retry the same order, window tightened
        }
      }
      break;
  }
}

// ---------------------------------------------------------------------------
// The demo policy: a player who kept the law. It arms plug orders
// before answering them and answers on the beat.
// ---------------------------------------------------------------------------

unsigned Game::demo_policy() {
  switch (screen) {
    case Screen::kGate:
      return screen_ms > 600 ? input::kConfirm : 0;
    case Screen::kTitle:
      return screen_ms > 700 ? input::kConfirm : 0;
    case Screen::kVictory:
    case Screen::kGameOver:
      return screen_ms > 1500 ? input::kConfirm : 0;
    case Screen::kPlay:
      break;
    default:
      return 0;
  }
  switch (phase) {
    case Phase::kTeach:
      return phase_ms > 500 ? input::kConfirm : 0;
    case Phase::kOrder: {
      const Order& ord = orders[order_idx];
      if (ord.plug && !armed) return input::kArm;
      int target = 0;
      for (int i = 0; i < 3; i++) {
        if (options[i] == ord.pair) target = i;
      }
      if (option_cursor < target) return input::kDown;
      if (option_cursor > target) return input::kUp;
      return phase_ms > 250 ? input::kConfirm : 0;
    }
    default:
      return 0;
  }
}

} // namespace byheart

// ---------------------------------------------------------------------------
// Draw-list emission (bounded writer) + rendering
// ---------------------------------------------------------------------------

namespace byheart {
namespace {

struct StrBuf {
  char bytes[160];
  int len = 0;
  void push_str(const char* s) {
    for (int i = 0; s[i] != '\0' && len < 159; i++) bytes[len++] = s[i];
    bytes[len] = '\0';
  }
  void push_int(long long v) {
    if (v < 0) {
      push_str("-");
      v = -v;
    }
    char digits[20];
    int n = 0;
    if (v == 0) digits[n++] = '0';
    while (v > 0) {
      digits[n++] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
    for (int i = n - 1; i >= 0; i--) {
      if (len < 159) bytes[len++] = digits[i];
    }
    bytes[len] = '\0';
  }
  const char* c_str() const { return bytes; }
};

struct Writer {
  char* buf;
  int cap;
  int pos = 0;
  int ops = 0;
  int dropped = 0;
  Writer(char* b, int c) : buf(b), cap(c) {}
  void raw(const char* s) {
    int n = str_len(s);
    if (pos + n > cap) {
      dropped++;
      return;
    }
    for (int i = 0; i < n; i++) buf[pos++] = s[i];
  }
  void num(int v) {
    StrBuf b;
    b.push_int(v);
    raw(b.c_str());
  }
  bool begin_op(const char* kind) {
    if (ops >= kMaxOps) {
      dropped++;
      return false;
    }
    ops++;
    raw(kind);
    return true;
  }
  void field(int v) {
    raw(" ");
    num(v);
  }
  void rect(int x, int y, int w, int h, int c) {
    if (!begin_op("r")) return;
    field(x); field(y); field(w); field(h); field(c);
    raw("\n");
  }
  void circle(int x, int y, int r, int c) {
    if (!begin_op("c")) return;
    field(x); field(y); field(r); field(c);
    raw("\n");
  }
  void line(int x1, int y1, int x2, int y2, int c, int w) {
    if (!begin_op("l")) return;
    field(x1); field(y1); field(x2); field(y2); field(c); field(w);
    raw("\n");
  }
  void tri(int x1, int y1, int x2, int y2, int x3, int y3, int c) {
    if (!begin_op("p")) return;
    field(x1); field(y1); field(x2); field(y2); field(x3); field(y3); field(c);
    raw("\n");
  }
  void text(int x, int y, int size, int c, const char* s) {
    if (!begin_op("t")) return;
    field(x); field(y); field(size); field(c);
    raw(" ");
    raw(s);
    raw("\n");
  }
};

int tsize(const Game& g, int base) { return g.text_large ? base * 5 / 4 : base; }

void render_room(Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  for (int i = 0; i < 40; i++) {
    w.circle((i * 397 + 53) % 940 + 10, (i * 613 + 91) % 940 + 10, 2, palette::kDim);
  }
  w.line(40, 845, 920, 845, palette::kPlum, 3);
}

void render_figure(const Game& g, Writer& w) {
  // Piper: twin-bun silhouette, level brow — protocol reads in the face.
  const int cx = 250;
  const int head_y = 390;
  w.tri(cx - 110, 845, cx + 110, 845, cx, 500, palette::kPlum);
  w.rect(cx - 48, 484, 96, 86, palette::kRose);
  w.circle(cx - 78, head_y - 78, 34, palette::kPlum);
  w.circle(cx + 78, head_y - 78, 34, palette::kPlum);
  w.circle(cx, head_y - 18, 98, palette::kPlum);
  w.circle(cx, head_y, 86, palette::kSkin);
  w.circle(cx - 44, head_y + 28, 13, palette::kRose);
  w.circle(cx + 44, head_y + 28, 13, palette::kRose);
  // Eyes: level, direct; a correction narrows them.
  if (g.corrections > 0 && g.phase == Phase::kCorrect) {
    w.line(cx - 42, head_y - 6, cx - 16, head_y - 3, palette::kBg, 4);
    w.line(cx + 16, head_y - 3, cx + 42, head_y - 6, palette::kBg, 4);
  } else {
    w.circle(cx - 29, head_y - 6, 7, palette::kBg);
    w.circle(cx + 29, head_y - 6, 7, palette::kBg);
  }
  w.line(cx - 44, head_y - 30, cx - 16, head_y - 30, palette::kPlum, 4);
  w.line(cx + 16, head_y - 30, cx + 44, head_y - 30, palette::kPlum, 4);
  w.line(cx - 11, head_y + 48, cx + 11, head_y + 48, palette::kPlum, 4);
  w.rect(cx - 24, 470, 48, 10, palette::kGold);
  (void)g;
}

void render_play(const Game& g, Writer& w) {
  const Level& lvl = kLevels[g.level_idx];
  render_room(w);
  render_figure(g, w);
  // Header: sitting + clock.
  w.text(36, 28, tsize(g, 26), palette::kPaper, lvl.name);
  StrBuf clock;
  clock.push_str("SITTING ");
  clock.push_int((g.sitting_ms_left + 999) / 1000);
  clock.push_str("S");
  w.text(660, 28, tsize(g, 20), palette::kMauve, clock.c_str());
  StrBuf prog;
  prog.push_str("ORDER ");
  prog.push_int(g.order_idx + 1);
  prog.push_str(" OF ");
  prog.push_int(g.order_total);
  w.text(36, 64, tsize(g, 20), palette::kMauve, prog.c_str());
  if (g.ladder > 0) {
    StrBuf lad;
    lad.push_str("CORRECTION ");
    lad.push_int(g.ladder);
    w.text(430, 64, tsize(g, 20), palette::kGold, lad.c_str());
  }
  const int px = 440;
  if (g.phase == Phase::kTeach) {
    int cards = lvl.pair_count + (lvl.plug_orders ? 1 : 0);
    w.text(px, 150, tsize(g, 26), palette::kGold, "THE LAW IS SPOKEN");
    if (g.teach_idx < lvl.pair_count) {
      const Pair& p = kPairs[g.teach_idx];
      w.text(px, 240, tsize(g, 44), palette::kPink, p.command);
      w.text(px, 320, tsize(g, 20), palette::kPaper, p.response);
    } else {
      w.text(px, 240, tsize(g, 30), palette::kPink, "THE PLUG");
      w.text(px, 320, tsize(g, 24), palette::kPaper, "IT LOCKS WITHOUT PERMISSION.");
      w.text(px, 360, tsize(g, 24), palette::kPaper, "THE PHRASE THAT ARMS IT:");
      w.text(px, 420, tsize(g, 30), palette::kGold, kTriggerPhrase);
      w.text(px, 480, tsize(g, 20), palette::kMauve, "PRESS B WHEN A PLUG ORDER COMES.");
    }
    StrBuf t;
    t.push_str("CARD ");
    t.push_int(g.teach_idx + 1);
    t.push_str(" OF ");
    t.push_int(cards);
    w.text(px, 560, tsize(g, 20), palette::kMauve, t.c_str());
  } else {
    const Order& ord = g.orders[g.order_idx];
    const Pair& p = kPairs[ord.pair];
    if (g.phase == Phase::kCorrect) {
      w.text(px, 150, tsize(g, 26), palette::kGold, "THE CORRECTION");
      w.text(px, 240, tsize(g, 40), palette::kPink, p.command);
      w.text(px, 320, tsize(g, 20), palette::kPaper, p.response);
      w.text(px, 400, tsize(g, 20), palette::kMauve,
             g.ladder >= 3 ? "THREE RUNGS. THE ORDER FAILS - RECORDED."
                           : "THE LADDER: TRAIN, CORRECT, RETRY.");
    } else {
      // Order view (also the praise flash's backdrop).
      if (ord.chain) {
        w.text(px, 118, tsize(g, 20), palette::kGold, "THE CULMINATION PACKAGE");
      }
      w.text(px, 150, tsize(g, 44), palette::kPink, p.command);
      if (ord.plug) {
        w.text(px, 215, tsize(g, 20), g.armed ? palette::kGold : palette::kRose,
               g.armed ? "THE PLUG: ARMED" : "PLUG LOCKED - B: SPEAK THE PHRASE");
      }
      // Window bar.
      w.rect(px, 262, 440, 16, palette::kDeep);
      long long fill = g.window_ms_full > 0 ? (g.window_ms_left * 440) / g.window_ms_full : 0;
      if (fill > 0) w.rect(px, 262, static_cast<int>(fill), 16, palette::kPink);
      if (g.phase == Phase::kOrder) {
        for (int i = 0; i < 3; i++) {
          int y = 330 + i * 74;
          int opt = g.options[i];
          const char* resp = opt >= kPairCount ? kPairs[opt - kPairCount].decoy
                                               : kPairs[opt].response;
          int c = i == g.option_cursor ? palette::kPink : palette::kPaper;
          if (i == g.option_cursor) w.text(px - 34, y, tsize(g, 26), palette::kPink, ">");
          w.text(px, y, tsize(g, 20), c, resp);
        }
        w.text(px, 580, tsize(g, 18), palette::kDim, "ARROWS CHOOSE - ENTER ANSWERS - B ARMS");
      } else {
        w.text(px, 340, tsize(g, 30), palette::kPaper, "WELL KEPT.");
      }
    }
  }
  if (g.caption_ms > 0) {
    w.text(90, 862, tsize(g, 20), palette::kRose, g.caption);
  }
}

void render_gate(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(230, 250, tsize(g, 64), palette::kPink, "BY HEART");
  w.text(230, 350, tsize(g, 26), palette::kPaper, "18+ ONLY BEYOND THIS GATE");
  w.text(230, 410, tsize(g, 19), palette::kMauve, "AN ADULT GAME OF PROTOCOL, BY NICE THOUGHTS RANCH.");
  w.text(230, 445, tsize(g, 19), palette::kMauve, "COMMAND AND RESPONSE, KEPT BY HEART.");
  w.text(230, 480, tsize(g, 19), palette::kMauve, "EVERYTHING IS ABSTRACT AND SYMBOLIC. NO ONE IS REAL.");
  w.text(230, 560, tsize(g, 20), palette::kGold, "PRESS ENTER OR TAP TO CONFIRM YOU ARE 18+");
}

void render_title(const Game& g, Writer& w) {
  render_room(w);
  w.text(230, 130, tsize(g, 72), palette::kPink, "BY HEART");
  w.text(230, 225, tsize(g, 20), palette::kMauve, "HER CORRECTION - THE LAW, KEPT BY HEART");
  const char* items[3] = {"BEGIN THE SITTING", "THE BRIEF", "SETTINGS"};
  for (int i = 0; i < 3; i++) {
    int y = 340 + i * 64;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(190, y, tsize(g, 34), palette::kPink, ">");
    w.text(230, y, tsize(g, 34), c, items[i]);
  }
  int y = 340 + 3 * 64;
  if (g.menu_cursor == 3) w.text(190, y, tsize(g, 34), palette::kPink, ">");
  StrBuf row;
  row.push_str("SITTING: ");
  row.push_str(kLevels[g.title_level].name);
  w.text(230, y, tsize(g, 34), palette::kPaper, row.c_str());
  const Level& lvl = kLevels[g.title_level];
  StrBuf det;
  det.push_str("PAIRS ");
  det.push_int(lvl.pair_count);
  det.push_str(" - WINDOW ");
  det.push_int(lvl.window_ms / 100);
  det.push_str(" - TAP ROW TO CHANGE");
  w.text(230, y + 48, tsize(g, 20), palette::kMauve, det.c_str());
  if (g.record_kept > 0) {
    StrBuf rec;
    rec.push_str("KEPT SITTINGS: ");
    rec.push_int(static_cast<long long>(g.record_kept));
    rec.push_str(" - BY HEART: ");
    rec.push_int(static_cast<long long>(g.record_flawless));
    w.text(230, 660, tsize(g, 20), palette::kGold, rec.c_str());
  }
  w.text(230, 880, tsize(g, 20), palette::kDim, "ARROWS CHOOSE - ENTER CONFIRMS");
}

void render_howto(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(110, 100, tsize(g, 40), palette::kPink, "THE BRIEF");
  const char* lines[] = {
      "FIRST, SHE SPEAKS THE LAW: COMMAND, THEN RESPONSE.",
      "KEEP THE PAIRS BY HEART. THERE IS NO PAPERWORK.",
      "",
      "THEN THE ORDERS COME, EACH ON A WINDOW. ANSWER",
      "FROM THE THREE RESPONSES BEFORE IT CLOSES.",
      "ANSWER INSIDE THE FIRST HALF AND IT IS ON THE BEAT.",
      "",
      "FAIL AND THE LADDER CLIMBS: TRAIN, CORRECT, RETRY.",
      "THE WINDOW TIGHTENS. THREE RUNGS FAIL THE ORDER -",
      "IT IS RECORDED, AND THE SITTING GOES ON. NEVER LOCKOUT.",
      "",
      "PLUG ORDERS ARE LOCKED. PRESS B TO SPEAK THE",
      "TRIGGER PHRASE AND ARM THE PLUG BEFORE ANSWERING.",
      "THE LAST ORDERS ARE THE CULMINATION PACKAGE, AT SPEED.",
  };
  for (int i = 0; i < 15; i++) {
    w.text(110, 185 + i * 42, tsize(g, 20), palette::kPaper, lines[i]);
  }
  w.text(110, 850, tsize(g, 20), palette::kGold, "ENTER OR ESC TO RETURN");
}

void render_settings(const Game& g, Writer& w) {
  w.rect(0, 0, kCanvasPx, kCanvasPx, palette::kBg);
  w.text(240, 160, tsize(g, 44), palette::kPink, "SETTINGS");
  StrBuf a;
  a.push_str("WINDOW GRACE: ");
  a.push_str(g.window_grace ? "LONG" : "STANDARD");
  StrBuf b;
  b.push_str("TEXT: ");
  b.push_str(g.text_large ? "LARGE" : "NORMAL");
  const char* rows[3] = {a.c_str(), b.c_str(), "BACK"};
  for (int i = 0; i < 3; i++) {
    int y = 300 + i * 70;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(200, y, tsize(g, 32), palette::kPink, ">");
    w.text(240, y, tsize(g, 32), c, rows[i]);
  }
  w.text(240, 560, tsize(g, 20), palette::kMauve, "SETTINGS ARE NEVER GATED. THEY APPLY AT ONCE");
  w.text(240, 590, tsize(g, 20), palette::kMauve, "AND ARE REMEMBERED BETWEEN VISITS.");
}

void render_pause(const Game& g, Writer& w) {
  render_play(g, w);
  w.rect(230, 260, 500, 400, palette::kDeep);
  w.text(350, 300, tsize(g, 44), palette::kPink, "HELD");
  const char* items[3] = {"RESUME", "RESTART SITTING", "QUIT TO TITLE"};
  for (int i = 0; i < 3; i++) {
    int y = 400 + i * 64;
    int c = g.menu_cursor == i ? palette::kPink : palette::kPaper;
    if (g.menu_cursor == i) w.text(310, y, tsize(g, 32), palette::kPink, ">");
    w.text(350, y, tsize(g, 32), c, items[i]);
  }
}

void render_end(const Game& g, Writer& w, bool won) {
  render_room(w);
  const Level& lvl = kLevels[g.level_idx];
  if (won) {
    const char* grade_word = g.grade() == 3   ? "BY HEART"
                             : g.grade() == 2 ? "CORRECTED, KEPT"
                                              : "DRILLED, KEPT";
    w.text(170, 220, tsize(g, 44), palette::kPink, grade_word);
    w.text(170, 300, tsize(g, 20), palette::kPaper, "THE SITTING IS KEPT. THE LAW HELD.");
  } else {
    w.text(170, 220, tsize(g, 44), palette::kPaper, "THE SITTING ENDS");
    w.text(170, 300, tsize(g, 20), palette::kMauve, "UNKEPT. THE LADDER WILL BE THERE TOMORROW.");
  }
  StrBuf s1;
  s1.push_str("SITTING: ");
  s1.push_str(lvl.name);
  w.text(170, 390, tsize(g, 20), palette::kPaper, s1.c_str());
  StrBuf s2;
  s2.push_str("ORDERS FAILED ");
  s2.push_int(g.failures);
  s2.push_str(" - CORRECTIONS ");
  s2.push_int(g.corrections);
  s2.push_str(" - ON THE BEAT ");
  s2.push_int(g.beats);
  w.text(170, 440, tsize(g, 20), palette::kGold, s2.c_str());
  w.text(170, 490, tsize(g, 20), palette::kPaper,
         g.culminated ? "THE PACKAGE COMPLETED. EXACTLY ONCE." : "THE PACKAGE DID NOT COMPLETE.");
  w.text(170, 620, tsize(g, 20), palette::kGold, "ENTER OR TAP TO RETURN");
}

} // namespace

int Game::render(char* buf, int cap) const {
  Writer w(buf, cap);
  switch (screen) {
    case Screen::kGate: render_gate(*this, w); break;
    case Screen::kTitle: render_title(*this, w); break;
    case Screen::kHowto: render_howto(*this, w); break;
    case Screen::kPlay: render_play(*this, w); break;
    case Screen::kPause: render_pause(*this, w); break;
    case Screen::kSettings: render_settings(*this, w); break;
    case Screen::kVictory: render_end(*this, w, true); break;
    case Screen::kGameOver: render_end(*this, w, false); break;
  }
  return w.pos;
}

} // namespace byheart

// ---------------------------------------------------------------------------
// FFI surface: the whole contract with the shell. One global game, one
// global draw buffer; single-threaded by the shell's construction.
// ---------------------------------------------------------------------------

#if defined(__wasm__)
namespace {
byheart::Game* g_game = nullptr;
char g_draw[byheart::kDrawBufferBytes];
int g_draw_len = 0;

byheart::Game& game_ref() {
  if (g_game == nullptr) {
    static byheart::Game fallback(1, false);
    return fallback;
  }
  return *g_game;
}

void render_global() {
  byheart::Game& g = game_ref();
  g_draw_len = g.render(g_draw, byheart::kDrawBufferBytes);
}
} // namespace

extern "C" {

__attribute__((export_name("init"))) unsigned init(unsigned seed, unsigned level) {
  if (level >= byheart::kLevelCount) return 1;
  static byheart::Game g(seed, false);
  g = byheart::Game(seed, false);
  g.title_level = level;
  g_game = &g;
  render_global();
  return 0;
}

__attribute__((export_name("init_demo"))) unsigned init_demo(unsigned seed, unsigned level) {
  if (level >= byheart::kLevelCount) return 1;
  static byheart::Game g(seed, true);
  g = byheart::Game(seed, true);
  g.title_level = level;
  g_game = &g;
  render_global();
  return 0;
}

__attribute__((export_name("start_level"))) unsigned start_level(unsigned idx) {
  bool ok = game_ref().start_level(idx);
  render_global();
  return ok ? 0u : 1u;
}

__attribute__((export_name("step"))) void step(unsigned dt_ms, unsigned bits) {
  game_ref().step(dt_ms, bits);
  render_global();
}

__attribute__((export_name("screen"))) unsigned screen() { return game_ref().screen_id(); }

__attribute__((export_name("level_idx"))) unsigned level_idx() { return game_ref().level_idx; }

__attribute__((export_name("apply_settings"))) void apply_settings(unsigned grace, unsigned large) {
  byheart::Game& g = game_ref();
  g.window_grace = grace != 0;
  g.text_large = large != 0;
  render_global();
}

__attribute__((export_name("settings_bits"))) unsigned settings_bits() {
  byheart::Game& g = game_ref();
  return (g.window_grace ? 1u : 0u) | (g.text_large ? 2u : 0u);
}

__attribute__((export_name("apply_record"))) void apply_record(unsigned kept, unsigned flawless) {
  byheart::Game& g = game_ref();
  g.record_kept = kept > 999 ? 999 : kept;
  g.record_flawless = flawless > kept ? kept : flawless;
}

__attribute__((export_name("record_kept"))) unsigned record_kept() { return game_ref().record_kept; }

__attribute__((export_name("record_flawless"))) unsigned record_flawless() {
  return game_ref().record_flawless;
}

__attribute__((export_name("event_code"))) unsigned event_code() { return game_ref().event_code; }

__attribute__((export_name("stat_failures"))) unsigned stat_failures() {
  return static_cast<unsigned>(game_ref().failures);
}

__attribute__((export_name("stat_beats"))) unsigned stat_beats() {
  return static_cast<unsigned>(game_ref().beats);
}

__attribute__((export_name("stat_grade"))) unsigned stat_grade() {
  return static_cast<unsigned>(game_ref().grade());
}

__attribute__((export_name("draw_ptr"))) const char* draw_ptr() { return g_draw; }

__attribute__((export_name("draw_len"))) unsigned draw_len() {
  return static_cast<unsigned>(g_draw_len);
}

} // extern "C"
#endif // defined(__wasm__)
