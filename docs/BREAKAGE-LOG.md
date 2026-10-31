# Breakage log — By Heart

Every breakage found in playtest or build lands here with: what broke, the
evidence, the bloch attempt id, and the fix or the filed finding. Copied to
bloch at the week's close (the log is the input, bloch is the record).

## 2026-10-30 — v1.0 build (sandbox)

- **B-1: freestanding link failed on `strlen`.** clang's loop-idiom
  recognition rewrote the core's own length loop into a call to
  strlen, which does not exist in a -nostdlib wasm module. Fix:
  `-fno-builtin` on the wasm target (it is in the justfile's
  WASM_FLAGS, marked load-bearing). Estate lesson for freestanding
  C/C++: assume idiom recognition will invent libc calls; switch it
  off at the flag line, not in the code.
- **B-2: the FFI skin leaked into the native build.** The export_name
  attribute is wasm-only; the shared translation unit failed the
  native test build under -Werror. Fix: the whole FFI section
  (globals + exports) is `#if defined(__wasm__)`; the native runner
  tests the Game class directly. Nothing about the core changed.
- **B-3: text overflow across the play panel (raster audit).** The
  estate raster-advance audit (Exposure B-4) flagged 49 lines: the
  right-hand panel is only 520 px wide from x=440, so response lines
  at size 24 and the sitting clock at x=700 ran past the edge of the
  rendered surface. Fix before first capture sign-off: panel text at
  size 20, clock moved and sized down, two correction lines and the
  gate's second line shortened. Audit CLEAN; the D-221 frame check
  then passed on the re-rendered previews.
- **B-4 (environment note): a compile read a phantom line 852.** One
  wasm compile reported a line of NUL bytes past the end of
  src/core.cpp; the file on disk verified byte-clean (27,993 bytes,
  zero NULs) immediately after, and the identical command then built
  green. Recorded as a transient read during a rapid edit/build
  sequence; no action taken, no recurrence in later builds.
- **B-5 (held, not a defect): the UE 5.8 pairing + seahawk session
  not run.** Unreal lives on primo; primo is unreachable tonight
  (tunnel outage since 03:15 PDT), and the sandbox has no UE. The
  shipped artefact is the Emscripten-seam C++ core the PLANNING names
  as the entry; the brokered playtest inside UE 5.8 is the primo
  follow-up. R-905's verify names the sandbox playtest for this
  reason.
- **B-6 (held, by design of the jam): GJ-07 live itch checks open.**
  Verified live at entry time; no entry is being submitted tonight.
  R-904 stays open. The game's own 18+ gate ships first regardless.
- **B-7 (design ruling): persistence scope.** What persists (shell
  localStorage): settings (window grace, text size) and the record
  (kept sittings, by-heart sittings). There is no mid-sitting save.
  Recorded here rather than silently narrowing R-202/R-203.
