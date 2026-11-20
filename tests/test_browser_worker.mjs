/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/test_browser_worker.mjs
 *
 * PURPOSE:
 *   Compare native C artifacts and results across the bounded browser profiles.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { mkdtempSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { test } from "node:test";
import { Worker } from "node:worker_threads";
import { runUmicomArithmetic, runUmicomBranches, runUmicomComparison, runUmicomIntInput,
    runUmicomIntegers, runUmicomLocals } from "../browser/arithmetic.mjs";

const compiler = process.env.UMICOM_C_BROWSER_TEST_COMPILER;
assert.ok(compiler, "Use the optional CMake browser test to select a native compiler");

const directory = mkdtempSync(join(process.cwd(), "browser-arithmetic-fixtures-"));

function native(args) {
    const output = spawnSync(compiler, args, { encoding: "utf8", timeout: 8000, windowsHide: true });
    assert.equal(output.error, undefined, output.error?.message);
    return output;
}

function fixture(name, expression) {
    return programFixture(name, `int main(void){return ${expression};}\n`);
}

function programFixture(name, program, compileCommand = "compile") {
    const source = join(directory, `${name}.uc`);
    const artifact = join(directory, `${name}.umcb`);
    writeFileSync(source, program, { flag: "wx" });
    const compilation = native([compileCommand, source, artifact]);
    assert.equal(compilation.status, 0, compilation.stderr);
    const buffer = Uint8Array.from(readFileSync(artifact)).buffer;
    return { artifact, buffer };
}

test("compiled arithmetic agrees with the native runtime", () => {
    const cases = [
        ["answer", "(20 + 1) * 2", 42], ["minus", "-(3 + 4) - 9", -16],
        ["division-left", "-25 / 4", -6], ["division-right", "25 / -4", -6],
        ["maximum", "2147483647", 2147483647],
        ["minimum", "-2147483647 - 1", -2147483648], ["zero", "0 * -9", 0],
    ];
    for (const [name, expression, expected] of cases) {
        const { artifact, buffer } = fixture(name, expression);
        const result = native(["exec", artifact]);
        assert.equal(result.status, 0, result.stderr);
        const count = new DataView(buffer).getUint16(6, true);
        assert.equal(native(["inspect-bytecode", artifact]).status, 0);
        const nativeReport = /^Result: (-?\d+)\r?\nInstructions: (\d+)$/.exec(result.stdout.trim());
        assert.ok(nativeReport, result.stdout);
        assert.equal(Number(nativeReport[1]), expected);
        assert.equal(Number(nativeReport[2]), count);
        assert.deepEqual(runUmicomArithmetic(buffer), { ok: true, value: expected, instructions: count });
    }
});

test("counted loops use the same portable branches and fuel as native execution", () => {
    // Compile the documented source through the real native tool. The browser
    // consumes its normal forward-branch artifact without reparsing the loop.
    const source = readFileSync(new URL("../examples/counted_for.uc", import.meta.url), "utf8");
    const { artifact, buffer } = programFixture("counted-loop", source);
    const wire = new DataView(buffer);
    assert.equal(wire.getUint8(4), 4);
    const observed = native(["exec", artifact]);
    assert.equal(observed.status, 0, observed.stderr);
    const result = /^Result: 42\r?\nInstructions: (\d+)$/.exec(observed.stdout.trim());
    assert.ok(result, observed.stdout);
    const fuel = Number(result[1]);
    assert.deepEqual(runUmicomBranches(buffer, fuel), { ok: true, value: 42, instructions: fuel });
    assert.equal(runUmicomBranches(buffer, fuel - 1).error, "fuel-exhausted");
});

test("division and overflow fail at the same bytecode instruction", () => {
    for (const [name, expression] of [
        ["division-zero", "8 / 0"], ["addition-overflow", "2147483647 + 1"],
        ["division-overflow", "(-2147483647 - 1) / -1"], ["multiplication-overflow", "2147483647 * 2"],
    ]) {
        const { artifact, buffer } = fixture(name, expression);
        const nativeResult = native(["exec", artifact]);
        assert.equal(nativeResult.status, 1, nativeResult.stderr);
        assert.match(nativeResult.stderr, /division by zero or signed arithmetic overflow/i);
        const result = runUmicomArithmetic(buffer);
        assert.equal(result.error, "arithmetic-error");
        assert.equal(Object.hasOwn(result, "value"), false);
        const wire = new DataView(buffer);
        const finalArithmeticPosition = 8 + (wire.getUint16(6, true) - 2) * 9;
        assert.equal(result.sourceOffset, wire.getUint32(finalArithmeticPosition + 5, true));
    }
});

test("budget counts every admitted instruction including return", () => {
    const { artifact, buffer } = fixture("budget", "14");
    assert.equal(native(["exec", artifact, "--max-steps", "1"]).status, 1);
    assert.deepEqual(runUmicomArithmetic(buffer, 1), {
        ok: false, error: "fuel-exhausted", sourceOffset: new DataView(buffer).getUint32(22, true),
    });
    assert.deepEqual(runUmicomArithmetic(buffer, 2), { ok: true, value: 14, instructions: 2 });
    assert.equal(runUmicomArithmetic(buffer, 0).error, "fuel-exhausted");
    for (const invalid of [-1, 257, 0.5, NaN, Infinity, "2"]) {
        assert.equal(runUmicomArithmetic(buffer, invalid).error, "invalid-request");
    }
});

test("complete format validation precedes arithmetic execution", () => {
    const { buffer } = fixture("malformed", "8 / 0");
    const mutate = (op) => {
        const copy = buffer.slice(0);
        op(new DataView(copy));
        return copy;
    };
    for (const refused of [
        new ArrayBuffer(0), buffer.slice(0, -1), new ArrayBuffer(2313),
        mutate((wire) => wire.setUint8(0, 0)), mutate((wire) => wire.setUint8(5, 4)),
        mutate((wire) => wire.setUint16(6, 0, true)),
        mutate((wire) => wire.setUint8(8, 255)), mutate((wire) => wire.setUint8(8, 2)),
        mutate((wire) => wire.setUint32(13, 65537, true)),
        mutate((wire) => wire.setUint8(wire.byteLength - 9, 1)),
        mutate((wire) => wire.setInt32(wire.byteLength - 8, 1, true)),
    ]) {
        assert.equal(runUmicomArithmetic(refused, 0).error, "bad-bytecode");
    }
    const nativeHigher = fixture("comparison", "1 < 2");
    assert.equal(runUmicomArithmetic(nativeHigher.buffer).error, "unsupported-profile");
    assert.equal(runUmicomArithmetic(new Uint8Array(buffer)).error, "invalid-request");
    assert.equal(runUmicomArithmetic(new SharedArrayBuffer(32)).error, "invalid-request");
    const stolen = buffer.slice(0);
    structuredClone(stolen, { transfer: [stolen] });
    assert.equal(runUmicomArithmetic(stolen).error, "bad-bytecode");
});

test("stack exceeds the format limit before any instruction executes", () => {
    const size = 66;
    const buffer = new ArrayBuffer(8 + size * 9);
    const view = new DataView(buffer);
    view.setUint32(0, 0x554d4342);
    view.setUint8(4, 1);
    view.setUint16(6, size, true);
    for (let i = 0; i + 1 < size; ++i) view.setUint8(8 + i * 9, 1);
    view.setUint8(8 + (size - 1) * 9, 7);
    assert.equal(runUmicomArithmetic(buffer, 0).error, "stack-overflow");
});

test("worker responds to independent requests and validates their IDs", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const values = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (reply) => {
            messages.push(reply);
            if (messages.length === 3) resolve(messages);
        });
    });
    const { buffer } = fixture("worker", "100 / 2");
    worker.postMessage({ id: 12, artifact: buffer }, [buffer]);
    worker.postMessage({ id: 24, artifact: new ArrayBuffer(2) });
    worker.postMessage({ id: -1, artifact: new ArrayBuffer(2) });
    const [allowed, refused, invalid] = await values;
    assert.deepEqual(allowed, { id: 12, ok: true, value: 50, instructions: 4 });
    assert.deepEqual(refused, { id: 24, ok: false, error: "bad-bytecode", sourceOffset: 0 });
    assert.deepEqual(invalid, { id: null, ok: false, error: "invalid-request", sourceOffset: 0 });
});

test("all six comparisons match the native integer results", () => {
    const cases = [
        ["less-true", "-1 < 0", 1], ["less-false", "1 < 1", 0],
        ["less-equal-true", "3 <= 3", 1], ["less-equal-false", "4 <= 3", 0],
        ["greater-true", "2147483647 > (-2147483647 - 1)", 1], ["greater-false", "-9 > -2", 0],
        ["greater-equal-true", "-2 >= -2", 1], ["greater-equal-false", "-2 >= -1", 0],
        ["equal-true", "(10 + 2) == (3 * 4)", 1], ["equal-false", "-2 == 2", 0],
        ["not-equal-true", "0 != -1", 1], ["not-equal-false", "-6 != -6", 0],
        ["arithmetic-after-compare", "(3 <= 4) * 42", 42],
    ];
    for (const [name, expression, expected] of cases) {
        const { artifact, buffer } = fixture(name, expression);
        const wire = new DataView(buffer);
        assert.equal(wire.getUint8(4), 2);
        const count = wire.getUint16(6, true);
        const result = native(["exec", artifact]);
        assert.equal(result.status, 0, result.stderr);
        const nativeReport = /^Result: (-?\d+)\r?\nInstructions: (\d+)$/.exec(result.stdout.trim());
        assert.ok(nativeReport, result.stdout);
        assert.equal(Number(nativeReport[1]), expected);
        assert.equal(Number(nativeReport[2]), count);
        assert.deepEqual(runUmicomComparison(buffer), { ok: true, value: expected, instructions: count });
        assert.equal(runUmicomArithmetic(buffer).error, "unsupported-profile");
    }
});

test("comparison checks canonical version, operands and stack before execution", () => {
    const { buffer } = fixture("comparison-invalid", "3 <= 4");
    const modify = (original, operation) => {
        const copy = original.slice(0);
        operation(new DataView(copy));
        return copy;
    };
    const { buffer: arithmetic } = fixture("comparison-upgraded-arithmetic", "2 + 3");
    for (const refused of [
        modify(buffer, (wire) => wire.setUint8(4, 1)),
        modify(arithmetic, (wire) => wire.setUint8(4, 2)),
        modify(buffer, (wire) => wire.setUint8(8, 8)),
        modify(buffer, (wire) => wire.setInt32(27, 1, true)),
        buffer.slice(0, -1),
    ]) {
        assert.equal(runUmicomComparison(refused, 0).error, "bad-bytecode");
    }
    assert.deepEqual(runUmicomComparison(arithmetic), { ok: true, value: 5, instructions: 4 });
    assert.equal(runUmicomComparison(modify(buffer, (wire) => wire.setUint8(4, 3))).error, "unsupported-profile");
});

test("comparison instructions obey the native fuel and arithmetic bounds", () => {
    const { artifact, buffer } = fixture("comparison-fuel", "7 != 7");
    assert.equal(native(["exec", artifact, "--max-steps", "2"]).status, 1);
    assert.deepEqual(runUmicomComparison(buffer, 2), {
        ok: false, error: "fuel-exhausted", sourceOffset: new DataView(buffer).getUint32(31, true),
    });
    assert.deepEqual(runUmicomComparison(buffer, 4), { ok: true, value: 0, instructions: 4 });
    const { artifact: badArtifact, buffer: badBuffer } = fixture("comparison-arithmetic-error", "(9 / 0) == 1");
    assert.equal(native(["exec", badArtifact]).status, 1);
    const result = runUmicomComparison(badBuffer);
    assert.equal(result.error, "arithmetic-error");
    assert.equal(Object.hasOwn(result, "value"), false);
});

test("the worker requires explicit opt-in for comparison", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const values = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (reply) => {
            messages.push(reply);
            if (messages.length === 2) resolve(messages);
        });
    });
    const { buffer } = fixture("comparison-worker", "-4 < 2");
    const optedIn = buffer.slice(0);
    worker.postMessage({ id: 22, artifact: buffer }, [buffer]);
    worker.postMessage({ id: 23, profile: "comparison", artifact: optedIn }, [optedIn]);
    const [legacy, allowed] = await values;
    assert.deepEqual(legacy, { id: 22, ok: false, error: "unsupported-profile", sourceOffset: 0 });
    assert.deepEqual(allowed, { id: 23, ok: true, value: 1, instructions: 5 });
});

test("native locals preserve typed assignment, comparison and exact fuel", () => {
    const cases = [
        ["local-update", "int value = 20; int total = value * 2; value = total + 2; return value;", 42],
        ["local-signed", "int low = -2147483647 - 1; return low < 0;", 1],
        ["local-from-comparison", "int one = 3 <= 4; return one * 42;", 42],
    ];
    for (const [name, body, expected] of cases) {
        const { artifact, buffer } = programFixture(name, `int main(void) { ${body} }\n`);
        const wire = new DataView(buffer);
        assert.equal(wire.getUint8(4), 3);
        const count = wire.getUint16(6, true);
        const nativeResult = native(["exec", artifact]);
        assert.equal(nativeResult.status, 0, nativeResult.stderr);
        assert.equal(nativeResult.stdout.trim().replace("\r\n", "\n"), `Result: ${expected}\nInstructions: ${count}`);
        assert.deepEqual(runUmicomLocals(buffer, count), { ok: true, value: expected, instructions: count });
        assert.equal(runUmicomLocals(buffer, count - 1).error, "fuel-exhausted");
        assert.equal(runUmicomComparison(buffer).error, "unsupported-profile");
        assert.equal(runUmicomArithmetic(buffer).error, "unsupported-profile");
    }
});

test("local verification refuses uninitialized or out-of-bounds slots", () => {
    const { buffer } = programFixture("locals-refusal", "int main(void) { int value = 7; return value; }\n");
    const modify = (operation) => {
        const copy = buffer.slice(0);
        operation(new DataView(copy));
        return copy;
    };
    for (const refused of [
        modify((wire) => wire.setUint8(4, 2)),
        modify((wire) => wire.setUint8(8, 14)),
        modify((wire) => wire.setUint8(8, 15)),
        modify((wire) => wire.setInt32(18, 32, true)),
        modify((wire) => wire.setInt32(27, -1, true)),
        modify((wire) => wire.setInt32(27, 1, true)),
        buffer.slice(0, -1),
    ]) {
        assert.equal(runUmicomLocals(refused, 0).error, "bad-bytecode");
    }
    assert.equal(runUmicomLocals(modify((wire) => wire.setUint8(4, 4))).error, "unsupported-profile");
    const expected = { ok: true, value: 7, instructions: 4 };
    assert.deepEqual(runUmicomLocals(buffer), expected);
    assert.deepEqual(runUmicomLocals(buffer), expected);
});

test("local-slot 31 is usable at the native bound", () => {
    const declarations = Array.from({ length: 32 }, (_, i) => `int value${i} = ${i};`).join(" ");
    const { artifact, buffer } = programFixture("locals-maximum", `int main(void) { ${declarations} return value31; }\n`);
    const nativeResult = native(["exec", artifact]);
    assert.equal(nativeResult.status, 0, nativeResult.stderr);
    assert.match(nativeResult.stdout, /Result: 31/);
    assert.deepEqual(runUmicomLocals(buffer), { ok: true, value: 31, instructions: 66 });
});

test("worker requests select local state without changing legacy defaults", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const values = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (reply) => {
            messages.push(reply);
            if (messages.length === 3) resolve(messages);
        });
    });
    const { buffer } = programFixture("locals-worker", "int main(void) { int value = 42; return value; }\n");
    const older = buffer.slice(0);
    const noStore = buffer.slice(0);
    new DataView(noStore).setUint8(8, 14);
    worker.postMessage({ id: 34, profile: "locals", artifact: buffer }, [buffer]);
    worker.postMessage({ id: 35, profile: "comparison", artifact: older }, [older]);
    worker.postMessage({ id: 36, profile: "locals", artifact: noStore }, [noStore]);
    const [allowed, declined, uninitialized] = await values;
    assert.deepEqual(allowed, { id: 34, ok: true, value: 42, instructions: 4 });
    assert.deepEqual(declined, { id: 35, ok: false, error: "unsupported-profile", sourceOffset: 0 });
    assert.equal(uninitialized.id, 36);
    assert.equal(uninitialized.error, "bad-bytecode");
    assert.equal(Object.hasOwn(uninitialized, "value"), false);
});

test("forward branches match the native path, result and fuel count", () => {
    const cases = [
        ["branch-true", "int x = 21; if (x > 10) { x = x * 2; } else { x = 7; } return x;", 42],
        ["branch-false", "int x = 0; if (x) { x = 9 / 0; } else { x = 42; } return x;", 42],
        ["branch-signed-test", "int x = -1; if (x) { x = 9; } else { x = 7; } return x;", 9],
    ];
    for (const [name, body, expected] of cases) {
        const { buffer, artifact } = programFixture(name, `int main(void) { ${body} }\n`);
        const wire = new DataView(buffer);
        assert.equal(wire.getUint8(4), 4);
        const result = native(["exec", artifact]);
        assert.equal(result.status, 0, result.stderr);
        const report = /^Result: (-?\d+)\r?\nInstructions: (\d+)$/.exec(result.stdout.trim());
        assert.ok(report, result.stdout);
        const steps = Number(report[2]);
        assert.equal(Number(report[1]), expected);
        assert.ok(steps < wire.getUint16(6, true));
        assert.deepEqual(runUmicomBranches(buffer, steps), { ok: true, value: expected, instructions: steps });
        assert.equal(runUmicomBranches(buffer, steps - 1).error, "fuel-exhausted");
        assert.equal(native(["exec", artifact, "--max-steps", String(steps - 1)]).status, 1);
        assert.equal(runUmicomLocals(buffer).error, "unsupported-profile");
    }
});

function encodeFixture(records, format = 4) {
    const buffer = new ArrayBuffer(8 + records.length * 9);
    const wire = new DataView(buffer);
    wire.setUint32(0, 0x554d4342);
    wire.setUint8(4, format);
    wire.setUint16(6, records.length, true);
    for (const [index, [opcode, argument]] of records.entries()) {
        const offset = 8 + index * 9;
        wire.setUint8(offset, opcode);
        wire.setInt32(offset + 1, argument, true);
        wire.setUint32(offset + 5, index, true);
    }
    return buffer;
}

test("forward joins require the same stack depth and initialized locals on all edges", () => {
    const mismatched = encodeFixture([[1, 0], [17, 4], [1, 42], [16, 4], [7, 0]]);
    const maybeLocal = encodeFixture([[1, 0], [17, 4], [1, 42], [15, 0], [14, 0], [7, 0]]);
    const unreachable = encodeFixture([[16, 2], [1, 9], [1, 42], [7, 0]]);
    const selfBranch = encodeFixture([[1, 0], [17, 1], [1, 42], [7, 0]]);
    const outside = encodeFixture([[1, 0], [17, 4], [1, 42], [7, 0]]);
    const both = encodeFixture([[1, 1], [17, 5], [1, 42], [15, 31], [16, 7],
        [1, 7], [15, 31], [14, 31], [7, 0]]);
    for (const [id, buffer, offset] of [
        ["depth", mismatched, 3], ["initialization", maybeLocal, 4], ["unreachable", unreachable, 1],
        ["backward", selfBranch, 1], ["outside", outside, 1],
    ]) {
        const artifact = join(directory, `refused-branch-${id}.umcb`);
        writeFileSync(artifact, new Uint8Array(buffer), { flag: "wx" });
        assert.equal(native(["exec", artifact]).status, 1);
        assert.deepEqual(runUmicomBranches(buffer, 0), { ok: false, error: "bad-bytecode", sourceOffset: offset });
    }
    assert.deepEqual(runUmicomBranches(both), { ok: true, value: 42, instructions: 7 });
});

test("a forward format without branches is refused before running", () => {
    const gratuitous = encodeFixture([[1, 42], [7, 0]]);
    assert.deepEqual(runUmicomBranches(gratuitous, 0), { ok: false, error: "bad-bytecode", sourceOffset: 0 });
    const { buffer } = fixture("branch-profile-arithmetic", "6 * 7");
    assert.deepEqual(runUmicomBranches(buffer), { ok: true, value: 42, instructions: 4 });
});

test("a worker explicitly opts into the verified branch profile", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const values = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (reply) => {
            messages.push(reply);
            if (messages.length === 3) resolve(messages);
        });
    });
    const good = encodeFixture([[1, 0], [17, 5], [1, 1], [15, 0], [16, 7], [1, 42], [15, 0], [14, 0], [7, 0]]);
    const legacy = good.slice(0);
    const invalid = encodeFixture([[16, 0], [1, 42], [7, 0]]);
    worker.postMessage({ id: 40, profile: "branches", artifact: good }, [good]);
    worker.postMessage({ id: 41, profile: "locals", artifact: legacy }, [legacy]);
    worker.postMessage({ id: 42, profile: "branches", artifact: invalid }, [invalid]);
    const [accepted, refused, malformed] = await values;
    assert.deepEqual(accepted, { id: 40, ok: true, value: 42, instructions: 6 });
    assert.deepEqual(refused, { id: 41, ok: false, error: "unsupported-profile", sourceOffset: 0 });
    assert.deepEqual(malformed, { id: 42, ok: false, error: "bad-bytecode", sourceOffset: 0 });
});

test("native remainder, bitwise and shift results match the browser", () => {
    const cases = [
        ["remainder-positive", "93 % 51", 5, 42],
        ["remainder-negative", "-50 % 8", 5, -2],
        ["remainder-negative-divisor", "50 % -8", 5, 2],
        ["and", "-1 & 42", 6, 42],
        ["xor", "(-2147483647 - 1) ^ -1", 6, 2147483647],
        ["or", "(-2147483647 - 1) | 42", 6, -2147483606],
        ["left", "21 << 1", 7, 42],
        ["right", "84 >> 1", 7, 42],
        ["largest-count", "2147483647 >> 31", 7, 0],
    ];
    for (const [name, expression, format, expected] of cases) {
        const { artifact, buffer } = fixture("integer-" + name, expression);
        assert.equal(new DataView(buffer).getUint8(4), format);
        const result = native(["exec", artifact]);
        assert.equal(result.status, 0, result.stderr);
        const report = /^Result: (-?\d+)\r?\nInstructions: (\d+)$/.exec(result.stdout.trim());
        assert.ok(report, result.stdout);
        assert.equal(Number(report[1]), expected);
        const steps = Number(report[2]);
        assert.deepEqual(runUmicomIntegers(buffer, steps), { ok: true, value: expected, instructions: steps });
        assert.equal(runUmicomIntegers(buffer, steps - 1).error, "fuel-exhausted");
        assert.equal(runUmicomBranches(buffer).error, "unsupported-profile");
    }
});

test("refused remainder and checked shifts preserve native source locations", () => {
    for (const [name, expression] of [
        ["zero-divisor", "42 % 0"], ["minimum-divisor", "(-2147483647 - 1) % -1"],
        ["negative-count", "42 << -1"], ["count-too-wide", "42 >> 32"],
        ["negative-input", "-1 >> 1"], ["left-overflow", "1 << 31"],
    ]) {
        const { artifact, buffer } = fixture("integer-" + name, expression);
        const nativeResult = native(["exec", artifact]);
        assert.equal(nativeResult.status, 1);
        assert.match(nativeResult.stderr, /division by zero or signed arithmetic overflow/i);
        const wire = new DataView(buffer);
        const operator = 8 + (wire.getUint16(6, true) - 2) * 9;
        assert.deepEqual(runUmicomIntegers(buffer), {
            ok: false, error: "arithmetic-error", sourceOffset: wire.getUint32(operator + 5, true),
        });
    }
});

test("mixed forward paths preserve the highest structural format", () => {
    for (const [name, body, format] of [
        ["remainder-first", "int x = 93 % 51; if (x) { x = x + 0; } return x;", 5],
        ["shift-first", "int x = 21 << 1; if (x) { x = x & 63; } return x;", 7],
        ["untaken-shift", "int x = 0; if (x) { x = 1 << 32; } else { x = 42; } return x;", 7],
    ]) {
        const { artifact, buffer } = programFixture("integer-" + name, "int main(void) { " + body + " }\n");
        assert.equal(new DataView(buffer).getUint8(4), format);
        const result = native(["exec", artifact]);
        assert.equal(result.status, 0, result.stderr);
        assert.match(result.stdout, /^Result: 42\r?\n/);
        const steps = Number(/Instructions: (\d+)/.exec(result.stdout)[1]);
        assert.deepEqual(runUmicomIntegers(buffer, steps), { ok: true, value: 42, instructions: steps });
    }
});

test("integer verification rejects early versions, operands and stack underflow before fuel", () => {
    for (const [opcode, format] of [[18, 5], [19, 6], [20, 6], [21, 6], [22, 7], [23, 7]]) {
        const program = [[1, 42], [1, 1], [opcode, 0], [7, 0]];
        for (const bytes of [
            encodeFixture(program, format - 1),
            encodeFixture([[1, 42], [1, 1], [opcode, 7], [7, 0]], format),
            encodeFixture([[1, 42], [opcode, 0], [7, 0]], format),
        ]) {
            assert.equal(runUmicomIntegers(bytes, 0).error, "bad-bytecode");
        }
        if (format < 7) {
            assert.deepEqual(runUmicomIntegers(encodeFixture(program, format + 1), 0),
                { ok: false, error: "bad-bytecode", sourceOffset: 0 });
        }
    }
});

test("the worker requires integer opt-in for the newest native artifacts", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const replies = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (reply) => {
            messages.push(reply);
            if (messages.length === 2) resolve(messages);
        });
    });
    const { buffer } = fixture("integer-worker", "85 >> 1");
    const selected = buffer.slice(0);
    worker.postMessage({ id: 43, profile: "branches", artifact: buffer }, [buffer]);
    worker.postMessage({ id: 44, profile: "integers", artifact: selected }, [selected]);
    assert.deepEqual(await replies, [
        { id: 43, ok: false, error: "unsupported-profile", sourceOffset: 0 },
        { id: 44, ok: true, value: 42, instructions: 4 },
    ]);
});

test("the same required-input artifact follows the native branches in the browser", () => {
    const { artifact, buffer } = programFixture("entry-alternatives",
        "int umicom_run(int x) { int total=x*2; if(x>0) { total+=2; } return total; }", "compile-input");
    const original = buffer.slice(0);
    assert.equal(new DataView(buffer).getUint8(5), 1);
    for (const [input, expected] of [[20, 42], [-5, -10], [0, 0]]) {
        const result = native(["exec-input", artifact, String(input)]);
        assert.equal(result.status, 0, result.stderr);
        const report = /^Result: (-?\d+)\r?\nInstructions: (\d+)$/.exec(result.stdout.trim());
        assert.ok(report, result.stdout);
        assert.equal(Number(report[1]), expected);
        const instructions = Number(report[2]);
        assert.deepEqual(runUmicomIntInput(buffer, input, instructions), { ok: true, value: expected, instructions });
        assert.equal(runUmicomIntInput(buffer, input, instructions - 1).error, "fuel-exhausted");
    }
    assert.deepEqual(buffer, original);
    assert.equal(runUmicomIntegers(buffer).error, "bad-bytecode");
});

test("the input profile checks the signed range, flags and placeholder", () => {
    const { buffer } = programFixture("entry-shape", "int umicom_run(int x) { return x; }", "compile-input");
    for (const input of [undefined, null, "42", 1.5, Infinity, -2147483649, 2147483648]) {
        assert.equal(runUmicomIntInput(buffer, input).error, "invalid-request");
    }
    for (const input of [-2147483648, 2147483647]) {
        assert.deepEqual(runUmicomIntInput(buffer, input), { ok: true, value: input, instructions: 4 });
    }
    const flags = buffer.slice(0);
    new DataView(flags).setUint8(5, 2);
    assert.equal(runUmicomIntInput(flags, 42, 0).error, "bad-bytecode");
    const placeholder = buffer.slice(0);
    new DataView(placeholder).setInt32(9, 1, true);
    assert.equal(runUmicomIntInput(placeholder, 42, 0).error, "bad-bytecode");
});

test("the worker admits input only on its requested profile", { timeout: 15000 }, async (t) => {
    const worker = new Worker(new URL("./browser_worker_thread.mjs", import.meta.url));
    t.after(() => worker.terminate());
    const replies = new Promise((resolve, reject) => {
        const messages = [];
        worker.once("error", reject);
        worker.on("message", (message) => {
            messages.push(message);
            if (messages.length === 4) resolve(messages);
        });
    });
    const { buffer } = programFixture("entry-worker", "int umicom_run(int x) { return x + 2; }", "compile-input");
    const legacy = fixture("entry-worker-legacy", "42").buffer;
    for (const request of [
        { id: 50, profile: "int-input", input: 40, artifact: buffer.slice(0) },
        { id: 51, profile: "int-input", artifact: buffer.slice(0) },
        { id: 52, profile: "integers", input: 40, artifact: buffer.slice(0) },
        { id: 53, profile: "int-input", input: 40, artifact: legacy },
    ]) worker.postMessage(request, [request.artifact]);
    assert.deepEqual(await replies, [
        { id: 50, ok: true, value: 42, instructions: 6 },
        { id: 51, ok: false, error: "invalid-request", sourceOffset: 0 },
        { id: 52, ok: false, error: "bad-bytecode", sourceOffset: 0 },
        { id: 53, ok: false, error: "bad-bytecode", sourceOffset: 0 },
    ]);
});
