// by-heart/src/core.hpp — By Heart core (C++20, freestanding wasm32).
//
// Tiger Style (tiger-style-cpp): explicit contracts, bounded work,
// disciplined ownership, no hidden behaviour. The core is a fixed-step
// deterministic simulation: the shell calls step() at 60 Hz; the core
// never reads a wall clock and reports everything it wants drawn as a
// draw list in a static buffer. No STL containers, no exceptions, no
// RTTI — the same translation unit compiles for wasm32-freestanding
// and for the native test runner.
//
// Game: Piper's Correction. The law is spoken in the teach phase;
// orders are then issued under time and the player answers from the
// heart. Failure climbs the correction ladder (train, correct,
// retry — never lockout). Plug orders stay locked until the trigger
// phrase arms them. The session's last orders form the culmination
// package, which completes exactly once.

#pragma once

namespace byheart {

// ---------------------------------------------------------------------------
// Domain constants
// ---------------------------------------------------------------------------

inline constexpr int kCanvasPx = 960;
inline constexpr int kMaxOps = 1024;
inline constexpr int kDrawBufferBytes = 64 * 1024;
inline constexpr int kStepMs = 16;
inline constexpr int kPairCount = 8;
inline constexpr int kMaxOrders = 16;

// Palette indices (shared with the shell + rasteriser; 0 = background).
namespace palette {
inline constexpr unsigned char kBg = 0;    // #0d0208
inline constexpr unsigned char kPink = 1;  // #ff2e88
inline constexpr unsigned char kSkin = 2;  // #f8f0ed
inline constexpr unsigned char kMauve = 3; // #ac9393
inline constexpr unsigned char kPlum = 4;  // #540e46
inline constexpr unsigned char kRose = 5;  // #ec8584
inline constexpr unsigned char kPaper = 6; // #eaeaea
inline constexpr unsigned char kDim = 7;   // #68636c
inline constexpr unsigned char kGold = 8;  // #ffd166
inline constexpr unsigned char kDeep = 9;  // #1a0510
} // namespace palette

// Input bitset delivered by the shell each step.
namespace input {
inline constexpr unsigned kTap = 1;
inline constexpr unsigned kConfirm = 2;
inline constexpr unsigned kBack = 4;
inline constexpr unsigned kPause = 8;
inline constexpr unsigned kUp = 16;
inline constexpr unsigned kDown = 32;
inline constexpr unsigned kArm = 64; // speak the trigger phrase
} // namespace input

// Event codes the shell sonifies; audio only ever answers input.
namespace event {
inline constexpr unsigned kNone = 0;
inline constexpr unsigned kCorrect = 1;
inline constexpr unsigned kCorrection = 2;
inline constexpr unsigned kOnTheBeat = 3;
inline constexpr unsigned kArmed = 4;
inline constexpr unsigned kCulmination = 5;
inline constexpr unsigned kVictory = 6;
inline constexpr unsigned kGameOver = 7;
inline constexpr unsigned kOrderIssued = 8;
} // namespace event

// Screens (the program-wide numbering).
enum class Screen : unsigned {
  kGate = 0,
  kTitle = 1,
  kHowto = 2,
  kPlay = 3,
  kPause = 4,
  kSettings = 5,
  kVictory = 6,
  kGameOver = 7,
};

// Play phases within a sitting.
enum class Phase : unsigned {
  kTeach = 0,  // the law is spoken, pair by pair
  kOrder = 1,  // an order is live, the window runs
  kPraise = 2, // well kept — a breath before the next order
  kCorrect = 3 // the pair is shown again; the ladder climbs
};

// One law pair: a command and the response kept by heart.
struct Pair {
  const char* command;
  const char* response;
  const char* decoy; // near-miss response (By Heart level only)
};

// One order in a sitting's sequence.
struct Order {
  int pair;      // index into the level's taught pairs
  bool plug;     // a Permission Plug order: locked until armed
  bool chain;    // part of the culmination package
};

// A session contract: the preset IS the level.
struct Level {
  const char* name;
  int pair_count;        // pairs taught this sitting
  int order_count;       // recall orders before the chain
  int chain_count;       // culmination package length
  int window_ms;         // response window per order
  int sitting_ms;        // the whole sitting's clock
  int allowed_failures;  // orders that may be failed and still kept
  bool plug_orders;      // plug orders appear
  bool decoys;           // near-miss options appear
};

inline constexpr Level kLevels[3] = {
    {"FIRST LESSONS", 4, 6, 2, 6000, 180000, 2, false, false},
    {"THE SPOKEN LAW", 6, 8, 3, 4500, 240000, 1, true, false},
    {"BY HEART", 8, 10, 4, 3500, 300000, 0, true, true},
};
inline constexpr unsigned kLevelCount = 3;

// The spoken law, whole. Order is teaching order.
inline constexpr Pair kPairs[kPairCount] = {
    {"PRESENT", "HANDS SHOWN. EYES DOWN.", "HANDS SHOWN. EYES UP."},
    {"KNEEL", "KNEES DOWN. BACK STRAIGHT.", "KNEES DOWN. BACK BENT."},
    {"SPEAK", "WHEN ASKED. NOT BEFORE.", "WHEN READY. NOT BEFORE."},
    {"HOLD", "STILL UNTIL RELEASED.", "STILL UNTIL SHE TIRES."},
    {"OFFER", "BOTH HANDS. PALMS UP.", "ONE HAND. PALMS UP."},
    {"COUNT", "ALOUD. ON HER NUMBER.", "ALOUD. ON MY NUMBER."},
    {"BREATHE", "SLOW. ON HER COUNT.", "SLOW. ON MY COUNT."},
    {"ATTEND", "EYES ON HER. ALWAYS.", "EYES ON HER. OFTEN."},
};

// The trigger phrase. The plug arms on it, and only on it.
inline constexpr const char* kTriggerPhrase = "PLEASE. MAY I?";

// ---------------------------------------------------------------------------
// Deterministic RNG (xorshift32; the jam cores' shared family)
// ---------------------------------------------------------------------------

struct Rng {
  unsigned state;
  explicit Rng(unsigned seed) : state(seed == 0 ? 0x9E3779B9u : seed) {}
  unsigned next() {
    unsigned x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
  }
  // Uniform integer in [lo, hi]. Contract: lo <= hi.
  int range(int lo, int hi) {
    return lo + static_cast<int>(next() % static_cast<unsigned>(hi - lo + 1));
  }
};

// ---------------------------------------------------------------------------
// The game
// ---------------------------------------------------------------------------

struct Game {
  Rng rng;
  bool demo;
  Screen screen = Screen::kGate;
  long long screen_ms = 0;
  unsigned level_idx = 0;
  int menu_cursor = 0;
  unsigned title_level = 0;

  // Sitting state (reset by start_level).
  Phase phase = Phase::kTeach;
  long long phase_ms = 0;
  int teach_idx = 0;
  Order orders[kMaxOrders];
  int order_total = 0;
  int order_idx = 0;
  int options[3]; // pair indices (or -decoy marker: pair + kPairCount)
  int option_count = 0;
  int option_cursor = 0;
  long long window_ms_left = 0;
  long long window_ms_full = 0;
  bool armed = false;
  int ladder = 0; // correction rung on the current order: 0..2 shown, 3 = failed
  int failures = 0;
  int corrections = 0;
  int beats = 0;
  bool culminated = false;
  long long sitting_ms_left = 0;

  // Presentation.
  const char* caption = "";
  long long caption_ms = 0;
  unsigned event_code = event::kNone;

  // Bridges (shell-persisted).
  bool window_grace = false;
  bool text_large = false;
  unsigned record_kept = 0;
  unsigned record_flawless = 0;

  Game(unsigned seed, bool demo_mode);
  bool start_level(unsigned idx);
  void step(unsigned dt_ms, unsigned input_bits);
  unsigned screen_id() const { return static_cast<unsigned>(screen); }
  int grade() const; // 0 none, 1 drilled, 2 corrected, 3 by heart (victory only)
  int render(char* buf, int cap) const;

 private:
  void set_screen(Screen s);
  void set_caption(const char* text, long long ms);
  void begin_order();
  void answer(int option_pos);
  void fail_order(const char* why);
  void advance_order();
  void finish_sitting(bool won);
  unsigned demo_policy();
  void step_play(long long dt, unsigned inp);
  void step_title(unsigned inp);
  void step_pause(unsigned inp);
  void step_settings(unsigned inp);
};

} // namespace byheart
