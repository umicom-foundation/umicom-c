/*-----------------------------------------------------------------------------
 * Umicom C
 * File: browser/worker.mjs
 *
 * PURPOSE:
 *   Host bounded checked integer programs on a browser worker thread.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

import { runUmicomArithmetic, runUmicomBranches, runUmicomComparison, runUmicomIntInput,
    runUmicomIntegers, runUmicomLocals } from "./arithmetic.mjs";

// The worker takes a request ID, byte buffer, budget and optional profile.
// Only the input profile accepts a signed integer. The worker owns scheduling
// and delegates native-format validation to the interpreter. Each message
// gets its own stack, and existing callers retain arithmetic admission.
self.addEventListener("message", ({ data }) => {
    if (data === null || typeof data !== "object" || !Number.isSafeInteger(data.id) || data.id < 0) {
        self.postMessage({ id: null, ok: false, error: "invalid-request", sourceOffset: 0 });
        return;
    }
    if (data.profile === "comparison") {
        self.postMessage({ id: data.id, ...runUmicomComparison(data.artifact, data.fuel) });
        return;
    }
    if (data.profile === "locals") {
        self.postMessage({ id: data.id, ...runUmicomLocals(data.artifact, data.fuel) });
        return;
    }
    if (data.profile === "branches") {
        self.postMessage({ id: data.id, ...runUmicomBranches(data.artifact, data.fuel) });
        return;
    }
    if (data.profile === "integers") {
        self.postMessage({ id: data.id, ...runUmicomIntegers(data.artifact, data.fuel) });
        return;
    }
    if (data.profile === "int-input") {
        self.postMessage({ id: data.id, ...runUmicomIntInput(data.artifact, data.input, data.fuel) });
        return;
    }
    self.postMessage({ id: data.id, ...runUmicomArithmetic(data.artifact, data.fuel) });
});
