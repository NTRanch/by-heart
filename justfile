# By Heart — task recipes (wired at the v1.0 build, 2026-10-30).
# clang 23 in the sandbox; the core is freestanding C++20, compiled
# natively for the test runner and to wasm32 for the page. -fno-builtin
# is load-bearing on the wasm target (breakage B-1).

CXX := env_var_or_default("CXX", "clang++")
WASM_FLAGS := "--target=wasm32 -std=c++20 -Os -nostdlib -fno-exceptions -fno-rtti -fno-builtin -Wall -Wextra -Werror -Wl,--no-entry -Wl,--export-memory"

# Assemble the single-file jam entry: C++ core -> wasm, inlined
build:
    {{CXX}} {{WASM_FLAGS}} -o build/byheart.wasm src/core.cpp
    node tools/assemble.mjs

# Run every gate: native core tests, bridges, perf, playtest
check:
    {{CXX}} -std=c++20 -Wall -Wextra -Werror -O1 -o build/test_core src/core.cpp tests/test_core.cpp
    ./build/test_core
    {{CXX}} {{WASM_FLAGS}} -o build/byheart.wasm src/core.cpp
    node tools/assemble.mjs
    node tools/settings-check.mjs
    node tools/perf.mjs
    node tools/playtest.mjs

# Render the per-level preview videos from the shipped core (demo mode)
previews:
    node tools/capture.mjs 0 20265
    node tools/capture.mjs 1 20265
    node tools/capture.mjs 2 20265

# Build the planning book
book:
    mdbook build book
