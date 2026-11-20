/*-----------------------------------------------------------------------------
 * Umicom C
 * File: tests/browser_worker_thread.mjs
 *
 * PURPOSE:
 *   Supply the browser message interface to the production worker in Node tests.
 *
 * AUTHOR AND ORGANISATION:
 * Sammy Hegab
 * Umicom Foundation
 *
 * LICENCE:
 * MIT
 *---------------------------------------------------------------------------*/

import { parentPort } from "node:worker_threads";

globalThis.self = {
    addEventListener(type, callback) {
        if (type !== "message") throw new Error("Unexpected worker event");
        parentPort.on(type, (data) => callback({ data }));
    },
    postMessage(message) {
        parentPort.postMessage(message);
    },
};

await import("../browser/worker.mjs");
