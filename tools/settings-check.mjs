// settings-check.mjs — gates R-202/R-203/R-403: the settings AND
// record bridges round-trip, and the settings screen is reachable
// from boot with no progression (safety/accessibility options are
// never gated).
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { createGame, INPUT } from "../web/game.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const wasmBytes = readFileSync(join(root, "build/byheart.wasm"));
const game = await createGame(wasmBytes, { seed: 7, level: 0, demo: false });

function steps(n, bits = 0) { for (let i = 0; i < n; i++) game.step(bits); }
function fail(msg) { console.error(`settings-check: ${msg}`); process.exit(1); }

if (game.screen() !== 0) fail("boot screen is not the 18+ gate");
game.applySettings(true, false);
if (game.settingsBits() !== 1) fail("apply_settings(window_grace) did not round-trip");
game.applySettings(false, true);
if (game.settingsBits() !== 2) fail("apply_settings(text_large) did not round-trip");
game.applySettings(true, true);

// The record bridge: kept sittings and by-heart sittings restore.
game.applyRecord(3, 2);
if (game.recordKept() !== 3) fail("apply_record kept did not round-trip");
if (game.recordFlawless() !== 2) fail("apply_record flawless did not round-trip");
game.applyRecord(5000, 99);
if (game.recordKept() !== 999) fail("apply_record did not clamp kept");
if (game.recordFlawless() !== 99) fail("apply_record flawless should follow the given value under kept");

// Navigate gate -> title -> settings with plain input, no play first.
steps(1, INPUT.confirm);
if (game.screen() !== 1) fail("confirm did not leave the gate");
steps(1, INPUT.down); steps(1, INPUT.down);
steps(1, INPUT.confirm);
if (game.screen() !== 5) fail(`settings not reachable from title (screen=${game.screen()})`);
steps(2);
if (!game.frameText().includes("LONG")) fail("settings screen does not show restored LONG grace");
if (!game.frameText().includes("LARGE")) fail("settings screen does not show restored LARGE text");
console.log("settings-check: settings + record bridges round-trip; settings reachable ungated; restored values render");
