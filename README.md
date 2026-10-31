# By Heart

**Nice Thoughts Ranch** · Torogao Jam Program week 7 (Wave B — Sound) · Piper · **C++ / Unreal**

The Correction — her core game, at prototype scale.

An 18+ game built for the weekly Gameotron jam on itch.io (NTRanch), as one
rung of the 14-week Torogao system-validation program (D-333/D-334/D-335/D-336).
The full planning book lives in [`book/`](book/) (mdBook, NTRanch theme);
this repo's bloch roadmap is [`.ai/roadmap.md`](.ai/roadmap.md).

- **Delivery:** one self-contained `index.html` (see `docs` in the book).
- **Cast/art law (D-292):** existing keeper art only; a missing asset is an
  abstract stand-in, logged — never a generation job.
- **Quality bar:** Laplace3D-class animation or the medium's equivalent
  (book chapter: Quality Bar).
- **Audio:** nothing is ever played aloud by an agent; the Owner presses play.

## Play

Open [`web/index.html`](web/index.html) in a browser — it runs fully
offline. First she speaks the law: command, then response, pair by
pair. Keep them **by heart** — there is no paperwork. Then the orders
come, each on a window: choose the response with the arrow keys and
answer with ENTER before the window closes. Inside the first half is
**on the beat**.

Fail, and the ladder climbs: train, correct, retry — the window
tightening each rung. Three rungs fail the order; it is recorded, and
the sitting goes on. **Never lockout.** Plug orders are locked: press
**B** to speak the trigger phrase and arm the plug before answering —
even the right words are refused without permission. The last orders
are the culmination package, at speed.

- **Level 1 — First Lessons:** 4 pairs, 6.0 s windows, 2 failures allowed.
- **Level 2 — The Spoken Law:** 6 pairs, plug orders, 4.5 s windows.
- **Level 3 — By Heart:** 8 pairs, near-miss decoys, 3.5 s, flawless or lost.

Settings (window grace, text size) and the record — sittings kept,
sittings kept by heart — persist in the browser. Settings are
reachable before any play.

## Preview videos (rendered from the shipped core, demo mode)

- [Level 1 — First Lessons](docs/previews/level-1-by-heart.mp4)
- [Level 2 — The Spoken Law](docs/previews/level-2-by-heart.mp4)
- [Level 3 — By Heart](docs/previews/level-3-by-heart.mp4)

## Build & verify

```sh
just build   # C++ core -> wasm + assemble web/index.html
just check   # 380 native checks, bridges, perf budget, playtest
just previews
```

Status: **v1.0.0** (2026-10-30) — playable, gated, tagged. The UE 5.8
pairing + seahawk session on primo and the live itch.io entry checks
(GJ-07) remain scheduled follow-ups; both are recorded in
`docs/BREAKAGE-LOG.md`.
