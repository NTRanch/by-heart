# C++ / Unreal Engine 5 — W7 *By Heart* + W14 *Standard Issue*

**Language / engine:** C++ on Unreal Engine 5.8 (the estate's broker target), plus a
standalone C++ core for the jam artifact.
**Weeks:** W7 *By Heart* (Piper — The Correction: protocol + culmination inside a real
command/response loop, administered through the Permission Plug) and W14 *Standard
Issue* (Piper — earn one Certified Standard: Fit + Signals exam, T-gate arithmetic,
book of record).
**Role in the program:** the Unreal weeks exist to validate two things at once — the
*designs* (Piper's protocol stack in W7, the certification loop in W14) inside the estate's
heaviest engine, and **seahawk**, the estate MCP that brokers a live UE 5.8 session:
the seahawk playtest session inside Unreal is part of each week's evidence, not a
convenience.

## Delivery shape and the named seam (read this first)

**UE5 has no HTML5 export. There is no supported path from a UE5 project to a browser
build.** Epic deprecated the HTML5 platform in 4.23 and migrated it out of the engine
in 4.24 to a community Platform Extension whose support ended at 4.24.3; UE5 never
regained a web target (Epic staff statement:
https://forums.unrealengine.com/t/html5-support/1172997). Any document, tool, or vendor
claiming "UE5 → HTML5 export" is selling the dead 4.24 extension or a streaming service;
neither is an export.

So the week splits into two artifacts with an honest wall between them:

1. **The jam entry (what itch.io serves):** the week's game logic written as a
   **standalone C++ core** — no UE runtime, no UObject, no Blueprint — compiled with
   **Emscripten** (emsdk: https://github.com/emscripten-core/emsdk.git) to a wasm module
   inlined (base64) into the single self-contained `index.html`. JS is loader, DOM
   bridge, and presentation shell only (Canvas/WebGL2 quads, WebAudio). The C++ core
   is written in Unreal *idiom* — the same state machines, phrase sets, and arithmetic
   the UE build runs — so the two implementations can be diffed behaviourally.
2. **The Unreal pairing (what the week validates):** the same design built as a real
   UE 5.8 C++ project on primo, played and probed through **seahawk's brokered UE
   session** (tool calls, screenshots, state reads) as the playtest evidence leg. This
   build targets desktop, never the browser. Pixel Streaming is Epic's only sanctioned
   "UE in a browser tab" route and it is a hosted stream, not a file — it cannot be a
   jam submission and is not used.

Everything in the parts matrix below is labelled **[JAM]** (runs inside the wasm entry),
**[UE]** (runs in the paired Unreal build), or **[BOTH]**.

## Parts matrix

### (a) 2D rendering in a 3D engine

| Candidate | Verdict |
|---|---|
| **Paper2D** (built-in plugin: sprites, flipbooks, tilemaps) | **[UE] Use with eyes open.** Paper2D still ships in UE5 and works, but Epic's own position, on record since 2017 and unchanged since, is: *"Paper2D is a supported part of the engine and we will continue to fix blocking bugs as they come up, but there is no feature development planned in the short or medium term"* (https://forums.unrealengine.com/t/has-paper2d-development-stopped/78288?page=4). Third-party 2026 comparisons bluntly list it as "deprecated / no longer actively maintained" (https://oceanviewgames.co.uk/blog/posts/unity-vs-godot-vs-unreal-2026). It is a frozen asset, not a growing one: fine for flipbook playback in a one-week build, a liability as a foundation. |
| **PaperZD** (Critical Failure Studio) | **[UE] Recommended animation layer over Paper2D.** Adds what Paper2D never got: 2D Animation Blueprints, state machines with transitions/conduits, and animation notifies for flipbooks — the 3D AnimBP workflow applied to sprites (https://www.criticalfailure-studio.com/?p=1838). Commercial, $44.99 on itch.io (https://criticalfailure-studio.itch.io/paperzd); current line targets UE 5.2+. Hard-depends on the Paper2D plugin for flipbook assets, so it inherits Paper2D's frozen base. |
| **UMG / Slate** (built-in UI) | **[UE] BOTH weeks' dialogue and exam UI.** For a VN-shaped week the "2D rendering" that matters most is text, portraits-as-widgets, and buttons; UMG does this natively and is fully supported. Portraits and backgrounds as UMG images over a minimal 3D room is the pragmatic UE shape for W7/W14. |
| **Skeletal-mesh 3D with toon rendering** (built-in) | **[UE] The Laplace route — see (b) and the quality bar.** UE's first-class path is not 2D at all: rigged 3D characters (SkeletalMesh + Animation Blueprints + Control Rig) with a cel/toon material. This is the only UE route whose ceiling matches the program's quality bar, because it *is* Laplace3D's medium. |
| **[JAM] entry rendering** | Custom. Emscripten's WebGL/OpenGL ES support under a hand-written quad/sprite renderer driven by the C++ core; no library named here survives the single-file constraint better than ~600 lines of our own renderer. Spine/Live2D web runtimes were considered and rejected for the entry (licence weight + a second parameter model to keep in sync); the entry plays the estate's own baked parameter/flipbook data. |

### (b) Skeletal / puppet animation

| Candidate | Verdict |
|---|---|
| **Native skeletal animation** (SkeletalMesh, AnimBP, Control Rig, Sequencer) | **[UE] First choice for 3D-rigged cast.** Built-in, Epic-maintained, 60 fps by construction, and the pipeline Laplace-class work already uses (Blender-keyframed rigs retargeted in). Control Rig gives in-engine procedural correction (hand placement, gaze targets) that the two-body/contact work needs. Licence: Unreal Engine EULA (royalty model); maturity: production, AAA-proven. |
| **Cubism SDK for Unreal Engine** (Live2D Inc.) | **[UE] Real but young — beta.** Live2D's official UE SDK exists: components on GitHub (https://github.com/Live2D/CubismUnrealEngineComponents/), manual at https://docs.live2d.com/en/cubism-sdk-manual/cubism-sdk-for-unreal-engine/. Honest maturity note: the manual pages still carry alpha/beta statements, the Cubism Core binary is proprietary and *not* on GitHub (download package only, per https://docs.live2d.com/en/cubism-sdk-manual/cubism-core/), and shipping requires a Live2D publication licence beyond the indie threshold (licence overview: https://www.live2d.com/en/sdk/about/). Usable for a Live2D-modeled girl in the UE build; do not make a jam week depend on beta SDK behaviour. |
| **spine-ue** (Esoteric Software) | **[UE] Mature, with known gaps.** The official Spine runtime for UE, C++ + Blueprint, actively versioned (docs track UE 5.5+ sample projects): http://es.esotericsoftware.com/spine-ue. Licence: the Spine Runtimes License — runtimes are usable only with a Spine editor licence, and the runtime cannot be distributed through the UE Marketplace because of that licence (vendor's own forum statement, via search results above). Documented gaps vs spine-unity: no multiply/screen blend modes, no pre-multiplied-alpha atlases, no two-colour tinting. If a girl's rig is Spine-authored, this is the route; the estate's rigs are not, so it is a contingency, not the plan. |
| **[JAM] entry** | Data-driven playback of the estate's own rig/parameter data by the custom renderer (see (a)). No third-party runtime. |

### (c) Audio

| Candidate | Verdict |
|---|---|
| **MetaSounds** (built-in) | **[UE] Default.** UE5's procedural audio system: a DSP graph ("the Material Editor for sound") with sample-accurate timing, parameter-driven playback, and per-instance synthesis — the right tool for command/response timing in W7 (a spoken command and its response click must land on the frame the protocol state changes) and exam feedback in W14 (https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-in-unreal-engine; quick start: https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-quick-start). Licence: engine EULA. Maturity: production since UE5.0. |
| **FMOD Studio for Unreal** (Firelight Technologies) | **[UE] Capable, unnecessary here.** Official integration, free to download, currently supports UE 4.27 and 5.5–5.8 (https://www.fmod.com/docs/unreal) — but releasing a title requires an FMOD licence (indie tier by budget), and nothing in W7/W14 needs FMOD's event authoring beyond what MetaSounds already does. Named so the choice is deliberate: not adopted. |
| **[JAM] entry** | WebAudio in the JS shell, driven by cue events emitted from the C++ core. Audio assets are pre-rendered files; per the estate audio-silence rule they are verified by file metrics during development, never played aloud by an agent. |
| Voice | No TTS/voice library is adopted for either week. Piper's lines are recorded/estate-produced assets triggered as data; the *timing contract* (command → response latency) is ours (see Write-from-scratch §3). |

### (d) Dialogue / visual-novel layer

| Candidate | Verdict |
|---|---|
| **Ink + Inkpot** | **[UE] Recommended.** Ink (inkle, MIT) is the narrative middleware; the UE container is **Inkpot**, The Chinese Room's C++ port of the ink runtime with a Blueprint-friendly wrapper, built for *Bloodlines 2* / *Still Wakes the Deep* and released free on their GitHub (story and links: https://www.paradoxinteractive.com/games/vampire-the-masquerade-bloodlines-2/news/mission-scripting-and-ink; ink itself: https://www.inklestudios.com/ink/). Maturity: production-proven in shipped AAA, community-maintained since. W7's command/response script and W14's exam script are exactly ink's shape: lines, choices, variables, external functions calling back into the protocol engine. |
| **Yarn Spinner for Unreal** | **[UE] Real, alpha.** Official YarnSpinnerTool integration for UE exists and is listed by Yarn Spinner themselves — labelled an **alpha release** (https://yarnspinner.dev/install/). Yarn's screenplay syntax is friendlier for pure dialogue; ink wins here because the weeks need ink's stronger variable/flow model for protocol state, and because Inkpot is production-proven while this is alpha. |
| **UnrealInk** | **[UE] Superseded.** The earlier UE4 C++ ink integration; still listed as ink-compatible in inkle's library, but Inkpot is the UE5-era successor. Not adopted. |
| **Visual Novel Machinery** (Fab/marketplace plugin) | **[UE] Not adopted.** A real no-code VN plugin for UE (node dialogue, save system, beta Yarn import — https://forums.unrealengine.com/t/al-fe-visual-novel-machinery/2695764), but a jam week that exists to validate *our* protocol systems gains nothing from a black-box VN framework, and marketplace-plugin licence/maintenance risk sits on the critical path. Named for completeness. |
| **[JAM] entry** | The ink story is compiled to JSON at build time and stepped by a small C++ story-stepper we write (see register) — or, honestly simpler and permitted by the design: dialogue in the entry is authored directly as data tables in the C++ core, since W7/W14 scripts are short and protocol-shaped rather than prose-branching. The UE build is where ink earns its keep. |

### (e) State machines / protocol tables

| Candidate | Verdict |
|---|---|
| **Gameplay Ability System (GAS)** | **[UE] Available, deliberately NOT adopted at jam scale.** GAS (AbilitySystemComponent, GameplayEffects, tags) is Epic's framework for abilities/attributes/effects, battle-tested in Fortnite/Paragon (overview and class map: https://ikrima.dev/ue4guide/gameplay-programming/gameplay-ability-system/overview/). Its value is replication, prediction, and attribute plumbing for networked combat — none of which a single-player protocol exam needs. Adopting GAS for W7/W14 would spend the week on framework ceremony. Verdict: plain C++ state machines now; GAS is the named candidate if these systems graduate into the networked main build. |
| **UE StateTree / AnimBP state machines** | **[UE] Partial.** AnimBP/PaperZD state machines drive *presentation* state (which animation/expression plays). StateTree (Epic's gameplay state-selection system, shipped since UE 5.1) is a legitimate home for exam flow, but at this size a hand-rolled `UEnum`-keyed transition table in C++ is smaller, diffable, and identical to the jam core's implementation — which is the week's whole point. |
| **Data Tables / Data Assets** (`UDataTable`, CSV/JSON import) | **[BOTH] The protocol tables live here.** Phrase sets, drill positions, exam questions, and T-gate thresholds are authored as data (CSV → UDataTable in UE; the same CSV compiled into the wasm core), never hardcoded. One source file, two runtimes — the conformance test in (g) diffs their behaviour. |
| **[JAM]/[UE] the machines themselves** | Custom C++ (see register §3): the command/response loop, phrase-set gate, correction ladder, and certification arithmetic are small deterministic machines, written once as a shared-design pair. |

### (f) Persistence

| Candidate | Verdict |
|---|---|
| **SaveGame system** (`USaveGame`, Save/Load Game to Slot) | **[UE] Adopted.** Built-in, documented (https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine?application_version=5.0). W14's book of record — certifications earned, exam history — is a SaveGame object; W7's session contract (armed plug state, correction count) persists the same way. |
| **[JAM] entry** | The shell's `localStorage`, written by the C++ core through the JS bridge, storing the same record schema (JSON) as the UE SaveGame exports. A record written by the entry imports into the UE build for the port-back comparison. |

### (g) Build tooling and testing

| Candidate | Verdict |
|---|---|
| **UnrealBuildTool + RunUAT** | **[UE]** Standard cook/package for the desktop pairing build; no web target exists (see seam). |
| **Emscripten (emsdk)** | **[JAM]** The entry toolchain: `emcc` compiles the standalone C++ core to wasm; the module is base64-inlined into `index.html`. No UE code crosses into this build — that is a build-system firewall, enforced by the core having zero engine includes (https://github.com/emscripten-core/emsdk.git). |
| **UE Automation Framework + Gauntlet** | **[UE] Testing.** Built-in: simple/complex automation tests for the C++ machines (phrase-set gating, T-gate arithmetic — pure functions, ideal unit targets), functional tests in-editor, Gauntlet for cooked-build orchestration (survey: https://helpmetest.com/blog/unreal-engine-testing/; practice notes: https://www.gamedevpensieve.com/engines/unreal/unreal_devops/unreal_testing). |
| **Conformance harness (custom)** | **[BOTH]** The same scripted input sequences (command streams, exam answer streams) are replayed against the wasm core and the UE build; state traces must match exactly. This harness is the week's determinism evidence and is written from scratch (register §3). |
| **seahawk session** | **[UE] Playtest evidence.** The brokered UE 5.8 session executes the playtest script — commands issued, plug state read back, exam administered — and its log ships with the week's breakage log. |

## Write-from-scratch register

Nothing below has an off-the-shelf equivalent at any licence. Each entry names why,
the layer it lives in, and what it rides on.

1. **Face-state system (F-code states, transitions, gaze channels).** *Why:* the F-code
   set (F-03 edge, F-05 peak, F-10 daze, and the ratified variants) is estate canon with
   estate measurement law (§1.7 full-disc gaze definition); no library knows what an
   F-state *is*. *Layer:* a deterministic parameter machine in C++ — states are
   parameter targets (lid, iris, blush, mouth, gaze vectors), transitions are timed
   curves. *Rides on:* AnimBP/Control Rig (3D) or PaperZD (2D) in UE; the custom
   renderer's parameter playback in the entry.
2. **Arousal engine + culmination package inside a command/response loop (W7 core).**
   *Why:* arousal-engine/1's tier law (T1 4+4 / T2 7+6 / T3 11+9 / T4 16+12 s) and the
   exactly-once culmination contract are estate specifications with estate tests
   (torogao-culmination/1, 15/15 in Rust); a UE implementation is a port of a spec,
   not a design task. *Layer:* pure C++ core, tick-driven by an injected clock so the
   automation tests and the conformance harness can run it deterministically.
   *Rides on:* MetaSounds for the audible surface of state changes; ink external
   functions call into it from the script.
3. **Permission Plug phrase-set system (W7).** *Why:* "arms on the trigger phrase,
   locks without permission" is a bespoke consent mechanic: a phrase table (DataTable),
   a recogniser over player input (typed or chosen phrases), armed/locked state, and a
   refusal path that is firm without being a lockout. No dialogue system ships this;
   ink/Yarn handle prose flow, not gated actuation. *Layer:* C++ machine + DataTable
   phrase sets per protocol level. *Rides on:* Inkpot external functions; UMG input.
4. **Protocol recall + correction ladder (W7).** *Why:* Piper's law is spoken and kept
   by heart — recall under time pressure, scored; failure routes train → correct →
   retry, never lockout. The scoring (what counts as a kept phrase, the time window,
   the ladder's rung effects on the plug state) is game design that exists only in
   estate canon. *Layer:* C++ machine over the phrase/recall tables. *Rides on:*
   MetaSounds timing for the recall window's audible pressure; SaveGame for the
   session's correction count.
5. **Certification / T-gate arithmetic + book of record (W14 core).** *Why:* "T2 needs
   Fit+Signals; T3 needs two Certified Standards" is arithmetic over estate
   achievement canon; the exam (Fit + Signals administered by spoken law) and the
   book-of-record entry (pass entered aloud and by heart) are the game. *Layer:* pure
   C++ functions over DataTable exam content; the record is a SaveGame-resident ledger
   with append-only semantics. *Rides on:* Inkpot for exam script flow, SaveGame for
   persistence, UMG for the book's presentation.
6. **Sex-scene staging / contact (both weeks' scenes).** *Why:* contact between two
   rigs — occlusion, hand-to-body placement, the held gaze during a guided hand
   (W4's problem, present in miniature in W7's guided correction) — is the pipeline's
   named research problem; libraries provide rigs, not believable contact. *Layer:*
   authored keyframes + Control Rig IK corrections in UE; in the entry, baked
   parameter tracks. *Rides on:* Control Rig, Sequencer; the estate's contact
   findings file is the spec.
7. **Appraisal bookkeeping (W14, shared with W9's Lua week).** *Why:* an appraisal is
   evidence logged and witnessed, and rank moves only at the ceremony — an
   append-only, witness-stamped ledger with estate semantics. *Layer:* C++ ledger
   module, serialised with the book of record. *Rides on:* SaveGame; the schema is
   shared verbatim with the Lua implementation so records inter-operate.
8. **The jam wasm core's story-stepper + conformance harness.** *Why:* the single-file
   constraint and the two-runtime diff requirement are estate-specific. *Layer:*
   build tooling + a minimal data-table interpreter. *Rides on:* Emscripten; the UE
   automation suite replays the same scripts.

## Quality bar — Laplace3D-class minimum

The floor for both weeks, measured on the final artifacts: **60 fps** sustained;
**motion on every frame** (no frozen holds outside an authored beat); **secondary-motion
lag ≤ 17 ms** behind primary motion (hair, clothing, plug cable, breath); **humanised
jitter** on held poses (deterministic, seeded — never random per run); **expression
readability** at delivery size (the F-state must be nameable from a still at itch-page
scale, D-221 expected-vs-seen); **loop seam ≤ 1.1** (loop-point discontinuity within the
estate's seam tolerance); **audio sync +12 ms ± 25 ms** against the visual event it
answers; **determinism** — same inputs, same trace, both runtimes (the conformance
harness is the proof).

**UE's honest position against this bar:** this is the one stack where the bar is
*native*. Laplace3D's own medium is 60 fps 3D rigs — Blender-keyframed skeletal
animation with layered secondary motion — and UE's SkeletalMesh/AnimBP/Control Rig
pipeline is that medium's runtime, with headroom to spare. The UE build can meet every
line of the bar; where it fails, it will fail on *authoring* (contact, expression
subtlety), not on engine capability. The **jam entry cannot inherit this**: a quad
renderer playing baked parameter tracks can hold 60 fps and the sync numbers, but its
secondary motion and contact are only as good as the baked data, and the entry is
scored as a *specification check* of the week's design, not as the quality artifact.
The quality artifact of a UE week is the UE build, and the report must say so plainly.

## Risks and seams

- **The HTML5 wall is absolute.** Any schedule that assumes "we'll just export the UE
  project for the jam page" is broken from the start. The week's budget must carry
  *two* implementations of the logic (shared design, shared data, conformance-diffed)
  — that duplication is the price of an Unreal week, known in advance.
- **Paper2D is frozen.** PaperZD mitigates the animation-authoring gap, but both sit
  on a plugin Epic maintains at bug-fix level only. A Paper2D bug that blocks a week
  has no upstream fix coming; the UMG-portrait fallback is the contingency.
- **Cubism SDK for UE is beta.** Component order sensitivities and beta limitations
  are documented by Live2D themselves; a Live2D-dependent week plan needs a
  flipbook fallback for the same girl.
- **spine-ue feature gaps** (blend modes, PMA atlases, two-colour tint) can silently
  change a rig's look; any Spine asset must be screenshot-diffed against its editor
  render before it counts as evidence.
- **Inkpot is community-maintained** since its studio release; pin the exact commit,
  and keep the ink subset used in W7/W14 scripts inside Inkpot's tested surface.
- **GAS temptation.** GAS is present, prestigious, and wrong-sized for these weeks;
  adopting it mid-week is the classic way an Unreal jam week dies. The decision above
  (plain C++ machines) is recorded so it isn't re-litigated under deadline pressure.
- **Emscripten core drift.** The jam core and the UE build implement the same design
  twice; without the conformance harness running in CI from day one, they *will*
  diverge, and the port-back note will describe a game that was never actually played
  in Unreal. The harness is a day-one deliverable, not a closing task.


## How the research applies to By Heart

- Language: **C++ / Unreal** under tiger-style-cpp.
- Canon core: The Correction — her core game, at prototype scale.
- The parts matrix in the dossier names, for every part of the full game
  (rendering, animation, audio, dialogue/UI, state, persistence, assets,
  build), the library or tool this entry pins — or states plainly that the
  part is written from scratch (see the next chapter).
