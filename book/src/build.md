# Build & Delivery — By Heart

**Language:** C++ / Unreal (tiger-style-cpp)

**Delivery shape:** Named seam: UE5 has no HTML5 export. The entry is C++ compiled via Emscripten and inlined as wasm; the Unreal pairing runs at language + broker level — seahawk playtests the design inside UE 5.8 on primo, and that session is part of the week's evidence. Toolchain home: primo (Unreal lives there).

**Jam rule:** one self-contained `index.html`, ≥6 screens (Title / Pause /
How to Play / Gameplay / Settings / Game Over-or-Victory), documented
process (GAMEOTRON orchestrates; the language build chain is part of the
documentation).

**Book:** `mdbook build book` renders this planning book to `book/html/`
with the NTRanch theme (dark background, hot pink).
